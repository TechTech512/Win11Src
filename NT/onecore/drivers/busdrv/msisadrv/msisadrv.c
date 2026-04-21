#pragma warning (disable:4996)

#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>

// GUIDs from hex dump
// {E676F854-D87D-11D0-92B2-00A0C9055FC5}
EXTERN_C const GUID GUID_BUS_TYPE_ISAPNP = 
    { 0xe676f854, 0xd87d, 0x11d0, { 0x92, 0xb2, 0x00, 0xa0, 0xc9, 0x05, 0x5f, 0xc5 } };

// {6C154A92-AACF-11D0-8D2A-00A0C906B244}
EXTERN_C const GUID GUID_TRANSLATOR_INTERFACE_STANDARD = 
    { 0x6c154a92, 0xaacf, 0x11d0, { 0x8d, 0x2a, 0x00, 0xa0, 0xc9, 0x06, 0xb2, 0x44 } };

// Global variables
KEVENT BusNumberLock;
RTL_BITMAP BusNumberBitMap;
ULONG BusNumberBuffer[8];

// Initialize WDF context type info
WDF_OBJECT_CONTEXT_TYPE_INFO _WDF_FDO_DATA_TYPE_INFO = {
    sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO),
    "FDO_DATA",
    0x08,
    &_WDF_FDO_DATA_TYPE_INFO,
    NULL
};

// Forward declarations
NTSTATUS MsIsaEvtDeviceAdd(WDFDRIVER *Driver, PWDFDEVICE_INIT DeviceInit);
void MsIsaEvtCleanupCallback(WDFDEVICE Device);
NTSTATUS MsIsaEvtDeviceProcessQueryInterface(WDFDEVICE Device, GUID *InterfaceType, PINTERFACE Interface, PVOID SpecificData);
NTSTATUS MsIsaPnPIrpPreProcessingCallback(WDFDEVICE Device, PIRP Irp);

// PnP minor function table
unsigned char PnPMinorFunctionTable[1] = { 0x18 };

// Driver entry point
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    struct {
        ULONG Size;
        void *EvtDeviceAdd;
        ULONG Unknown1;
        ULONG Unknown2;
        ULONG Unknown3;
    } driverConfig;
    
    driverConfig.Size = 0x14;
    driverConfig.EvtDeviceAdd = MsIsaEvtDeviceAdd;
    driverConfig.Unknown1 = 0;
    driverConfig.Unknown2 = 0;
    driverConfig.Unknown3 = 0;
    
    status = (*(long (__cdecl *)(void *, PDRIVER_OBJECT, PUNICODE_STRING, int, void *, int))WdfFunctions[0x74])(
        WdfDriverGlobals,
        DriverObject,
        RegistryPath,
        0,
        &driverConfig,
        0
    );
    
    KeInitializeEvent(&BusNumberLock, NotificationEvent, TRUE);
    RtlInitializeBitMap(&BusNumberBitMap, BusNumberBuffer, 0x100);
    RtlClearAllBits(&BusNumberBitMap);
    
    return status;
}

// Cleanup callback
void MsIsaEvtCleanupCallback(
    WDFDEVICE Device
)
{
    PVOID context;
    ULONG busNumber;
    
    context = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_FDO_DATA_TYPE_INFO.UniqueType);
    busNumber = *(PULONG)((PUCHAR)context + 4);
    
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&BusNumberLock, Executive, KernelMode, FALSE, NULL);
    RtlClearBits(&BusNumberBitMap, busNumber, 1);
    KeSetEvent(&BusNumberLock, 0, FALSE);
    KeLeaveCriticalRegion();
}

// Device add routine
NTSTATUS MsIsaEvtDeviceAdd(
    WDFDRIVER *Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    PVOID context;
    ULONG busNumber;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    PVOID contextBuffer[8];
    ULONG i;
    
    // Initialize context buffer
    for (i = 0; i < 8; i++) {
        contextBuffer[i] = 0;
    }
    
    // Set PnP IRP preprocessor callback
    status = (*(long (__cdecl *)(void *, PWDFDEVICE_INIT, PVOID, ULONG, unsigned char[1], int))WdfFunctions[0x49])(
        WdfDriverGlobals,
        DeviceInit,
        MsIsaPnPIrpPreProcessingCallback,
        IRP_MJ_PNP,
        PnPMinorFunctionTable,
        1
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Set cleanup callback
    WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    deviceAttributes.EvtCleanupCallback = MsIsaEvtCleanupCallback;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, FDO_DATA);
    
	MsIsaEvtCleanupCallback((WDFDEVICE)Driver);
    status = (*(long (__cdecl *)(void *, PWDFDEVICE_INIT *, WDF_OBJECT_ATTRIBUTES *, WDFDEVICE *))WdfFunctions[0x4b])(
        WdfDriverGlobals,
        &DeviceInit,
        &deviceAttributes,
        &device
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Set device state
    (*(void (__cdecl *)(void *, WDFDEVICE, int, int))WdfFunctions[0x31])(WdfDriverGlobals, device, 1, 1);
    (*(void (__cdecl *)(void *, WDFDEVICE, int, int))WdfFunctions[0x31])(WdfDriverGlobals, device, 2, 1);
    (*(void (__cdecl *)(void *, WDFDEVICE, int, int))WdfFunctions[0x31])(WdfDriverGlobals, device, 3, 1);
    
    // Get context and allocate bus number
    context = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_FDO_DATA_TYPE_INFO.UniqueType);
    *(PULONG)context = 0x6173494d;  // "MsIsa"
    
    KeEnterCriticalRegion();
    KeWaitForSingleObject(&BusNumberLock, Executive, KernelMode, FALSE, NULL);
    busNumber = RtlFindClearBitsAndSet(&BusNumberBitMap, 1, 0);
    *(PULONG)((PUCHAR)context + 4) = busNumber;
    KeSetEvent(&BusNumberLock, 0, FALSE);
    KeLeaveCriticalRegion();
    
    // Register translator interface using function table
	WDF_QUERY_INTERFACE_CONFIG interfaceConfig;
	interfaceConfig.Size = sizeof(WDF_QUERY_INTERFACE_CONFIG);
	interfaceConfig.EvtDeviceProcessQueryInterfaceRequest = (PFN_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST)MsIsaEvtDeviceProcessQueryInterface;
	interfaceConfig.InterfaceType = (GUID *)&GUID_TRANSLATOR_INTERFACE_STANDARD;
	interfaceConfig.SendQueryToParentStack = TRUE;
    
	status = (*(long (__cdecl *)(void *, WDFDEVICE, WDF_QUERY_INTERFACE_CONFIG *))WdfFunctions[0xe4])(
		WdfDriverGlobals,
		device,
		&interfaceConfig
	);
    
    return status;
}

// Process query interface
NTSTATUS MsIsaEvtDeviceProcessQueryInterface(
    WDFDEVICE Device,
    GUID *InterfaceType,
    PINTERFACE Interface,
    PVOID SpecificData
)
{
    PVOID translatorContext;
    PVOID pnpContext;
    NTSTATUS status;
    
    if (SpecificData != (PVOID)2 || Interface->Size <= 0x18) {
        return STATUS_NOT_SUPPORTED;
    }
    
    if (memcmp(InterfaceType, &GUID_TRANSLATOR_INTERFACE_STANDARD, sizeof(GUID)) != 0) {
        return STATUS_NOT_SUPPORTED;
    }
    
    status = (*(long (__cdecl *)(void *, WDFDEVICE, int, int, PVOID *, PVOID *))WdfFunctions[0x51])(
        WdfDriverGlobals,
        Device,
        0xd,
        4,
        &translatorContext,
        NULL
    );
    
    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }
    
    status = (*(long (__cdecl *)(void *, WDFDEVICE, int, int, PVOID *, PVOID *))WdfFunctions[0x51])(
        WdfDriverGlobals,
        Device,
        0xe,
        4,
        &pnpContext,
        NULL
    );
    
    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }
    
    // Call the translator interface function
    status = (*(long (__cdecl *)(PVOID, PVOID, int, USHORT, USHORT, PINTERFACE, PVOID *))0x51ca)(
        translatorContext,
        pnpContext,
        1,
        Interface->Size,
        Interface->Version,
        Interface,
        NULL
    );
    
    return status;
}

// PnP IRP pre-processing callback
NTSTATUS MsIsaPnPIrpPreProcessingCallback(
    WDFDEVICE Device,
    PIRP Irp
)
{
    PVOID context;
    PIO_STACK_LOCATION stack;
    PULONG buffer;
    ULONG busNumber;
    NTSTATUS status;
    
    context = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_FDO_DATA_TYPE_INFO.UniqueType);
    stack = IoGetCurrentIrpStackLocation(Irp);
    busNumber = *(PULONG)((PUCHAR)context + 4);
    
    if (stack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS && 
        stack->Parameters.QueryDeviceRelations.Type == BusRelations) {
        
        buffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, 0x18, 'MsIs');
        if (!buffer) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        buffer[0] = 0xe676f854;
        buffer[1] = 0x11d0d87d;
        buffer[2] = 0xa000b292;
        buffer[3] = 0xc55f05c9;
        buffer[4] = 1;
        buffer[5] = busNumber;
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR)buffer;
    }
    
    // Skip current stack location
    Irp->CurrentLocation++;
    IoSkipCurrentIrpStackLocation(Irp);
    
    status = (*(long (__cdecl *)(void *, WDFDEVICE, PIRP))WdfFunctions[0x22])(
        WdfDriverGlobals,
        Device,
        Irp
    );
    
    return status;
}

