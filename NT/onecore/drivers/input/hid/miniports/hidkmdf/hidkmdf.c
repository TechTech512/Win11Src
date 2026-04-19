#pragma warning (disable:4996)

#include <ntddk.h>
#include <wdm.h>
#include <hidport.h>

// Forward declarations
DRIVER_ADD_DEVICE HidKmdfAddDevice;
DRIVER_UNLOAD HidKmdfUnload;
DRIVER_DISPATCH HidKmdfPassThrough;
DRIVER_DISPATCH HidKmdfPnp;
DRIVER_DISPATCH HidKmdfPowerPassThrough;
void QueryForWdfHidInterface(PDEVICE_OBJECT DeviceObject);
void HidKmdfNotifyPresence(void);

// External functions (from HID library)
extern NTSTATUS HidRegisterMinidriver(PHID_MINIDRIVER_REGISTRATION Registration);

// GUID for WDF HID interface (typically defined elsewhere)
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
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = HidKmdfPassThrough;
    }
    
    // Set specific major functions
    DriverObject->MajorFunction[IRP_MJ_POWER] = HidKmdfPowerPassThrough;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HidKmdfPnp;
    
    // Set add device and unload routines
    DriverObject->DriverExtension->AddDevice = HidKmdfAddDevice;
    DriverObject->DriverUnload = HidKmdfUnload;
    
    // Register as HID minidriver
    registration.Revision = 1;
    registration.DriverObject = DriverObject;
    registration.RegistryPath = RegistryPath;
    registration.DeviceExtensionSize = 0x18;
    // registration.Reserved = 0;
    
    return HidRegisterMinidriver(&registration);
}

// Add device routine
NTSTATUS HidKmdfAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
)
{
    // Query for WDF HID interface
    QueryForWdfHidInterface(PhysicalDeviceObject);
    
    // Clear DO_DEVICE_INITIALIZING flag
    PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return STATUS_SUCCESS;
}

// Notify presence callback
void HidKmdfNotifyPresence(void)
{
    HidNotifyPresence(NULL, FALSE);
}

// Pass-through dispatch routine
NTSTATUS HidKmdfPassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location parameters (7 DWORDs = 28 bytes)
    *nextStack = *stack;
    
    // Clear control field (at offset -0x21 from stack)
    *(PUCHAR)((PUCHAR)nextStack + -0x21) = 0;  // Adjust offset as needed
    
    return IoCallDriver(DeviceObject->NextDevice, Irp);
}

// PnP dispatch routine
NTSTATUS HidKmdfPnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    PSHORT deviceExtension;
    PSHORT extensionData;
    ULONG i;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location parameters
    *nextStack = *stack;
    
    // Clear control field
    *(PUCHAR)((PUCHAR)nextStack + 0x1F) = 0;
    
    // Check for specific PnP minor function (IRP_MN_QUERY_CAPABILITIES or similar)
    if (stack->MinorFunction == 2)  // Example: IRP_MN_QUERY_CAPABILITIES
    {
        deviceExtension = (PSHORT)DeviceObject->DeviceExtension;
        extensionData = (PSHORT)((PUCHAR)deviceExtension + 8);
        
        if (*extensionData == 0x18 && extensionData[1] == 1)
        {
            // Call callback function from extension data
            void (*callback)(void*) = *(void (**)(void*))(extensionData + 6);
            void* context = *(void**)(extensionData + 2);
            
            if (callback)
            {
                callback(context);
            }
            
            // Clear extension data (6 WORDs = 12 bytes)
            for (i = 0; i < 6; i++)
            {
                extensionData[i] = 0;
            }
        }
    }
    
    return IoCallDriver(DeviceObject->NextDevice, Irp);
}

// Power dispatch routine
NTSTATUS HidKmdfPowerPassThrough(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_OBJECT targetDevice;
    
    // Start next power IRP
    PoStartNextPowerIrp(Irp);
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location parameters
    *nextStack = *stack;
    
    // Clear control field
    *(PUCHAR)((PUCHAR)nextStack + -0x21) = 0;
    
    // Get target device from device extension
    targetDevice = *(PDEVICE_OBJECT*)((PUCHAR)DeviceObject->DeviceExtension + 4);
    
    return PoCallDriver(targetDevice, Irp);
}

// Unload routine
VOID HidKmdfUnload(
    PDRIVER_OBJECT DriverObject
)
{
    // Nothing to clean up
    return;
}

// Query for WDF HID interface
void QueryForWdfHidInterface(
    PDEVICE_OBJECT DeviceObject
)
{
    PIRP irp;
    PIO_STACK_LOCATION stack;
    PDEVICE_OBJECT lowerDevice;
    PSHORT interfaceData;
    ULONG i;
    PDEVICE_OBJECT targetDevice;
    CCHAR stackSize;
    
    // Get lower device object from device extension
    lowerDevice = *(PDEVICE_OBJECT*)((PUCHAR)DeviceObject->DeviceExtension + 0x28);
    targetDevice = *(PDEVICE_OBJECT*)((PUCHAR)lowerDevice + 8);
    
    // Get the stack size for the target device
    stackSize = targetDevice->StackSize + 1;
    
    // Allocate interface data structure (6 DWORDs = 24 bytes)
    interfaceData = (PSHORT)ExAllocatePoolWithTag(NonPagedPool, 24, 'idH ');
    if (!interfaceData)
        return;
    
    // Initialize interface data
    for (i = 0; i < 6; i++)
    {
        interfaceData[i] = 0;
    }
    
    interfaceData[0] = 0x18;  // Size
    interfaceData[2] = 1;     // Version
    *(PVOID*)(interfaceData + 4) = HidKmdfNotifyPresence;  // NotifyPresence callback
    *(PDEVICE_OBJECT*)(interfaceData + 5) = DeviceObject;   // Context
    
    // Allocate IRP
    irp = IoAllocateIrp(stackSize, FALSE);
    if (!irp)
    {
        ExFreePoolWithTag(interfaceData, 'idH ');
        return;
    }
    
    // Set up IRP for synchronous query interface
    stack = IoGetNextIrpStackLocation(irp);
    stack->MajorFunction = IRP_MJ_PNP;
    stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    stack->Parameters.QueryInterface.InterfaceType = &GUID_WDF_HID_INTERFACE_STANDARD;
    stack->Parameters.QueryInterface.Size = sizeof(INTERFACE);
    stack->Parameters.QueryInterface.Version = 1;
    stack->Parameters.QueryInterface.Interface = (PINTERFACE)interfaceData;
    stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    
    // Set completion routine and call driver
    IoSetCompletionRoutine(irp, NULL, NULL, TRUE, TRUE, TRUE);
    
    IoCallDriver(targetDevice, irp);
	IoForwardIrpSynchronously(targetDevice, irp);
    
    if (irp->IoStatus.Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&irp->IoStatus, Executive, KernelMode, FALSE, NULL);
    }
    
    if (!NT_SUCCESS(irp->IoStatus.Status))
    {
        // Clear interface data on failure
        for (i = 0; i < 6; i++)
        {
            interfaceData[i] = 0;
        }
    }
    
    IoFreeIrp(irp);
}

