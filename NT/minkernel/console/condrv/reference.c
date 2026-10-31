#pragma warning (disable:4996)
#include "condrvp.h"

/* ------------------------------------------------------------------ */
/* External functions defined in other source files                   */
/* ------------------------------------------------------------------ */
extern VOID __cdecl CdDereferenceServer(PCD_SERVER Server);

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateReferenceObject(PIRP  Irp,
                                         ULONG Parameter);
PCD_SERVER __cdecl CdGetServerFromReference(PFILE_OBJECT FileObject);
NTSTATUS __cdecl CdpAllocateReferenceObject(PCD_REFERENCE *Reference,
                                            PCD_SERVER     Server);
NTSTATUS __cdecl CdpReferenceObjectClose(PCD_REFERENCE Reference,
                                         PIRP          Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdCreateReferenceObject)
#pragma alloc_text(PAGE,CdGetServerFromReference)
#pragma alloc_text(PAGE,CdpAllocateReferenceObject)
#pragma alloc_text(PAGE,CdpReferenceObjectClose)
#endif // ALLOC_PRAGMA

/* ------------------------------------------------------------------ */
/* External symbols defined in other source files                     */
/* ------------------------------------------------------------------ */
extern PDEVICE_OBJECT  CdDeviceObject;
extern CD_CONTEXT_TYPE CdServerType;
extern CD_CONTEXT_TYPE CdConnectionType;
CD_CONTEXT_TYPE CdReferenceType = {0,NULL,NULL,NULL,NULL,CdpReferenceObjectClose,NULL,NULL,NULL,NULL};

/* ------------------------------------------------------------------ */
/* CdCreateReferenceObject                                            */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdCreateReferenceObject(PIRP  Irp,
                                         ULONG Parameter)
{
    PFILE_OBJECT      fileObject;
    PFILE_OBJECT      relatedFileObject;
    PCD_CONTEXT_TYPE  contextType;
    PCD_SERVER        server;
    PCD_REFERENCE     reference;
    NTSTATUS          status;

    fileObject        = IoGetCurrentIrpStackLocation(Irp)->FileObject;
    relatedFileObject = fileObject->RelatedFileObject;

    if (relatedFileObject == NULL ||
        relatedFileObject->DeviceObject != CdDeviceObject ||
        relatedFileObject->FsContext == NULL) {
        status = STATUS_INVALID_HANDLE;
        goto Complete;
    }

    contextType = *(PCD_CONTEXT_TYPE *)relatedFileObject->FsContext;

    if (contextType == &CdServerType ||
        contextType == &CdConnectionType) {
        server = (PCD_SERVER)relatedFileObject->FsContext;
        status = CdpAllocateReferenceObject(&reference, server);
        if (NT_SUCCESS(status)) {
            fileObject->FsContext = Irp;
            Irp->IoStatus.Status      = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IofCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
    } else {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

Complete:
    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/* ------------------------------------------------------------------ */
/* CdGetServerFromReference                                           */
/* ------------------------------------------------------------------ */
PCD_SERVER __cdecl CdGetServerFromReference(PFILE_OBJECT FileObject)
{
    PCD_REFERENCE reference;

    if (FileObject != NULL &&
        FileObject->DeviceObject == CdDeviceObject &&
        (reference = (PCD_REFERENCE)FileObject->FsContext) != NULL &&
        reference->Header.ObjectType == &CdReferenceType) {
        return reference->Server;
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* CdpAllocateReferenceObject                                         */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpAllocateReferenceObject(PCD_REFERENCE *Reference,
                                            PCD_SERVER     Server)
{
    PCD_REFERENCE reference;

    reference = (PCD_REFERENCE)ExAllocatePoolWithQuotaTag(
        (POOL_TYPE)9,               /* PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE */
        sizeof(CD_REFERENCE),
        'fRdC');

    if (reference == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    reference->Header.ObjectType = &CdReferenceType;
    reference->Server            = Server;
    InterlockedIncrement((LONG *)&Server->RefCount);
    *Reference = reference;

    return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CdpReferenceObjectClose                                            */
/* ------------------------------------------------------------------ */
NTSTATUS __cdecl CdpReferenceObjectClose(PCD_REFERENCE Reference,
                                         PIRP          Irp)
{
    CdDereferenceServer(Reference->Server);
    ExFreePoolWithTag(Reference, 0);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
