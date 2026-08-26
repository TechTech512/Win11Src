/*
 * commlist.c - WIM Mount Driver Communication List Implementation
 *
 * This file contains the management of the global list of active WIM
 * communication entries, including insertion, lookup, removal, and reference
 * counting.
 */

#pragma warning (disable:4996)
#include "commlist.h"

// External global variables (defined elsewhere)
extern PFLT_FILTER gFilterHandle;
WM_COMM_LIST g_CommList = { 0 };

UCHAR
MatchByGuid(
    PWM_COMM_LIST_ENTRY Entry,
    PVOID Guid
);
PWM_COMM_LIST_ENTRY
PriGetEntry(
    unsigned char (__cdecl *MatchFn)(PWM_COMM_LIST_ENTRY, void*),
    PVOID Context
);
NTSTATUS
WMCommListCreate(
    VOID
);
VOID
WMCommListEntryAddRef(
    PWM_COMM_LIST_ENTRY Entry
);
VOID
WMCommListEntryRelease(
    PWM_COMM_LIST_ENTRY Entry
);
NTSTATUS
WMCommListGet(
    PGUID Guid,
    PWM_COMM_LIST_ENTRY *OutEntry
);
NTSTATUS
WMCommListInsert(
    PGUID Guid,
    PFLT_PORT PortHandle,
    ULONG ProcessId,
    PPF_VERSION Version
);
NTSTATUS
WMCommListRemove(
    PGUID Guid
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MatchByGuid)
#pragma alloc_text(PAGE,PriGetEntry)
#pragma alloc_text(PAGE,WMCommListCreate)
#pragma alloc_text(PAGE,WMCommListEntryAddRef)
#pragma alloc_text(PAGE,WMCommListEntryRelease)
#pragma alloc_text(PAGE,WMCommListGet)
#pragma alloc_text(PAGE,WMCommListInsert)
#pragma alloc_text(PAGE,WMCommListRemove)
#endif // ALLOC_PRAGMA


// ----------------------------------------------------------------------------
// MatchByGuid - callback for PriGetEntry to match a GUID
// ----------------------------------------------------------------------------
UCHAR
MatchByGuid(
    PWM_COMM_LIST_ENTRY Entry,
    PVOID Guid
)
{
    return (RtlCompareMemory(&Entry->Guid, Guid, sizeof(GUID)) == sizeof(GUID)) ? 1 : 0;
}

// ----------------------------------------------------------------------------
// PriGetEntry - traverse the list and call the match function
// ----------------------------------------------------------------------------
PWM_COMM_LIST_ENTRY
PriGetEntry(
    unsigned char (__cdecl *MatchFn)(PWM_COMM_LIST_ENTRY, void*),
    PVOID Context
)
{
    PWM_COMM_LIST pList = (PWM_COMM_LIST)g_CommList.ListHead.Flink;

    while (pList != &g_CommList)
    {
        if (MatchFn((PWM_COMM_LIST_ENTRY)pList, Context) != 0)
            break;
        pList = (PWM_COMM_LIST)(pList->ListHead.Flink);
    }

    return (pList == &g_CommList) ? NULL : (PWM_COMM_LIST_ENTRY)pList;
}

// ----------------------------------------------------------------------------
// WMCommListCreate - initialise the global list and resource
// ----------------------------------------------------------------------------
NTSTATUS
WMCommListCreate(
    VOID
)
{
    g_CommList.ListHead.Flink = &g_CommList.ListHead;
    g_CommList.ListHead.Blink = &g_CommList.ListHead;
    ExInitializeResourceLite(&g_CommList.ListLock);
    return STATUS_SUCCESS;
}

// ----------------------------------------------------------------------------
// WMCommListEntryAddRef - increment reference count of an entry
// ----------------------------------------------------------------------------
VOID
WMCommListEntryAddRef(
    PWM_COMM_LIST_ENTRY Entry
)
{
    InterlockedIncrement((LONG*)&Entry->RefCount);
}

// ----------------------------------------------------------------------------
// WMCommListEntryRelease - decrement reference count and free if zero
// ----------------------------------------------------------------------------
VOID
WMCommListEntryRelease(
    PWM_COMM_LIST_ENTRY Entry
)
{
    LONG NewRef;

    NewRef = InterlockedDecrement((LONG*)&Entry->RefCount);

    if (NewRef == 0 && Entry != NULL)
    {
        if (Entry->ProcessHandle != NULL)
            ZwClose((HANDLE)Entry->ProcessHandle);

        if (Entry->PortHandle != NULL)
            FltCloseClientPort(gFilterHandle, &Entry->PortHandle);

        ExFreePoolWithTag(Entry, 'emcW');   // 'Wmce'
    }
}

// ----------------------------------------------------------------------------
// WMCommListGet - find an entry by GUID and add a reference
// ----------------------------------------------------------------------------
NTSTATUS
WMCommListGet(
    PGUID Guid,
    PWM_COMM_LIST_ENTRY *OutEntry
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWM_COMM_LIST_ENTRY Found;

    FltAcquireResourceShared(&g_CommList.ListLock);

    Found = PriGetEntry((unsigned char (__cdecl *)(PWM_COMM_LIST_ENTRY,void *))MatchByGuid, Guid);
    if (Found == NULL)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        *OutEntry = Found;
        WMCommListEntryAddRef(Found);
    }

    FltReleaseResource(&g_CommList.ListLock);
    return Status;
}

// ----------------------------------------------------------------------------
// WMCommListInsert - create a new entry and insert into the global list
// ----------------------------------------------------------------------------
NTSTATUS
WMCommListInsert(
    PGUID Guid,
    PFLT_PORT PortHandle,
    ULONG ProcessId,
    PPF_VERSION Version
)
{
    NTSTATUS Status;
    PWM_COMM_LIST_ENTRY NewEntry;
    HANDLE hProcess = NULL;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES ObjAttr;
    PWM_COMM_LIST pList;
    PWM_COMM_LIST_ENTRY Existing;

    // Allocate new entry
    NewEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(WM_COMM_LIST_ENTRY), 'emcW');
    if (!NewEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(NewEntry, sizeof(WM_COMM_LIST_ENTRY));
    NewEntry->RefCount = 1;

    // Open a handle to the target process
    ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)ProcessId;
    ClientId.UniqueThread = NULL;
    InitializeObjectAttributes(&ObjAttr, NULL, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &ObjAttr, &ClientId);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(NewEntry, 'emcW');
        return Status;
    }

    NewEntry->ProcessHandle = (PVOID*)hProcess;
    NewEntry->ProcessId = ProcessId;
    NewEntry->PortHandle = PortHandle;
    NewEntry->Guid = *Guid;
    NewEntry->Version = *Version;

    // Insert into the global list (exclusive lock)
    FltAcquireResourceExclusive(&g_CommList.ListLock);

    Existing = PriGetEntry((unsigned char (__cdecl *)(PWM_COMM_LIST_ENTRY,void *))MatchByGuid, Guid);
    if (Existing != NULL)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
    }
    else
    {
        InsertTailList(&g_CommList.ListHead, &NewEntry->Link);
        Status = STATUS_SUCCESS;
    }

    FltReleaseResource(&g_CommList.ListLock);

    // Cleanup on failure
    if (!NT_SUCCESS(Status))
    {
        if (hProcess)
            ZwClose(hProcess);
        if (NewEntry->PortHandle)
            ZwClose((HANDLE)NewEntry->PortHandle);
        ExFreePoolWithTag(NewEntry, 'emcW');
    }

    return Status;
}

// ----------------------------------------------------------------------------
// WMCommListRemove - remove an entry by GUID
// ----------------------------------------------------------------------------
NTSTATUS
WMCommListRemove(
    PGUID Guid
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWM_COMM_LIST_ENTRY Found;
    PLIST_ENTRY pFlink, pBlink;

    FltAcquireResourceExclusive(&g_CommList.ListLock);

    Found = PriGetEntry((unsigned char (__cdecl *)(PWM_COMM_LIST_ENTRY,void *))MatchByGuid, Guid);
    if (Found == NULL)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Exit;
    }

    pFlink = Found->Link.Flink;
    pBlink = Found->Link.Blink;

    // Verify list integrity
    if (pFlink->Blink != &Found->Link || pBlink->Flink != &Found->Link)
    {
        // Corrupted list – crash the system (as the original did)
        KeBugCheckEx(
            DRIVER_IRQL_NOT_LESS_OR_EQUAL,
            (ULONG_PTR)pFlink,
            (ULONG_PTR)pBlink,
            (ULONG_PTR)&Found->Link,
            0
        );
    }

    RemoveEntryList(&Found->Link);
    WMCommListEntryRelease(Found);

Exit:
    FltReleaseResource(&g_CommList.ListLock);
    return Status;
}

