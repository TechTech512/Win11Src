#pragma warning (disable:4996)
#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions                                                  */
/* ------------------------------------------------------------------ */
extern VOID CdInitializeIoPipe(PCD_IO_PIPE Pipe, PVOID Context);
extern VOID CdDisconnectIoPipe(PCD_IO_PIPE Pipe);
extern NTSTATUS CdCompleteIo(PCD_IO_PIPE Pipe, UCHAR Mode,
                             PCD_IO_COMPLETE Complete, ULONG_PTR Reserved);
extern NTSTATUS CdReadIoInput(PCD_IO_PIPE Pipe, UCHAR Mode,
                              PCD_IO_COMPLETE Complete, PVOID Input,
                              PULONG OutputLength);
extern NTSTATUS CdWriteIoOutput(PCD_IO_PIPE Pipe, UCHAR Mode,
                                PCD_IO_COMPLETE Complete, PVOID Output,
                                PULONG OutputLength);
extern NTSTATUS CdpCompleteReadIoIrp(PCD_IO_PIPE Pipe, PIRP Irp,
                                     PCD_IO_COMPLETE Complete);
extern VOID CdpCancelReadIoIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
extern NTSTATUS __cdecl CdCompleteBrokerIoWithServer(PVOID        Broker,
                                              PLUID        RequestId,
                                              PCD_SERVER   Server,
                                              PEPROCESS    Process);

extern NTSTATUS SeQueryInformationToken(PACCESS_TOKEN Token,
                                        TOKEN_INFORMATION_CLASS Class,
                                        PVOID *Info);

extern NTSTATUS ObLogSecurityDescriptor(PSECURITY_DESCRIPTOR  SecurityDescriptor,
                                        PSECURITY_DESCRIPTOR *LoggedSecurityDescriptor,
                                        ULONG                 RefBias);

extern NTSTATUS SeQuerySecurityDescriptorInfo(PSECURITY_INFORMATION SecurityInformation,
                                              PSECURITY_DESCRIPTOR   SecurityDescriptor,
                                              PULONG                 Length,
                                              PSECURITY_DESCRIPTOR  *ObjectsSecurityDescriptor);

extern NTSTATUS SeSetSecurityDescriptorInfo(PVOID                    Object,
                                            PSECURITY_INFORMATION    SecurityInformation,
                                            PSECURITY_DESCRIPTOR     ModificationDescriptor,
                                            PSECURITY_DESCRIPTOR    *ObjectsSecurityDescriptor,
                                            POOL_TYPE                PoolType,
                                            PGENERIC_MAPPING         GenericMapping);

extern NTSTATUS ObOpenObjectByPointer(PVOID            Object,
                                      ULONG            HandleAttributes,
                                      PACCESS_STATE    AccessState,
                                      ACCESS_MASK      DesiredAccess,
                                      POBJECT_TYPE     ObjectType,
                                      KPROCESSOR_MODE  AccessMode,
                                      PHANDLE          Handle);

extern NTSTATUS PsAcquireProcessExitSynchronization(PEPROCESS Process);
extern VOID     PsReleaseProcessExitSynchronization(PEPROCESS Process);
extern VOID     PsResumeProcess(PEPROCESS Process);
extern PPEB     PsGetProcessPeb(PEPROCESS Process);
extern PCHAR    PsGetProcessImageFileName(PEPROCESS Process);
extern PACCESS_TOKEN PsReferencePrimaryToken(PEPROCESS Process);
extern VOID PsDereferencePrimaryToken(PACCESS_TOKEN PrimaryToken);
NTKERNELAPI
PEPROCESS
IoGetRequestorProcess(
    _In_ PIRP Irp
    );
VOID
NTAPI
ObDereferenceSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Count
    );

extern NTSTATUS ZwCreateUserProcess(PVOID                             ImagePath,
                                     PUNICODE_STRING                   ImageName,
                                     PRTL_USER_PROCESS_PARAMETERS      ProcessParameters,
                                     PSECURITY_DESCRIPTOR              ProcessDescriptor,
                                     PVOID                             ThreadDescriptor,
                                     ULONG                             Flags,
                                     PVOID                             StackZeroBits,
                                     PVOID                             StackReserve,
                                     PVOID                             StackCommit,
                                     PHANDLE                           ProcessHandle,
                                     PHANDLE                           ThreadHandle);

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpAllocateServer(PCD_SERVER *Server);
NTSTATUS __cdecl CdpServerQuerySecurity(PCD_SERVER Server,
                                        PIRP        Irp);
NTSTATUS __cdecl CdpServerSetSecurity(PCD_SERVER Server,
                                      PIRP        Irp);
UCHAR __cdecl CdpServerFastIoctl(PCD_SERVER        Server,
                                 PVOID             InputBuffer,
                                 ULONG             InputLength,
                                 PVOID             OutputBuffer,
                                 ULONG             OutputLength,
                                 ULONG             IoctlCode,
                                 PIO_STATUS_BLOCK  IoStatus);
NTSTATUS __cdecl CdpServerClose(PCD_SERVER Server,
                                PIRP        Irp);
NTSTATUS __cdecl CdpServerCleanup(PCD_SERVER Server,
                                  PIRP        Irp);
NTSTATUS __cdecl CdpServerIoctl(PCD_SERVER Server,
                                PIRP        Irp);
NTSTATUS __cdecl CdCreateSecurityDescriptor(PVOID         *SecurityDescriptor,
                                             PACCESS_STATE  AccessState);
NTSTATUS __cdecl CdpSetServerInformation(PCD_SERVER Server, UCHAR Mode,
                                  PCD_IO_SERVER_INFORMATION Info, ULONG Length);
NTSTATUS __cdecl CdpLaunchServerProcess(PCD_SERVER Server, UCHAR Mode,
                                 PRTL_USER_PROCESS_PARAMETERS Parameters);
ACCESS_MASK __cdecl CdAccessCheck(PCD_SERVER                Server,
                                  PSECURITY_SUBJECT_CONTEXT SubjectContext,
                                  ACCESS_MASK               DesiredAccess,
                                  BOOLEAN                   SubjectContextLocked);
NTSTATUS __cdecl CdCreateServer(PIRP  Irp,
                                ULONG Parameter);
PCD_SERVER __cdecl CdGetServerFromFileObject(PFILE_OBJECT FileObject);
NTSTATUS __cdecl CdpCreateProcess(PHANDLE                        ProcessHandle,
                                  PUNICODE_STRING                ImageName,
                                  PRTL_USER_PROCESS_PARAMETERS   Parameters,
                                  PVOID                          ThreadHandle);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdAccessCheck)
#pragma alloc_text(PAGE,CdCreateSecurityDescriptor)
#pragma alloc_text(PAGE,CdCreateServer)
#pragma alloc_text(PAGE,CdGetServerFromFileObject)
#pragma alloc_text(PAGE,CdpAllocateServer)
#pragma alloc_text(PAGE,CdpCreateProcess)
#pragma alloc_text(PAGE,CdpLaunchServerProcess)
#pragma alloc_text(PAGE,CdpServerCleanup)
#pragma alloc_text(PAGE,CdpServerClose)
#pragma alloc_text(PAGE,CdpServerFastIoctl)
#pragma alloc_text(PAGE,CdpServerIoctl)
#pragma alloc_text(PAGE,CdpServerQuerySecurity)
#pragma alloc_text(PAGE,CdpServerSetSecurity)
#pragma alloc_text(PAGE,CdpSetServerInformation)
#endif // ALLOC_PRAGMA

/* ------------------------------------------------------------------ */
/* External symbols                                                    */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT  CdDeviceObject;
CD_CONTEXT_TYPE CdServerType = {0,NULL,NULL,CdpServerIoctl,CdpServerCleanup,CdpServerClose,CdpServerFastIoctl,NULL,CdpServerQuerySecurity,CdpServerSetSecurity};
GENERIC_MAPPING CdpGenericMapping = { 0x120089, 0x120116, 0x1200A0, 0x1F01FF };
LIST_ENTRY      CdpServerList = { 0 };
EX_PUSH_LOCK    CdpServerListLock = { 0 };
extern POBJECT_TYPE    *IoFileObjectType;
POBJECT_TYPE    IoProcessObjectType;

/* ------------------------------------------------------------------ */
/* CdAccessCheck                                                      */
/* ------------------------------------------------------------------ */
ACCESS_MASK __cdecl CdAccessCheck(PCD_SERVER                Server,
                                  PSECURITY_SUBJECT_CONTEXT SubjectContext,
                                  ACCESS_MASK               DesiredAccess,
                                  BOOLEAN                   SubjectContextLocked)
{
    BOOLEAN      accessGranted;
    ACCESS_MASK  grantedAccess;
    NTSTATUS     accessStatus;
    PACCESS_TOKEN token;
    ULONG        tokenInfo;
    NTSTATUS     status;

    KeEnterCriticalRegion();
    ExAcquirePushLockSharedEx(&Server->Lock, 0);

    accessGranted = SeAccessCheck(Server->SecurityDescriptor,
                                  SubjectContext,
                                  SubjectContextLocked,
                                  DesiredAccess,
                                  0,
                                  NULL,
                                  &CdpGenericMapping,
                                  KernelMode,
                                  &grantedAccess,
                                  &accessStatus);

    if (!accessGranted) {
        if (Server->Flags & 1) {
            token = SubjectContext->ClientToken;
            if (token == NULL) {
                token = SubjectContext->PrimaryToken;
            }
            status = SeQueryInformationToken(token, TokenIsAppContainer, (PVOID *)&tokenInfo);
            if (NT_SUCCESS(status)) {
                if (tokenInfo != 0) {
                    grantedAccess = 0;
                }
            }
        }
    } else {
        grantedAccess = 0;
    }

    ExReleasePushLockSharedEx(&Server->Lock, 0);
    KeLeaveCriticalRegion();

    return grantedAccess;
}

/* ------------------------------------------------------------------ */
/* CdCreateSecurityDescriptor                                         */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateSecurityDescriptor(PVOID         *SecurityDescriptor,
                                             PACCESS_STATE  AccessState)
{
    PSECURITY_DESCRIPTOR newSD;
    PSECURITY_DESCRIPTOR loggedSD;
    NTSTATUS             status;

    status = SeAssignSecurityEx(NULL,
                                AccessState->SecurityDescriptor,
                                &newSD,
                                NULL,
                                FALSE,
                                0x700,
                                &AccessState->SubjectSecurityContext,
                                &CdpGenericMapping,
                                PagedPool);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ObLogSecurityDescriptor(newSD, &loggedSD, 1);
    SeDeassignSecurity(&newSD);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    *SecurityDescriptor = loggedSD;
    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdCreateServer                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateServer(PIRP  Irp,
                                ULONG Parameter)
{
    PIO_STACK_LOCATION  stack;
    PFILE_OBJECT        fileObject;
    PACCESS_STATE       accessState;
    PEPROCESS           requestorProcess;
    PCD_CREATE_SERVER   createInfo;
    PCD_BROKER          broker;
    PCD_SERVER          server;
    NTSTATUS            status;

    stack            = IoGetCurrentIrpStackLocation(Irp);
    fileObject       = stack->FileObject;
    accessState      = stack->Parameters.Create.SecurityContext->AccessState;
    requestorProcess = IoGetRequestorProcess(Irp);
    server           = NULL;
    broker           = NULL;

    if (requestorProcess == NULL) {
        status = STATUS_INVALID_CID;
        goto Complete;
    }

    if (stack->Parameters.DeviceIoControl.Type3InputBuffer != NULL) {
        if (stack->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(CD_CREATE_SERVER)) {
            status = STATUS_INVALID_BUFFER_SIZE;
            goto Complete;
        }
        createInfo = (PCD_CREATE_SERVER)stack->Parameters.DeviceIoControl.Type3InputBuffer;
        broker     = (PCD_BROKER)createInfo->BrokerHandle;
    }

    fileObject->FsContext = NULL;

    status = CdpAllocateServer(&server);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    if (broker == NULL) {
        status = CdCreateSecurityDescriptor(&server->SecurityDescriptor,
                                             accessState);
    } else {
        status = CdCompleteBrokerIoWithServer(broker, (PLUID)requestorProcess,
                                               server, (PEPROCESS)accessState);
    }

    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    fileObject->FsContext = server;

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdDereferenceServer                                                */
/* ------------------------------------------------------------------ */
VOID __cdecl CdDereferenceServer(PCD_SERVER Server) // do NOT page
{
    ULONG  oldCount;
    ULONG  newCount;
    KIRQL  irql;

    do {
        oldCount = Server->RefCount;
        newCount = oldCount - 1;
        if (oldCount == 2) {
            newCount |= 0x80000000;
        }
    } while (InterlockedCompareExchange((LONG *)&Server->RefCount,
                                         (LONG)newCount,
                                         (LONG)oldCount) != (LONG)oldCount);

    if (oldCount == 2) {
        CdDisconnectIoPipe(&Server->Pipe);
        InterlockedAdd((LONG *)&Server->RefCount, 0x80000000);
        newCount = oldCount + 0x80000001;
    }

    if (newCount == 1) {
        irql = KeGetCurrentIrql();
        if (irql < DISPATCH_LEVEL) {
            KeEnterCriticalRegion();
        }

        ExAcquirePushLockExclusiveEx(&CdpServerListLock, 0);
        RemoveEntryList(&Server->Links);
        ExReleasePushLockExclusiveEx(&CdpServerListLock, 0);

        if (irql < DISPATCH_LEVEL) {
            KeLeaveCriticalRegion();
        }

        if (Server->SecurityDescriptor != NULL) {
            ObDereferenceSecurityDescriptor(Server->SecurityDescriptor, 1);
        }
        if (Server->InputEvent != NULL) {
            ObfDereferenceObject(Server->InputEvent);
        }
        ExFreePoolWithTag(Server, 'CdSe');
    }
}

/* ------------------------------------------------------------------ */
/* CdGetServerFromFileObject                                          */
/* ------------------------------------------------------------------ */
PCD_SERVER __cdecl CdGetServerFromFileObject(PFILE_OBJECT FileObject)
{
    PCD_CONTEXT_TYPE contextType;

    if (FileObject != NULL &&
        FileObject->DeviceObject == CdDeviceObject &&
        FileObject->FsContext != NULL) {
        contextType = *(PCD_CONTEXT_TYPE *)FileObject->FsContext;
        if (contextType == &CdServerType) {
            return (PCD_SERVER)FileObject->FsContext;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* CdpAllocateServer                                                  */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpAllocateServer(PCD_SERVER *Server)
{
    PCD_SERVER server;

    server = ExAllocatePoolWithQuotaTag(
        (POOL_TYPE)(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
        sizeof(CD_SERVER),
        'CdSe');

    if (server == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(server, sizeof(CD_SERVER));

    server->Header.ObjectType = &CdServerType;
    server->RefCount          = 1;
    server->Flags             = 0;
    InitializeListHead(&server->ConnectionList);

    CdInitializeIoPipe(&server->Pipe, NULL);

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(&CdpServerListLock, 0);

    InsertTailList(&CdpServerList, &server->Links);

    ExReleasePushLockExclusiveEx(&CdpServerListLock, 0);
    KeLeaveCriticalRegion();

    *Server = server;
    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpCreateProcess                                                   */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpCreateProcess(PHANDLE                        ProcessHandle,
                                  PUNICODE_STRING                ImageName,
                                  PRTL_USER_PROCESS_PARAMETERS   Parameters,
                                  PVOID                          ThreadHandle)
{
    RTL_USER_PROCESS_PARAMETERS localParams;
    HANDLE                      threadHandle;
    NTSTATUS                    status;

    RtlZeroMemory(&localParams, sizeof(localParams));
    RtlZeroMemory(Parameters, sizeof(*Parameters));

    status = ZwCreateUserProcess(ImageName->Buffer,
                                  ImageName,
                                  Parameters,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL,
                                  ProcessHandle,
                                  &threadHandle);
    if (NT_SUCCESS(status)) {
        ObCloseHandle(threadHandle, 0);
    } else {
        *ProcessHandle = NULL;
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpLaunchServerProcess                                             */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpLaunchServerProcess(PCD_SERVER                    Server,
                                         UCHAR                         Mode,
                                         PRTL_USER_PROCESS_PARAMETERS  Parameters)
{
    UNICODE_STRING           imageName;
    HANDLE                   processHandle;
    HANDLE                   threadHandle;
    HANDLE                   tokenHandle;
    HANDLE                   mappedHandle;
    PEPROCESS                process;
    PEPROCESS                requestor;
    PACCESS_TOKEN            primaryToken;
    PCHAR                    imageFileName;
    PPEB                     peb;
    KAPC_STATE               apcState;
    BOOLEAN                  hasExclusiveSync;
    BOOLEAN                  attached;
    NTSTATUS                 status;
    PVOID                    buffer;
    ULONG                    bufferLength;

    if (Mode != 1) {
        return STATUS_INVALID_PARAMETER;
    }

    processHandle    = NULL;
    threadHandle     = NULL;
    tokenHandle      = NULL;
    mappedHandle     = NULL;
    process          = NULL;
    attached         = FALSE;
    hasExclusiveSync = FALSE;
    status           = STATUS_SUCCESS;

    requestor     = PsGetCurrentProcess();
    imageFileName = PsGetProcessImageFileName(requestor);

    status = CdpCreateProcess(&processHandle, &imageName, Parameters, NULL);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    primaryToken = PsReferencePrimaryToken(requestor);
    status = ObOpenObjectByPointer(primaryToken, 0x200, NULL, 0x0B,
                                    NULL, KernelMode, &tokenHandle);
    PsDereferencePrimaryToken(primaryToken);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = ObOpenObjectByPointer(requestor, 0x200, NULL, 0x1000,
                                    NULL, KernelMode, &mappedHandle);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = ZwCreateUserProcess(NULL, NULL, NULL, NULL, NULL, 0,
                                   NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = ObReferenceObjectByHandle(processHandle, 0, NULL, KernelMode,
                                        &process, NULL);
    if (!NT_SUCCESS(status)) {
        process = NULL;
        goto Cleanup;
    }

    status = PsAcquireProcessExitSynchronization(process);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    hasExclusiveSync = TRUE;

    KeStackAttachProcess((PRKPROCESS)process, &apcState);
    attached = TRUE;

    status = ObOpenObjectByPointer(Server->FileObject, 0, NULL, 0x10000000,
                                    (POBJECT_TYPE)IoFileObjectType, KernelMode, NULL);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    _stricmp(imageFileName, "ntvdm.exe");

    buffer       = Parameters->CommandLine.Buffer;
    bufferLength = Parameters->CommandLine.MaximumLength;

    status = RtlStringCbPrintfW(buffer, bufferLength, L"%wZ 0x%x%s", &imageName, 0, L"");
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    RtlInitUnicodeString(&Parameters->CommandLine, buffer);

    peb = PsGetProcessPeb(process);
    if (peb != NULL) {
        PVOID pebParams = peb->ProcessParameters;
        ProbeForWrite(pebParams, 0x2A4, 4);

        PVOID    targetCmd = ((RTL_USER_PROCESS_PARAMETERS *)pebParams)->CommandLine.Buffer;
        ProbeForWrite(targetCmd, bufferLength, 1);
        memcpy(targetCmd, buffer, bufferLength);

        ((RTL_USER_PROCESS_PARAMETERS *)pebParams)->CommandLine.Length        =
            Parameters->CommandLine.Length;
        ((RTL_USER_PROCESS_PARAMETERS *)pebParams)->CommandLine.MaximumLength =
            Parameters->CommandLine.MaximumLength;
    }

    PsResumeProcess(process);

Cleanup:
    if (tokenHandle != NULL) {
        ObCloseHandle(tokenHandle, 0);
    }
    if (mappedHandle != NULL) {
        ObCloseHandle(mappedHandle, 0);
    }
    if (attached) {
        KeUnstackDetachProcess(&apcState);
    }
    if (hasExclusiveSync) {
        PsReleaseProcessExitSynchronization(process);
    }
    if (processHandle != NULL) {
        if (!NT_SUCCESS(status)) {
            ZwTerminateProcess(processHandle, status);
        }
        ObCloseHandle(processHandle, 0);
    }
    if (process != NULL) {
        ObfDereferenceObject(process);
    }
    if (Parameters != NULL) {
        ExFreePoolWithTag(Parameters, 0);
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpServerCleanup                                                   */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpServerCleanup(PCD_SERVER Server,
                                  PIRP        Irp)
{
    CdDisconnectIoPipe(&Server->Pipe);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpServerClose                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpServerClose(PCD_SERVER Server,
                                PIRP        Irp)
{
    CdDereferenceServer(Server);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpServerFastIoctl                                                 */
/* ------------------------------------------------------------------ */
UCHAR __cdecl CdpServerFastIoctl(PCD_SERVER        Server,
                                 PVOID             InputBuffer,
                                 ULONG             InputLength,
                                 PVOID             OutputBuffer,
                                 ULONG             OutputLength,
                                 ULONG             IoctlCode,
                                 PIO_STATUS_BLOCK  IoStatus)
{
    KPROCESSOR_MODE mode;
    NTSTATUS        status;

    IoStatus->Information = 0;

    switch (IoctlCode) {
    case 0x500006: /* IOCTL_CONDRV_COMPLETE_IO */
        if (InputLength != 0) {
            if (InputLength != sizeof(CD_IO_COMPLETE)) {
                IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
                return TRUE;
            }
            mode = ExGetPreviousMode();
            CdCompleteIo(&Server->Pipe, (UCHAR)InputLength,
                         (PCD_IO_COMPLETE)InputBuffer, 0);
        }
        return FALSE;

    case 0x50000b: /* IOCTL_CONDRV_COMPLETE_SERVER_IO */
        mode   = ExGetPreviousMode();
        status = CdCompleteIo(&Server->Pipe, (UCHAR)InputLength,
                              (PCD_IO_COMPLETE)InputBuffer, 0);
        IoStatus->Status = status;
        return TRUE;

    case 0x50000f: /* IOCTL_CONDRV_READ_IO */
        mode   = ExGetPreviousMode();
        status = CdReadIoInput(&Server->Pipe, (UCHAR)InputLength,
                               (PCD_IO_COMPLETE)OutputBuffer,
                               InputBuffer, &OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x500013: /* IOCTL_CONDRV_WRITE_IO */
        mode   = ExGetPreviousMode();
        status = CdWriteIoOutput(&Server->Pipe, (UCHAR)InputLength,
                                 (PCD_IO_COMPLETE)OutputBuffer,
                                 InputBuffer, &OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x50001b: /* IOCTL_CONDRV_DISCONNECT_PIPE */
        CdDisconnectIoPipe(&Server->Pipe);
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;

    case 0x50001f: /* IOCTL_CONDRV_SET_SERVER_INFORMATION */
        mode   = ExGetPreviousMode();
        status = CdpSetServerInformation(Server, (UCHAR)InputLength,
                                          (PCD_IO_SERVER_INFORMATION)InputBuffer,
                                          OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x500033: /* IOCTL_CONDRV_SET_SERVER_FLAG */
        KeEnterCriticalRegion();
        ExAcquirePushLockExclusiveEx(&Server->Lock, 0);
        Server->Flags |= 1;
        ExReleasePushLockExclusiveEx(&Server->Lock, 0);
        KeLeaveCriticalRegion();
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;

    case 0x500037: /* IOCTL_CONDRV_LAUNCH_SERVER_PROCESS */
        mode   = ExGetPreviousMode();
        status = CdpLaunchServerProcess(Server, (UCHAR)InputLength,
                                         (PRTL_USER_PROCESS_PARAMETERS)OutputBuffer);
        IoStatus->Status = status;
        return TRUE;

    default:
        return FALSE;
    }
}

/* ------------------------------------------------------------------ */
/* CdpServerIoctl                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpServerIoctl(PCD_SERVER Server,
                                PIRP        Irp)
{
    PIO_STACK_LOCATION stack;
    PLIST_ENTRY        queue;
    NTSTATUS           status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.DeviceIoControl.IoControlCode != 0x500006) {
        Irp->IoStatus.Status      = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx((PULONG_PTR)&Server->Pipe.Lock, 0);

    if (stack->Parameters.DeviceIoControl.InputBufferLength < 0x20) {
        status = STATUS_INVALID_PARAMETER;
    } else if (Server->Pipe.Connected == FALSE) {
        status = STATUS_PIPE_DISCONNECTED;
    } else {
        queue = &Server->Pipe.DeliveryQueue;
        if (IsListEmpty(queue)) {
            IoSetCancelRoutine(Irp, CdpCancelReadIoIrp);
            if (Irp->Cancel) {
                if (IoSetCancelRoutine(Irp, NULL) != NULL) {
                    status = STATUS_CANCELLED;
                    goto Release;
                }
            }
            Server->Pipe.ReadIoIrp = Irp;
            IoMarkIrpPending(Irp);
            status = STATUS_PENDING;
        } else {
            status = CdpCompleteReadIoIrp(&Server->Pipe, Irp, NULL);
        }
    }

Release:
    ExReleasePushLockExclusiveEx((PULONG_PTR)&Server->Pipe.Lock, 0);
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status      = status;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpServerQuerySecurity                                             */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpServerQuerySecurity(PCD_SERVER Server,
                                        PIRP        Irp)
{
    PIO_STACK_LOCATION stack;
    PSECURITY_DESCRIPTOR descriptor;
    ULONG               length;
    NTSTATUS            status;

    stack  = IoGetCurrentIrpStackLocation(Irp);
    length = stack->Parameters.QuerySecurity.SecurityInformation;

    KeEnterCriticalRegion();
    ExAcquirePushLockSharedEx(&Server->Lock, 0);

    status = SeQuerySecurityDescriptorInfo(&stack->Parameters.QuerySecurity.SecurityInformation,
                                            Irp->UserBuffer,
                                            &length,
                                            &Server->SecurityDescriptor);

    ExReleasePushLockSharedEx(&Server->Lock, 0);
    KeLeaveCriticalRegion();

    if (status == STATUS_BUFFER_TOO_SMALL) {
        status = STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = length;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpServerSetSecurity                                               */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpServerSetSecurity(PCD_SERVER Server,
                                      PIRP        Irp)
{
    PIO_STACK_LOCATION   stack;
    PSECURITY_DESCRIPTOR newDescriptor;
    PSECURITY_DESCRIPTOR loggedDescriptor;
    NTSTATUS             status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(&Server->Lock, 0);

    newDescriptor = Server->SecurityDescriptor;

    status = SeSetSecurityDescriptorInfo(NULL,
                                          &stack->Parameters.SetSecurity.SecurityInformation,
                                          stack->Parameters.SetSecurity.SecurityDescriptor,
                                          &newDescriptor,
                                          PagedPool,
                                          &CdpGenericMapping);

    if (NT_SUCCESS(status)) {
        status = ObLogSecurityDescriptor(newDescriptor, &loggedDescriptor, 1);
        ExFreePoolWithTag(newDescriptor, 0);
        if (NT_SUCCESS(status)) {
            ObDereferenceSecurityDescriptor(Server->SecurityDescriptor, 1);
            Server->SecurityDescriptor = loggedDescriptor;
        }
    }

    ExReleasePushLockExclusiveEx(&Server->Lock, 0);
    KeLeaveCriticalRegion();

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpSetServerInformation                                            */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpSetServerInformation(PCD_SERVER                 Server,
                                          UCHAR                      Mode,
                                          PCD_IO_SERVER_INFORMATION  Information,
                                          ULONG                      Length)
{
    PVOID    inputEvent;
    NTSTATUS status;

    if (Length != 4) {
        return STATUS_INVALID_PARAMETER;
    }

    __try {
        if (Mode != KernelMode) {
            ProbeForRead(Information, Length, sizeof(ULONG));
        }
        inputEvent = Information->InputAvailableEvent;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_VIOLATION;
    }

    status = ObReferenceObjectByHandle(inputEvent, 0x100000, NULL, KernelMode,
                                        &Server->InputEvent, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(&Server->Lock, 0);

    if (Server->ServerProcessPid == NULL) {
        Server->ServerProcessPid = PsGetCurrentProcessId();
        Server->InputEvent       = Server->InputEvent;
    } else {
        status = STATUS_INVALID_PARAMETER;
        ObfDereferenceObject(Server->InputEvent);
    }

    ExReleasePushLockExclusiveEx(&Server->Lock, 0);
    KeLeaveCriticalRegion();

    return status;
}

