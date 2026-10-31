/* pipe.c
 *
 * Clean reconstruction of the condrv pipe logic. This keeps the original
 * function names and driver-level behavior while removing decompiler noise.
 */

#include "condrvp.h"

extern VOID __cdecl CdFreeMdlChain(PMDL Mdl);
NTSTATUS CdpReadIoInput(PIRP Irp, PCD_IO_BUFFER_DESCRIPTOR Buffer, PULONG BytesRead);
NTSTATUS CdpWriteIoOutput(PIRP Irp, PCD_IO_BUFFER_DESCRIPTOR Buffer, PULONG BytesWritten);
BOOLEAN
CdpPrepareIoCompletion(
    PIRP Irp,
    UCHAR Type
    );
BOOLEAN
CdpIsFunctionCancellable(
    ULONG Function
    );
VOID
CdpCancelReadIoIrpPaged(
    PIRP Irp
    );
VOID
CdpCancelIoIrpPaged(
    PIRP Irp
    );
VOID
CdpCancelIoIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );
NTSTATUS
CdWriteIoOutput(
    PCD_IO_PIPE Pipe,
    UCHAR Function,
    PCD_IO_OPERATION Operation,
    ULONG Length,
    PULONG BytesWritten
    );
NTSTATUS
CdReadNextIo(
    PCD_IO_PIPE Pipe,
    PIRP Irp
    );
NTSTATUS
CdReadIoInput(
    PCD_IO_PIPE Pipe,
    UCHAR Function,
    PCD_IO_OPERATION Operation,
    ULONG Length,
    PULONG BytesRead
    );
PIRP
CdpLookupIo(
    PCD_IO_TABLE Table,
    PLUID Identifier
    );
ULONG
CdpGetMdlChainSize(
    PMDL Mdl
    );
NTSTATUS
CdpCompleteReadIoIrp(
    PCD_IO_PIPE Pipe,
    PIRP Irp,
    UCHAR IsRead
    );
NTSTATUS
CdpCompleteReadIo(
    PCD_IO_PIPE Pipe,
    PCD_IO_DESCRIPTOR Descriptor,
    ULONG Length,
    PULONG BytesTransferred
    );
VOID
CdInitializeIoPipe(
    PCD_IO_PIPE Pipe,
    PEX_PUSH_LOCK Lock
    );
VOID
CdInitializeIo(
    PIRP Irp,
    PCD_IO_PIPE Pipe,
    UCHAR Type,
    ULONG_PTR Arg1,
    ULONG_PTR Arg2,
    PMDL InputMdl,
    PMDL OutputMdl
    );
NTSTATUS
CdCompleteIo(
    PCD_IO_PIPE Pipe,
    UCHAR FunctionCode,
    PCD_IO_COMPLETE Completion,
    ULONG Reserved
    );
VOID
CdpCancelReadIoIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdCompleteIo)
#pragma alloc_text(PAGE,CdInitializeIo)
#pragma alloc_text(PAGE,CdInitializeIoPipe)
#pragma alloc_text(PAGE,CdpCancelIoIrp)
#pragma alloc_text(PAGE,CdpCancelReadIoIrp)
#pragma alloc_text(PAGE,CdpCompleteReadIo)
#pragma alloc_text(PAGE,CdpCompleteReadIoIrp)
#pragma alloc_text(PAGE,CdpGetMdlChainSize)
#pragma alloc_text(PAGE,CdpIsFunctionCancellable)
#pragma alloc_text(PAGE,CdpLookupIo)
#pragma alloc_text(PAGE,CdpPrepareIoCompletion)
#pragma alloc_text(PAGE,CdpReadIoInput)
#pragma alloc_text(PAGE,CdpWriteIoOutput)
#pragma alloc_text(PAGE,CdReadIoInput)
#pragma alloc_text(PAGE,CdReadNextIo)
#pragma alloc_text(PAGE,CdWriteIoOutput)
#endif // ALLOC_PRAGMA

NTSTATUS
CdAddIoToPipe(
    PIRP Irp
    ) // do NOT page
{
    PCD_IO_PARAMETERS ioParameters;
    PCD_IO_PIPE pipe;
    PMDL mdl;
    ULONG inputTotal = 0;
    ULONG outputTotal = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN enteredCriticalRegion = FALSE;

    ioParameters = (PCD_IO_PARAMETERS)Irp->Tail.Overlay.DriverContext;
    pipe = &ioParameters->Pipe;

    if (Irp == NULL || pipe == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
        enteredCriticalRegion = TRUE;
    }

    ExAcquirePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);

    for (mdl = ioParameters->InputMdlChain; mdl != NULL; mdl = mdl->Next) {
        if ((mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) == 0) {
            if (MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority) == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Release;
            }
        }

        if (inputTotal > (ULONG)-1 - mdl->ByteCount) {
            status = STATUS_INVALID_PARAMETER;
            goto Release;
        }
        inputTotal += mdl->ByteCount;
    }

    for (mdl = ioParameters->OutputMdlChain; mdl != NULL; mdl = mdl->Next) {
        if ((mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) == 0) {
            if (MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority) == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Release;
            }
        }

        if (outputTotal > (ULONG)-1 - mdl->ByteCount) {
            status = STATUS_INVALID_PARAMETER;
            goto Release;
        }
        outputTotal += mdl->ByteCount;
    }

    if (!pipe->Connected) {
        status = STATUS_PIPE_DISCONNECTED;
        goto Release;
    }

    if (IoSetCancelRoutine(Irp, (PDRIVER_CANCEL)CdpCancelIoIrp) != NULL) {
        if (Irp->Cancel) {
            status = STATUS_CANCELLED;
            goto Release;
        }
    }

    InsertTailList(&pipe->DeliveryQueue, &Irp->Tail.Overlay.ListEntry);

    if (pipe->ReadIoIrp != NULL) {
        PIRP readIrp = pipe->ReadIoIrp;
        PDRIVER_CANCEL oldCancelRoutine;

        oldCancelRoutine = IoSetCancelRoutine(readIrp, NULL);
        if (oldCancelRoutine != NULL) {
            pipe->ReadIoIrp = NULL;
            status = STATUS_SUCCESS;
            ExReleasePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);

            if (enteredCriticalRegion) {
                KeLeaveCriticalRegion();
            }

            goto CompleteCaller;
        }
    }

    IoMarkIrpPending(Irp);
    status = STATUS_PENDING;

Release:
    ExReleasePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);

    if (enteredCriticalRegion) {
        KeLeaveCriticalRegion();
    }

    if (!NT_SUCCESS(status)) {
        CdFreeMdlChain(ioParameters->InputMdlChain);
        CdFreeMdlChain(ioParameters->OutputMdlChain);
    }

    return status;

CompleteCaller:
    return status;
}

NTSTATUS
CdCompleteIo(
    PCD_IO_PIPE Pipe,
    UCHAR FunctionCode,
    PCD_IO_COMPLETE Completion,
    ULONG Reserved
    )
{
    PCD_IO_TABLE table;
    PCD_IO_PIPE currentPipe = Pipe;
    PIO_STACK_LOCATION stack;
    PIRP irp = NULL;
    PLIST_ENTRY entry;
    ULONG bucketIndex;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN outputDone = FALSE;
    ULONG remainingBytes = 0;

    UNREFERENCED_PARAMETER(Reserved);

    if (Pipe == NULL || Completion == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (FunctionCode != 0x1C) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);

    bucketIndex = Completion->Identifier.LowPart % 5;
    table = &Pipe->DeliveredTable;

    if (table->Slot[bucketIndex].Flink == &table->Slot[bucketIndex]) {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    entry = table->Slot[bucketIndex].Flink;
    while (entry != &table->Slot[bucketIndex]) {
        PIRP candidate = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

        if (candidate->Tail.Overlay.OriginalFileObject != NULL &&
            candidate->Tail.Overlay.OriginalFileObject == (PVOID)Completion->Identifier.LowPart) {
            irp = candidate;
            break;
        }

        entry = entry->Flink;
    }

    if (irp == NULL) {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if (Completion->Write.Size != 0) {
        status = CdpWriteIoOutput(irp, &Completion->Write, &remainingBytes);
        if (NT_SUCCESS(status)) {
            outputDone = TRUE;
        }
    }

    RemoveEntryList(&irp->Tail.Overlay.ListEntry);

    if (Pipe->ReadIoIrp == NULL &&
        irp->CancelRoutine != NULL &&
        CdpIsFunctionCancellable(FunctionCode)) {
        IoSetCancelRoutine(irp, NULL);
    }

    if (irp->IoStatus.Status >= 0) {
        status = STATUS_SUCCESS;
    }

Exit:
    ExReleasePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);
    KeLeaveCriticalRegion();

    if (outputDone) {
        irp->IoStatus.Information = Completion->Write.Offset;
    }

    if (status == STATUS_SUCCESS) {
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}

VOID
CdDisconnectIoPipe(
    PCD_IO_PIPE Pipe
    ) // do NOT page
{
    PIRP irp;
    PLIST_ENTRY entry;
    ULONG index;
    BOOLEAN enteredCriticalRegion = FALSE;

    if (Pipe == NULL) {
        return;
    }

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
        enteredCriticalRegion = TRUE;
    }

    ExAcquirePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);

    Pipe->Connected = FALSE;

    if (Pipe->ReadIoIrp != NULL) {
        irp = Pipe->ReadIoIrp;
        Pipe->ReadIoIrp = NULL;
        IoSetCancelRoutine(irp, NULL);
        CdpPrepareIoCompletion(irp, 0);
    }

    for (index = 0; index < 5; index++) {
        entry = Pipe->DeliveredTable.Slot[index].Flink;
        while (entry != &Pipe->DeliveredTable.Slot[index]) {
            irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

            if (CdpPrepareIoCompletion(irp, 0)) {
                RemoveEntryList(entry);
                entry = entry->Flink;
                continue;
            }

            entry = entry->Flink;
        }
    }

    ExReleasePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);

    if (enteredCriticalRegion) {
        KeLeaveCriticalRegion();
    }
}

VOID
CdInitializeIo(
    PIRP Irp,
    PCD_IO_PIPE Pipe,
    UCHAR Type,
    ULONG_PTR Arg1,
    ULONG_PTR Arg2,
    PMDL InputMdl,
    PMDL OutputMdl
    )
{
    PCD_IO_PARAMETERS ioParameters;

    if (Irp == NULL) {
        return;
    }

    ioParameters = (PCD_IO_PARAMETERS)Irp->Tail.Overlay.DriverContext;
    ioParameters->Pipe = *Pipe;
    ioParameters->ObjectContext = Type;
    ioParameters->InputMdlChain = InputMdl;
    ioParameters->OutputMdlChain = OutputMdl;

    UNREFERENCED_PARAMETER(Arg1);
    UNREFERENCED_PARAMETER(Arg2);
}

VOID
CdInitializeIoPipe(
    PCD_IO_PIPE Pipe,
    PEX_PUSH_LOCK Lock
    )
{
    if (Pipe == NULL) {
        return;
    }

    Pipe->Lock = Lock;
    Pipe->Connected = TRUE;
    Pipe->ReadIoIrp = NULL;
    InitializeListHead(&Pipe->DeliveryQueue);

    for (int index = 0; index < 5; index++) {
        InitializeListHead(&Pipe->DeliveredTable.Slot[index]);
    }
}

VOID
CdpAddToIoTable(
    PCD_IO_TABLE Table,
    PIRP Irp
    )
{
    PLIST_ENTRY head;
    ULONG bucketIndex;
    PLIST_ENTRY insertionPoint;

    if (Table == NULL || Irp == NULL) {
        return;
    }

    bucketIndex = (ULONG)(Irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset.LowPart % 5);
    head = &Table->Slot[bucketIndex];

    if (head->Flink == head) {
        InsertHeadList(head, &Irp->Tail.Overlay.ListEntry);
        return;
    }

    insertionPoint = head->Blink;
    InsertTailList(head, &Irp->Tail.Overlay.ListEntry);
}

VOID
CdpCancelIoIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    CdpCancelIoIrpPaged(Irp);
}

VOID
CdpCancelIoIrpPaged(
    PIRP Irp
    ) // do NOT page
{
    BOOLEAN enteredCriticalRegion = FALSE;

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
        enteredCriticalRegion = TRUE;
    }

    ExAcquirePushLockExclusiveEx((PULONG_PTR)&((PCD_IO_PIPE)Irp->Tail.Overlay.DriverContext)->Lock, 0);

    CdpPrepareIoCompletion(Irp, 0);

    ExReleasePushLockExclusiveEx((PULONG_PTR)&((PCD_IO_PIPE)Irp->Tail.Overlay.DriverContext)->Lock, 0);

    if (enteredCriticalRegion) {
        KeLeaveCriticalRegion();
    }

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
CdpCancelReadIoIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    CdpCancelReadIoIrpPaged(Irp);
}

VOID
CdpCancelReadIoIrpPaged(
    PIRP Irp
    ) // do NOT page
{
    PCD_IO_PIPE pipe;
    BOOLEAN enteredCriticalRegion = FALSE;

    pipe = (PCD_IO_PIPE)Irp->Tail.Overlay.DriverContext;

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        KeEnterCriticalRegion();
        enteredCriticalRegion = TRUE;
    }

    ExAcquirePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);

    pipe->ReadIoIrp = NULL;

    ExReleasePushLockExclusiveEx((PULONG_PTR)&pipe->Lock, 0);

    if (enteredCriticalRegion) {
        KeLeaveCriticalRegion();
    }

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
CdpCompleteReadIo(
    PCD_IO_PIPE Pipe,
    PCD_IO_DESCRIPTOR Descriptor,
    ULONG Length,
    PULONG BytesTransferred
    )
{
    PCD_IO_BUFFER_DESCRIPTOR buffer;
    NTSTATUS status;
    ULONG copied;
    ULONG totalCopied = 0;

    UNREFERENCED_PARAMETER(Pipe);

    if (Descriptor == NULL || BytesTransferred == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    buffer = (PCD_IO_BUFFER_DESCRIPTOR)Descriptor;

    if (Length == 0) {
        *BytesTransferred = 0;
        return STATUS_SUCCESS;
    }

    status = CdpReadIoInput(NULL, buffer, &copied);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    totalCopied = copied;
    *BytesTransferred = totalCopied;

    return STATUS_SUCCESS;
}

NTSTATUS
CdpCompleteReadIoIrp(
    PCD_IO_PIPE Pipe,
    PIRP Irp,
    UCHAR IsRead
    )
{
    PMDL mdl;
    NTSTATUS status;
    ULONG transferred = 0;

    UNREFERENCED_PARAMETER(Pipe);

    if (Irp == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    mdl = Irp->MdlAddress;
    if (mdl == NULL) {
        status = STATUS_INVALID_PARAMETER;
    } else {
        status = CdpCompleteReadIo((PCD_IO_PIPE)Irp->Tail.Overlay.DriverContext,
                                  (PCD_IO_DESCRIPTOR)&Irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset.QuadPart,
                                  0,
                                  &transferred);
    }

    if (!NT_SUCCESS(status)) {
        if (IsRead) {
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        return status;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = transferred;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

ULONG
CdpGetMdlChainSize(
    PMDL Mdl
    )
{
    ULONG size = 0;

    while (Mdl != NULL) {
        size += Mdl->ByteCount;
        Mdl = Mdl->Next;
    }

    return size;
}

BOOLEAN
CdpIsFunctionCancellable(
    ULONG Function
    )
{
    return (Function != 2 && Function != 4);
}

PIRP
CdpLookupIo(
    PCD_IO_TABLE Table,
    PLUID Identifier
    )
{
    PLIST_ENTRY head;
    PLIST_ENTRY entry;

    if (Table == NULL || Identifier == NULL) {
        return NULL;
    }

    head = &Table->Slot[Identifier->LowPart % 5];

    for (entry = head->Flink; entry != head; entry = entry->Flink) {
        PIRP irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

        if ((ULONG)(irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset).LowPart == Identifier->LowPart &&
            (ULONG)(irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset).HighPart == Identifier->HighPart) {
            return irp;
        }
    }

    return NULL;
}

BOOLEAN
CdpPrepareIoCompletion(
    PIRP Irp,
    UCHAR Type
    )
{
    PCD_IO_PIPE pipe;
    PLIST_ENTRY head;
    PLIST_ENTRY entry;
    NTSTATUS status;

    if (Irp == NULL) {
        return FALSE;
    }

    pipe = (PCD_IO_PIPE)Irp->Tail.Overlay.DriverContext;

    if (pipe == NULL) {
        return FALSE;
    }

    head = &pipe->DeliveredTable.Slot[(ULONG)(Irp->Tail.Overlay.CurrentStackLocation->Parameters.Write.ByteOffset).LowPart % 5];

    for (entry = head->Flink; entry != head; entry = entry->Flink) {
        PIRP candidate = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

        if (candidate == Irp) {
            RemoveEntryList(entry);
            if (Type == 0 && CdpIsFunctionCancellable((ULONG)Type)) {
                IoSetCancelRoutine(Irp, NULL);
            }

            if (pipe->ReadIoIrp != NULL && pipe->ReadIoIrp == candidate) {
                pipe->ReadIoIrp = NULL;
            }

            status = CdpWriteIoOutput(Irp, &((PCD_IO_COMPLETE)Irp->Tail.Overlay.DriverContext)->Write, &(ULONG){0});
            if (!NT_SUCCESS(status)) {
                return FALSE;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
CdpReadIoInput(
    PIRP Irp,
    PCD_IO_BUFFER_DESCRIPTOR Buffer,
    PULONG BytesRead
    )
{
    PMDL mdl;
    ULONG remaining;
    ULONG transferCount;
    ULONG totalRead = 0;
    PVOID sourceAddress;

    if (Buffer == NULL || BytesRead == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    remaining = *BytesRead;
    mdl = (PMDL)Irp->MdlAddress;

    while (mdl != NULL && remaining > 0) {
        PVOID mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);

        if (mapping == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        transferCount = min(mdl->ByteCount, remaining);

        RtlCopyMemory((PVOID)((ULONG_PTR)Buffer->Data + totalRead),
                      (PVOID)((ULONG_PTR)mapping + Buffer->Offset),
                      transferCount);

        remaining -= transferCount;
        totalRead += transferCount;
        mdl = mdl->Next;
    }

    *BytesRead = totalRead;
    return STATUS_SUCCESS;
}

NTSTATUS
CdpWriteIoOutput(
    PIRP Irp,
    PCD_IO_BUFFER_DESCRIPTOR Buffer,
    PULONG BytesWritten
    )
{
    PMDL mdl;
    ULONG remaining;
    ULONG transferCount;
    ULONG totalWritten = 0;
    PVOID sourceAddress;

    if (Buffer == NULL || BytesWritten == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    remaining = *BytesWritten;
    mdl = (PMDL)Irp->MdlAddress;

    while (mdl != NULL && remaining > 0) {
        PVOID mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);

        if (mapping == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        transferCount = min(mdl->ByteCount, remaining);

        RtlCopyMemory((PVOID)((ULONG_PTR)mapping + Buffer->Offset),
                      (PVOID)((ULONG_PTR)Buffer->Data + totalWritten),
                      transferCount);

        remaining -= transferCount;
        totalWritten += transferCount;
        mdl = mdl->Next;
    }

    *BytesWritten = totalWritten;
    return STATUS_SUCCESS;
}

NTSTATUS
CdReadIoInput(
    PCD_IO_PIPE Pipe,
    UCHAR Function,
    PCD_IO_OPERATION Operation,
    ULONG Length,
    PULONG BytesRead
    )
{
    PIRP irp;
    NTSTATUS status;
    PCD_IO_TABLE table;
    PLUID identifier;

    UNREFERENCED_PARAMETER(Pipe);
    UNREFERENCED_PARAMETER(Length);

    if (Operation == NULL || BytesRead == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    identifier = &Operation->Identifier;
    table = &Pipe->DeliveredTable;

    irp = CdpLookupIo(table, identifier);
    if (irp == NULL) {
        return STATUS_NOT_FOUND;
    }

    status = CdpReadIoInput(irp, &Operation->Buffer, BytesRead);
    return status;
}

NTSTATUS
CdReadNextIo(
    PCD_IO_PIPE Pipe,
    PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Pipe == NULL || Irp == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);

    if (Pipe->ReadIoIrp != NULL) {
        status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    Pipe->ReadIoIrp = Irp;
    IoSetCancelRoutine(Irp, CdpCancelReadIoIrp);

    status = STATUS_PENDING;

Exit:
    ExReleasePushLockExclusiveEx((PULONG_PTR)&Pipe->Lock, 0);
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
CdWriteIoOutput(
    PCD_IO_PIPE Pipe,
    UCHAR Function,
    PCD_IO_OPERATION Operation,
    ULONG Length,
    PULONG BytesWritten
    )
{
    PIRP irp;
    PCD_IO_TABLE table;
    PLUID identifier;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Function);

    if (Pipe == NULL || Operation == NULL || BytesWritten == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    identifier = &Operation->Identifier;
    table = &Pipe->DeliveredTable;

    irp = CdpLookupIo(table, identifier);
    if (irp == NULL) {
        return STATUS_NOT_FOUND;
    }

    status = CdpWriteIoOutput(irp, &Operation->Buffer, BytesWritten);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

