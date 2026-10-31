#pragma warning (disable:4996)
#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions defined in other source files                   */
/* ------------------------------------------------------------------ */
extern VOID CdInitializeIoPipe(PCD_IO_PIPE Pipe, PVOID Context);
extern VOID CdDisconnectIoPipe(PCD_IO_PIPE Pipe);
extern NTSTATUS CdReadNextIo(PCD_IO_PIPE Pipe, PIRP Irp);
extern NTSTATUS CdReadIoInput(PCD_IO_PIPE Pipe, UCHAR Mode,
                              PCD_IO_COMPLETE Complete, PVOID Input,
                              PULONG OutputLength);
extern NTSTATUS CdCompleteIo(PCD_IO_PIPE Pipe, UCHAR Mode,
                             PCD_IO_COMPLETE Complete, ULONG_PTR Reserved);
extern VOID __cdecl CdDereferenceServer(PCD_SERVER Server);

extern BOOLEAN SePrivilegeCheck(PLUID_AND_ATTRIBUTES RequiredPrivileges,
                                PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
                                KPROCESSOR_MODE AccessMode);

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpCompleteBrokerIo(PCD_BROKER      Broker,
                                     UCHAR           Mode,
                                     PCD_IO_COMPLETE Complete,
                                     ULONG_PTR       Reserved);
NTSTATUS __cdecl CdpBrokerIoctl(PCD_BROKER Broker,
                                PIRP        Irp);
NTSTATUS __cdecl CdpBrokerCleanup(PCD_BROKER Broker,
                                  PIRP        Irp);
NTSTATUS __cdecl CdpBrokerClose(PCD_BROKER Broker,
                                PIRP        Irp);
UCHAR __cdecl CdpBrokerFastIoctl(PCD_BROKER        Broker,
                                 PVOID             InputBuffer,
                                 ULONG             InputLength,
                                 PVOID             OutputBuffer,
                                 ULONG             OutputLength,
                                 ULONG             IoctlCode,
                                 PIO_STATUS_BLOCK  IoStatus);
PCD_BROKER __cdecl CdFindBroker(VOID);
VOID __cdecl CdDereferenceBroker(PCD_BROKER Broker);
NTSTATUS __cdecl CdCompleteBrokerIoWithServer(PVOID        Broker,
                                              PLUID        RequestId,
                                              PCD_SERVER   Server,
                                              PEPROCESS    Process);
NTSTATUS __cdecl CdCreateBroker(PIRP  Irp,
                                ULONG Parameter);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdCompleteBrokerIoWithServer)
#pragma alloc_text(PAGE,CdCreateBroker)
#pragma alloc_text(PAGE,CdFindBroker)
#pragma alloc_text(PAGE,CdpBrokerCleanup)
#pragma alloc_text(PAGE,CdpBrokerClose)
#pragma alloc_text(PAGE,CdpBrokerFastIoctl)
#pragma alloc_text(PAGE,CdpBrokerIoctl)
#pragma alloc_text(PAGE,CdpCompleteBrokerIo)
#endif // ALLOC_PRAGMA

/* ------------------------------------------------------------------ */
/* External symbols defined in other source files                     */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT  CdDeviceObject;
CD_CONTEXT_TYPE CdBrokerType = {0,NULL,NULL,CdpBrokerIoctl,CdpBrokerCleanup,CdpBrokerClose,CdpBrokerFastIoctl,NULL,NULL,NULL};

LIST_ENTRY      CdpBrokerList = { 0 };
EX_PUSH_LOCK    CdpBrokerListLock = { 0 };
ULONG           CdpBrokerFlags;

POBJECT_TYPE   *IoFileObjectType;

/* ------------------------------------------------------------------ */
/* CdCompleteBrokerIoWithServer                                       */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCompleteBrokerIoWithServer(PVOID        Broker,
                                              PLUID        RequestId,
                                              PCD_SERVER   Server,
                                              PEPROCESS    Process)
{
    KAPC_STATE    apcState;
    NTSTATUS      status;
    HANDLE        handle;
    PVOID         object;
    CD_IO_PIPE    pipe;
    CD_IO_COMPLETE complete;
    PCD_BROKER    broker;

    broker  = (PCD_BROKER)Broker;
    handle  = (HANDLE)RequestId; /* LUID is used as a handle in this path */

    KeStackAttachProcess((PRKPROCESS)Process, &apcState);

    status = ObReferenceObjectByHandle(handle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &object,
                                       NULL);

    KeUnstackDetachProcess(&apcState);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Validate that the object is a broker file object */
    if (((PFILE_OBJECT)object)->DeviceObject != CdDeviceObject ||
        ((PFILE_OBJECT)object)->FsContext == NULL ||
        *(PCD_CONTEXT_TYPE *)((PFILE_OBJECT)object)->FsContext != &CdBrokerType) {
        ObfDereferenceObject(object);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    RtlZeroMemory(&pipe, sizeof(pipe));
    RtlZeroMemory(&complete, sizeof(complete));

    InterlockedIncrement((LONG *)&broker->RefCount);

    status = CdCompleteIo(&pipe, 0x1c, &complete, 0);

    ObfDereferenceObject(object);

    if (!NT_SUCCESS(status)) {
        CdDereferenceServer(Server);
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* CdCreateBroker                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateBroker(PIRP  Irp,
                                ULONG Parameter)
{
    PIO_STACK_LOCATION   stack;
    PFILE_OBJECT         fileObject;
    PCD_BROKER           broker;
    LUID_AND_ATTRIBUTES  privilege;
    PVOID                serverSilo;
    NTSTATUS             status;

    stack      = IoGetCurrentIrpStackLocation(Irp);
    fileObject = stack->FileObject;

    privilege.Luid.LowPart  = 1;
    privilege.Luid.HighPart = 0;
    privilege.Attributes    = 0;

    if (!SePrivilegeCheck(&privilege,
                          (PSECURITY_SUBJECT_CONTEXT)
                          ((PUCHAR)fileObject->RelatedFileObject->FsContext + 0x1c),
                          Irp->RequestorMode)) {
        status = STATUS_PRIVILEGE_NOT_HELD;
        goto Complete;
    }

    broker = ExAllocatePoolWithQuotaTag(
        (POOL_TYPE)(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
        sizeof(CD_BROKER),
        'rBdC');

    if (broker == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    broker->Header.ObjectType = &CdBrokerType;
    broker->RefCount          = 1;
    broker->Lock              = 0;

    CdInitializeIoPipe(&broker->Pipe, NULL);

    serverSilo = PsGetCurrentServerSilo();
    broker->Silo = serverSilo;
    if (serverSilo != NULL) {
        ObfReferenceObjectWithTag(serverSilo, 'rBdC');
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(&CdpBrokerListLock, 0);

    if (CdpBrokerList.Flink == &CdpBrokerList) {
        InterlockedOr((LONG *)&CdpBrokerFlags, 0x40);
    }

    InsertTailList(&CdpBrokerList, &broker->Links);

    ExReleasePushLockExclusiveEx(&CdpBrokerListLock, 0);
    KeLeaveCriticalRegion();

    fileObject->FsContext = broker;

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
/* CdDereferenceBroker                                                */
/* ------------------------------------------------------------------ */
VOID __cdecl CdDereferenceBroker(PCD_BROKER Broker)
{
    LONG   newCount;
    KIRQL  irql;

    newCount = InterlockedDecrement((LONG *)&Broker->RefCount);
    if (newCount > 0) {
        return;
    }

    irql = KeGetCurrentIrql();
    if (irql < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
    }

    ExAcquirePushLockExclusiveEx(&CdpBrokerListLock, 0);

    RemoveEntryList(&Broker->Links);

    if (CdpBrokerList.Flink == &CdpBrokerList) {
        InterlockedAnd((LONG *)&CdpBrokerFlags, ~0x40);
    }

    ExReleasePushLockExclusiveEx(&CdpBrokerListLock, 0);

    if (irql < DISPATCH_LEVEL) {
        KeLeaveCriticalRegion();
    }

    if (Broker->Silo != NULL) {
        ObfDereferenceObjectWithTag(Broker->Silo, 'rBdC');
    }

    ExFreePoolWithTag(Broker, 'rBdC');
}

/* ------------------------------------------------------------------ */
/* CdFindBroker                                                       */
/* ------------------------------------------------------------------ */
PCD_BROKER __cdecl CdFindBroker(VOID)
{
    PVOID        serverSilo;
    PLIST_ENTRY  entry;
    PCD_BROKER   broker;

    serverSilo = PsGetCurrentServerSilo();

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx(&CdpBrokerListLock, 0);

    for (entry = CdpBrokerList.Flink; entry != &CdpBrokerList; entry = entry->Flink) {
        broker = CONTAINING_RECORD(entry, CD_BROKER, Links);
        if (broker->Silo == serverSilo) {
            InterlockedIncrement((LONG *)&broker->RefCount);
            ExReleasePushLockExclusiveEx(&CdpBrokerListLock, 0);
            KeLeaveCriticalRegion();
            return broker;
        }
    }

    ExReleasePushLockExclusiveEx(&CdpBrokerListLock, 0);
    KeLeaveCriticalRegion();

    return NULL;
}

/* ------------------------------------------------------------------ */
/* CdpBrokerCleanup                                                   */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpBrokerCleanup(PCD_BROKER Broker,
                                  PIRP        Irp)
{
    CdDisconnectIoPipe(&Broker->Pipe);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpBrokerClose                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpBrokerClose(PCD_BROKER Broker,
                                PIRP        Irp)
{
    CdDereferenceBroker(Broker);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpBrokerFastIoctl                                                 */
/* ------------------------------------------------------------------ */
UCHAR __cdecl CdpBrokerFastIoctl(PCD_BROKER        Broker,
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
            CdpCompleteBrokerIo(Broker, 0x1c, (PCD_IO_COMPLETE)InputBuffer, 0);
        }
        return FALSE;

    case 0x50000b: /* IOCTL_CONDRV_COMPLETE_BROKER_IO */
        mode   = ExGetPreviousMode();
        status = CdpCompleteBrokerIo(Broker,
                                     (UCHAR)InputLength,
                                     (PCD_IO_COMPLETE)InputBuffer,
                                     0);
        IoStatus->Status = status;
        return TRUE;

    case 0x50000f: /* IOCTL_CONDRV_READ_IO */
        mode   = ExGetPreviousMode();
        status = CdReadIoInput(&Broker->Pipe,
                               (UCHAR)InputLength,
                               (PCD_IO_COMPLETE)OutputBuffer,
                               InputBuffer,
                               &OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x50001b: /* IOCTL_CONDRV_DISCONNECT_PIPE */
        CdDisconnectIoPipe(&Broker->Pipe);
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;

    default:
        return FALSE;
    }
}

/* ------------------------------------------------------------------ */
/* CdpBrokerIoctl                                                     */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpBrokerIoctl(PCD_BROKER Broker,
                                PIRP        Irp)
{
    PIO_STACK_LOCATION stack;
    NTSTATUS           status;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.DeviceIoControl.IoControlCode == 0x500006) {
        status = CdReadNextIo(&Broker->Pipe, Irp);
    } else {
        Irp->IoStatus.Status      = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return status;
}

/* ------------------------------------------------------------------ */
/* CdpCompleteBrokerIo                                                */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpCompleteBrokerIo(PCD_BROKER      Broker,
                                     UCHAR           Mode,
                                     PCD_IO_COMPLETE Complete,
                                     ULONG_PTR       Reserved)
{
    CD_IO_PIPE  pipe;
    PCD_SERVER  server;
    NTSTATUS    status;

    if (Mode != 0x1c) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&pipe, &Broker->Pipe, sizeof(CD_IO_PIPE));

    if (Complete->Write.Data != NULL) {
        /* The caller supplied a handle to a server object. */
        status = CdCompleteIo(&pipe, Mode, Complete, Reserved);
        if (!NT_SUCCESS(status)) {
            server = (PCD_SERVER)Complete->Write.Data;
            CdDereferenceServer(server);
        }
    } else {
        status = CdCompleteIo(&pipe, Mode, Complete, Reserved);
    }

    return status;
}
