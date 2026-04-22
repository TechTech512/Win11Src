#pragma warning (disable:4996)

#include <ntddk.h>
#include <wdm.h>

// Microcode update lock structure
typedef struct _MICROCODE_UPDATE_LOCK {
    FAST_MUTEX LockMutex;
    ULONG WaitCount;
    KEVENT WaitEvent;
} MICROCODE_UPDATE_LOCK, *PMICROCODE_UPDATE_LOCK;

// Global variables
MICROCODE_UPDATE_LOCK ExpMicrocodeUpdateLock;
PVOID ExpMicrocodeImageHandle = NULL;
LONG ExpUserModeCallerCount = 0;

// Forward declarations
void ExpAcquireMicrocodeUpdateLock(PMICROCODE_UPDATE_LOCK Lock);
void ExpReleaseMicrocodeUpdateLock(PMICROCODE_UPDATE_LOCK Lock);
void ExpInitializeMicrocodeUpdateLock(PMICROCODE_UPDATE_LOCK Lock);
NTSTATUS ExpGetMicrocodeImageFileName(PUNICODE_STRING FileName);
NTSTATUS ExpLoadMicrocodeImage(void);
NTSTATUS ExpUnloadMicrocodeImage(void);
NTSTATUS ExpMicrocodeInformationLoad(void);
NTSTATUS ExpMicrocodeInformationUnload(void);
NTSTATUS ExpMicrocodeInitialization(ULONG Phase);
void ExpMicrocodePowerStateCallback(PVOID CallbackContext, PVOID Argument1, PVOID Argument2);
void ExpWorkMicrocode(PVOID Context);

// External function pointers (provided by external source)
NTSTATUS (*g_MicrocodeImageInit)(PVOID ImageHandle) = NULL;
NTSTATUS (*g_MicrocodeImageCleanup)(void) = NULL;
VOID (*g_MicrocodeImageNotify)(void) = NULL;
PULONG g_PrivilegeValues = NULL;

// Acquire microcode update lock
void ExpAcquireMicrocodeUpdateLock(
    PMICROCODE_UPDATE_LOCK Lock
)
{
    BOOLEAN hasWaiters;
    
    KeAcquireGuardedMutex(&ExpMicrocodeUpdateLock.LockMutex);
    hasWaiters = (ExpMicrocodeUpdateLock.WaitCount != 0);
    ExpMicrocodeUpdateLock.WaitCount++;
    KeReleaseGuardedMutex(&ExpMicrocodeUpdateLock.LockMutex);
    
    if (hasWaiters) {
        KeWaitForSingleObject(&ExpMicrocodeUpdateLock.WaitEvent, Executive, KernelMode, FALSE, NULL);
    }
}

// Release microcode update lock
void ExpReleaseMicrocodeUpdateLock(
    PMICROCODE_UPDATE_LOCK Lock
)
{
    KeAcquireGuardedMutex(&ExpMicrocodeUpdateLock.LockMutex);
    ExpMicrocodeUpdateLock.WaitCount--;
    if (ExpMicrocodeUpdateLock.WaitCount != 0) {
        KeSetEvent(&ExpMicrocodeUpdateLock.WaitEvent, 0, FALSE);
    }
    KeReleaseGuardedMutex(&ExpMicrocodeUpdateLock.LockMutex);
}

// Initialize microcode update lock
void ExpInitializeMicrocodeUpdateLock(
    PMICROCODE_UPDATE_LOCK Lock
)
{
    KeInitializeGuardedMutex(&ExpMicrocodeUpdateLock.LockMutex);
    ExpMicrocodeUpdateLock.WaitCount = 0;
    KeInitializeEvent(&ExpMicrocodeUpdateLock.WaitEvent, NotificationEvent, FALSE);
}

// Get microcode image file name
NTSTATUS ExpGetMicrocodeImageFileName(
    PUNICODE_STRING FileName
)
{
    NTSTATUS status;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING fullPath;
    PWCHAR buffer;
    ULONG length;
    UCHAR vendor[16];
    UINT32 cpuInfo[4];
    
    RtlInitUnicodeString(&fullPath, NULL);
    RtlInitUnicodeString(&unicodeString, NULL);
	
	memset(vendor, 0, sizeof(vendor));
    
    // Get processor vendor using CPUID
    __cpuid(cpuInfo, 0);
    *(UINT32*)(vendor) = cpuInfo[1];
    *(UINT32*)(vendor + 4) = cpuInfo[3];
    *(UINT32*)(vendor + 8) = cpuInfo[2];
    vendor[12] = 0;
    
    RtlInitAnsiString(&ansiString, (PCSZ)vendor);
    
    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    length = unicodeString.Length + 0x46;
    buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, length, 'CodU');
    if (!buffer) {
        RtlFreeUnicodeString(&unicodeString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(buffer, length);
    RtlInitUnicodeString(&fullPath, buffer);
    fullPath.MaximumLength = (USHORT)length;
    
    status = RtlAppendUnicodeToString(&fullPath, L"\\SystemRoot\\System32\\mcupdate_");
    if (NT_SUCCESS(status)) {
        status = RtlAppendUnicodeStringToString(&fullPath, &unicodeString);
        if (NT_SUCCESS(status)) {
            status = RtlAppendUnicodeToString(&fullPath, L".dll");
            if (NT_SUCCESS(status)) {
                FileName->Buffer = fullPath.Buffer;
                FileName->Length = fullPath.Length;
                FileName->MaximumLength = fullPath.MaximumLength;
            }
        }
    }
    
    RtlFreeUnicodeString(&unicodeString);
    
    if (!NT_SUCCESS(status) && buffer) {
        ExFreePoolWithTag(buffer, 'CodU');
    }
    
    return status;
}

// Load microcode image
NTSTATUS ExpLoadMicrocodeImage(void)
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    UNICODE_STRING fallbackName;
    PVOID imageHandle;
    PVOID unused;
    
    imageHandle = NULL;
    RtlInitUnicodeString(&fileName, NULL);
    
    if (ExpMicrocodeImageHandle != NULL) {
        return STATUS_DEVICE_ALREADY_ATTACHED;
    }
    
    status = ExpGetMicrocodeImageFileName(&fileName);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    RtlInitUnicodeString(&fallbackName, L"mcupdate.dll");
    status = ZwLoadDriver(&fileName);
    
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }
        goto cleanup;
    }
    
    if (g_MicrocodeImageInit) {
        status = g_MicrocodeImageInit(imageHandle);
        if (!NT_SUCCESS(status)) {
            goto unload;
        }
    }
    
    ExpMicrocodeImageHandle = imageHandle;
    if (g_MicrocodeImageNotify) {
        g_MicrocodeImageNotify();
    }
    goto cleanup;
    
unload:
    if (imageHandle) {
        ZwUnloadDriver(imageHandle);
    }
    
cleanup:
    if (fileName.Buffer) {
        ExFreePoolWithTag(fileName.Buffer, 'CodU');
    }
    
    return status;
}

// Unload microcode image
NTSTATUS ExpUnloadMicrocodeImage(void)
{
    NTSTATUS status;
    
    if (!ExpMicrocodeImageHandle) {
        return STATUS_SUCCESS;
    }
    
    if (g_MicrocodeImageCleanup) {
        status = g_MicrocodeImageCleanup();
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
    
    status = ZwUnloadDriver(ExpMicrocodeImageHandle);
    if (NT_SUCCESS(status)) {
        ExpMicrocodeImageHandle = NULL;
    }
    
    return status;
}

// Microcode information load
NTSTATUS ExpMicrocodeInformationLoad(void)
{
    NTSTATUS status;
    BOOLEAN isUserMode;
    KEVENT event;
    WORK_QUEUE_ITEM workItem;
    BOOLEAN hasPrivilege;
    LUID luid;
    
    isUserMode = (ExGetPreviousMode() != KernelMode);
    
    if (isUserMode) {
        luid.LowPart = g_PrivilegeValues[0];
        luid.HighPart = g_PrivilegeValues[1];
        hasPrivilege = SeSinglePrivilegeCheck(luid, UserMode);
        
        if (!hasPrivilege) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
        
        if (ExpUserModeCallerCount != 0) {
            return STATUS_ALREADY_COMMITTED;
        }
    }
    
    ExpUserModeCallerCount++;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    workItem.WorkerRoutine = ExpWorkMicrocode;
    workItem.Parameter = &ExpLoadMicrocodeImage;
    
    ExQueueWorkItem(&workItem, CriticalWorkQueue);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    
    status = (*(NTSTATUS (__cdecl *)(void))workItem.Parameter)();
    
    if (isUserMode) {
        ExpUserModeCallerCount--;
    }
    
    return status;
}

// Microcode information unload
NTSTATUS ExpMicrocodeInformationUnload(void)
{
    NTSTATUS status;
    BOOLEAN isUserMode;
    KEVENT event;
    WORK_QUEUE_ITEM workItem;
    BOOLEAN hasPrivilege;
    LUID luid;
    
    isUserMode = (ExGetPreviousMode() != KernelMode);
    
    if (isUserMode) {
        luid.LowPart = g_PrivilegeValues[0];
        luid.HighPart = g_PrivilegeValues[1];
        hasPrivilege = SeSinglePrivilegeCheck(luid, UserMode);
        
        if (!hasPrivilege) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
        
        if (ExpUserModeCallerCount != 0) {
            return STATUS_ALREADY_COMMITTED;
        }
    }
    
    ExpUserModeCallerCount++;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    workItem.WorkerRoutine = ExpWorkMicrocode;
    workItem.Parameter = &ExpUnloadMicrocodeImage;
    
    ExQueueWorkItem(&workItem, CriticalWorkQueue);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    
    status = (*(NTSTATUS (__cdecl *)(void))workItem.Parameter)();
    
    if (isUserMode) {
        ExpUserModeCallerCount--;
    }
    
    return status;
}

// Microcode initialization
NTSTATUS ExpMicrocodeInitialization(
    ULONG Phase
)
{
    NTSTATUS status;
    UNICODE_STRING callbackName;
    PVOID callbackHandle;
    UNICODE_STRING dllName;
    OBJECT_ATTRIBUTES objectAttributes;
    
    if (Phase == 1) {
        RtlInitUnicodeString(&dllName, L"mcupdate.dll");
        ExpInitializeMicrocodeUpdateLock(&ExpMicrocodeUpdateLock);
        
        RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");
        InitializeObjectAttributes(&objectAttributes, &callbackName, OBJ_KERNEL_HANDLE, NULL, NULL);
        status = ExCreateCallback((PCALLBACK_OBJECT*)&callbackHandle, &objectAttributes, FALSE, TRUE);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        
        ExRegisterCallback(callbackHandle, ExpMicrocodePowerStateCallback, NULL);
    } else if (Phase == 2) {
        if (g_MicrocodeImageNotify) {
            g_MicrocodeImageNotify();
        }
    }
    
    return STATUS_SUCCESS;
}

// Microcode power state callback
void ExpMicrocodePowerStateCallback(
    PVOID CallbackContext,
    PVOID Argument1,
    PVOID Argument2
)
{
    if (Argument1 == (PVOID)3) {
        if (Argument2 == (PVOID)0) {
            ExpAcquireMicrocodeUpdateLock(&ExpMicrocodeUpdateLock);
        } else if (Argument2 == (PVOID)1) {
            ExpReleaseMicrocodeUpdateLock(&ExpMicrocodeUpdateLock);
            if (g_MicrocodeImageNotify) {
                g_MicrocodeImageNotify();
            }
        }
    }
}

// Work microcode routine
void ExpWorkMicrocode(
    PVOID Context
)
{
    NTSTATUS status;
    PWORK_QUEUE_ITEM workItem = (PWORK_QUEUE_ITEM)Context;
    NTSTATUS (__cdecl *function)(void);
    
    function = (NTSTATUS (__cdecl *)(void))workItem->Parameter;
    
    ExpAcquireMicrocodeUpdateLock(&ExpMicrocodeUpdateLock);
    status = function();
    workItem->Parameter = (PVOID)status;
    ExpReleaseMicrocodeUpdateLock(&ExpMicrocodeUpdateLock);
    
    KeSetEvent((PKEVENT)((PUCHAR)workItem + 4), 0, FALSE);
}

