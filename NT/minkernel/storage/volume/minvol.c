#include <ntddk.h>

// Forward declarations
NTSTATUS VolumeAddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject);
NTSTATUS VolumePassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS VolumePnp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS VolumeDeviceUsageNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID VolumeUnload(PDRIVER_OBJECT DriverObject);

// Driver entry point
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    ULONG i;
    
    DriverObject->DriverExtension->AddDevice = VolumeAddDevice;
    
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = VolumePassThrough;
    }
    
    DriverObject->MajorFunction[IRP_MJ_PNP] = VolumePnp;
    DriverObject->DriverUnload = VolumeUnload;
    
    return STATUS_SUCCESS;
}

// Add device routine
NTSTATUS VolumeAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
)
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    PVOID extension;
    PDEVICE_OBJECT targetDevice;
    PULONG extensionData = NULL;
    
    status = IoCreateDevice(
        DriverObject,
        0xc,
        NULL,
        0x2d,
        0x100,
        FALSE,
        &deviceObject
    );
    
    if (!NT_SUCCESS(status)) {
        deviceObject = NULL;
        goto error;
    }
    
    extensionData = (PULONG)deviceObject->DeviceExtension;
    extensionData[0] = 0;
    extensionData[1] = 0;
    extensionData[2] = 0;
    
    targetDevice = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    extensionData[1] = (ULONG)targetDevice;
    
    if (!targetDevice) {
        status = STATUS_NO_SUCH_DEVICE;
        goto error;
    }
    
    extensionData[0] = (ULONG)deviceObject;
    deviceObject->StackSize = targetDevice->StackSize + 1;
    deviceObject->Flags |= DO_BUFFERED_IO;
    
    if (targetDevice->Flags & DO_DIRECT_IO) {
        deviceObject->Flags |= DO_DIRECT_IO;
    }
    
    if (targetDevice->Flags & DO_POWER_PAGABLE) {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }
    
    deviceObject->Characteristics |= (PhysicalDeviceObject->Characteristics & 0x5010f);
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return STATUS_SUCCESS;
    
error:
    if (deviceObject) {
        if (extensionData && extensionData[1]) {
            IoDetachDevice((PDEVICE_OBJECT)extensionData[1]);
        }
        IoDeleteDevice(deviceObject);
    }
    
    return status;
}

// Device usage notification
NTSTATUS VolumeDeviceUsageNotification(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PULONG extensionData;
    PIRP targetIrp;
    NTSTATUS status;
    PULONG referenceCount;
    LONG oldCount;
    
    extensionData = (PULONG)DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);
    
    // Forward the IRP to the target device
    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver((PDEVICE_OBJECT)extensionData[1], Irp);
    
    if (NT_SUCCESS(status)) {
        if (stack->Parameters.UsageNotification.Type == 1) {
            referenceCount = &extensionData[2];
            if (*(PUCHAR)((PUCHAR)stack + 4) == 0) {
                KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)&DeviceObject->DeviceLock);
                oldCount = *referenceCount;
                *referenceCount = *referenceCount - 1;
                KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)&DeviceObject->DeviceLock);
                
                if (oldCount == 1) {
                    DeviceObject->Flags |= DO_POWER_PAGABLE;
                }
            } else {
                KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)&DeviceObject->DeviceLock);
                oldCount = *referenceCount;
                *referenceCount = *referenceCount + 1;
                KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)&DeviceObject->DeviceLock);
                
                if (oldCount == 0) {
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                }
            }
        }
    }
    
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}

// Pass-through dispatch routine
NTSTATUS VolumePassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PULONG extensionData;
    
    extensionData = (PULONG)DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver((PDEVICE_OBJECT)extensionData[1], Irp);
}

// PnP dispatch routine
NTSTATUS VolumePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PULONG extensionData;
    NTSTATUS status;
    
    extensionData = (PULONG)DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation(Irp);
    
    switch (stack->MinorFunction) {
        case IRP_MN_START_DEVICE:
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver((PDEVICE_OBJECT)extensionData[1], Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
            
        case IRP_MN_REMOVE_DEVICE:
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver((PDEVICE_OBJECT)extensionData[1], Irp);
            IoDetachDevice((PDEVICE_OBJECT)extensionData[1]);
            IoDeleteDevice((PDEVICE_OBJECT)extensionData[0]);
            break;
            
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            status = VolumeDeviceUsageNotification(DeviceObject, Irp);
            break;
            
        default:
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver((PDEVICE_OBJECT)extensionData[1], Irp);
            break;
    }
    
    return status;
}

// Unload routine
VOID VolumeUnload(
    PDRIVER_OBJECT DriverObject
)
{
    return;
}

