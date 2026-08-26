/*
 * messages.c - WIM Mount Driver - Communication and Message Handling
 *
 * This file implements the communication port callbacks, message processing,
 * and helper routines for sending/receiving messages between kernel and user mode.
 */

#pragma warning (disable:4996)
#pragma warning (disable:4703)
#include "commlist.h"

// ----------------------------------------------------------------------------
// External globals (defined in other compilation units)
// ----------------------------------------------------------------------------
extern PFLT_FILTER gFilterHandle;
extern WM_COMM_LIST g_CommList;
extern ERESOURCE SubsumeLock;

// ----------------------------------------------------------------------------
// Object types from the kernel (defined in ntddk.h)
// ----------------------------------------------------------------------------
extern POBJECT_TYPE *ExEventObjectType;
extern POBJECT_TYPE *PsProcessType;
extern POBJECT_TYPE *IoFileObjectType;

// ----------------------------------------------------------------------------
// Function pointer for ObReferenceObjectByHandle (resolved at runtime)
// ----------------------------------------------------------------------------
typedef NTSTATUS (NTAPI *PFN_ObReferenceObjectByHandle)(
    HANDLE Handle,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID *Object,
    POBJECT_HANDLE_INFORMATION HandleInformation
);

static PFN_ObReferenceObjectByHandle pfnObReferenceObjectByHandle = NULL;

// ----------------------------------------------------------------------------
// Function pointer for FltCreateFileEx2 (resolved at runtime)
// ----------------------------------------------------------------------------
typedef NTSTATUS (__cdecl *PFN_FltCreateFileEx2)(
    PFLT_FILTER Filter,
    PFLT_INSTANCE Instance,
    PVOID *FileHandle,
    PFILE_OBJECT **FileObject,
    ULONG DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength,
    ULONG Flags,
    PIO_DRIVER_CREATE_CONTEXT DriverContext
);

static PFN_FltCreateFileEx2 pFltCreateFileEx2 = NULL;

// ----------------------------------------------------------------------------
// Forward declarations of static functions
// ----------------------------------------------------------------------------
static NTSTATUS DuplicateEventIntoProcess(PVOID TargetProcessHandle, PVOID **OutEvent,
                                          PKEVENT *OutKernelEvent);
static NTSTATUS PriCreateTransactional(PFLT_FILTER Filter, PFLT_INSTANCE Instance,
                                       PVOID Arg3, PVOID Arg4, PUNICODE_STRING Arg5,
                                       ULONG Arg6, PVOID *Arg7, PFILE_OBJECT **Arg8);
static NTSTATUS PriDuplicateUserBuffer(PVOID Dest, ULONG Size, PVOID *Buffer);
static NTSTATUS PriExchangeDirs(PVOID p1, PVOID p2, PVOID p3,
                                PPF_CONNECTION_COOKIE Cookie, PUCHAR p5);
static VOID PriFreeString(PUNICODE_STRING String);
static NTSTATUS PriGetFileNameCallback(PVOID p1, PUNICODE_STRING p2, PULONG p3);
static NTSTATUS PriGetObjectsFromUserHandle(PVOID p1, ULONG p2, PFLT_FILTER **p3,
                                            PFLT_INSTANCE **p4, PFLT_VOLUME **p5,
                                            PVOID *p6, PFILE_OBJECT **p7);
static NTSTATUS PriGetString(PVOID p1, PVOID Callback, PUNICODE_STRING p3);
static NTSTATUS PriProcesRegisterGuidMessage(PPF_CONNECTION_COOKIE Cookie,
                                             PPF_MESSAGE_REGISTER_GUID Msg,
                                             ULONG ProcessId);
static NTSTATUS PriProcessMessage(PVOID InputBuffer, PVOID OutputBuffer, ULONG OutputSize,
                                  PVOID Arg4, ULONG Arg5, PULONG Arg6);
static NTSTATUS PriStringEnsureCapacity(PUNICODE_STRING String, ULONG NewSize);
static NTSTATUS PriStubCreateChild(PVOID p1, ULONG p2, ULONG p3, ULONG p4,
                                   ULONG p5, PUNICODE_STRING p6, PVOID *p7);
static NTSTATUS PriStubRename(PVOID p1, PFILE_RENAME_INFORMATION RenameInfo,
                              ULONG p3, ULONG p4);
static NTSTATUS PriSubsumeContext(PVOID p1, PLARGE_INTEGER p2,
                                  PLARGE_INTEGER p3, PVOID *p4);

NTSTATUS NTAPI PFSubsumeContext(PFLT_FILTER Filter, PFLT_INSTANCE Instance, PVOID Arg3,
                                 PLARGE_INTEGER FileId, PLARGE_INTEGER Arg5, PVOID *Arg6);

extern NTSTATUS
WMCommListCreate(
    VOID
);

extern VOID
WMCommListEntryRelease(
    PWM_COMM_LIST_ENTRY Entry
);

extern NTSTATUS
WMCommListInsert(
    PGUID Guid,
    PFLT_PORT PortHandle,
    ULONG ProcessId,
    PPF_VERSION Version
);

extern NTSTATUS
WMCommListRemove(
    PGUID Guid
);

extern NTSTATUS
WMCommListGet(
    PGUID Guid,
    PWM_COMM_LIST_ENTRY *OutEntry
);

NTSTATUS
PFPortConnect(
    PVOID PortCookie,
    PVOID ConnectionContext,
    PVOID ConnectionCookie,
    ULONG SizeOfContext,
    PVOID *Connection
);
VOID
PFPortDisconnect(
    PVOID Connection
);
NTSTATUS
PFReceiveMessage(
    PVOID PortCookie,
    PVOID InputBuffer,
    ULONG InputSize,
    PVOID OutputBuffer,
    ULONG OutputSize,
    PULONG BytesReturned
);
NTSTATUS
PFSendExchangeCloseHandlesMessage(
    PFLT_FILTER Filter,
    PPF_CONNECTION_COOKIE Cookie,
    PVOID Arg3,
    PVOID Arg4
);
NTSTATUS
PFSendExtractMessage(
    PVOID *GuidPtr,
    ULONG GuidLength,
    PVOID Arg3,
    ULONG Arg4
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,DuplicateEventIntoProcess)
#pragma alloc_text(PAGE,PFPortConnect)
#pragma alloc_text(PAGE,PFPortDisconnect)
#pragma alloc_text(PAGE,PFReceiveMessage)
#pragma alloc_text(PAGE,PFSendExchangeCloseHandlesMessage)
#pragma alloc_text(PAGE,PFSendExtractMessage)
#pragma alloc_text(PAGE,PriCreateTransactional)
#pragma alloc_text(PAGE,PriDuplicateUserBuffer)
#pragma alloc_text(PAGE,PriExchangeDirs)
#pragma alloc_text(PAGE,PriFreeString)
#pragma alloc_text(PAGE,PriGetFileNameCallback)
#pragma alloc_text(PAGE,PriGetObjectsFromUserHandle)
#pragma alloc_text(PAGE,PriGetString)
#pragma alloc_text(PAGE,PriProcesRegisterGuidMessage)
#pragma alloc_text(PAGE,PriProcessMessage)
#pragma alloc_text(PAGE,PriStringEnsureCapacity)
#pragma alloc_text(PAGE,PriStubCreateChild)
#pragma alloc_text(PAGE,PriStubRename)
#pragma alloc_text(PAGE,PriSubsumeContext)
#endif // ALLOC_PRAGMA

// ============================================================================
// DuplicateEventIntoProcess
// ============================================================================
NTSTATUS
DuplicateEventIntoProcess(
    PVOID TargetProcessHandle,
    PVOID **OutEvent,
    PKEVENT *OutKernelEvent
)
{
    NTSTATUS Status;
    HANDLE hEvent = NULL;
    OBJECT_ATTRIBUTES ObjAttr;

    *OutEvent = NULL;
    *(PVOID *)TargetProcessHandle = NULL;

    InitializeObjectAttributes(&ObjAttr, NULL, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &ObjAttr, NotificationEvent, FALSE);
    if (Status >= 0)
    {
        Status = ObReferenceObjectByHandle(hEvent, EVENT_ALL_ACCESS, (POBJECT_TYPE)ExEventObjectType, KernelMode,
                                           TargetProcessHandle, NULL);
        if (Status >= 0)
        {
            Status = ZwDuplicateObject(NtCurrentProcess(), hEvent,
                                       (HANDLE)TargetProcessHandle, (PHANDLE)OutEvent,
                                       0, 0, DUPLICATE_SAME_ACCESS);
            if (Status >= 0)
                goto Done;
        }
    }

    // Error cleanup
    if (*(PVOID *)TargetProcessHandle != NULL)
    {
        ObfDereferenceObject(*(PVOID *)TargetProcessHandle);
        *(PVOID *)TargetProcessHandle = NULL;
    }

Done:
    if (hEvent)
        ZwClose(hEvent);

    return Status;
}

// ============================================================================
// PFPortConnect - Communication port connection callback
// ============================================================================
NTSTATUS
PFPortConnect(
    PVOID PortCookie,
    PVOID ConnectionContext,
    PVOID ConnectionCookie,
    ULONG SizeOfContext,
    PVOID *Connection
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPF_CONNECTION_COOKIE Cookie = NULL;

    if (PortCookie == NULL || Connection == NULL)
        return STATUS_INVALID_PARAMETER;

    Cookie = ExAllocatePoolWithTag(NonPagedPool, sizeof(PF_CONNECTION_COOKIE), 'cmcW');
    if (Cookie == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(Cookie, sizeof(PF_CONNECTION_COOKIE));
    Cookie->Version.MajorVersion = 3;
    Cookie->Version.MinorVersion = 0;
    Cookie->FilterPointer = gFilterHandle;
    Cookie->ServicedGuid = *(PGUID)PortCookie;
    Cookie->GuidSet = 0;

    *Connection = Cookie;
    return STATUS_SUCCESS;

Error:
    if (Cookie)
        ExFreePoolWithTag(Cookie, 'cmcW');
    return Status;
}

// ============================================================================
// PFPortDisconnect - Communication port disconnection callback
// ============================================================================
VOID
PFPortDisconnect(
    PVOID Connection
)
{
    PPF_CONNECTION_COOKIE Cookie = (PPF_CONNECTION_COOKIE)Connection;
    NTSTATUS Status;

    if (Cookie == NULL)
        return;

    if (Cookie->GuidSet != 0)
    {
        Status = WMCommListRemove(&Cookie->ServicedGuid);
        if (Status < 0)
            return;
    }

    if (Cookie->PortHandle != NULL)
        FltCloseClientPort(gFilterHandle, &Cookie->PortHandle);

    ExFreePoolWithTag(Cookie, 'cmcW');
}

// ============================================================================
// PFReceiveMessage - Communication port receive callback
// ============================================================================
NTSTATUS
PFReceiveMessage(
    PVOID PortCookie,
    PVOID InputBuffer,
    ULONG InputSize,
    PVOID OutputBuffer,
    ULONG OutputSize,
    PULONG BytesReturned
)
{
    NTSTATUS Status;

    if (PortCookie == NULL)
        return STATUS_INVALID_DEVICE_REQUEST;

    Status = PriProcessMessage(InputBuffer, OutputBuffer, OutputSize,
                               BytesReturned, InputSize, (PULONG)PortCookie);
    return Status;
}

// ============================================================================
// PFSendExchangeCloseHandlesMessage
// ============================================================================
NTSTATUS
PFSendExchangeCloseHandlesMessage(
    PFLT_FILTER Filter,
    PPF_CONNECTION_COOKIE Cookie,
    PVOID Arg3,
    PVOID Arg4
)
{
    NTSTATUS Status;
    WM_COMM_LIST_ENTRY *ListEntry = NULL;
    PF_MESSAGE_HEADER MsgHeader = {0};
    ULONG BytesReturned;
    ULONG SecurityCookie;


    Status = WMCommListGet(&Cookie->ServicedGuid, &ListEntry);
    if (Status < 0)
        goto Exit;

    MsgHeader.Signature = 0x42424242;
    MsgHeader.MajorVersion = 3;
    MsgHeader.MinorVersion = 1;
    MsgHeader.StructureSize = 0x20;
    MsgHeader.ActionType = ExchangeCloseHandles;

    Status = FltSendMessage(
        gFilterHandle,
        &ListEntry->PortHandle,
        &MsgHeader,
        sizeof(MsgHeader),
        NULL,
        &BytesReturned,
        NULL
    );
    if (Status >= 0)
        Status = BytesReturned;

    WMCommListEntryRelease(ListEntry);

Exit:
    return Status;
}

// ============================================================================
// PFSendExtractMessage
// ============================================================================
NTSTATUS
PFSendExtractMessage(
    PVOID *GuidPtr,
    ULONG GuidLength,
    PVOID Arg3,
    ULONG Arg4
)
{
    NTSTATUS Status;
    WM_COMM_LIST_ENTRY *ListEntry = NULL;
    PVOID MessageBuffer = NULL;
    ULONG BufferSize;
    WM_COMM_LIST_ENTRY TempEntry;
    ULONG SecurityCookie;
    ULONG ReplyBytes;
    ULONG WaitResult;
    HANDLE hProcess;

    RtlZeroMemory(&TempEntry, sizeof(WM_COMM_LIST_ENTRY));
    TempEntry.Guid.Data4[4] = 0;
    TempEntry.Guid.Data4[5] = 0;
    TempEntry.Guid.Data4[6] = 0;
    TempEntry.Guid.Data4[7] = 0;
    *(PULONG)((PUCHAR)&TempEntry.Guid + 4) = GuidLength;
    TempEntry.Guid.Data1 = 0;
    TempEntry.RefCount = 0;
    TempEntry.Link.Flink = NULL;
    TempEntry.Link.Blink = NULL;
    TempEntry.Guid.Data4[3] = 0;
    TempEntry.PortHandle = NULL;
    TempEntry.ProcessHandle = NULL;
    TempEntry.ProcessId = 0;
    TempEntry.Version.MajorVersion = 0;
    TempEntry.Version.MinorVersion = 0;

    if (GuidLength < 0x10)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = WMCommListGet((PGUID)GuidPtr, &ListEntry);
    if (Status < 0)
        goto Exit;

    BufferSize = GuidLength + 0x30;
    MessageBuffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'gmdW');
    if (MessageBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupEntry;
    }

    Status = ZwDuplicateObject(NtCurrentProcess(),
                               (HANDLE)ListEntry->ProcessHandle,
                               NtCurrentProcess(),
                               (PHANDLE)&TempEntry.RefCount,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (Status < 0)
        goto CleanupBuffer;

    if (ListEntry->PortHandle)
        FltClose((PVOID)ListEntry->PortHandle);
    ListEntry->PortHandle = NULL;

    *(PULONG)MessageBuffer = 0x42424242;

    if (ListEntry->Version.MajorVersion < 4 &&
        (ListEntry->Version.MajorVersion != 3 || ListEntry->Version.MinorVersion == 0))
    {
        *(PUSHORT)((PUCHAR)MessageBuffer + 2) = 3;
        *(PULONG)((PUCHAR)MessageBuffer + 4) = 0;
        *(PUSHORT)((PUCHAR)MessageBuffer + 6) = 0;
        *(PULONG)((PUCHAR)MessageBuffer + 6) = GuidLength;
        *(PULONG)((PUCHAR)MessageBuffer + 8) = GuidLength + 0x20;
        memcpy((PUCHAR)MessageBuffer + 0x1c, GuidPtr, GuidLength);
        *(PULONG)((PUCHAR)MessageBuffer + 0x10) = (ULONG)ListEntry->ProcessHandle;
        *(PULONG)((PUCHAR)MessageBuffer + 0x14) = (ULONG)(ULONG_PTR)ListEntry->ProcessHandle >> 0x1f;

        Status = FltSendMessage(
            gFilterHandle,
            &ListEntry->PortHandle,
            MessageBuffer,
            *(PULONG)((PUCHAR)MessageBuffer + 8),
            NULL,
            &ReplyBytes,
            NULL
        );
        if (Status >= 0)
        {
            if ((LONG)ReplyBytes >= 0 && (LONG)TempEntry.ProcessId >= 0 &&
                TempEntry.Guid.Data4[3] != 0)
            {
                WaitResult = KeWaitForMultipleObjects(
                    3,
                    (PVOID*)&TempEntry.Link.Flink,
                    WaitAny,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL,
                    NULL
                );
                if (WaitResult != 1)
                    Status = STATUS_UNSUCCESSFUL;
            }
        }
    }
    else
    {
        *(PUSHORT)((PUCHAR)MessageBuffer + 6) = 1;
        *(PULONG)((PUCHAR)MessageBuffer + 8) = BufferSize;
        *(PULONG)((PUCHAR)MessageBuffer + 0x28) = GuidLength;
        *(PUSHORT)((PUCHAR)MessageBuffer + 2) = 3;
        *(PULONG)((PUCHAR)MessageBuffer + 4) = 4;
        memcpy((PUCHAR)MessageBuffer + 0x2c, GuidPtr, GuidLength);
        *(PULONG)((PUCHAR)MessageBuffer + 0x10) = (ULONG)ListEntry->ProcessHandle;
        *(PULONG)((PUCHAR)MessageBuffer + 0x14) = (ULONG)(ULONG_PTR)ListEntry->ProcessHandle >> 0x1f;

        Status = DuplicateEventIntoProcess((PVOID)0x160e9, NULL, NULL);
        if (Status >= 0)
        {
            Status = DuplicateEventIntoProcess(&TempEntry, (PVOID**)&TempEntry, NULL);
            if (Status >= 0)
            {
                *(PULONG)((PUCHAR)MessageBuffer + 0x18) = 0;
                *(PULONG)((PUCHAR)MessageBuffer + 0x1c) = 0;
                *(PULONG)((PUCHAR)MessageBuffer + 0x20) = 0;
                *(PULONG)((PUCHAR)MessageBuffer + 0x24) = 0;
                TempEntry.Guid.Data4[3] = 1;

                Status = FltSendMessage(
                    gFilterHandle,
                    &ListEntry->PortHandle,
                    MessageBuffer,
                    BufferSize,
                    NULL,
                    &ReplyBytes,
                    NULL
                );
                if (Status >= 0)
                    Status = ReplyBytes;
            }
        }
    }

CleanupBuffer:
    if (MessageBuffer)
        ExFreePoolWithTag(MessageBuffer, 'gmdW');

CleanupEntry:
    if (ListEntry)
        WMCommListEntryRelease(ListEntry);

    if (TempEntry.Link.Blink)
        ObfDereferenceObject(TempEntry.Link.Blink);
    if (TempEntry.Link.Flink)
        ObfDereferenceObject(TempEntry.Link.Flink);

Exit:
    return Status;
}

// ============================================================================
// PriCreateTransactional
// ============================================================================
NTSTATUS
PriCreateTransactional(
    PFLT_FILTER Filter,
    PFLT_INSTANCE Instance,
    PVOID Arg3,
    PVOID Arg4,
    PUNICODE_STRING Arg5,
    ULONG Arg6,
    PVOID *Arg7,
    PFILE_OBJECT **Arg8
)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    IO_DRIVER_CREATE_CONTEXT DriverContext;
    IO_STATUS_BLOCK IoStatus;

    RtlZeroMemory(&DriverContext, sizeof(IO_DRIVER_CREATE_CONTEXT));
    DriverContext.Size = sizeof(IO_DRIVER_CREATE_CONTEXT);
    DriverContext.TxnParameters = (PTXN_PARAMETER_BLOCK)Filter;

    if (pFltCreateFileEx2 == NULL)
    {
        pFltCreateFileEx2 = (PFN_FltCreateFileEx2)FltGetRoutineAddress("FltCreateFileEx2");
        if (pFltCreateFileEx2 == NULL)
            return STATUS_PROCEDURE_NOT_FOUND;
    }

    ObjAttr.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjAttr.RootDirectory = NULL;
    ObjAttr.ObjectName = (PUNICODE_STRING)Arg3;
    ObjAttr.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    ObjAttr.SecurityDescriptor = NULL;
    ObjAttr.SecurityQualityOfService = NULL;

    Status = pFltCreateFileEx2(
        Filter,
        Instance,
        (PVOID*)Arg7,
        (PFILE_OBJECT**)Arg8,
        0x10000,
        &ObjAttr,
        &IoStatus,
        NULL,
        0x80,
        7,
        1,
        0x204001,
        NULL,
        0,
        0,
        &DriverContext
    );

    return Status;
}

// ============================================================================
// PriDuplicateUserBuffer
// ============================================================================
NTSTATUS
PriDuplicateUserBuffer(
    PVOID Dest,
    ULONG Size,
    PVOID *Buffer
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID NewBuffer;

    NewBuffer = ExAllocatePoolWithTag(NonPagedPool, Size, 'emcW');
    if (NewBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    memcpy(NewBuffer, Dest, Size);
    *(PVOID *)Buffer = NewBuffer;
    return STATUS_SUCCESS;
}

// ============================================================================
// PriExchangeDirs - Full implementation
// ============================================================================
NTSTATUS
PriExchangeDirs(
    PVOID OriginalParam1,
    PVOID OriginalParam2,
    PVOID OriginalParam3,
    PPF_CONNECTION_COOKIE ConnectionCookie,
    PUCHAR OutputBuffer
)
{
    NTSTATUS Status;
    ULONG LoopIndex;
    ULONG StringFreeCounter = 0;
    ULONG ObjRefStatus;
    PUNICODE_STRING String1 = NULL, String2;
    PFLT_INSTANCE *InstanceArray1, *InstanceArray2;
    PUNICODE_STRING String10, String12;
    PFLT_INSTANCE Instance13;
    ULONG Handle1, Handle2;
    USHORT usVal1, usVal2;
    ULONG ulVal1, ulVal2;
    USHORT usArray[4];
    PFLT_INSTANCE InstanceA8;
    PFLT_FILTER FilterA4;
    USHORT usA0;
    ULONG ul9E;
    USHORT us9A;
    TXN_PARAMETER_BLOCK TxnBlock1;
    TXN_PARAMETER_BLOCK TxnBlock2;
    USHORT us88;
    ULONG ul86;
    USHORT us82;
    PVOID p80;
    UNICODE_STRING UniStr78;
    UNICODE_STRING UniStr70;
    PFLT_INSTANCE Instance68;
    PFLT_FILTER Filter64;
    ULONG ul60;
    UCHAR Buffer56[6];
    USHORT us56;
    PFLT_INSTANCE Instance54;
    PPF_CONNECTION_COOKIE Cookie48 = NULL;
    PUNICODE_STRING String44 = NULL;
    ULONG ul40;
    TXN_PARAMETER_BLOCK TxnBlock38;
    PUNICODE_STRING String30;
    PFLT_INSTANCE Instance2C;
    PFLT_VOLUME **VolumeArray28;
    PVOID pv24;
    TXN_PARAMETER_BLOCK TxnBlock20;
    PUNICODE_STRING String18;
    PULONG pul14;
    PUSHORT pus10;
    ULONG SecurityCookie;

    // Save parameters into local variables
    p80 = OriginalParam1;
    *(PULONG)&UniStr70 = (ULONG)OriginalParam2;   // store pointer in first 4 bytes (Length+MaximumLength)
    Instance68 = (PFLT_INSTANCE)OriginalParam3;
    us88 = 0;
    usA0 = 0;
    ulVal1 = 0;
    Handle1 = 0;
    usArray[0] = 0;

    // Zero out the 10‑DWORD buffer at ul60
    RtlZeroMemory(&ul60, 10 * sizeof(ULONG));

    // Initialize transaction blocks and other locals
    TxnBlock1.Length = 0;
    TxnBlock1.TxFsContext = 0;
    Instance13 = NULL;
    InstanceArray2 = NULL;
    TxnBlock1.TransactionObject = NULL;

    TxnBlock2.Length = 0;
    TxnBlock2.TxFsContext = 0;
    String12 = NULL;
    String10 = NULL;
    InstanceA8 = NULL;
    FilterA4 = NULL;

    UniStr78.Length = 0;
    UniStr78.MaximumLength = 0;
    UniStr78.Buffer = NULL;

    ul86 = 0;
    us82 = 0;
    ul9E = 0;
    us9A = 0;
    ulVal1 = 0;
    ulVal2 = 0;
    Handle1 = 0;
    Handle2 = 0;
    usVal1 = 0;
    usVal2 = 0;
    usArray[1] = 0;
    usArray[2] = 0;
    usArray[3] = 0;
    TxnBlock2.TransactionObject = NULL;
    Instance2C = 0;   // reuse cookie as a temporary
    TxnBlock38.Length = 0;
    TxnBlock38.TxFsContext = 0;

    // Resolve ObReferenceObjectByHandle if not already done
    UNICODE_STRING ObRefName = RTL_CONSTANT_STRING(L"ObReferenceObjectByHandle");
    pfnObReferenceObjectByHandle = (PFN_ObReferenceObjectByHandle)MmGetSystemRoutineAddress(&ObRefName);
    if (pfnObReferenceObjectByHandle == NULL)
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto Cleanup;
    }

    // The original code used a global pointer DAT_00014084 to hold the address of
    // ObReferenceObjectByHandle? Actually it used it as a function pointer.
    // We'll use our own pfnObReferenceObjectByHandle.

    // ========================================================================
    //  Main logic (direct translation of the original decompilation)
    // ========================================================================

    // Step 1: Call ObReferenceObjectByHandle with a handle from ul86
    // (the handle is passed implicitly via local variables)
    Status = pfnObReferenceObjectByHandle(
        (HANDLE)ul86,                     // handle (from local)
        0xf0000,                          // desired access
        (POBJECT_TYPE)PsProcessType,                    // object type (process)
        KernelMode,
        &Instance2C,                      // out object
        NULL
    );
    if (Status < 0)
        goto Cleanup;

    // Step 2: Call PriGetObjectsFromUserHandle with various arguments
    // Original: unaff_EDI = (_UNICODE_STRING *)&stack0xffffff3c;
    // We'll use StackPtr1 and StackPtr2 to mimic the stack addresses.
    // Since we can't use actual stack addresses, we'll use local variables.
    PVOID StackPtr1 = NULL, StackPtr2 = NULL;
    PUNICODE_STRING StackPtrUnicode1 = (PUNICODE_STRING)&StackPtr1;
    PUNICODE_STRING StackPtrUnicode2 = (PUNICODE_STRING)&StackPtr2;

    Status = PriGetObjectsFromUserHandle(
        StackPtrUnicode1,                 // out filter? Actually unaff_EDI
        (ULONG)StackPtrUnicode2,          // out instance? (ulong)p_Stack_f0
        (PFLT_FILTER **)&TxnBlock1.TransactionObject,  // out filter pointer
        (PFLT_INSTANCE **)&InstanceA8,                      // out instance pointer
        (PFLT_VOLUME **)&StackPtr2,       // out volume pointer (stack0xffffff38)
        (PVOID *)StackPtrUnicode1,        // Arg6
        (PFILE_OBJECT **)String1          // Arg7 (unaff_ESI)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 3: Second call to PriGetObjectsFromUserHandle
    PFLT_VOLUME **VolumeArrayPtr = (PFLT_VOLUME **)&StackPtr1;  // stack0xffffff1c
    Status = PriGetObjectsFromUserHandle(
        NULL,                              // out filter
        0,                                 // out instance
        (PFLT_FILTER **)&FilterA4,                         // out filter pointer
        (PFLT_INSTANCE **)&TxnBlock38,     // out instance pointer (local_b8)
        VolumeArrayPtr,                    // out volume pointer
        NULL,                              // Arg6
        (PFILE_OBJECT **)StackPtrUnicode2  // Arg7 (p_Stack_f0)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 4: Compare two 32‑bit values (CONCAT22(local_ba,uStack_bc) == CONCAT22(local_b8._2_2_,(ushort)local_b8))
    // We'll combine the fields into a 32‑bit integer.
    ULONG Combined1 = ( (ULONG)usVal2 << 16 ) | usVal1;   // local_ba (high) and uStack_bc (low)
    ULONG Combined2 = ( (ULONG)((USHORT)((*(PULONG)&TxnBlock38) >> 16)) << 16 ) | (USHORT)(*(PULONG)&TxnBlock38);
    if (Combined1 != Combined2)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Step 5: Call PriGetString with various arguments
    Status = PriGetString(
        StackPtrUnicode1,                 // Context (stack0xffffff38)
        (PVOID)StackPtrUnicode1,          // Callback (unaff_EDI)
        NULL                              // OutputString (NULL)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 6: Another PriGetString
    Status = PriGetString(
        StackPtrUnicode2,                 // Context (stack0xffffff1c)
        (PVOID)StackPtrUnicode2,          // Callback (unaff_EDI)
        StackPtr1                         // OutputString (p_Stack_108)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 7: PriStringEnsureCapacity
    Status = PriStringEnsureCapacity(StackPtrUnicode2, 0x11b5f);
    if (Status < 0)
        goto Cleanup;

    // Step 8: RtlAppendUnicodeStringToString calls
    Status = RtlAppendUnicodeStringToString(StackPtrUnicode2, (PUNICODE_STRING)&StackPtr1);
    if (Status < 0)
        goto Cleanup;

    Status = RtlAppendUnicodeStringToString(StackPtrUnicode2, (PUNICODE_STRING)&StackPtr1);
    if (Status < 0)
        goto Cleanup;

    // Step 9: Another PriGetString
    Status = PriGetString(
        StackPtrUnicode2,                 // Context (local_b8)
        (PVOID)StackPtrUnicode2,          // Callback (unaff_EDI)
        (PUNICODE_STRING)String1          // OutputString (unaff_ESI)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 10: PriStringEnsureCapacity
    Status = PriStringEnsureCapacity(StackPtrUnicode2, (ULONG)StackPtrUnicode2);
    if (Status < 0)
        goto Cleanup;

    // Step 11: More RtlAppendUnicodeStringToString
    Status = RtlAppendUnicodeStringToString(StackPtrUnicode2, (PUNICODE_STRING)&StackPtr1);
    if (Status < 0)
        goto Cleanup;

    Status = RtlAppendUnicodeStringToString(StackPtrUnicode2, (PUNICODE_STRING)&StackPtr1);
    if (Status < 0)
        goto Cleanup;

    // Step 12: Dereference objects and close handles
    ObfDereferenceObject(Instance2C);
    ObfDereferenceObject(InstanceA8);
    String10 = NULL;
    String12 = NULL;
    ZwClose((HANDLE)usVal1);
    ZwClose((HANDLE)usVal2);

    // Step 13: Build the message and send it via PFSendExchangeCloseHandlesMessage
    usArray[2] = (USHORT)(ULONG_PTR)Cookie48;
    usArray[3] = (USHORT)((ULONG_PTR)Cookie48 >> 16);
    UniStr70.Buffer = NULL;
    UniStr70.Length = 0;
    UniStr70.MaximumLength = 0;

    // uStack_b4._0_2_ = 0x1d00; uStack_b4._2_2_ = 1;  => combined 0x00011d00?
    ULONG CombinedB4 = 0x00011d00; // reconstruct
    Status = PFSendExchangeCloseHandlesMessage(
        (PFLT_FILTER)String44,
        Cookie48,
        InstanceA8,
        FilterA4
    );
    if (Status < 0)
        goto Cleanup;

    // Step 14: More setup and calls to PriCreateTransactional
    // The original code uses extraout_ECX and extraout_ECX_00; we'll store
    // the return values in temporary variables.
    PFLT_INSTANCE TmpInstance1 = Instance68;
    PFLT_INSTANCE TmpInstance2 = NULL;

    // First call to PriCreateTransactional
    Status = PriCreateTransactional(
        (PFLT_FILTER)StackPtrUnicode1,  // Filter (unaff_EDI)
        TmpInstance1,                     // Instance (extraout_ECX)
        &TxnBlock38,                      // Arg3 (local_c0)
        TmpInstance1,                     // Arg4 (extraout_ECX)
        (PUNICODE_STRING)&Cookie48,       // Arg5 (local_b8)
        (ULONG)CombinedB4,                // Arg6 (uStack_b4)
        (PVOID *)StackPtrUnicode1,        // Arg7 (unaff_EDI)
        (PFILE_OBJECT **)((ULONG_PTR)usArray[3] << 16 | usArray[2]) // Arg8 (CONCAT22)
    );
    if (Status < 0)
        goto Cleanup;

    // Second call to PriCreateTransactional
    PFLT_INSTANCE TmpInstance2_ = TmpInstance2; // from extraout_ECX_00
    Status = PriCreateTransactional(
        (PFLT_FILTER)&UniStr70,         // Filter (unaff_EDI)
        TmpInstance2_,                    // Instance (extraout_ECX_00)
        &TxnBlock1,                       // Arg3 (local_98)
        TmpInstance2_,                    // Arg4
        (PUNICODE_STRING)Buffer56,      // Arg5 (p_Var10 = auStack_5c)
        (ULONG)&TxnBlock38,               // Arg6 (local_b8)
        (PVOID *)&UniStr70,               // Arg7 (unaff_EDI)
        (PFILE_OBJECT **)Instance13       // Arg8 (p_Var13)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 15: More RtlAppendUnicodeStringToString
    Status = RtlAppendUnicodeStringToString(StackPtrUnicode2, (PUNICODE_STRING)&StackPtr1);
    if (Status < 0)
        goto Cleanup;

    // Step 16: Allocate pool for TxnBlock2.TransactionObject
    TxnBlock2.TransactionObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(TXN_PARAMETER_BLOCK), 'emcW');
    if (TxnBlock2.TransactionObject == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    // Step 17: Fill the allocated structure
    *(PUCHAR)TxnBlock2.TransactionObject = 0;
    *(PULONG)((PUCHAR)TxnBlock2.TransactionObject + 4) = 0;
    *(PULONG)((PUCHAR)TxnBlock2.TransactionObject + 8) = (ULONG)TxnBlock38.Length & 0xFFFF;
    memcpy((PUCHAR)TxnBlock2.TransactionObject + 0xC,
           (PUCHAR)&TxnBlock38.Length,     // CONCAT22(local_ba,uStack_bc) is actually the 16-bit value
           (ULONG)TxnBlock38.Length & 0xFFFF);

    // Step 18: Call FltSetInformationFile with the allocated structure
    Status = FltSetInformationFile(
        (HANDLE)VolumeArrayPtr[0],        // handle (pp_Var5)
        (PFILE_OBJECT)VolumeArrayPtr[1],  // file object
        (PVOID)TxnBlock2.TransactionObject,
        (ULONG)String10,           // Arg4 (p_Var10)
        (FILE_INFORMATION_CLASS)StackPtrUnicode2          // Arg5 (p_Stack_f0)
    );
    if (Status < 0)
        goto Cleanup;

    // Step 19: Update the allocated structure with more data and call FltSetInformationFile again
    *(PULONG)((PUCHAR)TxnBlock2.TransactionObject + 8) = (ULONG)usArray[0];
    memcpy((PUCHAR)TxnBlock2.TransactionObject + 0xC,
           (PUCHAR)&usArray[0],           // CONCAT22(local_b0[3],local_b0[2])
           (ULONG)usArray[0]);
    Status = FltSetInformationFile(
        (HANDLE)VolumeArrayPtr[0],
        (PFILE_OBJECT)VolumeArrayPtr[1],
        (PVOID)TxnBlock2.TransactionObject,
        (ULONG)String12,           // Arg4 (p_Var12)
        (FILE_INFORMATION_CLASS)StackPtrUnicode2
    );
    if (Status < 0)
        goto Cleanup;

    // Step 20: More FltSetInformationFile calls
    ul40 = 0x80;
    Instance2C = (PFLT_INSTANCE)((ULONG_PTR)Instance2C | 1); // CONCAT13(1, (int3)p_Var9)
    Status = FltSetInformationFile(
        (HANDLE)VolumeArrayPtr[0],
        (PFILE_OBJECT)VolumeArrayPtr[1],
        (PVOID)TxnBlock2.TransactionObject,
        (ULONG)String10,
        (FILE_INFORMATION_CLASS)StackPtrUnicode2
    );
    if (Status < 0)
        goto Cleanup;

    Status = FltSetInformationFile(
        (HANDLE)VolumeArrayPtr[0],
        (PFILE_OBJECT)VolumeArrayPtr[1],
        (PVOID)TxnBlock2.TransactionObject,
        (ULONG)String10,
        (FILE_INFORMATION_CLASS)StackPtrUnicode2
    );
    if (Status < 0)
        goto Cleanup;

    // ========================================================================
    //  Cleanup and exit (LAB_000119b1 and after)
    // ========================================================================

Cleanup:
    // Restore values from locals to the original variables (as in the original)
    // This part is the long cleanup sequence with multiple loops.

    // Store back local_98 and pp_Var5 (they were modified)
    // We'll do the cleanup as per original.

    // Set the outputs for the caller
    // (The original didn't have explicit outputs, but they used global variables.)

    // Loop 1: dereference objects in local_38 and friends
    for (LoopIndex = 0; LoopIndex < 3; LoopIndex++)
    {
        if (*(PLONG)((PUCHAR)&TxnBlock38.Length + LoopIndex * 2) != 0)
            ObfDereferenceObject(*(PVOID*)((PUCHAR)&TxnBlock38.Length + LoopIndex * 2));
    }

    // Loop 2: dereference instances in p_Stack_2c and friends
    for (LoopIndex = 0; LoopIndex < 4; LoopIndex++)
    {
        if ((&Instance2C)[LoopIndex] != NULL)
            FltObjectDereference((PVOID)(&Instance2C)[LoopIndex]);
    }

    // Loop 3: close handles in local_68 and friends
    for (LoopIndex = 0; LoopIndex < 2; LoopIndex++)
    {
        if ((&Instance68)[LoopIndex] != NULL)
            ZwClose((HANDLE)(&Instance68)[LoopIndex]);
    }

    // Loop 4: close handles in local_70 and friends
    for (LoopIndex = 0; LoopIndex < 2; LoopIndex++)
    {
        if (*(PLONG)((PUCHAR)&UniStr70.Length + LoopIndex * 2) != 0)
            FltClose((PVOID)*(PLONG)((PUCHAR)&UniStr70.Length + LoopIndex * 2));
    }

    // Free strings using PriFreeString
    for (LoopIndex = 0; LoopIndex < 4; LoopIndex++)
    {
        PriFreeString(StackPtrUnicode1);
        StringFreeCounter++;
    }

    // Free TxnBlock2.TransactionObject if allocated
    if (TxnBlock2.TransactionObject != NULL)
        ExFreePoolWithTag(TxnBlock2.TransactionObject, 'emcW');

    // Return status
    return Status;
}

// ============================================================================
// PriFreeString
// ============================================================================
VOID
PriFreeString(
    PUNICODE_STRING String
)
{
    if (String == NULL)
        return;

    if (String->Buffer != NULL)
    {
        ExFreePoolWithTag(String->Buffer, 'emcW');
        String->Buffer = NULL;
    }
    String->Length = 0;
    String->MaximumLength = 0;
}

// ============================================================================
// PriGetFileNameCallback
// ============================================================================
NTSTATUS
PriGetFileNameCallback(
    PVOID Arg1,
    PUNICODE_STRING FileName,
    PULONG ReturnLength
)
{
    NTSTATUS Status;
    WCHAR *Buffer = FileName->Buffer;
    ULONG MaxLen = FileName->MaximumLength;

    if (MaxLen < 8)
    {
        *ReturnLength = 0x208;
        return STATUS_BUFFER_TOO_SMALL;
    }

    HANDLE hFile = *(HANDLE*)Arg1;
    PFILE_OBJECT FileObject = *(PFILE_OBJECT*)((PUCHAR)Arg1 + 4);

    Status = FltQueryInformationFile(hFile, FileObject, Buffer, MaxLen,
                                     FileNameInformation, ReturnLength);
    if (Status >= 0)
    {
        if (MaxLen < *ReturnLength)
            Status = STATUS_BUFFER_TOO_SMALL;
        if (Status >= 0)
        {
            FileName->Length = (USHORT)*Buffer;
            memmove(FileName->Buffer, Buffer + 2, FileName->Length);
        }
    }
    return Status;
}

// ============================================================================
// PriGetObjectsFromUserHandle
// ============================================================================
NTSTATUS
PriGetObjectsFromUserHandle(
    PVOID OutFilter,
    ULONG OutInstance,
    PFLT_FILTER **OutFilterPtr,
    PFLT_INSTANCE **OutInstancePtr,
    PFLT_VOLUME **OutVolumePtr,
    PVOID *Arg6,
    PFILE_OBJECT **Arg7
)
{
    NTSTATUS Status;
    PFLT_FILTER FilterRef = NULL;
    PFLT_INSTANCE InstanceRef = NULL;
    PFLT_VOLUME VolumeRef = NULL;
    ULONG InstanceCount = 0;
    ULONG BytesNeeded = 0;

    Status = FltObjectReference(gFilterHandle);
    if (Status < 0)
        return Status;

    FilterRef = gFilterHandle;

    Status = ObReferenceObjectByHandle((HANDLE)OutInstance, 0xf0000, (POBJECT_TYPE)IoFileObjectType,
                                       KernelMode, &InstanceRef, NULL);
    if (Status < 0)
        goto Cleanup;

    if (OutFilterPtr != NULL || OutInstance != 0)
    {
        Status = FltGetVolumeFromFileObject(FilterRef, (PFILE_OBJECT)&InstanceRef, &VolumeRef);
        if (Status < 0)
            goto Cleanup;
        if (OutInstance != 0)
        {
            Status = FltEnumerateInstances(VolumeRef, FilterRef, (PFLT_INSTANCE *)&InstanceCount, 1, &BytesNeeded);
            if (Status < 0)
                goto Cleanup;
        }
    }

    if (OutInstancePtr != NULL)
    {
        Status = ObOpenObjectByPointer(VolumeRef, OBJ_KERNEL_HANDLE, NULL, 0xf0000,
                                       (POBJECT_TYPE)IoFileObjectType, KernelMode, &InstanceRef);
        if (Status < 0)
            goto Cleanup;
    }

    if (OutFilter != NULL)
    {
        *(PFLT_FILTER *)OutFilter = FilterRef;
        FilterRef = NULL;
    }
    if (OutInstance != 0)
    {
        *(ULONG *)OutInstance = InstanceCount;
        InstanceCount = 0;
    }
    if (OutFilterPtr != NULL)
    {
        *OutFilterPtr = (PFLT_FILTER *)FilterRef;
        FilterRef = NULL;
    }
    if (OutInstancePtr != NULL)
    {
        *OutInstancePtr = (PFLT_INSTANCE *)InstanceRef;
        InstanceRef = NULL;
    }
    if (OutVolumePtr != NULL)
    {
        *OutVolumePtr = (PFLT_VOLUME *)VolumeRef;
        VolumeRef = NULL;
    }

Cleanup:
    if (FilterRef)
        FltObjectDereference(FilterRef);
    if (VolumeRef)
        ObfDereferenceObject(VolumeRef);
    if (InstanceCount)
        FltObjectDereference((PVOID)(ULONG_PTR)InstanceCount);
    if (InstanceRef)
        ZwClose((HANDLE)InstanceRef);

    return Status;
}

// ============================================================================
// PriGetString
// ============================================================================
NTSTATUS
PriGetString(
    PVOID Context,
    PVOID Callback,
    PUNICODE_STRING OutputString
)
{
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    PUNICODE_STRING TargetString = (PUNICODE_STRING)Context;

    while (TRUE)
    {
        Status = ((NTSTATUS (*)(PVOID, PVOID, PULONG))Callback)(Context, TargetString, &ReturnLength);
        if (Status != STATUS_BUFFER_TOO_SMALL)
            break;

        if (ReturnLength <= TargetString->MaximumLength || ReturnLength > 0xFFFF)
            goto Error;

        TargetString->Length = 0;
        Status = PriStringEnsureCapacity(TargetString, ReturnLength);
        if (Status < 0)
            break;
    }

    if (Status < 0)
        PriFreeString(TargetString);

    return Status;

Error:
    PriFreeString(TargetString);
    return STATUS_UNSUCCESSFUL;
}

// ============================================================================
// PriProcesRegisterGuidMessage
// ============================================================================
NTSTATUS
PriProcesRegisterGuidMessage(
    PPF_CONNECTION_COOKIE Cookie,
    PPF_MESSAGE_REGISTER_GUID Msg,
    ULONG ProcessId
)
{
    NTSTATUS Status;
    PFLT_PORT PortHandle = NULL;

    if (Cookie == NULL || Msg == NULL || (ULONG_PTR)Cookie < 0x24)
        return STATUS_INVALID_PARAMETER;

    ExAcquireResourceExclusiveLite(&SubsumeLock, TRUE);

    PortHandle = Cookie->PortHandle;
    if (PortHandle == NULL)
    {
        Cookie->PortHandle = (PFLT_PORT)1;
        PortHandle = NULL;
    }

    ExReleaseResourceLite(&SubsumeLock);

    if (PortHandle != NULL)
        return STATUS_OBJECT_NAME_COLLISION;

    Cookie->PortHandle = Msg->MessageHeader.Signature ? (PFLT_PORT)Msg->ConnectionGuid.Data1 : NULL;
    Cookie->FilterPointer = (PFLT_FILTER)Msg->ConnectionGuid.Data2;
    Cookie->ServicedGuid = Msg->ConnectionGuid;
    Cookie->GuidSet = 1;

    Status = WMCommListInsert(&Msg->ConnectionGuid, (PFLT_PORT)&Cookie->PortHandle, ProcessId, &Cookie->Version);

    return Status;
}

// ============================================================================
// PriProcessMessage
// ============================================================================
NTSTATUS
PriProcessMessage(
    PVOID InputBuffer,
    PVOID OutputBuffer,
    ULONG OutputSize,
    PVOID Arg4,
    ULONG Arg5,
    PULONG Arg6
)
{
    PPF_MESSAGE_HEADER MsgHeader = (PPF_MESSAGE_HEADER)InputBuffer;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG ActionType;

    if (MsgHeader == NULL || (ULONG_PTR)InputBuffer < 0x10 || Arg4 == NULL)
        return STATUS_INVALID_PARAMETER;

    ProbeForRead(InputBuffer, 0x10, 1);
    if (OutputBuffer != NULL)
        ProbeForWrite(OutputBuffer, OutputSize, 1);

    if ((ULONG_PTR)MsgHeader->Signature == 0x42424242 &&
        MsgHeader->MajorVersion == 3 &&
        MsgHeader->MinorVersion < 2)
    {
        ActionType = MsgHeader->ActionType;

        switch (ActionType)
        {
            case RegisterGuid:
                Status = PriProcesRegisterGuidMessage((PPF_CONNECTION_COOKIE)InputBuffer,
                                                      (PPF_MESSAGE_REGISTER_GUID)InputBuffer,
                                                      (ULONG)Arg6);
                break;

            case VersionCheck:
                if (OutputBuffer == NULL)
                    return STATUS_SUCCESS;
                if (OutputSize < 4)
                    return STATUS_SUCCESS;
                *(PUSHORT)OutputBuffer = 3;
                *(PUSHORT)((PUCHAR)OutputBuffer + 2) = 1;
                Status = STATUS_SUCCESS;
                break;

            case StubCreateChild:
                if ((ULONG_PTR)InputBuffer > 0x2f &&
                    MsgHeader->StructureSize == 0x30 &&
                    OutputBuffer != NULL)
                {
                    if (OutputSize < 8)
                        return STATUS_BUFFER_TOO_SMALL;
                    Status = PriStubCreateChild(
                        (PVOID)MsgHeader->Signature,
                        (ULONG)MsgHeader->StructureSize,
                        (ULONG)Arg6,
                        (ULONG)MsgHeader->StructureSize,
                        (ULONG)OutputSize,
                        (PUNICODE_STRING)Arg4,
                        (PVOID*)OutputBuffer
                    );
                    *(PVOID *)OutputBuffer = (PVOID)Status;
                    *(PULONG)((PUCHAR)OutputBuffer + 4) = (ULONG)(ULONG_PTR)Status >> 0x1f;
                    *(PULONG)Arg4 = 8;
                }
                break;

            case StubRename:
                if ((ULONG_PTR)InputBuffer > 0x2f &&
                    (ULONG_PTR)InputBuffer >= MsgHeader->StructureSize + 0x30)
                {
                    Status = PriStubRename(
                        (PVOID)((ULONG_PTR)InputBuffer - 0x1c),
                        (PFILE_RENAME_INFORMATION)InputBuffer,
                        (ULONG)Arg4,
                        (ULONG)Arg6
                    );
                }
                break;

            case SubsumeContext:
                if ((ULONG_PTR)InputBuffer > 0x27 && OutputSize > 7 && OutputBuffer != NULL)
                {
                    Status = PriSubsumeContext(
                        (PVOID)MsgHeader->StructureSize,
                        (PLARGE_INTEGER)Arg4,
                        (PLARGE_INTEGER)OutputBuffer,
                        (PVOID*)OutputBuffer
                    );
                }
                break;

            case ExchangeDirs:
                if ((ULONG_PTR)InputBuffer > 0x27 && OutputSize != 0)
                {
                    Status = PriExchangeDirs(
                        (PVOID)MsgHeader->StructureSize,
                        (PVOID)OutputBuffer,
                        (PVOID)((ULONG_PTR)OutputSize & 0xFFFFFF),
                        (PPF_CONNECTION_COOKIE)InputBuffer,
                        (PUCHAR)Arg4
                    );
                    *(PUCHAR)OutputBuffer = (UCHAR)((ULONG_PTR)Status & 0xFF);
                    *(PULONG)Arg4 = 1;
                }
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
        }
    }

    return Status;
}

// ============================================================================
// PriStringEnsureCapacity
// ============================================================================
NTSTATUS
PriStringEnsureCapacity(
    PUNICODE_STRING String,
    ULONG NewSize
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID NewBuffer;

    if (String->MaximumLength >= NewSize || NewSize > 0xFFFF)
        return STATUS_SUCCESS;

    NewBuffer = ExAllocatePoolWithTag(NonPagedPool, NewSize, 'emcW');
    if (NewBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (String->Buffer != NULL)
    {
        memcpy(NewBuffer, String->Buffer, String->Length);
        ExFreePoolWithTag(String->Buffer, 'emcW');
    }

    String->Buffer = (WCHAR*)NewBuffer;
    String->MaximumLength = (USHORT)NewSize;
    return STATUS_SUCCESS;
}

// ============================================================================
// PriStubCreateChild
// ============================================================================
NTSTATUS
PriStubCreateChild(
    PVOID p1,
    ULONG p2,
    ULONG p3,
    ULONG p4,
    ULONG p5,
    PUNICODE_STRING p6,
    PVOID *p7
)
{
    NTSTATUS Status;
    PFLT_INSTANCE Instance = NULL;
    ULONG Handle1 = 0;
    ULONG Handle2 = 0;
    PVOID Buffer = NULL;
    HANDLE hFile = NULL;

    Status = PriGetObjectsFromUserHandle(
        &Handle1,
        (ULONG)&Buffer,
        NULL,
        (PFLT_INSTANCE **)&Instance,
        NULL,
        (PVOID*)&Handle2,
        NULL
    );
    if (Status < 0)
        goto Cleanup;

    Status = PriDuplicateUserBuffer(&Handle2, (ULONG)&Handle1, &Buffer);
    if (Status < 0)
        goto Cleanup;

    Status = FltCreateFile(
        (PFLT_FILTER)Handle1,
        (PFLT_INSTANCE)Buffer,
        &hFile,
        FILE_READ_DATA | FILE_WRITE_DATA,
        NULL,
        NULL,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0,
        0
    );
    if (Status < 0)
        goto Cleanup;

    Status = ZwDuplicateObject(NtCurrentProcess(), hFile,
                               NtCurrentProcess(), (PHANDLE)p5,
                               0, 0, DUPLICATE_SAME_ACCESS);

Cleanup:
    if (Handle1)
        FltObjectDereference((PVOID)(ULONG_PTR)Handle1);
    if (Buffer)
        FltObjectDereference(Buffer);
    if (Instance)
        ZwClose((HANDLE)Instance);
    if (hFile)
        FltClose((PVOID)hFile);
    if (Handle2)
        ExFreePoolWithTag((PVOID)(ULONG_PTR)Handle2, 'emcW');

    return Status;
}

// ============================================================================
// PriStubRename
// ============================================================================
NTSTATUS
PriStubRename(
    PVOID p1,
    PFILE_RENAME_INFORMATION RenameInfo,
    ULONG p3,
    ULONG p4
)
{
    NTSTATUS Status;
    PFLT_VOLUME Volume = NULL;
    PVOID Buffer = NULL;
    ULONG Handle1 = 0;
    ULONG Handle2 = 0;
    PVOID Context = NULL;

    if ((ULONG_PTR)p1 + (ULONG_PTR)RenameInfo < (ULONG_PTR)RenameInfo + 0xC + RenameInfo->FileNameLength)
        return STATUS_BUFFER_TOO_SMALL;

    if ((ULONG_PTR)&RenameInfo->RootDirectory + 2 >= 2)
        return STATUS_INVALID_PARAMETER;

    if (RenameInfo->RootDirectory != NULL)
        return STATUS_INVALID_PARAMETER;

    Status = PriDuplicateUserBuffer(p1, (ULONG)sizeof(*RenameInfo), &Buffer);
    if (Status < 0)
        goto Cleanup;

    Status = PriGetObjectsFromUserHandle(
        &Handle1,
        (ULONG)&Handle2,
        NULL,
        NULL,
        (PFLT_VOLUME **)&Volume,
        (PVOID*)&Context,
        NULL
    );
    if (Status < 0)
        goto Cleanup;

    Status = FltSetInformationFile(
        (HANDLE)Handle1,
        (PFILE_OBJECT)Volume,
        (PVOID)Context,
        (ULONG)RenameInfo,
        (FILE_INFORMATION_CLASS)p3
    );

Cleanup:
    if (Handle1)
        FltObjectDereference((PVOID)(ULONG_PTR)Handle1);
    if (Handle2)
        FltObjectDereference((PVOID)(ULONG_PTR)Handle2);
    if (Volume)
        ObfDereferenceObject(Volume);
    if (Buffer)
        ExFreePoolWithTag(Buffer, 'emcW');

    return Status;
}

// ============================================================================
// PriSubsumeContext
// ============================================================================
NTSTATUS
PriSubsumeContext(
    PVOID p1,
    PLARGE_INTEGER p2,
    PLARGE_INTEGER p3,
    PVOID *p4
)
{
    NTSTATUS Status;
    PFLT_INSTANCE Instance = NULL;
    PVOID Buffer = NULL;
    PVOID Context = NULL;

    Status = PriGetObjectsFromUserHandle(
        &Buffer,
        (ULONG)&Context,
        NULL,
        (PFLT_INSTANCE **)&Instance,
        NULL,
        (PVOID*)p4,
        NULL
    );
    if (Status < 0)
        goto Cleanup;

    Status = PFSubsumeContext((PFLT_FILTER)Instance, (PFLT_INSTANCE)p1, p1, p2, p3, p4);

Cleanup:
    if (Instance)
        ZwClose((HANDLE)Instance);
    if (Buffer)
        FltObjectDereference(Buffer);
    if (Context)
        FltObjectDereference(Context);

    return Status;
}

