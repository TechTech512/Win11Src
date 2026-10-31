#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions                                                  */
/* ------------------------------------------------------------------ */
extern VOID     CdInitializeIo(PIRP Irp, PCD_IO_PIPE Pipe, UCHAR Type,
                                ULONG_PTR Arg1, ULONG_PTR Arg2,
                                PMDL InputMdl, PMDL OutputMdl);
extern NTSTATUS CdAddIoToPipe(PIRP Irp);
extern NTSTATUS CdAllocateMdlForEaBufferItem(PMDL *Mdl, PVOID A, PVOID B);
NTKERNELAPI
PEPROCESS
IoGetRequestorProcess(
    _In_ PIRP Irp
    );
extern NTSTATUS CdGetProcessConnection(PEPROCESS Process,
                                        PVOID *Connection,
                                        PEPROCESS Requestor);
extern VOID     CdDisconnectIoPipe(PCD_IO_PIPE Pipe);
extern VOID __cdecl CdDereferenceServer(PCD_SERVER Server);
extern PVOID    CdGetConnectionFromFileObject(PFILE_OBJECT FileObject,
                                               PEPROCESS Process);
extern PCD_SERVER __cdecl CdGetServerFromFileObject(PFILE_OBJECT FileObject);
extern VOID     CdpCancelIoIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);

extern NTSTATUS CdpReadIoInput(PCD_IO_PIPE Pipe,
                                PCD_IO_BUFFER_DESCRIPTOR Buffer,
                                PULONG BytesRead);

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */
PCD_CLIENT __cdecl CdpAllocateClient(VOID);
VOID __cdecl CdpFreeClient(PCD_CLIENT Client);
NTSTATUS __cdecl CdClientUserIo(PCD_CLIENT Client, PIRP Irp);
NTSTATUS __cdecl CdpGetClientTarget(PCD_CLIENT        Target,
                                     PIRP               Irp,
                                     PCD_CLIENT_TARGET  Reserved);
NTSTATUS __cdecl CdpConvertClientDesiredAccess(PULONG      DesiredAccess,
                                               ACCESS_MASK AccessMask);
NTSTATUS __cdecl CdpCompleteClientCreation(PDEVICE_OBJECT DeviceObject,
                                            PIRP         Irp,
                                            PVOID        Context);
NTSTATUS __cdecl CdpClientRead(PCD_CLIENT Client, PIRP Irp);
NTSTATUS __cdecl CdpClientWrite(PCD_CLIENT Client, PIRP Irp);
NTSTATUS __cdecl CdpClientIoctl(PCD_CLIENT Client, PIRP Irp);
NTSTATUS __cdecl CdpClientClose(PCD_CLIENT Client, PIRP Irp);
NTSTATUS __cdecl CdpClientFlushBuffers(PCD_CLIENT Client, PIRP Irp);
VOID __cdecl CdFreeMdlChain(PMDL Mdl);
NTSTATUS __cdecl CdCreateDefaultObjectClient(PIRP Irp, ULONG Parameter);
NTSTATUS __cdecl CdCreateClient(PIRP Irp, ULONG Parameter);
NTSTATUS __cdecl CdCompleteRoutineFreeIrp(PDEVICE_OBJECT DeviceObject,
                                           PIRP         Irp,
                                           PVOID        Context);
NTSTATUS __cdecl CdSubmitUserIo(PCD_SERVER Server,
                                 PIRP       Irp,
                                 ULONG      InputCount,
                                 ULONG      OutputCount);
NTSTATUS __cdecl CdpCreateMdlChain(PMDL          *FirstMdl,
                                    PCD_IO_BUFFER  Buffer,
                                    ULONG          IsOutput,
                                    PVOID          AccessMode,
                                    PVOID          Reserved);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdClientUserIo)
#pragma alloc_text(PAGE,CdCompleteRoutineFreeIrp)
#pragma alloc_text(PAGE,CdCreateClient)
#pragma alloc_text(PAGE,CdCreateDefaultObjectClient)
#pragma alloc_text(PAGE,CdFreeMdlChain)
#pragma alloc_text(PAGE,CdpAllocateClient)
#pragma alloc_text(PAGE,CdpClientClose)
#pragma alloc_text(PAGE,CdpClientFlushBuffers)
#pragma alloc_text(PAGE,CdpClientIoctl)
#pragma alloc_text(PAGE,CdpClientRead)
#pragma alloc_text(PAGE,CdpClientWrite)
#pragma alloc_text(PAGE,CdpCompleteClientCreation)
#pragma alloc_text(PAGE,CdpConvertClientDesiredAccess)
#pragma alloc_text(PAGE,CdpCreateMdlChain)
#pragma alloc_text(PAGE,CdpFreeClient)
#pragma alloc_text(PAGE,CdpGetClientTarget)
#endif // ALLOC_PRAGMA
/* ------------------------------------------------------------------ */
/* External symbols                                                    */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT  CdDeviceObject;
CD_CONTEXT_TYPE CdClientType = {0, CdpClientRead, CdpClientWrite, CdpClientIoctl, NULL, CdpClientClose, NULL, CdpClientFlushBuffers, NULL, NULL};
extern CD_CONTEXT_TYPE CdServerType;
POBJECT_TYPE    *IoFileObjectType;
extern GENERIC_MAPPING CdpGenericMapping;

/* ------------------------------------------------------------------ */
/* CdClientUserIo                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdClientUserIo(PCD_CLIENT Client, PIRP Irp)
{
    CD_CLIENT localClient;
    NTSTATUS  status;

    status = CdpGetClientTarget(&localClient, Irp, NULL);
    if (NT_SUCCESS(status)) {
        status = CdSubmitUserIo((PCD_SERVER)localClient.Header.ObjectType,
                                 (PIRP)localClient.Server,
                                 0, 0);
        ObfDereferenceObject(localClient.Server);
        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdCompleteRoutineFreeIrp                                           */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCompleteRoutineFreeIrp(PDEVICE_OBJECT DeviceObject,
                                           PIRP         Irp,
                                           PVOID        Context)
{
    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* ------------------------------------------------------------------ */
/* CdCreateClient                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateClient(PIRP Irp, ULONG Parameter)
{
    PIO_STACK_LOCATION   stack;
    PIO_SECURITY_CONTEXT securityContext;
    PEPROCESS            requestor;
    PCD_SERVER           connection;
    PCD_CLIENT           client;
    PKEVENT              inputEvent;
    PMDL                 clientMdl;
    PMDL                 eventMdl;
    PVOID                eaBuffer;
    PUCHAR               clientData;
    ACCESS_MASK          desiredAccess;
    NTSTATUS             status;

    stack             = IoGetCurrentIrpStackLocation(Irp);
    securityContext   = stack->Parameters.Create.SecurityContext;
    requestor         = IoGetRequestorProcess(Irp);
    connection        = NULL;
    client            = NULL;
    inputEvent        = NULL;
    clientMdl         = NULL;
    eventMdl          = NULL;
    eaBuffer          = NULL;
    clientData        = NULL;

    if (requestor == NULL) {
        status = STATUS_INVALID_CID;
        goto Complete;
    }

    clientData = ExAllocatePoolWithQuotaTag(
        (POOL_TYPE)(NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
        0x10, 'dCcl');

    if (clientData == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    status = CdGetProcessConnection(requestor, &connection, requestor);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockSharedEx(&connection->Lock, 0);
    inputEvent = connection->InputEvent;
    ExReleasePushLockSharedEx(&connection->Lock, 0);
    KeLeaveCriticalRegion();

    if (inputEvent == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto Complete;
    }

    desiredAccess = securityContext->DesiredAccess;
    status = CdpConvertClientDesiredAccess((PULONG)&stack->Parameters.Create.SecurityContext->DesiredAccess,
                                            desiredAccess);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    clientMdl = IoAllocateMdl(clientData, 0xc, FALSE, TRUE, NULL);
    if (clientMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    MmBuildMdlForNonPagedPool(clientMdl);

    status = CdAllocateMdlForEaBufferItem(&eventMdl,
                                           Irp->AssociatedIrp.SystemBuffer,
                                           (PVOID)(ULONG_PTR)stack->Parameters.Create.EaLength);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    client = CdpAllocateClient();
    if (client == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    client->Server     = connection;
    InterlockedIncrement((LONG *)&connection->RefCount);
    client->InputEvent = inputEvent;
    ObfReferenceObject(inputEvent);

    stack->FileObject->FsContext       = client;
    stack->FileObject->FsContext2      = inputEvent;
    stack->FileObject->Flags          |= FO_SYNCHRONOUS_IO;

    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)CdpCompleteClientCreation, clientData,
                            TRUE, TRUE, TRUE);

    CdInitializeIo(Irp, &((PCD_SERVER)connection)->Pipe,
                    (UCHAR)Parameter, (ULONG_PTR)clientMdl,
                    0, (PMDL)client, (PMDL)inputEvent);

    status = CdAddIoToPipe(Irp);
    if (NT_SUCCESS(status)) {
        ObfDereferenceObject(connection);
        return STATUS_PENDING;
    }

Complete:
    CdFreeMdlChain((PMDL)clientData);
    if (clientData != NULL) {
        ExFreePoolWithTag(clientData, 0);
    }
    if (client != NULL) {
        CdpFreeClient(client);
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdCreateDefaultObjectClient                                        */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateDefaultObjectClient(PIRP Irp, ULONG Parameter)
{
    PIO_STACK_LOCATION stack;
    PEPROCESS          requestor;
    PCD_CONNECTION     connection;
    PCD_SERVER         server;
    PCD_CLIENT         client;
    PKEVENT            inputEvent;
    NTSTATUS           status;

    stack     = IoGetCurrentIrpStackLocation(Irp);
    requestor = IoGetRequestorProcess(Irp);

    if (requestor == NULL) {
        status = STATUS_INVALID_CID;
        goto Complete;
    }

    if ((stack->Parameters.Create.SecurityContext->DesiredAccess & 0xFFEDFE40) != 0) {
        status = STATUS_ACCESS_DENIED;
        goto Complete;
    }

    connection = NULL;
    server     = NULL;

    if (stack->FileObject->RelatedFileObject == NULL) {
        status = CdGetProcessConnection(requestor, &connection, requestor);
        if (!NT_SUCCESS(status)) {
            goto Complete;
        }
    } else {
        connection = CdGetConnectionFromFileObject(stack->FileObject, requestor);
        if (connection == NULL) {
            server = CdGetServerFromFileObject(stack->FileObject);
            if (server == NULL) {
                status = STATUS_INVALID_HANDLE;
                goto Complete;
            }
        } else {
            server = connection->Server;
        }
        ObfReferenceObject(server);
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockSharedEx(&((PCD_SERVER)connection)->Lock, 0);
    inputEvent = ((PCD_SERVER)connection)->InputEvent;
    if (inputEvent != NULL) {
        ObfReferenceObject(inputEvent);
    }
    ExReleasePushLockSharedEx(&((PCD_SERVER)connection)->Lock, 0);
    KeLeaveCriticalRegion();

    ObfDereferenceObject(connection);

    if (inputEvent == NULL) {
        status = STATUS_INVALID_HANDLE;
        goto Complete;
    }

    client = CdpAllocateClient();
    if (client == NULL) {
        ObfDereferenceObject(inputEvent);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    client->Server            = NULL;
    client->u.ObjectContext   = Parameter;
    client->InputEvent        = inputEvent;

    stack->FileObject->FsContext  = client;
    stack->FileObject->FsContext2 = inputEvent;
    stack->FileObject->Flags     |= FO_SYNCHRONOUS_IO;

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
/* CdFreeMdlChain                                                     */
/* ------------------------------------------------------------------ */
VOID __cdecl CdFreeMdlChain(PMDL Mdl)
{
    PMDL next;

    while (Mdl != NULL) {
        next = Mdl->Next;
        if (Mdl->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages(Mdl);
        }
        IoFreeMdl(Mdl);
        Mdl = next;
    }
}

/* ------------------------------------------------------------------ */
/* CdpAllocateClient                                                  */
/* ------------------------------------------------------------------ */
PCD_CLIENT __cdecl CdpAllocateClient(VOID)
{
    PCD_CLIENT client;

    client = ExAllocatePoolWithTag(NonPagedPool, sizeof(CD_CLIENT), 'ClCd');
    if (client == NULL) {
        return NULL;
    }

    client->Header.ObjectType = NULL;
    client->Server            = NULL;
    client->u.ObjectContext   = 0;
    client->InputEvent        = NULL;
    client->Header.ObjectType = &CdClientType;

    return client;
}

/* ------------------------------------------------------------------ */
/* CdpClientClose                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpClientClose(PCD_CLIENT Client, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    PIRP               newIrp;
    ULONG              stackSize;
    NTSTATUS           status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (Client->Server == NULL) {
        CdpFreeClient(Client);
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    stackSize = CdDeviceObject->StackSize;
    newIrp    = IoAllocateIrp((CCHAR)stackSize, FALSE);
    if (newIrp != NULL) {
        IoSetCompletionRoutine(newIrp, (PIO_COMPLETION_ROUTINE)CdCompleteRoutineFreeIrp, NULL,
                                TRUE, TRUE, TRUE);
    }

    CdInitializeIo(newIrp != NULL ? newIrp : Irp,
                    NULL,
                    (UCHAR)Client->u.ObjectContext,
                    0, 0, NULL, NULL);

    status = CdAddIoToPipe(newIrp != NULL ? newIrp : Irp);

    CdpFreeClient(Client);

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

/* ------------------------------------------------------------------ */
/* CdpClientFlushBuffers                                              */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpClientFlushBuffers(PCD_CLIENT Client, PIRP Irp)
{
    CD_CLIENT localClient;
    NTSTATUS  status;

    status = CdpGetClientTarget(&localClient, Irp, NULL);
    if (NT_SUCCESS(status)) {
        CdInitializeIo(Irp, (PCD_IO_PIPE)localClient.Server,
                        (UCHAR)localClient.u.ObjectContext,
                        0, 0, NULL, NULL);
        status = CdAddIoToPipe(Irp);
        ObfDereferenceObject(localClient.Server);
        if (NT_SUCCESS(status)) {
            return STATUS_PENDING;
        }
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpClientIoctl                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpClientIoctl(PCD_CLIENT Client, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    NTSTATUS           status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.DeviceIoControl.IoControlCode != 0x500016) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }

    if ((stack->FileObject->Flags & FO_SYNCHRONOUS_IO) == 0) {
        status = STATUS_INVALID_PARAMETER;
        goto Complete;
    }

    return CdClientUserIo(Client, Irp);

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpClientRead                                                      */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpClientRead(PCD_CLIENT Client, PIRP Irp)
{
    CD_CLIENT localClient;
    NTSTATUS  status;

    status = CdpGetClientTarget(&localClient, Irp, NULL);
    if (NT_SUCCESS(status)) {
        CdInitializeIo(Irp, (PCD_IO_PIPE)localClient.Server,
                        (UCHAR)localClient.u.ObjectContext,
                        0, (ULONG_PTR)Irp->MdlAddress,
                        NULL, NULL);
        status = CdAddIoToPipe(Irp);
        ObfDereferenceObject(localClient.Server);
        if (NT_SUCCESS(status)) {
            return STATUS_PENDING;
        }
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpClientWrite                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpClientWrite(PCD_CLIENT Client, PIRP Irp)
{
    CD_CLIENT          localClient;
    PIO_STACK_LOCATION stack;
    NTSTATUS           status;

    status = CdpGetClientTarget(&localClient, Irp, NULL);
    if (!NT_SUCCESS(status)) {
        goto Complete;
    }

    stack = IoGetCurrentIrpStackLocation(Irp);

    stack->Parameters.Write.Length                = 0x50F;
    stack->Parameters.Write.ByteOffset.LowPart    = localClient.u.ObjectContext + 0x20;
    stack->Parameters.Write.Key                   = 0;
    stack->Parameters.Write.ByteOffset.HighPart   = 0;

    Irp->Tail.Overlay.DriverContext[0] = localClient.Header.ObjectType;

    ZwAllocateLocallyUniqueId((PLUID)&Irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset.QuadPart);

    status = CdAddIoToPipe(Irp);
    ObfDereferenceObject(localClient.Server);
    if (NT_SUCCESS(status)) {
        return STATUS_PENDING;
    }

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpCompleteClientCreation                                          */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpCompleteClientCreation(PDEVICE_OBJECT DeviceObject,
                                            PIRP         Irp,
                                            PVOID        Context)
{
    PCD_CLIENT client;

    client = (PCD_CLIENT)Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        CdpFreeClient(client);
    } else {
        *((PULONG)client + 2) = (ULONG)Irp->IoStatus.Information;
    }

    ExFreePoolWithTag(Context, 0);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpConvertClientDesiredAccess                                      */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpConvertClientDesiredAccess(PULONG      DesiredAccess,
                                               ACCESS_MASK AccessMask)
{
    ULONG converted;

    converted = 0;

    if ((AccessMask & 0x120089) == 0x120089) {
        converted |= 0x80000000;
    }
    if ((AccessMask & 0x120116) == 0x120116) {
        converted |= 0x40000000;
    }
    if ((AccessMask & 0x1200A0) == 0x1200A0) {
        converted |= 0x20000000;
    }
    if ((AccessMask & 0xFFEDFE40) == 0) {
        *DesiredAccess = converted;
        return STATUS_SUCCESS;
    }
    return STATUS_ACCESS_DENIED;
}

/* ------------------------------------------------------------------ */
/* CdpCreateMdlChain                                                  */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpCreateMdlChain(PMDL          *FirstMdl,
                                    PCD_IO_BUFFER  Buffer,
                                    ULONG          IsOutput,
                                    PVOID          AccessMode,
                                    PVOID          Reserved)
{
    PMDL    head;
    PMDL    tail;
    ULONG   index;
    SIZE_T  length;
    PVOID   address;
    PMDL    mdl;

    head = NULL;
    tail = NULL;

    for (index = 0; index < Buffer->Size; index++) {
        length  = ((PULONG)Buffer->Buffer)[index * 2];
        address = (PVOID)((PULONG_PTR)Buffer->Buffer)[index * 2 + 1];

        if (length == 0) {
            continue;
        }

        mdl = IoAllocateMdl(address, (ULONG)length, FALSE, TRUE, NULL);
        if (mdl == NULL) {
            CdFreeMdlChain(head);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        __try {
            MmProbeAndLockPages(mdl, (KPROCESSOR_MODE)(ULONG_PTR)AccessMode,
                                 IsOutput ? IoWriteAccess : IoReadAccess);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            IoFreeMdl(mdl);
            CdFreeMdlChain(head);
            return STATUS_ACCESS_VIOLATION;
        }

        if (head == NULL) {
            head = mdl;
        } else {
            tail->Next = mdl;
        }
        tail = mdl;
    }

    *FirstMdl = head;
    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpFreeClient                                                      */
/* ------------------------------------------------------------------ */
VOID __cdecl CdpFreeClient(PCD_CLIENT Client)
{
    if (Client->Server != NULL) {
        CdDereferenceServer(Client->Server);
    }
    if (Client->InputEvent != NULL) {
        ObfDereferenceObject(Client->InputEvent);
    }
    ExFreePoolWithTag(Client, 'ClCd');
}

/* ------------------------------------------------------------------ */
/* CdpGetClientTarget                                                 */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpGetClientTarget(PCD_CLIENT        Target,
                                     PIRP               Irp,
                                     PCD_CLIENT_TARGET  Reserved)
{
    PEPROCESS      requestor;
    PVOID          connection;
    PCD_CLIENT     client;
    NTSTATUS       status;

    requestor  = IoGetRequestorProcess(Irp);
    connection = NULL;

    if (requestor == NULL) {
        return STATUS_INVALID_CID;
    }

    status = CdGetProcessConnection(requestor, &connection, requestor);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    client = (PCD_CLIENT)((PIO_STACK_LOCATION)Irp->Tail.Overlay.CurrentStackLocation)->FileObject->FsContext;

    if (client->Server != NULL &&
        client->Server != ((PCD_CONNECTION)connection)->Server) {
        ObfDereferenceObject(connection);
        return STATUS_INVALID_HANDLE;
    }

    Target->u          = ((PCD_CLIENT)connection)->u;
    Target->InputEvent = ((PCD_CLIENT)connection)->InputEvent;
    Target->Header.ObjectType = *(PCD_CONTEXT_TYPE *)((PUCHAR)connection + 0x14);

    if (client->Server != NULL) {
        Target->Server = client->Server;
        return STATUS_SUCCESS;
    }

    if (client->u.ObjectContext == 1) {
        Target->Server = ((PCD_CONNECTION)connection)->Server;
        return STATUS_SUCCESS;
    }

    Target->Server = ((PCD_CONNECTION)connection)->Server;
    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdSubmitUserIo                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdSubmitUserIo(PCD_SERVER Server,
                                 PIRP       Irp,
                                 ULONG      InputCount,
                                 ULONG      OutputCount) // do NOT page
{
    PIO_STACK_LOCATION   stack;
    PCD_IO_BUFFER        userIo;
    PMDL                 inputMdl;
    PMDL                 outputMdl;
    PCD_IO_PIPE          pipe;
    PCD_CLIENT           client;
    PKEVENT              inputEvent;
    KIRQL                irql;
    ULONG                i;
    PVOID                inputSystemVa;
    PVOID                outputSystemVa;
    ULONG                inputTotal;
    ULONG                outputTotal;
    NTSTATUS             status;
    BOOLEAN              locked;
    PMDL                 mdl;
    BOOLEAN              pending;

    if (PsGetCurrentProcess() != IoGetRequestorProcess(Irp)) {
        return STATUS_INVALID_PARAMETER;
    }

    stack  = IoGetCurrentIrpStackLocation(Irp);
    client = (PCD_CLIENT)stack->FileObject->FsContext;

    if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CD_USER_DEFINED_IO)) {
        return STATUS_INVALID_PARAMETER;
    }

    userIo = (PCD_IO_BUFFER)stack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (userIo->Size > (stack->Parameters.DeviceIoControl.InputBufferLength - 0xc) / 8) {
        return STATUS_INVALID_PARAMETER;
    }

    inputMdl  = NULL;
    outputMdl = NULL;

    status = CdpCreateMdlChain(&inputMdl, userIo, FALSE,
                                (PVOID)(ULONG_PTR)Irp->RequestorMode, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = CdpCreateMdlChain(&outputMdl, (PCD_IO_BUFFER)((PUCHAR)userIo + 8),
                                TRUE,
                                (PVOID)(ULONG_PTR)Irp->RequestorMode,
                                (PVOID)(ULONG_PTR)inputMdl);
    if (!NT_SUCCESS(status)) {
        CdFreeMdlChain(inputMdl);
        return status;
    }

    IoSetNextIrpStackLocation(Irp);
    stack = IoGetCurrentIrpStackLocation(Irp);
    stack->Parameters.Write.Length             = sizeof(CD_IO_COMPLETE);
    stack->Parameters.Write.ByteOffset.QuadPart = (ULONG_PTR)userIo + 0x20;
    stack->Parameters.Write.Key                = 0;

    Irp->Tail.Overlay.DriverContext[0] = NULL;

    ZwAllocateLocallyUniqueId((PLUID)&Irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset.QuadPart);

    pipe    = &Server->Pipe;
    irql    = KeGetCurrentIrql();
    locked  = FALSE;
    status  = STATUS_SUCCESS;

    if (irql < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
    }
    ExAcquirePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);
    locked = TRUE;

    /* -------------------------------------------------------------- */
    /* Map MDLs into system space                                     */
    /* -------------------------------------------------------------- */
    inputTotal  = 0;
    outputTotal = 0;

    for (mdl = inputMdl; mdl != NULL; mdl = mdl->Next) {
        if ((mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) == 0) {
            if (MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached,
                                              NULL, FALSE,
                                              NormalPagePriority) == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Release;
            }
        }
        inputTotal += mdl->ByteCount;
        if (inputTotal < mdl->ByteCount) {
            status = STATUS_INVALID_PARAMETER;
            goto Release;
        }
    }

    for (mdl = outputMdl; mdl != NULL; mdl = mdl->Next) {
        if ((mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) == 0) {
            if (MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached,
                                              NULL, FALSE,
                                              NormalPagePriority) == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Release;
            }
        }
        outputTotal += mdl->ByteCount;
        if (outputTotal < mdl->ByteCount) {
            status = STATUS_INVALID_PARAMETER;
            goto Release;
        }
    }

    if (!pipe->Connected) {
        status = STATUS_PIPE_DISCONNECTED;
        goto Release;
    }

    /* -------------------------------------------------------------- */
    /* Advance IRP to next location, set cancel routine, insert       */
    /* -------------------------------------------------------------- */
    if (stack->MinorFunction != IRP_MN_READ_CONFIG && stack->MinorFunction != IRP_MN_WRITE_CONFIG) {
        IoSetCancelRoutine(Irp, (PDRIVER_CANCEL)CdpCancelIoIrp);
        if (Irp->Cancel) {
            if (IoSetCancelRoutine(Irp, NULL) != NULL) {
                status = STATUS_CANCELLED;
                goto Release;
            }
        }
    }

    InsertTailList(&pipe->DeliveryQueue, &Irp->Tail.Overlay.ListEntry);

    if (pipe->ReadIoIrp != NULL) {
        PIRP readIrp = pipe->ReadIoIrp;
        PDRIVER_CANCEL cancel;

        cancel = IoSetCancelRoutine(readIrp, NULL);
        if (cancel != NULL) {
            pipe->ReadIoIrp = NULL;
            pending = FALSE;
            if (!pending) {
                /* Complete the pending read IRP with our IO */
                status = STATUS_SUCCESS;
                ExReleasePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);
                if (irql < DISPATCH_LEVEL) {
                    KeLeaveCriticalRegion();
                }
                /* fall through to caller-driven completion */
            }
        }
    }

    IoMarkIrpPending(Irp);
    status = STATUS_PENDING;

Release:
    ExReleasePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);
    if (irql < DISPATCH_LEVEL) {
        KeLeaveCriticalRegion();
    }

    if (!NT_SUCCESS(status)) {
        CdFreeMdlChain(inputMdl);
        CdFreeMdlChain(outputMdl);
    }

    return status;
}

