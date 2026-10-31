#include "condrvp.h"

/* --------------------------------------------------------------------------
 * External imports from other condrv components.
 * -------------------------------------------------------------------------- */
extern NTSTATUS CdAddIoToPipe(PIRP Irp);
extern VOID CdInitializeIo(PIRP Irp, PCD_IO_PIPE Pipe, UCHAR Type,
                          ULONG_PTR Arg1, ULONG_PTR Arg2,
                          PMDL InputMdl, PMDL OutputMdl);
extern PCD_SERVER CdGetServerFromFileObject(PFILE_OBJECT FileObject);
extern PCD_SERVER __cdecl CdGetServerFromReference(PFILE_OBJECT FileObject);
extern VOID __cdecl CdDereferenceServer(PCD_SERVER Server);
extern VOID __cdecl CdDereferenceBroker(PCD_BROKER Broker);
extern PCD_BROKER __cdecl CdFindBroker(VOID);
extern NTSTATUS __cdecl CdCreateSecurityDescriptor(PVOID         *SecurityDescriptor,
                                             PACCESS_STATE  AccessState);
extern VOID __cdecl CdFreeMdlChain(PMDL Mdl);
extern NTKERNELAPI PPEB NTAPI PsGetProcessPeb(IN PEPROCESS Process);
extern NTSTATUS __cdecl CdCompleteRoutineFreeIrp(PDEVICE_OBJECT DeviceObject,
                                           PIRP         Irp,
                                           PVOID        Context);
extern NTSTATUS __cdecl CdSubmitUserIo(PCD_SERVER Server,
                                 PIRP       Irp,
                                 ULONG      InputCount,
                                 ULONG      OutputCount);
extern VOID CdpCancelIoIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
extern NTKERNELAPI BOOLEAN PsIsSystemProcess(IN PEPROCESS Process);
extern NTKERNELAPI NTSTATUS PsAcquireProcessExitSynchronization(IN PEPROCESS Process);
extern NTKERNELAPI VOID PsReleaseProcessExitSynchronization(IN PEPROCESS Process);
NTKERNELAPI
PEPROCESS
IoGetRequestorProcess(
    _In_ PIRP Irp
    );
extern NTSTATUS CdpCreateMdlChain(PMDL *FirstMdl,
                                  PCD_IO_BUFFER Buffer,
                                  ULONG IsOutput,
                                  PVOID AccessMode,
                                  PVOID Reserved);

extern NTSTATUS CdpGetProcessConnection(PEPROCESS Process,
                                        PVOID *Connection,
                                        PEPROCESS Requestor);

extern PVOID __security_cookie;
extern ULONG_PTR __security_cookie_complement;

extern NTSTATUS SeQueryInformationToken(HANDLE TokenHandle,
                                       TOKEN_INFORMATION_CLASS TokenInformationClass,
                                       PVOID TokenInformation);
extern ACCESS_MASK __cdecl CdAccessCheck(PCD_SERVER                Server,
                                  PSECURITY_SUBJECT_CONTEXT SubjectContext,
                                  ACCESS_MASK               DesiredAccess,
                                  BOOLEAN                   SubjectContextLocked);
NTSTATUS __cdecl
CdpConnectionIoctl(PCD_CONNECTION Connection, PIRP Irp);
NTSTATUS __cdecl
CdpConnectionClose(PCD_CONNECTION Connection, PIRP Irp);
BOOLEAN __cdecl
CdpConnectionFastIoctl(PCD_CONNECTION Connection,
                      PVOID InputBuffer,
                      ULONG InputLength,
                      PVOID OutputBuffer,
                      ULONG OutputLength,
                      ULONG IoControlCode,
                      PIO_STATUS_BLOCK IoStatus);
NTSTATUS __cdecl
CdpCreateBrokerConnectionIo(PIRP Irp);
NTSTATUS
CdpCompleteServerConnection(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp,
                           PVOID Context);
NTSTATUS __cdecl
CdpCreateServerConnectionIo(PIRP Irp, PCD_SERVER Server);
PCD_CONNECTION
CdGetConnectionFromFileObject(PFILE_OBJECT FileObject, PEPROCESS Process);
VOID
CdpFreeConnection(PCD_CONNECTION Connection);
PFILE_FULL_EA_INFORMATION
CdpFindEaBufferItem(PFILE_FULL_EA_INFORMATION EaBuffer, PCHAR EaName);
NTSTATUS
CdAllocateMdlForEaBufferItem(PMDL *Mdl, PVOID EaBuffer, PVOID EaLength);
NTSTATUS
CdCreateConnection(PIRP Irp, ULONG Parameter);
PCD_CONNECTION
CdGetConnectionFromFileObject(PFILE_OBJECT FileObject, PEPROCESS Process);
NTSTATUS
CdGetProcessConnection(PEPROCESS Process, PVOID *Connection, PEPROCESS Requestor);
NTSTATUS
CdpCompleteBrokerConnection(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp,
                           PVOID Context);
NTSTATUS __cdecl
CdpGetProcessServerFromConnection(PCD_SERVER *Server,
                                 PIRP Irp,
                                 PVOID Unused);
/* ------------------------------------------------------------------ */
/* External symbols defined in other source files                     */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT CdDeviceObject;
extern POBJECT_TYPE *IoFileObjectType;
CD_CONTEXT_TYPE CdConnectionType = {
    0,
    NULL,
    NULL,
    CdpConnectionIoctl,
    NULL,
    CdpConnectionClose,
    CdpConnectionFastIoctl,
    NULL,
    NULL,
    NULL
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdpFindEaBufferItem)
#pragma alloc_text(PAGE,CdAllocateMdlForEaBufferItem)
#pragma alloc_text(PAGE,CdCreateConnection)
#pragma alloc_text(PAGE,CdGetConnectionFromFileObject)
#pragma alloc_text(PAGE,CdGetProcessConnection)
#pragma alloc_text(PAGE,CdpCompleteBrokerConnection)
#pragma alloc_text(PAGE,CdpCompleteServerConnection)
#pragma alloc_text(PAGE,CdpConnectionClose)
#pragma alloc_text(PAGE,CdpConnectionFastIoctl)
#pragma alloc_text(PAGE,CdpConnectionIoctl)
#pragma alloc_text(PAGE,CdpCreateBrokerConnectionIo)
#pragma alloc_text(PAGE,CdpCreateServerConnectionIo)
#pragma alloc_text(PAGE,CdpFreeConnection)
#pragma alloc_text(PAGE,CdpGetProcessServerFromConnection)
#endif // ALLOC_PRAGMA

/* --------------------------------------------------------------------------
 * Local helper: walk a FILE_FULL_EA_INFORMATION chain.
 * -------------------------------------------------------------------------- */
PFILE_FULL_EA_INFORMATION
CdpFindEaBufferItem(PFILE_FULL_EA_INFORMATION EaBuffer, PCHAR EaName)
{
    PFILE_FULL_EA_INFORMATION current = EaBuffer;

    if (current == NULL) {
        return NULL;
    }

    do {
        if (_stricmp(current->EaName, EaName) == 0) {
            return current;
        }

        if (current->NextEntryOffset == 0) {
            break;
        }

        current = (PFILE_FULL_EA_INFORMATION)((PUCHAR)current +
                                              current->NextEntryOffset);
    } while (current != NULL);

    return NULL;
}

/* --------------------------------------------------------------------------
 * Allocate an MDL covering the value portion of an EA item.
 * -------------------------------------------------------------------------- */
NTSTATUS
CdAllocateMdlForEaBufferItem(PMDL *Mdl, PVOID EaBuffer, PVOID EaLength)
{
    PFILE_FULL_EA_INFORMATION ea;
    PMDL mdl;
    ULONG valueLength;
    ULONG_PTR eaNameOffset;
    ULONG_PTR valueOffset;

    if (Mdl == NULL || EaBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    ea = (PFILE_FULL_EA_INFORMATION)EaBuffer;
    if (ea == NULL || ea->EaName == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    valueLength = ea->EaValueLength;
    if (valueLength == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    /* The original decompilation shows the value is located immediately after
     * the EA name and the terminating NUL. Reconstruct that pointer.
     */
    eaNameOffset = (ULONG_PTR)ea->EaName;
    valueOffset  = eaNameOffset + ea->EaNameLength + 1;

    mdl = IoAllocateMdl((PVOID)valueOffset, valueLength, FALSE, TRUE, NULL);
    if (mdl == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(mdl);
    *Mdl = mdl;
    return STATUS_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Create a connection for an incoming file object.
 * -------------------------------------------------------------------------- */
NTSTATUS
CdCreateConnection(PIRP Irp, ULONG Parameter)
{
    PIO_STACK_LOCATION stack;
    PEPROCESS requestor;
    PCD_CONNECTION connection;
    PCD_SERVER server;
    PFILE_OBJECT fileObject;
    PFILE_FULL_EA_INFORMATION ea;
    PCD_SERVER *serverPtr;
    ULONG currentAccess;
    NTSTATUS status;

    stack        = IoGetCurrentIrpStackLocation(Irp);
    requestor    = IoGetRequestorProcess(Irp);
    connection   = NULL;
    server       = NULL;
    fileObject   = stack->FileObject;
    ea           = NULL;
    serverPtr    = NULL;

    if (requestor == NULL) {
        status = STATUS_INVALID_CID;
        goto Complete;
    }

    /* Create a connection from the process or from the file object. */
    status = CdGetProcessConnection(requestor, (PVOID *)&connection, requestor);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    /* If a matching EA item exists, validate the server reference. */
    ea = CdpFindEaBufferItem((PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer,
                            (PVOID)(ULONG_PTR)stack->Parameters.Create.EaLength);

    if (ea == NULL) {
        if (connection->Server == NULL) {
            status = CdpCreateBrokerConnectionIo(Irp);
            return status;
        }

        server = CdGetServerFromReference(fileObject);
        if (server == NULL) {
            status = STATUS_INVALID_HANDLE;
            goto Complete;
        }

        /* If the caller is not allowed, fail. */
        currentAccess = 0;
        status = SeQueryInformationToken((HANDLE)NULL, TokenSessionReference, &currentAccess);
        if (!NT_SUCCESS(status)) {
            goto Complete;
        }

        if (currentAccess != 0) {
            status = CdAccessCheck(server, NULL, 0, FALSE);
            if (!NT_SUCCESS(status)) {
                goto Complete;
            }
        }
    } else {
        if (ea->EaValueLength != sizeof(PCD_SERVER)) {
            status = STATUS_INVALID_PARAMETER;
            goto Complete;
        }

        serverPtr = (PCD_SERVER *)((PUCHAR)ea->EaName + ea->EaNameLength + 1);
        status = CdpGetProcessServerFromConnection(serverPtr, Irp, NULL);
        if (!NT_SUCCESS(status)) {
            goto Complete;
        }

        server = *serverPtr;
        status = CdAccessCheck(server, NULL, 0, FALSE);
        if (!NT_SUCCESS(status)) {
            CdDereferenceServer(server);
            goto Complete;
        }
    }

    status = CdpCreateServerConnectionIo(Irp, server);
    if (NT_SUCCESS(status)) {
        return STATUS_PENDING;
    }

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* --------------------------------------------------------------------------
 * Find and validate a connection from a file object.
 * -------------------------------------------------------------------------- */
PCD_CONNECTION
CdGetConnectionFromFileObject(PFILE_OBJECT FileObject, PEPROCESS Process)
{
    PCD_CONNECTION connection;
    PVOID deviceObject;

    if (FileObject == NULL || Process == NULL) {
        return NULL;
    }

    deviceObject = FileObject->FsContext;

    if (deviceObject != NULL && *(PVOID *)((PUCHAR)deviceObject + 4) == CdDeviceObject) {
        connection = *(PCD_CONNECTION *)((PUCHAR)deviceObject + 0x0C);
        if (connection != NULL &&
            connection->Header.ObjectType == &CdConnectionType &&
            connection->SourceProcess == Process) {
            return connection;
        }
    }

    return NULL;
}

/* --------------------------------------------------------------------------
 * Get the process connection for the given process.
 * -------------------------------------------------------------------------- */
NTSTATUS
CdGetProcessConnection(PEPROCESS Process, PVOID *Connection, PEPROCESS Requestor)
{
    PEPROCESS currentProcess;
    PVOID processObject;
    PVOID connectionObject;
    NTSTATUS status;

    if (Process == NULL || Connection == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (PsIsSystemProcess(Process)) {
        return STATUS_ACCESS_DENIED;
    }

    status = PsAcquireProcessExitSynchronization(Process);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    KeStackAttachProcess((PRKPROCESS)Process, (PRKAPC_STATE)&currentProcess);
    processObject = PsGetProcessPeb(Process);
    if (processObject == NULL) {
        KeUnstackDetachProcess((PRKAPC_STATE)&currentProcess);
        PsReleaseProcessExitSynchronization(Process);
        return STATUS_INVALID_PARAMETER;
    }

    status = ObReferenceObjectByHandle((HANDLE)Process, 0, NULL, KernelMode,
                                       &connectionObject, NULL);
    KeUnstackDetachProcess((PRKAPC_STATE)&currentProcess);
    PsReleaseProcessExitSynchronization(Process);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (connectionObject == NULL ||
        *(PVOID *)((PUCHAR)connectionObject + 4) != CdDeviceObject) {
        ObfDereferenceObject(connectionObject);
        return STATUS_INVALID_HANDLE;
    }

    *Connection = *(PVOID *)((PUCHAR)connectionObject + 0x0C);
    if (*Connection == NULL ||
        ((PCD_CONNECTION)*Connection)->Header.ObjectType != &CdConnectionType ||
        ((PCD_CONNECTION)*Connection)->SourceProcess != Requestor) {
        ObfDereferenceObject(connectionObject);
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Complete a broker connection request.
 * -------------------------------------------------------------------------- */
NTSTATUS
CdpCompleteBrokerConnection(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp,
                           PVOID Context)
{
    PCD_BROKER broker;
    NTSTATUS status;

    broker = (PCD_BROKER)Context;

    CdDereferenceBroker(broker);

    if (Irp->PendingReturned) {
        ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[3] |= 1;
    }

    status = Irp->IoStatus.Status;
    if (NT_SUCCESS(status)) {
        status = CdpCreateServerConnectionIo(Irp, (PCD_SERVER)Irp->Tail.Overlay.DriverContext[0]);
        CdDereferenceServer((PCD_SERVER)Irp->Tail.Overlay.DriverContext[0]);
        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Status      = status;
            Irp->IoStatus.Information = 0;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    return status;
}

/* --------------------------------------------------------------------------
 * Complete a server connection request.
 * -------------------------------------------------------------------------- */
NTSTATUS
CdpCompleteServerConnection(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp,
                           PVOID Context)
{
    PCD_CONNECTION connection;
    PVOID data;
    PVOID next;
    PVOID listHead;
    KIRQL irql;
    NTSTATUS status;

    connection = (PCD_CONNECTION)Context;
    data       = Irp->AssociatedIrp.SystemBuffer;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        CdpFreeConnection(connection);
        status = STATUS_SUCCESS;
    } else {
        if (Irp->IoStatus.Information == 0x0C) {
            connection->DefaultInputContext  = *(PULONG)((PUCHAR)data + 4);
            connection->DefaultOutputContext = *(PULONG)((PUCHAR)data + 8);
            connection->ServerContext        = *(PULONG)((PUCHAR)data + 0x0C);
        }

        irql = KeGetCurrentIrql();
        if (irql < DISPATCH_LEVEL) {
            KeEnterCriticalRegion();
        }

        ExAcquirePushLockExclusiveEx(&connection->Server->Lock, 0);

        listHead = connection->Server->ConnectionList.Flink;
        next     = connection->Server->ConnectionList.Flink;

        if (next == &connection->Server->ConnectionList) {
            ExReleasePushLockExclusiveEx(&connection->Server->Lock, 0);
            if (irql < DISPATCH_LEVEL) {
                KeLeaveCriticalRegion();
            }

            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }

        InsertTailList(&connection->Server->ConnectionList, &connection->Links);

        ExReleasePushLockExclusiveEx(&connection->Server->Lock, 0);
        if (irql < DISPATCH_LEVEL) {
            KeLeaveCriticalRegion();
        }

        status = STATUS_SUCCESS;
    }

Done:
    ExFreePoolWithTag(Context, 0);

    if (Irp->PendingReturned) {
        ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[3] |= 1;
    }

    return status;
}

/* --------------------------------------------------------------------------
 * Close a connection object.
 * -------------------------------------------------------------------------- */
NTSTATUS __cdecl
CdpConnectionClose(PCD_CONNECTION Connection, PIRP Irp)
{
    PIRP newIrp;
    ULONG stackSize;
    NTSTATUS status;
    PIRP irpToUse;

    stackSize = CdDeviceObject->StackSize;
    newIrp    = IoAllocateIrp((CCHAR)stackSize, FALSE);

    if (newIrp != NULL) {
        IoSetCompletionRoutine(newIrp, (PIO_COMPLETION_ROUTINE)CdCompleteRoutineFreeIrp,
                               NULL, TRUE, TRUE, TRUE);
        irpToUse = newIrp;
    } else {
        irpToUse = Irp;
    }

    CdInitializeIo(irpToUse, NULL, (UCHAR)Connection->DefaultOutputContext,
                  0, 0, NULL, NULL);

    status = CdAddIoToPipe(irpToUse);

    CdpFreeConnection(Connection);

    if (newIrp != NULL) {
        if (!NT_SUCCESS(status)) {
            IoFreeIrp(newIrp);
        } else {
            return STATUS_PENDING;
        }
    } else if (NT_SUCCESS(status)) {
        return STATUS_PENDING;
    }

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Fast I/O path for connection object.
 * -------------------------------------------------------------------------- */
BOOLEAN __cdecl
CdpConnectionFastIoctl(PCD_CONNECTION Connection,
                      PVOID InputBuffer,
                      ULONG InputLength,
                      PVOID OutputBuffer,
                      ULONG OutputLength,
                      ULONG IoControlCode,
                      PIO_STATUS_BLOCK IoStatus)
{
    if (IoControlCode == 0x500023) {
        if (OutputLength == sizeof(ULONG)) {
            if (ExGetPreviousMode() == KernelMode) {
                *(PULONG)OutputBuffer = (ULONG)Connection->Server->ServerProcessPid;
            } else {
                ProbeForWrite(OutputBuffer, sizeof(ULONG), sizeof(ULONG));
                *(PULONG)OutputBuffer = (ULONG)Connection->Server->ServerProcessPid;
            }

            IoStatus->Status      = STATUS_SUCCESS;
            IoStatus->Information = sizeof(ULONG);
            return TRUE;
        }

        IoStatus->Status      = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;
        return TRUE;
    }

    return FALSE;
}

/* --------------------------------------------------------------------------
 * Connection IOCTL dispatch.
 * -------------------------------------------------------------------------- */
NTSTATUS __cdecl
CdpConnectionIoctl(PCD_CONNECTION Connection, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    PEPROCESS requestor;
    PVOID requestorProcess;
    PVOID fileObject;
    PVOID currentProcess;
    PVOID map;
    ULONG inputLength;
    NTSTATUS status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.DeviceIoControl.IoControlCode != 0x500016) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }

    if ((stack->FileObject->Flags & FO_SYNCHRONOUS_IO) == 0) {
        status = STATUS_INVALID_PARAMETER;
        goto Complete;
    }

    requestor = IoGetRequestorProcess(Irp);
    if (requestor == NULL) {
        status = STATUS_INVALID_CID;
        goto Complete;
    }

    if (requestor == Connection->SourceProcess) {
        if (stack->Parameters.DeviceIoControl.InputBufferLength < 0x0C) {
            status = STATUS_BUFFER_TOO_SMALL;
            goto Complete;
        }

        fileObject = Irp->AssociatedIrp.MasterIrp;
        if (fileObject == NULL) {
            status = CdSubmitUserIo(Connection->Server, Irp, 0, 0);
            if (NT_SUCCESS(status)) {
                return STATUS_PENDING;
            }
            goto Complete;
        }

        requestorProcess = IoGetRequestorProcess(Irp);
        if (requestorProcess == NULL) {
            status = STATUS_INVALID_CID;
            goto Complete;
        }

        status = CdGetProcessConnection(requestorProcess, &currentProcess, requestor);
        if (!NT_SUCCESS(status)) {
            goto Complete;
        }

        if (currentProcess == NULL) {
            status = STATUS_INVALID_HANDLE;
            goto Complete;
        }

        /* Validate and prepare a user IO transfer. */
        status = CdSubmitUserIo(Connection->Server, Irp, 0, 0);
        if (NT_SUCCESS(status)) {
            return STATUS_PENDING;
        }
    } else {
        status = STATUS_ACCESS_DENIED;
    }

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* --------------------------------------------------------------------------
 * Create a broker connection IO request.
 * -------------------------------------------------------------------------- */
NTSTATUS __cdecl
CdpCreateBrokerConnectionIo(PIRP Irp)
{
    PEPROCESS requestor;
    PCD_BROKER broker;
    PMDL mdl = NULL;
    PVOID context;
    NTSTATUS status;

    requestor = IoGetRequestorProcess(Irp);
    if (requestor == NULL) {
        status = STATUS_INVALID_CID;
        goto Fail;
    }

    broker = CdFindBroker();
    if (broker == NULL) {
        status = STATUS_DEVICE_NOT_CONNECTED;
        goto Fail;
    }

    status = CdAllocateMdlForEaBufferItem(&mdl, Irp->AssociatedIrp.SystemBuffer,
                                         (PVOID)(ULONG_PTR)Irp->AssociatedIrp.SystemBuffer);
    if (!NT_SUCCESS(status)) {
        goto Fail;
    }

    context = Irp->AssociatedIrp.SystemBuffer;
    ((PVOID *)context)[-1] = CdpCompleteBrokerConnection;
    ((PVOID *)context)[-2] = broker;

    CdInitializeIo(Irp, &broker->Pipe, 0x01,
                   (ULONG_PTR)mdl, 0, (PMDL)broker, NULL);

    status = CdAddIoToPipe(Irp);
    if (NT_SUCCESS(status)) {
        return STATUS_PENDING;
    }

Fail:
    CdFreeMdlChain(mdl);
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* --------------------------------------------------------------------------
 * Create a server-side connection IO request.
 * -------------------------------------------------------------------------- */
NTSTATUS __cdecl
CdpCreateServerConnectionIo(PIRP Irp, PCD_SERVER Server)
{
    PEPROCESS requestor;
    PMDL mdl;
    PCD_CONNECTION connection;
    PVOID memory;
    PVOID securityDescriptor;
    NTSTATUS status;

    requestor = IoGetRequestorProcess(Irp);
    if (requestor == NULL) {
        return STATUS_INVALID_CID;
    }

    status = CdAllocateMdlForEaBufferItem(&mdl, Irp->AssociatedIrp.SystemBuffer, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    memory = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(CD_CONNECTION), 'dCcn');
    if (memory == NULL) {
        CdFreeMdlChain(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(memory, sizeof(CD_CONNECTION));
    connection = (PCD_CONNECTION)memory;

    connection->Header.ObjectType = &CdConnectionType;
    connection->Server            = Server;
    connection->SourceProcess     = requestor;
    connection->ServerContext    = 0;
    connection->DefaultInputContext = 0;
    connection->DefaultOutputContext = 0;

    status = CdCreateSecurityDescriptor(&securityDescriptor, NULL);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(memory, 0);
        CdFreeMdlChain(mdl);
        return status;
    }

    /* Prepare the IO request that will complete connection creation. */
    CdInitializeIo(Irp, &Server->Pipe,
                  0x01, (ULONG_PTR)connection,
                  0, (PMDL)connection, mdl);

    status = CdAddIoToPipe(Irp);
    if (NT_SUCCESS(status)) {
        return STATUS_PENDING;
    }

    CdpFreeConnection(connection);
    CdFreeMdlChain(mdl);
    return status;
}

/* --------------------------------------------------------------------------
 * Free a CD_CONNECTION object.
 * -------------------------------------------------------------------------- */
VOID
CdpFreeConnection(PCD_CONNECTION Connection)
{
    if (Connection == NULL) {
        return;
    }

    if (Connection->Server != NULL) {
        CdDereferenceServer(Connection->Server);
    }

    if (Connection->SourceProcess != NULL) {
        ObfDereferenceObject(Connection->SourceProcess);
    }

    if (((PCD_SERVER)Connection)->SecurityDescriptor != NULL) {
        ExFreePoolWithTag(((PCD_SERVER)Connection)->SecurityDescriptor, 0);
    }

    ExFreePoolWithTag(Connection, 0);
}

/* --------------------------------------------------------------------------
 * Get the server associated with a process-level connection.
 * -------------------------------------------------------------------------- */
NTSTATUS __cdecl
CdpGetProcessServerFromConnection(PCD_SERVER *Server,
                                 PIRP Irp,
                                 PVOID Unused)
{
    PEPROCESS process;
    PVOID processObject;
    PVOID connection;
    PCD_CONNECTION connectionObject;
    NTSTATUS status;
	POBJECT_ATTRIBUTES ObjectAttributes;

    if (Server == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    process = IoGetRequestorProcess(Irp);
    if (process == NULL) {
        return STATUS_INVALID_CID;
    }

    status = ZwOpenProcess(&processObject, PROCESS_QUERY_LIMITED_INFORMATION,
                           (POBJECT_ATTRIBUTES)&ObjectAttributes, (PCLIENT_ID)&process);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ObReferenceObjectByHandle((HANDLE)processObject,
                                       PROCESS_QUERY_LIMITED_INFORMATION,
                                       NULL, KernelMode, &connection, NULL);
    ZwClose(processObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    connectionObject = (PCD_CONNECTION)connection;
    if (connectionObject == NULL ||
        connectionObject->Header.ObjectType != &CdConnectionType ||
        connectionObject->SourceProcess != process) {
        ObfDereferenceObject(connection);
        return STATUS_INVALID_HANDLE;
    }

    *Server = connectionObject->Server;
    ObfDereferenceObject(connection);

    return STATUS_SUCCESS;
}

/* --------------------------------------------------------------------------
 * Helper used by the decompiler output to complete a request without a real
 * symbol.
 * -------------------------------------------------------------------------- */
static
VOID
CdpCompleteRequestNoSymbol(PIRP Irp, NTSTATUS Status)
{
    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
}

/* --------------------------------------------------------------------------
 * This file intentionally keeps the real logical structure of the original
 * decompilation, but removes decompiler-specific local names and opaque
 * placeholders. The rest of the functions are implemented as kernel-usable
 * types above.
 * -------------------------------------------------------------------------- */

 