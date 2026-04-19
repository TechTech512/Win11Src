#pragma warning (disable:4996)

#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>

// Forward declarations
DRIVER_ADD_DEVICE HidUmdfAddDevice;
DRIVER_UNLOAD HidUmdfUnload;
DRIVER_DISPATCH HidUmdfPassThrough;
DRIVER_DISPATCH HidUmdfPnp;
DRIVER_DISPATCH HidUmdfPowerPassThrough;
DRIVER_DISPATCH HidUmdfCreateCleanupClose;
DRIVER_DISPATCH HidUmdfInternalIoctl;
NTSTATUS HidUmdfInternalIoctlWorker(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS HandleIdleNotification(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS HandleQueryId(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS HidUmdfGetSettings(PDEVICE_OBJECT DeviceObject);
NTSTATUS UpdateBufferLocationAndIoctl(PIRP Irp, PULONG IoctlCode);
NTSTATUS SendIrpSynchronously(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS SyncIrpCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS UserIoctlCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
void QueryForWdfHidInterface(PDEVICE_OBJECT DeviceObject);
void HidUmdfNotifyPresence(void);
void IdleNotificationCancellation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
void IoctlWorkItemEx(PVOID Context, PIO_WORKITEM WorkItem);

// External functions
extern NTSTATUS HidRegisterMinidriver(PHID_MINIDRIVER_REGISTRATION Registration);

// Scratch buffer for IOCTL operations (255 bytes)
UCHAR G_ScratchBuffer[255] = {0};

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT TargetDevice;
    KSPIN_LOCK SpinLock;
    PIRP PendingIrp;
    PVOID CancellationRoutine;
    BOOLEAN EnableIdleHandler;
    PVOID InterfaceData;
    ULONG SomeField;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// GUID for WDF HID interface
// {FFAD15A2-A6F8-4E60-99C7-2B92624DDC25}
EXTERN_C const GUID GUID_WDF_HID_INTERFACE_STANDARD = 
    { 0xffad15a2, 0xa6f8, 0x4e60, { 0x99, 0xc7, 0x2b, 0x92, 0x62, 0x4d, 0xdc, 0x25 } };

// Driver entry point
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    HID_MINIDRIVER_REGISTRATION registration;
    ULONG i;
    
    // Initialize all major functions to pass-through
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = HidUmdfPassThrough;
    }
    
    // Set specific major functions
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidUmdfInternalIoctl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidUmdfCreateCleanupClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = HidUmdfCreateCleanupClose;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = HidUmdfCreateCleanupClose;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HidUmdfPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = HidUmdfPowerPassThrough;
    
    // Set add device and unload routines
    DriverObject->DriverExtension->AddDevice = HidUmdfAddDevice;
    DriverObject->DriverUnload = HidUmdfUnload;
    
    // Register as HID minidriver
    RtlZeroMemory(&registration, sizeof(registration));
    registration.Revision = 1;
    registration.DriverObject = DriverObject;
    registration.RegistryPath = RegistryPath;
    registration.DeviceExtensionSize = 0x24;
    
    return HidRegisterMinidriver(&registration);
}

// Add device routine
NTSTATUS HidUmdfAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
)
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    
    // Query for WDF HID interface
    QueryForWdfHidInterface(PhysicalDeviceObject);
    
    // Initialize device extension
    deviceExtension = (PDEVICE_EXTENSION)PhysicalDeviceObject->DeviceExtension;
    deviceExtension->SomeField = 0;
    deviceExtension->EnableIdleHandler = FALSE;
    
    KeInitializeSpinLock(&deviceExtension->SpinLock);
    
    status = HidUmdfGetSettings(PhysicalDeviceObject);
    
    if (NT_SUCCESS(status))
    {
        // Clear DO_DEVICE_INITIALIZING flag
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    
    return status;
}

// Create/Cleanup/Close dispatch routine
NTSTATUS HidUmdfCreateCleanupClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    
    Irp->IoStatus.Information = 0;
    
    if (stack->MajorFunction == IRP_MJ_CREATE ||
        stack->MajorFunction == IRP_MJ_CLOSE ||
        stack->MajorFunction == IRP_MJ_CLEANUP)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }
    else
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    }
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}

// Internal IOCTL dispatch routine
NTSTATUS HidUmdfInternalIoctl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    NTSTATUS status;
    
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        status = HidUmdfInternalIoctlWorker(DeviceObject, Irp);
    }
    else
    {
        PIO_WORKITEM workItem;
        
        workItem = IoAllocateWorkItem(DeviceObject);
        if (!workItem)
        {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            // Mark IRP pending
            IoMarkIrpPending(Irp);
            IoQueueWorkItemEx(workItem, (PIO_WORKITEM_ROUTINE_EX)IoctlWorkItemEx, CriticalWorkQueue, Irp);
            status = STATUS_PENDING;
        }
    }
    
    return status;
}

// Internal IOCTL worker routine
NTSTATUS HidUmdfInternalIoctlWorker(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    ULONG ioctlCode;
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN setCompletion = FALSE;
    NTSTATUS status;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    // Copy stack location
    *nextStack = *stack;
    
    ioctlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    
    switch (ioctlCode)
    {
        case 0xb0195:  // IOCTL_HID_GET_STRING
        case 0xb000f:  // IOCTL_HID_GET_DEVICE_ATTRIBUTES
        case 0xb0191:  // Some custom IOCTL
        case 0xb0192:  // Some custom IOCTL
            status = UpdateBufferLocationAndIoctl(Irp, &ioctlCode);
            if (!NT_SUCCESS(status))
            {
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
            }
            setCompletion = TRUE;
            break;
            
        case 0xb0013:  // IOCTL_HID_GET_DEVICE_DESCRIPTOR
            // Swap buffer pointers
            Irp->UserBuffer = (PVOID)stack->Parameters.DeviceIoControl.Type3InputBuffer;
            setCompletion = TRUE;
            Irp->IoStatus.Information = 4;
            break;
            
        case 0xb002b:  // IOCTL_HID_ENABLE_IDLE_NOTIFICATION
            if (deviceExtension->EnableIdleHandler)
            {
                return HandleIdleNotification(DeviceObject, Irp);
            }
            break;
            
        case 0xb019a:  // Some custom IOCTL
            ioctlCode = 0xb0063;
            break;
            
        case 0xb01a2:  // Some custom IOCTL
            // Update buffer location
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = (ULONG)((PUCHAR)stack + 0x14);
            setCompletion = TRUE;
            Irp->IoStatus.Information = 4;
            break;
            
        case 0xb01e2:  // Some custom IOCTL
            stack->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID)Irp->UserBuffer;
            Irp->UserBuffer = (PVOID)((PUCHAR)stack + 0x14);
            setCompletion = TRUE;
            Irp->IoStatus.Information = 4;
            break;
    }
    
    // Update IOCTL code if changed
    nextStack->Parameters.DeviceIoControl.IoControlCode = ioctlCode;
    
    // Set completion routine if needed
    if (setCompletion)
    {
        IoSetCompletionRoutine(Irp, UserIoctlCompletion, NULL, TRUE, TRUE, TRUE);
    }
    
    return IoCallDriver(DeviceObject->NextDevice, Irp);
}

// Handle idle notification
NTSTATUS HandleIdleNotification(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PDEVICE_EXTENSION deviceExtension;
    PIRP targetIrp;
    KIRQL oldIrql;
    NTSTATUS status;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    // Check input buffer size
    if (stack->Parameters.DeviceIoControl.InputBufferLength < 8)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // Call idle notification callback
    targetIrp = (PIRP)stack->Parameters.DeviceIoControl.Type3InputBuffer;
    
    KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);
    
    deviceExtension->CancellationRoutine = IdleNotificationCancellation;
    
    KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);
    
    if (!Irp->Cancel)
    {
        // Mark IRP pending
        IoMarkIrpPending(Irp);
        deviceExtension->PendingIrp = Irp;
        return STATUS_PENDING;
    }
    else
    {
        KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);
        deviceExtension->CancellationRoutine = NULL;
        KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);
        
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_CANCELLED;
    }
}

// Idle notification cancellation routine
void IdleNotificationCancellation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PDEVICE_EXTENSION deviceExtension;
    KIRQL oldIrql;
    
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    
    KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);
    deviceExtension->PendingIrp = NULL;
    KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);
    
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

// Handle query ID
NTSTATUS HandleQueryId(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    ULONG idType;
    NTSTATUS status;
    PWSTR idString;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location
    *nextStack = *stack;
    
    idType = stack->Parameters.QueryId.IdType;
    
    status = SendIrpSynchronously(DeviceObject, Irp);
    
    // If querying device ID and failed, return empty string
    if (idType == 0 && idType == 1 && !NT_SUCCESS(status))
    {
        idString = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, 6, 'HidU');
        if (idString)
        {
            idString[0] = 0;
            idString[1] = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR)idString;
        }
    }
    
    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

// Get settings from registry
NTSTATUS HidUmdfGetSettings(
    PDEVICE_OBJECT DeviceObject
)
{
    NTSTATUS status;
    HANDLE keyHandle;
    UNICODE_STRING valueName;
    KEY_VALUE_PARTIAL_INFORMATION keyInfo;
    ULONG resultLength;
    PDEVICE_EXTENSION deviceExtension;
    
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    RtlInitUnicodeString(&valueName, L"EnableDefaultIdleNotificationHandler");
    
    status = IoOpenDeviceRegistryKey(
        DeviceObject,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_READ,
        &keyHandle
    );
    
    if (NT_SUCCESS(status))
    {
        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            &keyInfo,
            sizeof(keyInfo),
            &resultLength
        );
        
        if (NT_SUCCESS(status) && keyInfo.Type == REG_DWORD)
        {
            deviceExtension->EnableIdleHandler = (*(PULONG)keyInfo.Data != 0);
        }
        
        ZwClose(keyHandle);
    }
    
    return STATUS_SUCCESS;
}

// Pass-through dispatch routine
NTSTATUS HidUmdfPassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location
    *nextStack = *stack;
    
    return IoCallDriver(DeviceObject->NextDevice, Irp);
}

// PnP dispatch routine
NTSTATUS HidUmdfPnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension;
    PSHORT interfaceData;
    ULONG i;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location
    *nextStack = *stack;
    
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    if (stack->MinorFunction == IRP_MN_QUERY_CAPABILITIES)
    {
        interfaceData = (PSHORT)deviceExtension->InterfaceData;
        
        if (interfaceData && *interfaceData == 0x18 && interfaceData[1] == 1)
        {
            // Call notify presence callback
            void (*callback)(void*) = *(void (**)(void*))(interfaceData + 6);
            void* context = *(void**)(interfaceData + 2);
            
            if (callback)
            {
                callback(context);
            }
            
            // Clear interface data
            for (i = 0; i < 6; i++)
            {
                interfaceData[i] = 0;
            }
        }
    }
    else if (stack->MinorFunction == IRP_MN_QUERY_ID)
    {
        return HandleQueryId(DeviceObject, Irp);
    }
    
    return IoCallDriver(DeviceObject->NextDevice, Irp);
}

// Power dispatch routine
NTSTATUS HidUmdfPowerPassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_OBJECT targetDevice;
    PDEVICE_EXTENSION deviceExtension;
    
    PoStartNextPowerIrp(Irp);
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location
    *nextStack = *stack;
    
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    targetDevice = deviceExtension->TargetDevice;
    
    return PoCallDriver(targetDevice, Irp);
}

// Unload routine
VOID HidUmdfUnload(
    PDRIVER_OBJECT DriverObject
)
{
    return;
}

// Notify presence callback
void HidUmdfNotifyPresence(void)
{
    HidNotifyPresence(NULL, FALSE);
}

// Query for WDF HID interface
void QueryForWdfHidInterface(
    PDEVICE_OBJECT DeviceObject
)
{
    PIRP irp;
    PIO_STACK_LOCATION stack;
    PDEVICE_OBJECT lowerDevice;
    PDEVICE_OBJECT targetDevice;
    PSHORT interfaceData;
    ULONG i;
    CCHAR stackSize;
    NTSTATUS status;
    
    // Get target device
    lowerDevice = *(PDEVICE_OBJECT*)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    targetDevice = *(PDEVICE_OBJECT*)((PUCHAR)lowerDevice + 8);
    
    // Get stack size
    stackSize = targetDevice->StackSize + 1;
    
    // Allocate interface data (6 DWORDs = 24 bytes)
    interfaceData = (PSHORT)ExAllocatePoolWithTag(NonPagedPool, 24, 'HidU');
    if (!interfaceData)
        return;
    
    // Initialize interface data
    for (i = 0; i < 6; i++)
    {
        interfaceData[i] = 0;
    }
    
    interfaceData[0] = 0x18;  // Size
    interfaceData[2] = 1;     // Version
    *(PVOID*)(interfaceData + 4) = HidUmdfNotifyPresence;  // NotifyPresence callback
    *(PDEVICE_OBJECT*)(interfaceData + 5) = DeviceObject;   // Context
    
    // Allocate IRP
    irp = IoAllocateIrp(stackSize, FALSE);
    if (!irp)
    {
        ExFreePoolWithTag(interfaceData, 'HidU');
        return;
    }
    
    // Set up IRP for query interface
    stack = IoGetNextIrpStackLocation(irp);
    stack->MajorFunction = IRP_MJ_PNP;
    stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    stack->Parameters.QueryInterface.InterfaceType = &GUID_WDF_HID_INTERFACE_STANDARD;
    stack->Parameters.QueryInterface.Size = sizeof(INTERFACE);
    stack->Parameters.QueryInterface.Version = 1;
    stack->Parameters.QueryInterface.Interface = (PINTERFACE)interfaceData;
    stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    
    // Send IRP synchronously
    status = SendIrpSynchronously(targetDevice, irp);
    
    if (!NT_SUCCESS(status))
    {
        // Clear interface data on failure
        for (i = 0; i < 6; i++)
        {
            interfaceData[i] = 0;
        }
    }
    
    IoFreeIrp(irp);
}

// Send IRP synchronously
NTSTATUS SendIrpSynchronously(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    KEVENT event;
    NTSTATUS status;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    IoSetCompletionRoutine(Irp, SyncIrpCompletion, &event, TRUE, TRUE, TRUE);
    
    status = IoCallDriver(DeviceObject, Irp);
    
    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }
    
    return status;
}

// Synchronous IRP completion routine
NTSTATUS SyncIrpCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    if (Context && Irp->PendingReturned)
    {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// User IOCTL completion routine
NTSTATUS UserIoctlCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    PIO_STACK_LOCATION stack;
    ULONG ioctlCode;
    
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    ioctlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    
    if (ioctlCode == 0xb000f ||      // IOCTL_HID_GET_DEVICE_ATTRIBUTES
        (ioctlCode >= 0xb0191 && ioctlCode <= 0xb0192) ||
        ioctlCode == 0xb0195 ||
        ioctlCode == 0xb01a2 ||
        ioctlCode == 0xb01e2)
    {
        Irp->UserBuffer = Irp->AssociatedIrp.SystemBuffer;
    }
    else if (ioctlCode == 0xb0013)   // IOCTL_HID_GET_DEVICE_DESCRIPTOR
    {
        Irp->AssociatedIrp.SystemBuffer = Irp->UserBuffer;
    }
    
    return STATUS_SUCCESS;
}

// Update buffer location and IOCTL code
NTSTATUS UpdateBufferLocationAndIoctl(
    PIRP Irp,
    PULONG IoctlCode
)
{
    PIO_STACK_LOCATION stack;
    PVOID buffer;
    ULONG originalIoctl;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    buffer = stack->Parameters.DeviceIoControl.Type3InputBuffer;
    originalIoctl = *IoctlCode;
    
    if (!buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    switch (originalIoctl)
    {
        case 0xb000f:  // IOCTL_HID_GET_DEVICE_ATTRIBUTES
        case 0xb0191:  // Some custom IOCTL
            if (stack->Parameters.DeviceIoControl.InputBufferLength < 12)
                return STATUS_BUFFER_TOO_SMALL;
            
            // Update buffer locations
            *(PULONG)((PUCHAR)stack - 0x14) = *(PULONG)buffer;
            *(PULONG)((PUCHAR)stack - 0x1C) = ((PULONG)buffer)[1];
            *(PUCHAR)((PUCHAR)stack - 0x20) = ((PUCHAR)buffer)[2];
            
            // Set scratch buffer
            stack->Parameters.DeviceIoControl.Type3InputBuffer = G_ScratchBuffer;
            break;
            
        case 0xb0192:  // Some custom IOCTL
        case 0xb01a2:  // Some custom IOCTL
            if (stack->Parameters.DeviceIoControl.OutputBufferLength < 12)
                return STATUS_BUFFER_TOO_SMALL;
            
            *(PULONG)((PUCHAR)stack - 0x1C) = 1;
            *(PVOID*)((PUCHAR)stack - 0x14) = (PUCHAR)buffer + 8;
            stack->Parameters.DeviceIoControl.Type3InputBuffer = *(PVOID*)buffer;
            *(PULONG)((PUCHAR)stack - 0x20) = ((PULONG)buffer)[1];
            break;
            
        case 0xb0195:  // Some custom IOCTL
            // Same as 0xb000f case
            if (stack->Parameters.DeviceIoControl.InputBufferLength < 12)
                return STATUS_BUFFER_TOO_SMALL;
            
            *(PULONG)((PUCHAR)stack - 0x14) = *(PULONG)buffer;
            *(PULONG)((PUCHAR)stack - 0x1C) = ((PULONG)buffer)[1];
            *(PUCHAR)((PUCHAR)stack - 0x20) = ((PUCHAR)buffer)[2];
            stack->Parameters.DeviceIoControl.Type3InputBuffer = G_ScratchBuffer;
            break;
    }
    
    // Update IOCTL code
    if (originalIoctl == 0xb0191)
        *IoctlCode = 0xb0053;
    else if (originalIoctl == 0xb0192)
        *IoctlCode = 0xb0057;
    else if (originalIoctl == 0xb0195)
        *IoctlCode = 0xb005b;
    else if (originalIoctl == 0xb01a2)
        *IoctlCode = 0xb005f;
    
    return STATUS_SUCCESS;
}

// IOCTL work item
void IoctlWorkItemEx(
    PVOID Context,
    PIO_WORKITEM WorkItem
)
{
    PIRP Irp = (PIRP)Context;
    PDEVICE_OBJECT DeviceObject;
    
    if (Irp)
    {
        DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
        HidUmdfInternalIoctlWorker(DeviceObject, Irp);
    }
    
    IoFreeWorkItem(WorkItem);
}
