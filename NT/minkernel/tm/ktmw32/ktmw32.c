#include <windows.h>
#include <winternl.h>
#include <guiddef.h>
#include <strsafe.h>

typedef struct _PEB *PPEB;
PPEB __cdecl NtCurrentPeb(VOID);

// External function declarations
extern int NtCommitComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtCommitEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtCommitTransaction(void *TransactionHandle, int Wait);
extern int NtCreateEnlistment(void **EnlistmentHandle, unsigned long DesiredAccess, void *ResourceManagerHandle, void *TransactionHandle, void *ObjectAttributes, unsigned long CreateOptions, unsigned long NotificationMask, void *EnlistmentKey);
extern int NtCreateResourceManager(void **ResourceManagerHandle, unsigned long DesiredAccess, void *TransactionManagerHandle, GUID *ResourceManagerId, void *ObjectAttributes, unsigned long CreateOptions, void *Description);
extern int NtCreateTransaction(void **TransactionHandle, unsigned long DesiredAccess, void *ObjectAttributes, GUID *TransactionId, void *Uow, unsigned long CreateOptions, unsigned long IsolationLevel, unsigned long IsolationFlags, PLARGE_INTEGER Timeout, void *Description);
extern int NtCreateTransactionManager(void **TmHandle, unsigned long DesiredAccess, void *ObjectAttributes, void *LogFileName, unsigned long CreateOptions, unsigned long CommitStrength);
extern int NtGetNotificationResourceManager(void *ResourceManagerHandle, TRANSACTION_NOTIFICATION *TransactionNotification, unsigned long NotificationLength, void *Timeout, unsigned long *ReturnLength, int Asynchronous, OVERLAPPED *Overlapped);
extern int NtOpenEnlistment(void **EnlistmentHandle, unsigned long DesiredAccess, void *ResourceManagerHandle, GUID *EnlistmentId, void *ObjectAttributes);
extern int NtOpenResourceManager(void **ResourceManagerHandle, unsigned long DesiredAccess, void *TransactionManagerHandle, GUID *ResourceManagerId, void *ObjectAttributes);
extern int NtOpenTransaction(void **TransactionHandle, unsigned long DesiredAccess, void *ObjectAttributes, GUID *TransactionId, int OpenOptions);
extern int NtOpenTransactionManager(void **TmHandle, unsigned long DesiredAccess, void *ObjectAttributes, void *LogFileName, GUID *TmIdentity, unsigned long OpenOptions);
extern int NtPrepareComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtPrepareEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtPrePrepareComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtPrePrepareEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtQueryInformationEnlistment(void *EnlistmentHandle, int EnlistmentInformationClass, void *EnlistmentInformation, unsigned long EnlistmentInformationLength, unsigned long *ReturnLength);
extern int NtQueryInformationTransaction(void *TransactionHandle, int TransactionInformationClass, void *TransactionInformation, unsigned long TransactionInformationLength, unsigned long *ReturnLength);
extern int NtQueryInformationTransactionManager(void *TmHandle, int TmInformationClass, void *TmInformation, unsigned long TmInformationLength, unsigned long *ReturnLength);
extern int NtReadOnlyEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtRecoverEnlistment(void *EnlistmentHandle, void *EnlistmentKey);
extern int NtRecoverResourceManager(void *ResourceManagerHandle);
extern int NtRecoverTransactionManager(void *TmHandle);
extern int NtRenameTransactionManager(void *LogFileName, GUID *ExistingTmIdentity);
extern int NtRollbackComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtRollbackEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtRollbackTransaction(void *TransactionHandle, int Wait);
extern int NtRollforwardTransactionManager(void *TmHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtSetInformationEnlistment(void *EnlistmentHandle, int EnlistmentInformationClass, void *EnlistmentInformation, unsigned long EnlistmentInformationLength);
extern int NtSetInformationResourceManager(void *ResourceManagerHandle, int ResourceManagerInformationClass, void *ResourceManagerInformation, unsigned long ResourceManagerInformationLength);
extern int NtSetInformationTransaction(void *TransactionHandle, int TransactionInformationClass, void *TransactionInformation, unsigned long TransactionInformationLength);
extern int NtSetInformationTransactionManager(void *TmHandle, int TmInformationClass, void *TmInformation, unsigned long TmInformationLength);
extern int NtSinglePhaseReject(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock);
extern int NtPropagationComplete(void *ResourceManagerHandle, unsigned long RequestCookie, unsigned long BufferLength, void *Buffer);
extern int NtPropagationFailed(void *ResourceManagerHandle, unsigned long RequestCookie, int PropStatus);
extern int NtRegisterProtocolAddressInformation(void *ResourceManagerHandle, GUID *ProtocolId, unsigned long ProtocolInformationSize, void *ProtocolInformation, unsigned long CompletionPort);

// External RTL functions
extern int RtlDosPathNameToNtPathName_U_WithStatus(wchar_t *DosPath, void *NtPath, int *FilePath, void *RelativeName);
extern void RtlFreeHeap(void *HeapHandle, unsigned long Flags, void *BaseAddress);
extern void *RtlAllocateHeap(void *HeapHandle, unsigned long Flags, unsigned long Size);
extern void RtlInitUnicodeStringEx(void *DestinationString, wchar_t *SourceString);

void * __cdecl PrivCreateTransaction(SECURITY_ATTRIBUTES *SecurityAttributes, GUID *TransactionId, unsigned long CreateOptions, unsigned long IsolationLevel, unsigned long IsolationFlags, unsigned long Timeout, wchar_t *Description);

int __cdecl CommitComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtCommitComplete(EnlistmentHandle, TmVirtualClock);
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    return (status >= 0);
}

int __cdecl CommitEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtCommitEnlistment(EnlistmentHandle, TmVirtualClock);
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    return (status >= 0);
}

int __cdecl CommitTransaction(void *TransactionHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtCommitTransaction(TransactionHandle, 1);
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    return (status >= 0);
}

int __cdecl CommitTransactionAsync(void *TransactionHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtCommitTransaction(TransactionHandle, 0);
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    return (status >= 0);
}

void * __cdecl CreateEnlistment(SECURITY_ATTRIBUTES *SecurityAttributes, void *ResourceManagerHandle, void *TransactionHandle, unsigned long CreateOptions, unsigned long NotificationMask, void *EnlistmentKey)
{
    int status;
    unsigned long errorCode;
    void *enlistmentHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG securityFlags = 0;
    
    enlistmentHandle = (void *)-1;
    
    if ((SecurityAttributes != NULL) && (SecurityAttributes->bInheritHandle != 0)) {
        securityFlags = 2;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = securityFlags;
    objectAttributes.SecurityDescriptor = (SecurityAttributes == NULL) ? NULL : SecurityAttributes->lpSecurityDescriptor;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtCreateEnlistment(&enlistmentHandle, 0xF001F, ResourceManagerHandle, TransactionHandle, &objectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        enlistmentHandle = (void *)-1;
    } else {
        if (status == 0x40000000) {
            errorCode = 0xB7;
        } else {
            errorCode = 0;
        }
        SetLastError(errorCode);
    }
    
    return enlistmentHandle;
}

void * __cdecl CreateResourceManager(SECURITY_ATTRIBUTES *SecurityAttributes, GUID *ResourceManagerId, unsigned long CreateOptions, void *TransactionManagerHandle, wchar_t *Description)
{
    int status;
    unsigned long errorCode;
    void *resourceManagerHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING descriptionString;
    void *descriptionPtr = NULL;
    ULONG securityFlags = 0;
    
    resourceManagerHandle = (void *)-1;
    
    if ((SecurityAttributes != NULL) && (SecurityAttributes->bInheritHandle != 0)) {
        securityFlags = 2;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = securityFlags;
    objectAttributes.SecurityDescriptor = (SecurityAttributes == NULL) ? NULL : SecurityAttributes->lpSecurityDescriptor;
    objectAttributes.SecurityQualityOfService = NULL;
    
    if (Description != NULL) {
        RtlInitUnicodeStringEx(&descriptionString, Description);
        descriptionPtr = &descriptionString;
    }
    
    status = NtCreateResourceManager(&resourceManagerHandle, 0x1F007F, TransactionManagerHandle, ResourceManagerId, &objectAttributes, CreateOptions, descriptionPtr);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        resourceManagerHandle = (void *)-1;
    } else {
        if (status == 0x40000000) {
            errorCode = 0xB7;
        } else {
            errorCode = 0;
        }
        SetLastError(errorCode);
    }
    
    return resourceManagerHandle;
}

void * __cdecl CreateTransaction(SECURITY_ATTRIBUTES *SecurityAttributes, GUID *TransactionId, unsigned long CreateOptions, unsigned long IsolationLevel, unsigned long IsolationFlags, unsigned long Timeout, wchar_t *Description)
{
    void *result;
    
    if (TransactionId != NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    result = PrivCreateTransaction(SecurityAttributes, NULL, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    return result;
}

void * __cdecl CreateTransactionManager(SECURITY_ATTRIBUTES *SecurityAttributes, wchar_t *LogFileName, unsigned long CreateOptions, unsigned long CommitStrength)
{
    int status;
    unsigned long errorCode;
    void *tmHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING logPathString;
    int pathAllocated = 0;
    ULONG securityFlags = 0;
    
    tmHandle = (void *)-1;
    
    if ((LogFileName == NULL) || (status = RtlDosPathNameToNtPathName_U_WithStatus(LogFileName, &logPathString, NULL, NULL), status >= 0)) {
        if ((SecurityAttributes != NULL) && (SecurityAttributes->bInheritHandle != 0)) {
            securityFlags = 2;
        }
        
        objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        objectAttributes.RootDirectory = NULL;
        objectAttributes.ObjectName = NULL;
        objectAttributes.Attributes = securityFlags;
        objectAttributes.SecurityDescriptor = (SecurityAttributes == NULL) ? NULL : SecurityAttributes->lpSecurityDescriptor;
        objectAttributes.SecurityQualityOfService = NULL;
        
        status = NtCreateTransactionManager(&tmHandle, 0xF003F, &objectAttributes, 
                                          (LogFileName != NULL) ? &logPathString : NULL, 
                                          CreateOptions, CommitStrength);
        
        if (status < 0) {
            errorCode = RtlNtStatusToDosError(status);
            SetLastError(errorCode);
            tmHandle = (void *)-1;
        } else if (status == 0x40000000) {
            SetLastError(0xB7);
        } else {
            SetLastError(0);
        }
        
        if (pathAllocated) {
            RtlFreeHeap(GetProcessHeap(), 0, logPathString.Buffer);
        }
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        tmHandle = (void *)-1;
    }
    
    return tmHandle;
}

int __cdecl GetCurrentClockTransactionManager(void *TmHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    TRANSACTIONMANAGER_BASIC_INFORMATION basicInfo;
    
    status = NtQueryInformationTransactionManager(TmHandle, 0, &basicInfo, sizeof(basicInfo), NULL);
    
    if (status >= 0) {
        TmVirtualClock->QuadPart = basicInfo.VirtualClock.QuadPart;
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl GetEnlistmentId(void *EnlistmentHandle, GUID *EnlistmentId)
{
    int status;
    unsigned long errorCode;
    ENLISTMENT_BASIC_INFORMATION basicInfo;
    unsigned long returnLength;
    
    status = NtQueryInformationEnlistment(EnlistmentHandle, 0, &basicInfo, sizeof(basicInfo), &returnLength);
    
    if (status >= 0) {
        *EnlistmentId = basicInfo.EnlistmentId;
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl GetEnlistmentRecoveryInformation(void *EnlistmentHandle, unsigned long BufferLength, void *Buffer, unsigned long *ReturnLength)
{
    int status;
    unsigned long errorCode;
    
    status = NtQueryInformationEnlistment(EnlistmentHandle, 1, Buffer, BufferLength, ReturnLength);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl GetNotificationResourceManager(void *ResourceManagerHandle, TRANSACTION_NOTIFICATION *TransactionNotification, unsigned long TransactionNotificationLength, unsigned long Timeout, unsigned long *ReturnLength)
{
    LARGE_INTEGER timeout;
    int status;
    unsigned long errorCode;
    
    if (Timeout == 0xFFFFFFFF) {
        status = NtGetNotificationResourceManager(ResourceManagerHandle, TransactionNotification, TransactionNotificationLength, NULL, ReturnLength, 0, NULL);
    } else {
        timeout.QuadPart = -(LONGLONG)Timeout * 10000;
        status = NtGetNotificationResourceManager(ResourceManagerHandle, TransactionNotification, TransactionNotificationLength, &timeout, ReturnLength, 0, NULL);
    }
    
    if (status < 0) {
        if (status != 0x102) {
            errorCode = RtlNtStatusToDosError(status);
            SetLastError(errorCode);
        } else {
            SetLastError(0x102);
        }
        return 0;
    } else if (status == 0x102) {
        SetLastError(0x102);
        return 0;
    }
    
    return 1;
}

int __cdecl GetNotificationResourceManagerAsync(void *ResourceManagerHandle, TRANSACTION_NOTIFICATION *TransactionNotification, unsigned long TransactionNotificationLength, unsigned long *ReturnLength, OVERLAPPED *Overlapped)
{
    int status;
    unsigned long errorCode;
    
    if (Overlapped == NULL) {
        errorCode = 0x57;
        SetLastError(errorCode);
        return 0;
    }
    
    status = NtGetNotificationResourceManager(ResourceManagerHandle, TransactionNotification, TransactionNotificationLength, NULL, ReturnLength, 1, Overlapped);
    
    if (status == 0x103) {
        errorCode = 0x3E5;
        SetLastError(errorCode);
        return 0;
    } else if (status >= 0) {
        return 1;
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        return 0;
    }
}

int __cdecl GetTransactionId(void *TransactionHandle, GUID *TransactionId)
{
    int status;
    unsigned long errorCode;
    TRANSACTION_BASIC_INFORMATION basicInfo;
    unsigned long returnLength;
    
    status = NtQueryInformationTransaction(TransactionHandle, 0, &basicInfo, sizeof(basicInfo), &returnLength);
    
    if (status >= 0) {
        *TransactionId = basicInfo.TransactionId;
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl GetTransactionInformation(void *TransactionHandle, unsigned long *Outcome, unsigned long *IsolationLevel, unsigned long *IsolationFlags, unsigned long *Timeout, unsigned long BufferLength, wchar_t *Description)
{
    TRANSACTION_PROPERTIES_INFORMATION *propInfo;
    unsigned long errorCode;
    int status;
    unsigned long timeVal;
    
    propInfo = (TRANSACTION_PROPERTIES_INFORMATION *)RtlAllocateHeap(GetProcessHeap(), 0, sizeof(TRANSACTION_PROPERTIES_INFORMATION) + 256);
    
    if (propInfo == NULL) {
        errorCode = RtlNtStatusToDosError(0xC0000017);
        SetLastError(errorCode);
        status = 0;
    } else {
        status = NtQueryInformationTransaction(TransactionHandle, 1, propInfo, sizeof(TRANSACTION_PROPERTIES_INFORMATION) + 256, NULL);
        
        if (status < 0) {
            errorCode = RtlNtStatusToDosError(status);
            SetLastError(errorCode);
            RtlFreeHeap(GetProcessHeap(), 0, propInfo);
            status = 0;
        } else {
            if (Outcome != NULL) {
                *Outcome = propInfo->Outcome;
            }
            if (IsolationLevel != NULL) {
                *IsolationLevel = propInfo->IsolationLevel;
            }
            if (IsolationFlags != NULL) {
                *IsolationFlags = propInfo->IsolationFlags;
            }
            if (Timeout != NULL) {
                timeVal = (unsigned long)propInfo->Timeout.QuadPart;
                if ((timeVal == 0xFFFFFFFF) || (timeVal == 0)) {
                    *Timeout = 0xFFFFFFFF;
                } else {
                    *Timeout = timeVal;
                }
            }
            if (Description != NULL) {
                if ((BufferLength * 2 < propInfo->DescriptionLength) || (BufferLength * 2 < propInfo->DescriptionLength + 2)) {
                    errorCode = RtlNtStatusToDosError(0x80000005);
                    SetLastError(errorCode);
                    RtlFreeHeap(GetProcessHeap(), 0, propInfo);
                    return 0;
                }
                memcpy(Description, propInfo->Description, propInfo->DescriptionLength);
                Description[propInfo->DescriptionLength / 2] = 0;
            }
            RtlFreeHeap(GetProcessHeap(), 0, propInfo);
            status = 1;
        }
    }
    
    return status;
}

int __cdecl GetTransactionManagerId(void *TmHandle, GUID *TmIdentity)
{
    int status;
    unsigned long errorCode;
    TRANSACTIONMANAGER_BASIC_INFORMATION basicInfo;
    
    status = NtQueryInformationTransactionManager(TmHandle, 0, &basicInfo, sizeof(basicInfo), NULL);
    
    if (status >= 0) {
        *TmIdentity = basicInfo.TmIdentity;
    } else {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

void * __cdecl OpenEnlistment(unsigned long DesiredAccess, void *ResourceManagerHandle, GUID *EnlistmentId)
{
    int status;
    unsigned long errorCode;
    void *enlistmentHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    
    enlistmentHandle = (void *)-1;
    
    if (EnlistmentId == NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = 0;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtOpenEnlistment(&enlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentId, &objectAttributes);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        enlistmentHandle = (void *)-1;
    }
    
    return enlistmentHandle;
}

void * __cdecl OpenResourceManager(unsigned long DesiredAccess, void *TransactionManagerHandle, GUID *ResourceManagerId)
{
    int status;
    unsigned long errorCode;
    void *resourceManagerHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    
    resourceManagerHandle = (void *)-1;
    
    if (ResourceManagerId == NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = 0;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtOpenResourceManager(&resourceManagerHandle, DesiredAccess, TransactionManagerHandle, ResourceManagerId, &objectAttributes);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        resourceManagerHandle = (void *)-1;
    }
    
    return resourceManagerHandle;
}

void * __cdecl OpenTransaction(unsigned long DesiredAccess, GUID *TransactionId)
{
    int status;
    unsigned long errorCode;
    void *transactionHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    
    transactionHandle = (void *)-1;
    
    if (TransactionId == NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = 0;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtOpenTransaction(&transactionHandle, DesiredAccess, &objectAttributes, TransactionId, 0);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        transactionHandle = (void *)-1;
    }
    
    return transactionHandle;
}

void * __cdecl OpenTransactionManager(wchar_t *LogFileName, unsigned long DesiredAccess, unsigned long OpenOptions)
{
    int status;
    unsigned long errorCode;
    void *tmHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING logPathString;
    int pathAllocated = 0;
    
    tmHandle = (void *)-1;
    
    if (LogFileName == NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    status = RtlDosPathNameToNtPathName_U_WithStatus(LogFileName, &logPathString, NULL, NULL);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        return (void *)-1;
    }
    
    pathAllocated = 1;
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = 0;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtOpenTransactionManager(&tmHandle, DesiredAccess, &objectAttributes, &logPathString, NULL, OpenOptions);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        tmHandle = (void *)-1;
    }
    
    if (pathAllocated) {
        RtlFreeHeap(GetProcessHeap(), 0, logPathString.Buffer);
    }
    
    return tmHandle;
}

void * __cdecl OpenTransactionManagerById(GUID *TmIdentity, unsigned long DesiredAccess, unsigned long OpenOptions)
{
    int status;
    unsigned long errorCode;
    void *tmHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    
    tmHandle = (void *)-1;
    
    if (TmIdentity == NULL) {
        SetLastError(0x57);
        return (void *)-1;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = 0;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    status = NtOpenTransactionManager(&tmHandle, DesiredAccess, &objectAttributes, NULL, TmIdentity, OpenOptions);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        tmHandle = (void *)-1;
    }
    
    return tmHandle;
}

int __cdecl PrepareComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtPrepareComplete(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrepareEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtPrepareEnlistment(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrePrepareComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtPrePrepareComplete(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrePrepareEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtPrePrepareEnlistment(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

void * __cdecl PrivCreateTransaction(SECURITY_ATTRIBUTES *SecurityAttributes, GUID *TransactionId, unsigned long CreateOptions, unsigned long IsolationLevel, unsigned long IsolationFlags, unsigned long Timeout, wchar_t *Description)
{
    int status;
    unsigned long errorCode;
    void *transactionHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    LARGE_INTEGER timeout;
    LARGE_INTEGER *timeoutPtr = NULL;
    UNICODE_STRING descriptionString;
    void *descriptionPtr = NULL;
    ULONG securityFlags = 0;
    
    transactionHandle = (void *)-1;
    
    if ((SecurityAttributes != NULL) && (SecurityAttributes->bInheritHandle != 0)) {
        securityFlags = 2;
    }
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = NULL;
    objectAttributes.Attributes = securityFlags;
    objectAttributes.SecurityDescriptor = (SecurityAttributes == NULL) ? NULL : SecurityAttributes->lpSecurityDescriptor;
    objectAttributes.SecurityQualityOfService = NULL;
    
    if ((Timeout != 0xFFFFFFFF) && (Timeout != 0)) {
        timeout.QuadPart = -(LONGLONG)Timeout * 10000;
        timeoutPtr = &timeout;
    }
    
    if (Description != NULL) {
        RtlInitUnicodeStringEx(&descriptionString, Description);
        descriptionPtr = &descriptionString;
    }
    
    status = NtCreateTransaction(&transactionHandle, 0x1F003F, &objectAttributes, TransactionId, NULL, CreateOptions, IsolationLevel, IsolationFlags, timeoutPtr, descriptionPtr);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        transactionHandle = (void *)-1;
    } else {
        if (status == 0x40000000) {
            errorCode = 0xB7;
        } else {
            errorCode = 0;
        }
        SetLastError(errorCode);
    }
    
    return transactionHandle;
}

int __cdecl PrivIsLogWritableTransactionManager(void *TmHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtSetInformationTransactionManager(TmHandle, 3, NULL, 0);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrivPropagationComplete(void *ResourceManagerHandle, unsigned long RequestCookie, unsigned long BufferLength, void *Buffer)
{
    int status;
    unsigned long errorCode;
    
    status = NtPropagationComplete(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrivPropagationFailed(void *ResourceManagerHandle, unsigned long RequestCookie)
{
    int status;
    unsigned long errorCode;
    
    status = NtPropagationFailed(ResourceManagerHandle, RequestCookie, 0);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl PrivRegisterProtocolAddressInformation(void *ResourceManagerHandle, GUID *ProtocolId, unsigned long ProtocolInformationSize, void *ProtocolInformation, unsigned long CompletionPort)
{
    int status;
    unsigned long errorCode;
    
    status = NtRegisterProtocolAddressInformation(ResourceManagerHandle, ProtocolId, ProtocolInformationSize, ProtocolInformation, CompletionPort);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl ReadOnlyEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtReadOnlyEnlistment(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RecoverEnlistment(void *EnlistmentHandle, void *EnlistmentKey)
{
    int status;
    unsigned long errorCode;
    
    status = NtRecoverEnlistment(EnlistmentHandle, EnlistmentKey);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RecoverResourceManager(void *ResourceManagerHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtRecoverResourceManager(ResourceManagerHandle);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RecoverTransactionManager(void *TmHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtRecoverTransactionManager(TmHandle);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RenameTransactionManager(wchar_t *LogFileName, GUID *ExistingTmIdentity)
{
    int status;
    unsigned long errorCode;
    UNICODE_STRING logPathString;
    int pathAllocated = 0;
    int result;
    
    if (LogFileName == NULL) {
        SetLastError(0x57);
        return 0;
    }
    
    status = RtlDosPathNameToNtPathName_U_WithStatus(LogFileName, &logPathString, NULL, NULL);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
        return 0;
    }
    
    pathAllocated = 1;
    
    status = NtRenameTransactionManager(&logPathString, ExistingTmIdentity);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    result = (status >= 0);
    
    if (pathAllocated) {
        RtlFreeHeap(GetProcessHeap(), 0, logPathString.Buffer);
    }
    
    return result;
}

int __cdecl RollbackComplete(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtRollbackComplete(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RollbackEnlistment(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtRollbackEnlistment(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RollbackTransaction(void *TransactionHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtRollbackTransaction(TransactionHandle, 1);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RollbackTransactionAsync(void *TransactionHandle)
{
    int status;
    unsigned long errorCode;
    
    status = NtRollbackTransaction(TransactionHandle, 0);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl RollforwardTransactionManager(void *TmHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtRollforwardTransactionManager(TmHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl SetEnlistmentRecoveryInformation(void *EnlistmentHandle, unsigned long BufferSize, void *Buffer)
{
    int status;
    unsigned long errorCode;
    
    status = NtSetInformationEnlistment(EnlistmentHandle, 1, Buffer, BufferSize);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl SetResourceManagerCompletionPort(void *ResourceManagerHandle, void *CompletionPort, unsigned long CompletionKey)
{
    int status;
    unsigned long errorCode;
    RESOURCEMANAGER_COMPLETION_INFORMATION completionInfo;
    
    completionInfo.IoCompletionPortHandle = CompletionPort;
    completionInfo.CompletionKey = CompletionKey;
    
    status = NtSetInformationResourceManager(ResourceManagerHandle, 1, &completionInfo, sizeof(completionInfo));
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

int __cdecl SetTransactionInformation(void *TransactionHandle, unsigned long IsolationLevel, unsigned long IsolationFlags, unsigned long Timeout, wchar_t *Description)
{
    int result;
    int strStatus;
    wchar_t *descPtr;
    unsigned long descLength;
    unsigned long bufferSize;
    TRANSACTION_PROPERTIES_INFORMATION propInfo;
    
    propInfo.IsolationLevel = IsolationLevel;
    propInfo.IsolationFlags = IsolationFlags;
    propInfo.Timeout.QuadPart = (LONGLONG)Timeout;
    
    bufferSize = sizeof(TRANSACTION_PROPERTIES_INFORMATION);
    
    if ((Timeout == 0xFFFFFFFF) || (Timeout == 0)) {
        propInfo.Timeout.QuadPart = (LONGLONG)0;
    } else {
        propInfo.Timeout.QuadPart = (LONGLONG)Timeout;
    }
    
    propInfo.DescriptionLength = 0;
    
    if (Description != NULL) {
        strStatus = StringCchLengthW(Description, 0x40, &descLength);
        if (strStatus >= 0) {
            if (descLength > 0x40) {
                SetLastError(0x57);
                return 0;
            }
            strStatus = StringCchCopyW(propInfo.Description, descLength + 1, Description);
            if (strStatus >= 0) {
                propInfo.DescriptionLength = descLength * 2;
                bufferSize += descLength * 2;
            }
        }
    }
    
    result = NtSetInformationTransaction(TransactionHandle, 1, &propInfo, bufferSize);
    
    if (result < 0) {
        result = RtlNtStatusToDosError(result);
        SetLastError(result);
        return 0;
    }
    
    return 1;
}

int __cdecl SinglePhaseReject(void *EnlistmentHandle, LARGE_INTEGER *TmVirtualClock)
{
    int status;
    unsigned long errorCode;
    
    status = NtSinglePhaseReject(EnlistmentHandle, TmVirtualClock);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosError(status);
        SetLastError(errorCode);
    }
    
    return (status >= 0);
}

