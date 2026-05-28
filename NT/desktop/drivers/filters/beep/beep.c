#include <ntddk.h>
#include <wdm.h>

// Forward declarations
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD BeepUnload;
DRIVER_DISPATCH BeepOpen;
DRIVER_DISPATCH BeepClose;
DRIVER_DISPATCH BeepDeviceControl;
DRIVER_DISPATCH BeepCleanup;
DRIVER_CANCEL BeepCancel;
DRIVER_STARTIO BeepStartIo;

// External functions from redirect.c
extern NTSTATUS BeepRedirectMakeBeep(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG Frequency, ULONG Duration);
extern void BeepRedirectCleanupQueue(PDEVICE_OBJECT DeviceObject);
extern NTSTATUS BeepRedirectCsqInsertIrp(PIO_CSQ Csq, PIRP Irp);
extern void BeepRedirectCsqRemoveIrp(PIO_CSQ Csq, PIRP Irp);
extern PIRP BeepRedirectCsqPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext);
extern void BeepRedirectCsqAcquireLock(PIO_CSQ Csq, PKIRQL OldIrql);
extern void BeepRedirectCsqReleaseLock(PIO_CSQ Csq, KIRQL OldIrql);
extern void BeepRedirectCsqCompleteCanceledIrp(PIO_CSQ Csq, PIRP Irp);

// Device extension structure
typedef struct _BEEP_DEVICE_EXTENSION {
    ULONG OpenCount;
    ULONG ImageSection;
    KSPIN_LOCK SpinLock;
    KEVENT Event;
    IO_CSQ Csq;
    LIST_ENTRY QueueHead;
    KSPIN_LOCK QueueLock;
    ULONG Unknown1;
    ULONG Unknown2;
} BEEP_DEVICE_EXTENSION, *PBEEP_DEVICE_EXTENSION;

// Cancel routine
void BeepCancel(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    KIRQL cancelIrql;
    BOOLEAN removed;
    
    if (Irp == DeviceObject->CurrentIrp)
    {
        cancelIrql = Irp->CancelIrql;
    }
    else
    {
        removed = KeRemoveEntryDeviceQueue(&DeviceObject->DeviceQueue, &Irp->Tail.Overlay.DeviceQueueEntry);
        cancelIrql = Irp->CancelIrql;
        
        if (removed)
        {
            IoReleaseCancelSpinLock(cancelIrql);
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return;
        }
    }
    
    IoReleaseCancelSpinLock(cancelIrql);
}

// Cleanup routine
NTSTATUS BeepCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PBEEP_DEVICE_EXTENSION deviceExtension;
    KIRQL oldIrql;
    PIRP currentIrp;
    PKDPC dpc;
    NTSTATUS status;
    
    deviceExtension = (PBEEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    oldIrql = KeRaiseIrqlToDpcLevel();
    IoAcquireCancelSpinLock(&oldIrql);
    
    currentIrp = DeviceObject->CurrentIrp;
    DeviceObject->CurrentIrp = NULL;
    
    if (currentIrp)
    {
        while ((dpc = (PKDPC)KeRemoveDeviceQueue(&DeviceObject->DeviceQueue)) != NULL)
        {
            PIRP queuedIrp = CONTAINING_RECORD(dpc, IRP, Tail.Overlay.DeviceQueueEntry);
            
            IoReleaseCancelSpinLock(oldIrql);
            
            queuedIrp->IoStatus.Status = STATUS_CANCELLED;
            queuedIrp->IoStatus.Information = 0;
            IoCompleteRequest(queuedIrp, IO_NO_INCREMENT);
            
            IoAcquireCancelSpinLock(&oldIrql);
        }
    }
    
    IoReleaseCancelSpinLock(oldIrql);
    KeLowerIrql(oldIrql);
    
    BeepRedirectMakeBeep(DeviceObject, Irp, 0, 0);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

// Close routine
NTSTATUS BeepClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PBEEP_DEVICE_EXTENSION deviceExtension;
    
    deviceExtension = (PBEEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    ExAcquireFastMutex((PFAST_MUTEX)&deviceExtension->SpinLock);
    
    deviceExtension->OpenCount--;
    
    if (deviceExtension->OpenCount == 0)
    {
        MmUnlockPagableImageSection((PVOID)(ULONG_PTR)deviceExtension->ImageSection);
    }
    
    ExReleaseFastMutex((PFAST_MUTEX)&deviceExtension->SpinLock);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

// Device control routine
NTSTATUS BeepDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIRP masterIrp;
    PBEEP_DEVICE_EXTENSION deviceExtension;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PBEEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    if (stack->Parameters.DeviceIoControl.IoControlCode == 0x10000)  // IOCTL_BEEP_SET
    {
        masterIrp = Irp->AssociatedIrp.MasterIrp;
        
        if (stack->Parameters.DeviceIoControl.InputBufferLength > 8)
        {
            if (masterIrp && masterIrp->MdlAddress)
            {
                Irp->IoStatus.Status = STATUS_PENDING;
                Irp->IoStatus.Information = 0;
                IoMarkIrpPending(Irp);
                IoStartPacket(DeviceObject, Irp, 0, BeepCancel);
                return STATUS_PENDING;
            }
            else
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    else if (stack->Parameters.DeviceIoControl.IoControlCode == 0x10004 &&  // IOCTL_BEEP_QUEUE
             stack->Parameters.DeviceIoControl.OutputBufferLength > 8)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);
        IoCsqInsertIrp(&deviceExtension->Csq, Irp, NULL);
        return STATUS_PENDING;
    }
    
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}

// Open routine
NTSTATUS BeepOpen(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PBEEP_DEVICE_EXTENSION deviceExtension;
    
    deviceExtension = (PBEEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    ExAcquireFastMutex((PFAST_MUTEX)&deviceExtension->SpinLock);
    
    deviceExtension->OpenCount++;
    
    if (deviceExtension->OpenCount == 1)
    {
        deviceExtension->ImageSection = (ULONG)MmLockPagableDataSection(BeepOpen);
    }
    
    ExReleaseFastMutex((PFAST_MUTEX)&deviceExtension->SpinLock);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

// Start I/O routine
void BeepStartIo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    KIRQL oldIrql;
    PIRP masterIrp;
    NTSTATUS status;
    
    IoAcquireCancelSpinLock(&oldIrql);
    
    if (!Irp)
    {
        IoReleaseCancelSpinLock(oldIrql);
        return;
    }
    
    Irp->CancelRoutine = NULL;
    
    IoReleaseCancelSpinLock(oldIrql);
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    
    if (stack->Parameters.DeviceIoControl.IoControlCode == 0x10000)  // IOCTL_BEEP_SET
    {
        masterIrp = Irp->AssociatedIrp.MasterIrp;
        status = BeepRedirectMakeBeep(
            DeviceObject,
            masterIrp,
            *(PULONG)masterIrp->MdlAddress,
            *((PULONG)masterIrp->MdlAddress + 1)
        );
        Irp->IoStatus.Status = status;
    }
    else
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    
    Irp->IoStatus.Information = 0;
    IoStartNextPacket(DeviceObject, TRUE);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

// Unload routine
void BeepUnload(
    PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject;
    
    deviceObject = DriverObject->DeviceObject;
    BeepRedirectCleanupQueue(deviceObject);
    IoDeleteDevice(deviceObject);
}

// Driver entry point
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject;
    PBEEP_DEVICE_EXTENSION deviceExtension;
    
    RtlInitUnicodeString(&deviceName, L"\\Device\\Beep");
    
    status = IoCreateDevice(
        DriverObject,
        sizeof(BEEP_DEVICE_EXTENSION),
        &deviceName,
        FILE_DEVICE_BEEP,
        0,
        FALSE,
        &deviceObject
    );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    deviceExtension = (PBEEP_DEVICE_EXTENSION)deviceObject->DeviceExtension;
    
    // Initialize device object flags
    deviceObject->Flags |= DO_BUFFERED_IO;
    
    // Initialize device extension
    deviceExtension->OpenCount = 0;
    deviceExtension->ImageSection = 0;
    KeInitializeEvent(&deviceExtension->Event, NotificationEvent, FALSE);
    KeInitializeSpinLock(&deviceExtension->SpinLock);
    
    // Initialize CSQ
    InitializeListHead(&deviceExtension->QueueHead);
    KeInitializeSpinLock(&deviceExtension->QueueLock);
    
    IoCsqInitialize(
        &deviceExtension->Csq,
        (PIO_CSQ_INSERT_IRP)BeepRedirectCsqInsertIrp,
        BeepRedirectCsqRemoveIrp,
        BeepRedirectCsqPeekNextIrp,
        BeepRedirectCsqAcquireLock,
        BeepRedirectCsqReleaseLock,
        BeepRedirectCsqCompleteCanceledIrp
    );
    
    // Set driver entry points
    DriverObject->DriverStartIo = BeepStartIo;
    DriverObject->DriverUnload = BeepUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BeepClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BeepDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;
    
    IoSetStartIoAttributes(deviceObject, TRUE, FALSE);
    
    // Clear DO_DEVICE_INITIALIZING flag
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return STATUS_SUCCESS;
}

