#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions                                                  */
/* ------------------------------------------------------------------ */
extern NTSTATUS __cdecl CdpServerIoctl(PCD_SERVER Server,
                                PIRP        Irp);
extern NTSTATUS __cdecl
CdpConnectionIoctl(PCD_CONNECTION Connection, PIRP Irp);
extern NTSTATUS
CdCreateConnection(PIRP Irp, ULONG Parameter);
NTSTATUS __cdecl CdpDispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchQuerySecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchSetSecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __cdecl CdpDispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
UCHAR __cdecl CdpFastIoDeviceControl(PFILE_OBJECT    FileObject,
                                      BOOLEAN         Wait,
                                      PVOID           InputBuffer,
                                      ULONG           InputLength,
                                      PVOID           OutputBuffer,
                                      ULONG           OutputLength,
                                      ULONG           IoControlCode,
                                      PIO_STATUS_BLOCK IoStatus,
                                      PDEVICE_OBJECT  DeviceObject);
NTSTATUS __cdecl CdpDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID __cdecl CdpUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                              PUNICODE_STRING RegistryPath);
extern NTSTATUS __cdecl CdCreateDefaultObjectClient(PIRP Irp, ULONG Parameter);
extern NTSTATUS __cdecl CdCreateClient(PIRP Irp, ULONG Parameter);
extern NTSTATUS __cdecl CdCreateDisplayObject(PIRP  Irp,
                                       ULONG Parameter);
extern NTSTATUS __cdecl CdCreateBroker(PIRP  Irp,
                                ULONG Parameter);
extern NTSTATUS __cdecl CdCreateServer(PIRP  Irp,
                                ULONG Parameter);
extern NTSTATUS __cdecl CdCreateReferenceObject(PIRP  Irp,
                                         ULONG Parameter);
extern UCHAR __cdecl CdpServerFastIoctl(PCD_SERVER        Server,
                                 PVOID             InputBuffer,
                                 ULONG             InputLength,
                                 PVOID             OutputBuffer,
                                 ULONG             OutputLength,
                                 ULONG             IoctlCode,
                                 PIO_STATUS_BLOCK  IoStatus);

extern VOID     CdpCancelReadIoIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
extern NTSTATUS CdpCompleteReadIoIrp(PCD_IO_PIPE Pipe, PIRP Irp,
                                      PCD_IO_COMPLETE Complete);
extern NTSTATUS CdCompleteIo(PCD_IO_PIPE Pipe, UCHAR Mode,
                             PCD_IO_COMPLETE Complete, ULONG_PTR Reserved);
extern NTSTATUS CdReadIoInput(PCD_IO_PIPE Pipe, UCHAR Mode,
                              PCD_IO_COMPLETE Complete, PVOID Input,
                              PULONG OutputLength);
extern NTSTATUS CdWriteIoOutput(PCD_IO_PIPE Pipe, UCHAR Mode,
                                PCD_IO_COMPLETE Complete, PVOID Output,
                                PULONG OutputLength);
extern VOID     CdDisconnectIoPipe(PCD_IO_PIPE Pipe);
extern NTSTATUS __cdecl CdpSetServerInformation(PCD_SERVER                 Server,
                                          UCHAR                      Mode,
                                          PCD_IO_SERVER_INFORMATION  Information,
                                          ULONG                      Length);
extern NTSTATUS __cdecl CdpLaunchServerProcess(PCD_SERVER                    Server,
                                         UCHAR                         Mode,
                                         PRTL_USER_PROCESS_PARAMETERS  Parameters);
extern NTSTATUS __cdecl CdSubmitUserIo(PCD_SERVER Server,
                                 PIRP       Irp,
                                 ULONG      InputCount,
                                 ULONG      OutputCount);
extern NTSTATUS CdGetProcessConnection(PEPROCESS Process, PVOID *Connection,
                                        PEPROCESS Requestor);
extern NTSTATUS CdpCreateMdlChain(PMDL *FirstMdl, PCD_IO_BUFFER Buffer,
                                   BOOLEAN IsOutput, PVOID Param4,
                                   PVOID Param5);
extern VOID     CdFreeMdlChain(PMDL Mdl);
NTSTATUS __cdecl CdpDispatchCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);
extern NTSTATUS CdAddIoToPipe(PIRP Irp);
NTKERNELAPI
PEPROCESS
IoGetRequestorProcess(
    _In_ PIRP Irp
    );

#ifndef FO_SYNCHRONOUS_IO
#define FO_SYNCHRONOUS_IO 0x00000002
#endif
#ifndef DO_DIRECT_IO
#define DO_DIRECT_IO      0x00000010
#endif
#ifndef DO_DEVICE_INITIALIZING
#define DO_DEVICE_INITIALIZING 0x00000080
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,CdpFastIoDeviceControl)
#pragma alloc_text(PAGE,CdpUnload)
#endif // ALLOC_PRAGMA

/* ------------------------------------------------------------------ */
/* External symbols                                                    */
/* ------------------------------------------------------------------ */
PDEVICE_OBJECT   CdDeviceObject = { 0 };
extern CD_CONTEXT_TYPE  CdReferenceType;
extern CD_CONTEXT_TYPE  CdServerType;
extern CD_CONTEXT_TYPE  CdConnectionType;
extern CD_CONTEXT_TYPE  CdDisplayType;
extern CD_CONTEXT_TYPE  CdClientType;
extern CD_CONTEXT_TYPE  CdBrokerType;
extern LIST_ENTRY       CdpBrokerList;
extern EX_PUSH_LOCK     CdpBrokerListLock;
extern LIST_ENTRY       CdpServerList;
extern EX_PUSH_LOCK     CdpServerListLock;
extern RTL_RUN_ONCE     CdpDisplayInitialized;
FAST_IO_DISPATCH CdpFastIoDispatchTable = { 70 };
extern POBJECT_TYPE     *IoFileObjectType;

/* ------------------------------------------------------------------ */
/* Object creation table entry                                        */
/* ------------------------------------------------------------------ */
CD_OBJECT_CREATION_ENTRY CdpObjectCreationTable[11] = {
    { { 0x10, 0x12, L"\\Connect"      }, (PFN_CD_DISPATCH)CdCreateConnection,          0 },
    { { 0x14, 0x16, L"\\Reference"    }, (PFN_CD_DISPATCH)CdCreateReferenceObject,     0 },
    { { 0x0C, 0x0E, L"\\Input"        }, (PFN_CD_DISPATCH)CdCreateDefaultObjectClient, 1 },
    { { 0x0E, 0x10, L"\\Output"       }, (PFN_CD_DISPATCH)CdCreateDefaultObjectClient, 2 },
    { { 0x0E, 0x10, L"\\Server"       }, (PFN_CD_DISPATCH)CdCreateServer,              0 },
    { { 0x10, 0x12, L"\\Console"      }, (PFN_CD_DISPATCH)CdCreateClient,              4 },
    { { 0x14, 0x16, L"\\CurrentIn"    }, (PFN_CD_DISPATCH)CdCreateClient,              1 },
    { { 0x16, 0x18, L"\\CurrentOut"   }, (PFN_CD_DISPATCH)CdCreateClient,              2 },
    { { 0x1A, 0x1C, L"\\ScreenBuffer" }, (PFN_CD_DISPATCH)CdCreateClient,              3 },
    { { 0x0E, 0x10, L"\\Broker"       }, (PFN_CD_DISPATCH)CdCreateBroker,              0 },
    { { 0x10, 0x12, L"\\Display"      }, (PFN_CD_DISPATCH)CdCreateDisplayObject,       0 },
};

/* ------------------------------------------------------------------ */
/* Common dispatch helper                                             */
/* ------------------------------------------------------------------ */
static NTSTATUS CdpDispatchCommon(PIRP Irp, ULONG DispatchOffset,
                                   NTSTATUS MissingStatus)
{
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT       fileObject    = stackLocation->FileObject;
    PCD_CONTEXT_HEADER context;
    PCD_CONTEXT_TYPE   contextType;
    PFN_CD_DISPATCH    handler;
    PVOID              object;

    if (fileObject == NULL) {
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    context     = (PCD_CONTEXT_HEADER)fileObject->FsContext;
    contextType = context->ObjectType;
    handler     = *(PFN_CD_DISPATCH *)((PUCHAR)contextType + DispatchOffset);

    if (handler == NULL) {
        Irp->IoStatus.Status      = MissingStatus;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return MissingStatus;
    }

    object = (PVOID)((PUCHAR)context - contextType->HeaderOffset);
    return handler(object, Irp);
}

/* ------------------------------------------------------------------ */
/* Simple dispatchers                                                 */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpDispatchCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, Cleanup),
                             STATUS_SUCCESS);
}

NTSTATUS __cdecl CdpDispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, Close),
                             STATUS_SUCCESS);
}

NTSTATUS __cdecl CdpDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, Read),
                             STATUS_NOT_SUPPORTED);
}

NTSTATUS __cdecl CdpDispatchWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, Write),
                             STATUS_NOT_SUPPORTED);
}

NTSTATUS __cdecl CdpDispatchFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, FlushBuffers),
                             STATUS_NOT_SUPPORTED);
}

NTSTATUS __cdecl CdpDispatchQuerySecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, QuerySecurity),
                             STATUS_NOT_SUPPORTED);
}

NTSTATUS __cdecl CdpDispatchSetSecurity(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return CdpDispatchCommon(Irp, FIELD_OFFSET(CD_CONTEXT_TYPE, SetSecurity),
                             STATUS_NOT_SUPPORTED);
}

/* ------------------------------------------------------------------ */
/* CdpDispatchCreate                                                  */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpDispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT       fileObject    = stackLocation->FileObject;
    ULONG              i;

    if (fileObject == NULL) {
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < 11; i++) {
        if (RtlCompareUnicodeString(&fileObject->FileName,
                                     &CdpObjectCreationTable[i].Name,
                                     TRUE) == 0) {
            return CdpObjectCreationTable[i].Function(Irp,
                                                       (PIRP)CdpObjectCreationTable[i].Argument);
        }
    }

    Irp->IoStatus.Status      = STATUS_OBJECT_NAME_NOT_FOUND;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/* ------------------------------------------------------------------ */
/* CdpDispatchDeviceControl                                           */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stackLocation;
    PFILE_OBJECT       fileObject;
    PCD_CONTEXT_HEADER context;
    PCD_CONTEXT_TYPE   contextType;
    PFN_CD_DISPATCH    handler;
    PVOID              object;
    PEPROCESS          requestorProcess;
    PIRP               masterIrp;
    NTSTATUS           status;
    ULONG              ioctlCode;
    ULONG              clientHandleValue;
    KAPC_STATE         apcState;
    PFILE_OBJECT       clientFileObject;
    PCD_CONTEXT_HEADER clientContext;
    PVOID              connection;

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    fileObject    = stackLocation->FileObject;

    if (fileObject == NULL) {
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    context     = (PCD_CONTEXT_HEADER)fileObject->FsContext;
    contextType = context->ObjectType;
    handler     = contextType->Ioctl;

    if (handler == NULL) {
        Irp->IoStatus.Status      = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    object = (PVOID)((PUCHAR)context - contextType->HeaderOffset);

    if (handler == (PFN_CD_DISPATCH)CdpServerIoctl) {
        /* Inline fast-path equivalent to calling CdpServerIoctl */
        return CdpServerIoctl(object, Irp);
    }

    if (handler != (PFN_CD_DISPATCH)CdpConnectionIoctl) {
        return handler(object, Irp);
    }

    /* -------------------------------------------------------------- */
    /* Connection IOCTL path                                          */
    /* -------------------------------------------------------------- */
    ioctlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    if (ioctlCode != 0x500016) {
        Irp->IoStatus.Status      = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((fileObject->Flags & FO_SYNCHRONOUS_IO) == 0) {
        status = STATUS_INVALID_PARAMETER;
        goto completeWithStatus;
    }

    requestorProcess = IoGetRequestorProcess(Irp);
    if (requestorProcess == NULL) {
        status = STATUS_INVALID_CID;
        goto completeWithStatus;
    }

    if (requestorProcess != *(PEPROCESS *)((PUCHAR)object + 0x10)) {
        status = STATUS_INVALID_HANDLE;
        goto completeWithStatus;
    }

    if (stackLocation->Parameters.DeviceIoControl.InputBufferLength < 0xc) {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto completeWithStatus;
    }

    masterIrp         = Irp->AssociatedIrp.MasterIrp;
    clientHandleValue = (ULONG)masterIrp->Type | ((ULONG)masterIrp->Size << 16);

    if (clientHandleValue == 0) {
        return CdSubmitUserIo(*(PCD_SERVER *)((PUCHAR)object + 0x14),
                              NULL, 0, 0);
    }

    KeStackAttachProcess((PRKPROCESS)requestorProcess, &apcState);
    status = ObReferenceObjectByHandle((HANDLE)clientHandleValue,
                                        0, (POBJECT_TYPE)IoFileObjectType, UserMode,
                                        (PVOID *)&clientFileObject, NULL);
    KeUnstackDetachProcess(&apcState);

    if (!NT_SUCCESS(status)) {
        goto completeWithStatus;
    }

    clientContext = (PCD_CONTEXT_HEADER)clientFileObject->FsContext;
    if (clientFileObject->DeviceObject != CdDeviceObject ||
        clientContext == NULL ||
        clientContext->ObjectType != &CdClientType) {
        ObfDereferenceObject(clientFileObject);
        status = STATUS_INVALID_HANDLE;
        goto completeWithStatus;
    }

    connection = NULL;
    status = CdGetProcessConnection(requestorProcess, &connection,
                                    requestorProcess);
    if (!NT_SUCCESS(status)) {
        ObfDereferenceObject(clientFileObject);
        goto completeWithStatus;
    }

    ObfDereferenceObject(clientFileObject);
    status = STATUS_SUCCESS;

completeWithStatus:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdpFastIoDeviceControl                                             */
/* ------------------------------------------------------------------ */
UCHAR __cdecl CdpFastIoDeviceControl(PFILE_OBJECT    FileObject,
                                      BOOLEAN         Wait,
                                      PVOID           InputBuffer,
                                      ULONG           InputLength,
                                      PVOID           OutputBuffer,
                                      ULONG           OutputLength,
                                      ULONG           IoControlCode,
                                      PIO_STATUS_BLOCK IoStatus,
                                      PDEVICE_OBJECT  DeviceObject)
{
    PCD_CONTEXT_HEADER context;
    PCD_CONTEXT_TYPE   contextType;
    PFN_CD_FAST_IOCTL  handler;
    PVOID              object;
    NTSTATUS           status;
    KPROCESSOR_MODE    mode;

    if (FileObject == NULL) {
        IoStatus->Information = 0;
        IoStatus->Status      = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    context     = (PCD_CONTEXT_HEADER)FileObject->FsContext;
    contextType = context->ObjectType;
    handler     = contextType->FastIoctl;

    if (handler == NULL) {
        return FALSE;
    }

    object = (PVOID)((PUCHAR)context - contextType->HeaderOffset);

    if (handler != (PFN_CD_FAST_IOCTL)CdpServerFastIoctl) {
        return handler(object, InputBuffer, InputLength, OutputBuffer,
                        OutputLength, IoControlCode, IoStatus);
    }

    IoStatus->Information = 0;

    switch (IoControlCode) {
    case 0x500006: /* IOCTL_CONDRV_COMPLETE_IO */
        if (InputLength != 0) {
            if (InputLength != 0x1c) {
                IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
                return TRUE;
            }
            mode = ExGetPreviousMode();
            CdCompleteIo((PCD_IO_PIPE)InputBuffer, (UCHAR)InputLength,
                          (PCD_IO_COMPLETE)OutputBuffer, 0);
        }
        return FALSE;

    case 0x50000b: /* IOCTL_CONDRV_COMPLETE_SERVER_IO */
        mode = ExGetPreviousMode();
        status = CdCompleteIo((PCD_IO_PIPE)InputBuffer, (UCHAR)InputLength,
                                (PCD_IO_COMPLETE)OutputBuffer, 0);
        IoStatus->Status = status;
        return TRUE;

    case 0x50000f: /* IOCTL_CONDRV_READ_IO */
        mode = ExGetPreviousMode();
        status = CdReadIoInput((PCD_IO_PIPE)InputBuffer, (UCHAR)InputLength,
                                 (PCD_IO_COMPLETE)OutputBuffer, NULL,
                                 &OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x500013: /* IOCTL_CONDRV_WRITE_IO */
        mode = ExGetPreviousMode();
        status = CdWriteIoOutput((PCD_IO_PIPE)InputBuffer, (UCHAR)InputLength,
                                   (PCD_IO_COMPLETE)OutputBuffer, NULL,
                                   &OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x50001b: /* IOCTL_CONDRV_DISCONNECT_PIPE */
        CdDisconnectIoPipe((PCD_IO_PIPE)InputBuffer);
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;

    case 0x50001f: /* IOCTL_CONDRV_SET_SERVER_INFORMATION */
        mode = ExGetPreviousMode();
        status = CdpSetServerInformation((PCD_SERVER)InputBuffer,
                                          (UCHAR)InputLength,
                                          (PCD_IO_SERVER_INFORMATION)OutputBuffer,
                                          OutputLength);
        IoStatus->Status = status;
        return TRUE;

    case 0x500033: /* IOCTL_CONDRV_SET_SERVER_FLAG */
        KeEnterCriticalRegion();
        ExAcquirePushLockExclusiveEx(&((PCD_SERVER)object)->Lock, 0);
        ((PCD_SERVER)object)->Flags |= 1;
        ExReleasePushLockExclusiveEx(&((PCD_SERVER)object)->Lock, 0);
        KeLeaveCriticalRegion();
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;

    case 0x500037: /* IOCTL_CONDRV_LAUNCH_SERVER_PROCESS */
        mode = ExGetPreviousMode();
        status = CdpLaunchServerProcess((PCD_SERVER)InputBuffer,
                                          (UCHAR)InputLength,
                                          (PRTL_USER_PROCESS_PARAMETERS)OutputBuffer);
        IoStatus->Status = status;
        return TRUE;
    }

    return FALSE;
}

/* ------------------------------------------------------------------ */
/* CdpUnload                                                          */
/* ------------------------------------------------------------------ */
VOID __cdecl CdpUnload(PDRIVER_OBJECT DriverObject)
{
    if (DriverObject->DeviceObject != NULL) {
        IoDeleteDevice(DriverObject->DeviceObject);
    }
}

/* ------------------------------------------------------------------ */
/* DriverEntry                                                        */
/* ------------------------------------------------------------------ */
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
                              PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING  deviceName;
    PDEVICE_OBJECT  deviceObject;
    NTSTATUS        status;

    *(ULONG *)&CdpBrokerListLock = 0;
    InitializeListHead(&CdpBrokerList);

    *(ULONG *)&CdpServerListLock = 0;
    InitializeListHead(&CdpServerList);

    RtlRunOnceInitialize(&CdpDisplayInitialized);

    DriverObject->FastIoDispatch                     = &CdpFastIoDispatchTable;
    DriverObject->DriverUnload                       = (PDRIVER_UNLOAD)CdpUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]       = (PDRIVER_DISPATCH)CdpDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_WRITE]        = (PDRIVER_DISPATCH)CdpDispatchWrite;
    DriverObject->MajorFunction[IRP_MJ_READ]         = (PDRIVER_DISPATCH)CdpDispatchRead;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]      = (PDRIVER_DISPATCH)CdpDispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]        = (PDRIVER_DISPATCH)CdpDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = (PDRIVER_DISPATCH)CdpDispatchSetSecurity;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]= (PDRIVER_DISPATCH)CdpDispatchFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]= (PDRIVER_DISPATCH)CdpDispatchQuerySecurity;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]= (PDRIVER_DISPATCH)CdpDispatchDeviceControl;

    RtlInitUnicodeString(&deviceName, L"\\Device\\ConDrv");

    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_CONSOLE,
                             0, FALSE, &deviceObject);
    if (NT_SUCCESS(status)) {
        CdDeviceObject = deviceObject;

        deviceObject->StackSize = 2;
        deviceObject->Flags    |= DO_DIRECT_IO;
        deviceObject->Flags    &= ~DO_DEVICE_INITIALIZING;

        status = STATUS_SUCCESS;
    }

    return status;
}
