#include <ntddk.h>
#ifdef PEPROCESS
#ifdef PETHREAD
#undef PEPROCESS
#undef PETHREAD
#include <ntifs.h>
#endif
#endif

// CSQ context structure (matches original layout)
typedef struct _BEEP_CSQ_CONTEXT {
    IO_CSQ Csq;
    LIST_ENTRY QueueHead;
    KSPIN_LOCK QueueLock;
} BEEP_CSQ_CONTEXT, *PBEEP_CSQ_CONTEXT;

extern NTKERNELAPI
NTSTATUS
IoGetRequestorSessionId(
    _In_  PIRP Irp,
    _Out_ PULONG pSessionId
    );

// Forward declarations
void BeepRedirectCleanupQueue(PDEVICE_OBJECT DeviceObject);
void BeepRedirectCsqAcquireLock(PIO_CSQ Csq, PKIRQL OldIrql);
void BeepRedirectCsqReleaseLock(PIO_CSQ Csq, KIRQL OldIrql);
void BeepRedirectCsqCompleteCanceledIrp(PIO_CSQ Csq, PIRP Irp);
NTSTATUS BeepRedirectCsqInsertIrp(PIO_CSQ Csq, PIRP Irp);
PIRP BeepRedirectCsqPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext);
void BeepRedirectCsqRemoveIrp(PIO_CSQ Csq, PIRP Irp);
NTSTATUS BeepRedirectMakeBeep(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG Frequency, ULONG Duration);
void BeepRedirectCancelRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Cleanup queue - cancel all pending IRPs
void BeepRedirectCleanupQueue(
    PDEVICE_OBJECT DeviceObject
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    PIRP irp;
    
    csqContext = (PBEEP_CSQ_CONTEXT)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    
    irp = IoCsqRemoveNextIrp(&csqContext->Csq, NULL);
    while (irp)
    {
        irp->IoStatus.Status = STATUS_CANCELLED;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        irp = IoCsqRemoveNextIrp(&csqContext->Csq, NULL);
    }
}

// CSQ: Acquire spin lock
void BeepRedirectCsqAcquireLock(
    PIO_CSQ Csq,
    PKIRQL OldIrql
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    
    csqContext = CONTAINING_RECORD(Csq, BEEP_CSQ_CONTEXT, Csq);
    KeAcquireSpinLock(&csqContext->QueueLock, OldIrql);
}

// CSQ: Release spin lock
void BeepRedirectCsqReleaseLock(
    PIO_CSQ Csq,
    KIRQL OldIrql
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    
    csqContext = CONTAINING_RECORD(Csq, BEEP_CSQ_CONTEXT, Csq);
    KeReleaseSpinLock(&csqContext->QueueLock, OldIrql);
}

// CSQ: Complete canceled IRP
void BeepRedirectCsqCompleteCanceledIrp(
    PIO_CSQ Csq,
    PIRP Irp
)
{
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

// CSQ: Insert IRP into queue
NTSTATUS BeepRedirectCsqInsertIrp(
    PIO_CSQ Csq,
    PIRP Irp
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    PLIST_ENTRY entry;
    
    csqContext = CONTAINING_RECORD(Csq, BEEP_CSQ_CONTEXT, Csq);
    entry = &Irp->Tail.Overlay.ListEntry;
    
    InsertTailList(&csqContext->QueueHead, entry);
    
    return STATUS_SUCCESS;
}

// CSQ: Peek at next IRP in queue
PIRP BeepRedirectCsqPeekNextIrp(
    PIO_CSQ Csq,
    PIRP Irp,
    PVOID PeekContext
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    PLIST_ENTRY nextEntry;
    PIRP nextIrp;
    
    csqContext = CONTAINING_RECORD(Csq, BEEP_CSQ_CONTEXT, Csq);
    
    if (Irp == NULL)
    {
        nextEntry = csqContext->QueueHead.Flink;
    }
    else
    {
        nextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }
    
    if (nextEntry == &csqContext->QueueHead)
    {
        return NULL;
    }
    
    nextIrp = CONTAINING_RECORD(nextEntry, IRP, Tail.Overlay.ListEntry);
    
    return nextIrp;
}

// CSQ: Remove IRP from queue
void BeepRedirectCsqRemoveIrp(
    PIO_CSQ Csq,
    PIRP Irp
)
{
    PLIST_ENTRY entry;
    
    entry = &Irp->Tail.Overlay.ListEntry;
    RemoveEntryList(entry);
    InitializeListHead(entry);
}

// Make beep by processing queued IRP
NTSTATUS BeepRedirectMakeBeep(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG Frequency,
    ULONG Duration
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    PIRP queuedIrp;
    NTSTATUS status;
    ULONG sessionId;
    
    csqContext = (PBEEP_CSQ_CONTEXT)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    
    status = IoGetRequestorSessionId(Irp, &sessionId);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    queuedIrp = IoCsqRemoveNextIrp(&csqContext->Csq, (PVOID)(ULONG_PTR)sessionId);
    
    if (!queuedIrp)
    {
        return STATUS_DEVICE_BUSY;
    }
    
    // Fill in the beep parameters into the queued IRP
    PULONG buffer = (PULONG)queuedIrp->AssociatedIrp.SystemBuffer;
    if (buffer)
    {
        buffer[0] = Frequency;
        buffer[1] = Duration;
    }
    
    queuedIrp->IoStatus.Status = STATUS_SUCCESS;
    queuedIrp->IoStatus.Information = 8;
    IoCompleteRequest(queuedIrp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

// Cancel routine for IRPs in the queue
void BeepRedirectCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    KIRQL oldIrql;
    
    csqContext = (PBEEP_CSQ_CONTEXT)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    
    IoAcquireCancelSpinLock(&oldIrql);
    
    if (Irp->Cancel)
    {
        IoCsqRemoveIrp(&csqContext->Csq, (PIO_CSQ_IRP_CONTEXT)Irp);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    IoReleaseCancelSpinLock(oldIrql);
}

// Initialize CSQ for a device
NTSTATUS BeepRedirectInitializeCsq(
    PDEVICE_OBJECT DeviceObject
)
{
    PBEEP_CSQ_CONTEXT csqContext;
    
    csqContext = (PBEEP_CSQ_CONTEXT)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    
    InitializeListHead(&csqContext->QueueHead);
    KeInitializeSpinLock(&csqContext->QueueLock);
    
    IoCsqInitialize(
        &csqContext->Csq,
        BeepRedirectCsqInsertIrp,
        BeepRedirectCsqRemoveIrp,
        BeepRedirectCsqPeekNextIrp,
        BeepRedirectCsqAcquireLock,
        BeepRedirectCsqReleaseLock,
        BeepRedirectCsqCompleteCanceledIrp
    );
    
    return STATUS_SUCCESS;
}

