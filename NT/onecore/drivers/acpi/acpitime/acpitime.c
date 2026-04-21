#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>

// GUIDs
// GUID for ACPI interface standard from hex dump
// {B091A08A-BA97-11D0-BD14-00AABB7B2A??}
EXTERN_C const GUID GUID_ACPI_INTERFACE_STANDARD = 
    { 0xb091a08a, 0xba97, 0x11d0, { 0xbd, 0x14, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };
// GUID for device ACPI time interface from hex dump
// {97F99BF6-4497-4F18-BB22-4B9FB2FBEF9C}
EXTERN_C const GUID GUID_DEVICE_ACPI_TIME = 
    { 0x97f99bf6, 0x4497, 0x4f18, { 0xbb, 0x22, 0x4b, 0x9f, 0xb2, 0xfb, 0xef, 0x9c } };

// Global variables
SYSTEM_POWER_STATE g_DeepestWakeSystemState = 0;

// External context type info
// WDF object context type info for WA_EXTENSION
WDF_OBJECT_CONTEXT_TYPE_INFO _WDF_WA_EXTENSION_TYPE_INFO = {
    sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO),
    "WA_EXTENSION",
    0x34,  // ContextSize = 52 bytes
    &_WDF_WA_EXTENSION_TYPE_INFO,
    NULL
};

// Function prototype for PoSetFixedWakeSource
NTKERNELAPI
VOID
PoSetFixedWakeSource(
    _In_ ULONG Flags
    );

// Forward declarations
NTSTATUS WaDeviceAdd(WDFDRIVER *Driver, PWDFDEVICE_INIT DeviceInit);
NTSTATUS WaAcpiEvtDeviceWdmIrpQueryCapabilitiesCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
void WaAcpiNotifyCallback(PVOID Context, ULONG NotificationType);
NTSTATUS WaAcpiTimeWdmPreprocessQueryCapabilitiesIrp(WDFDEVICE Device, PIRP Irp);
NTSTATUS WaClearWakeStatus(WDFDEVICE Device, ULONG Param2);
NTSTATUS WaEvtDevicePrepareHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesRaw, WDFCMRESLIST ResourcesTranslated);
NTSTATUS WaEvtDeviceReleaseHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated);
NTSTATUS WaGetCapabilities(WDFDEVICE Device, PULONG Result);
NTSTATUS WaGetWakeStatus(WDFDEVICE Device, ULONG Param2, PULONG Result);
NTSTATUS WaHandleAcpiGetRealTime(WDFDEVICE Device, PVOID RealTime, PULONG Result);
NTSTATUS WaHandleAcpiSetRealTime(WDFDEVICE Device, PVOID RealTime, PULONG Result);
NTSTATUS WaHandleWakeAlarmControl(WDFDEVICE Device, ULONG ControlCode, PVOID InputBuffer, PVOID OutputBuffer, PULONG Result);
void WaIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode);
NTSTATUS WaIsAlarmExpired(WDFDEVICE Device, ULONG Param2, PULONG Result);

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
    
    g_DeepestWakeSystemState = 0;
    
    driverConfig.Size = 0x14;
    driverConfig.EvtDeviceAdd = WaDeviceAdd;
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
    
    return status;
}

// Query capabilities completion routine
NTSTATUS WaAcpiEvtDeviceWdmIrpQueryCapabilitiesCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    g_DeepestWakeSystemState = *(PULONG)((PUCHAR)Context + 4 + 0x2c);
    return STATUS_SUCCESS;
}

// ACPI notify callback
void WaAcpiNotifyCallback(
    PVOID Context,
    ULONG NotificationType
)
{
    WDFDEVICE Device;
    PVOID extension;
    NTSTATUS status;
    ULONG wakeStatus1;
    ULONG wakeStatus2;
    ULONG wakeFlags;
    BOOLEAN hasWakeStatus;
    
    DbgPrint("WaAcpiNotifyCallback(%p, %#08lx)\n", Context, NotificationType);
    
    if (NotificationType != 2) {
        return;
    }
    
    Device = (WDFDEVICE)Context;
    wakeFlags = 0;
    hasWakeStatus = FALSE;
    
    status = WaGetWakeStatus(Device, 0, &wakeStatus1);
    if (NT_SUCCESS(status)) {
        hasWakeStatus = TRUE;
        if ((wakeStatus1 & 2) != 0) {
            wakeFlags = 8;
        }
    }
    
    status = WaGetWakeStatus(Device, 0, &wakeStatus2);
    if (!NT_SUCCESS(status)) {
        if (!hasWakeStatus) {
            extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
            
            if ((*(PUCHAR)((PUCHAR)extension + 0x2c) & 8) != 0) {
                status = WaIsAlarmExpired(Device, 0, NULL);
                if (!NT_SUCCESS(status) || (NotificationType != 0)) {
                    wakeFlags |= 8;
                }
            }
            
            if ((*(PUCHAR)((PUCHAR)extension + 0x2c) & 0x10) != 0) {
                status = WaIsAlarmExpired(Device, 0, NULL);
                if (!NT_SUCCESS(status) || (NotificationType != 0)) {
                    wakeFlags |= 0x10;
                }
            }
            
            PoSetFixedWakeSource(wakeFlags);
        }
    } else {
        hasWakeStatus = TRUE;
        if ((wakeStatus2 & 2) != 0) {
            wakeFlags |= 0x10;
        }
        WaClearWakeStatus(Device, 0);
        WaClearWakeStatus(Device, 0);
        PoSetFixedWakeSource(wakeFlags);
    }
}

// Preprocess query capabilities IRP
NTSTATUS WaAcpiTimeWdmPreprocessQueryCapabilitiesIrp(
    WDFDEVICE Device,
    PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PIO_STACK_LOCATION nextStack;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    
    // Copy stack location
    *nextStack = *stack;
    
    // Set completion routine
    IoSetCompletionRoutine(Irp, WaAcpiEvtDeviceWdmIrpQueryCapabilitiesCompletion, Device, TRUE, TRUE, TRUE);
    
    return (*(long (__cdecl *)(void *, WDFDEVICE, PIRP))WdfFunctions[0x22])(WdfDriverGlobals, Device, Irp);
}

// Clear wake status
NTSTATUS WaClearWakeStatus(
    WDFDEVICE Device,
    ULONG Param2
)
{
    NTSTATUS status;
    ULONG evalBuffer[4];
    ULONG inputBuffer;
    ULONG outputBufferSize;
    ULONG outputBuffer[4];
    
    evalBuffer[0] = 0x49696541;
    evalBuffer[1] = 0x5357435f;
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    inputBuffer = 1;
    outputBufferSize = 0x14;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        &inputBuffer,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (NT_SUCCESS(status)) {
        if (outputBuffer[0] != 0x14 || outputBuffer[3] != 1 || (SHORT)outputBuffer[1] != 0 || outputBuffer[2] != 0) {
            status = STATUS_UNSUCCESSFUL;
        }
    }
    
    return status;
}

// Device add routine
NTSTATUS WaDeviceAdd(
    WDFDRIVER *Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    PVOID context;
    ULONG processorCount;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    
    // Set PnP power callbacks
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = WaEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = WaEvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);
    
    // Set WDM IRP preprocessor
    (*(void (__cdecl *)(void *, PWDFDEVICE_INIT, PVOID, ULONG, ULONG))WdfFunctions[0x49])(WdfDriverGlobals, DeviceInit, WaAcpiTimeWdmPreprocessQueryCapabilitiesIrp, IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES);
    
    // Create device
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, WA_EXTENSION);
    
    status = (*(long (__cdecl *)(void *, PWDFDEVICE_INIT *, WDF_OBJECT_ATTRIBUTES *, WDFDEVICE *))WdfFunctions[0x4b])(
        WdfDriverGlobals,
        &DeviceInit,
        &deviceAttributes,
        &device
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Get extension context
    context = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    
    // Initialize queue
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = WaIoDeviceControl;
    
    status = (*(long (__cdecl *)(void *, WDFDEVICE, WDF_IO_QUEUE_CONFIG *, WDFQUEUE *))WdfFunctions[0x98])(
        WdfDriverGlobals,
        device,
        &queueConfig,
        &queue
    );
    
    if (NT_SUCCESS(status)) {
        status = (*(long (__cdecl *)(void *, WDFDEVICE, GUID *, int))WdfFunctions[0x4d])(
            WdfDriverGlobals,
            device,
            (GUID *)&GUID_DEVICE_ACPI_TIME,
            0
        );
    }
    
    return status;
}

// Prepare hardware routine
NTSTATUS WaEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PVOID extension;
    PDEVICE_OBJECT wdmDevice;
    NTSTATUS status;
    ULONG capabilities;
    ULONG wakeStatus1;
    ULONG wakeStatus2;
    ULONG wakeFlags;
    BOOLEAN hasWakeStatus;
    
    extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    wakeFlags = 0;
    
    // Acquire ACPI interface
    status = (*(long (__cdecl *)(void *, WDFDEVICE, GUID *, PVOID, ULONG, int, int))WdfFunctions[0x83])(
        WdfDriverGlobals,
        Device,
        (GUID *)&GUID_ACPI_INTERFACE_STANDARD,
        extension,
        0x2c,
        1,
        0
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Get WDM device object
    wdmDevice = (PDEVICE_OBJECT)(*(long (__cdecl *)(void *, WDFDEVICE))WdfFunctions[0x21])(WdfDriverGlobals, Device);
    
    // Register ACPI notify callback
    status = (*(long (__cdecl *)(PDEVICE_OBJECT, PVOID, WDFDEVICE))((PVOID*)extension)[9])(wdmDevice, WaAcpiNotifyCallback, Device);
    
    if (!NT_SUCCESS(status) && ((PVOID*)extension)[3]) {
        (*(void (__cdecl *)(PVOID))((PVOID*)extension)[3])(((PVOID*)extension)[1]);
    }
    
    // Get capabilities
    status = WaGetCapabilities(Device, &capabilities);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    // Check if wake is supported
    if ((*(PUCHAR)((PUCHAR)extension + 0x30) & 0x10) == 0) {
        return status;
    }
    
    if (g_DeepestWakeSystemState != 6) {
        return status;
    }
    
    hasWakeStatus = FALSE;
    
    status = WaGetWakeStatus(Device, 0, &wakeStatus1);
    if (NT_SUCCESS(status)) {
        hasWakeStatus = TRUE;
        if ((wakeStatus1 & 2) != 0) {
            wakeFlags = 8;
        }
    }
    
    status = WaGetWakeStatus(Device, 0, &wakeStatus2);
    if (!NT_SUCCESS(status)) {
        if (!hasWakeStatus) {
            PoSetFixedWakeSource(wakeFlags);
            return status;
        }
    } else {
        hasWakeStatus = TRUE;
        if ((wakeStatus2 & 2) != 0) {
            wakeFlags |= 0x10;
        }
        WaClearWakeStatus(Device, 0);
        WaClearWakeStatus(Device, 0);
    }
    
    PoSetFixedWakeSource(wakeFlags);
    return status;
}

// Release hardware routine
NTSTATUS WaEvtDeviceReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PVOID extension;
    PDEVICE_OBJECT wdmDevice;
    
    extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    
    if (((PVOID*)extension)[10]) {
        wdmDevice = (PDEVICE_OBJECT)(*(long (__cdecl *)(void *, WDFDEVICE))WdfFunctions[0x21])(WdfDriverGlobals, Device);
        (*(void (__cdecl *)(PDEVICE_OBJECT))((PVOID*)extension)[10])(wdmDevice);
    }
    
    if (((PVOID*)extension)[3]) {
        (*(void (__cdecl *)(PVOID))((PVOID*)extension)[3])(((PVOID*)extension)[1]);
    }
    
    return STATUS_SUCCESS;
}

// Get capabilities
NTSTATUS WaGetCapabilities(
    WDFDEVICE Device,
    PULONG Result
)
{
    NTSTATUS status;
    ULONG evalBuffer[4];
    ULONG inputBuffer;
    ULONG outputBufferSize;
    ULONG outputBuffer[4];
    
    evalBuffer[0] = 0x42696541;
    evalBuffer[1] = 0x5043475f;
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    inputBuffer = 1;
    outputBufferSize = 0x14;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        &inputBuffer,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_BUFFER_TOO_SMALL) {
            *Result = 0xff;
            status = STATUS_SUCCESS;
        }
        return status;
    }
    
    if (outputBuffer[0] != 0x14 || outputBuffer[3] != 1 || (SHORT)outputBuffer[1] != 0) {
        return STATUS_UNSUCCESSFUL;
    }
    
    *Result = outputBuffer[2];
    return STATUS_SUCCESS;
}

// Get wake status
NTSTATUS WaGetWakeStatus(
    WDFDEVICE Device,
    ULONG Param2,
    PULONG Result
)
{
    NTSTATUS status;
    ULONG evalBuffer[4];
    ULONG inputBuffer;
    ULONG outputBufferSize;
    ULONG outputBuffer[4];
    
    evalBuffer[0] = 0x49696541;
    evalBuffer[1] = 0x5357475f;
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    inputBuffer = 1;
    outputBufferSize = 0x14;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        &inputBuffer,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (NT_SUCCESS(status)) {
        if (outputBuffer[0] == 0x14 && outputBuffer[3] == 1 && (SHORT)outputBuffer[1] == 0) {
            *Result = outputBuffer[2];
            return STATUS_SUCCESS;
        }
        return STATUS_UNSUCCESSFUL;
    }
    
    return status;
}

// Handle ACPI get real time
NTSTATUS WaHandleAcpiGetRealTime(
    WDFDEVICE Device,
    PVOID RealTime,
    PULONG Result
)
{
    PVOID extension;
    NTSTATUS status;
    ULONG evalBuffer[4];
    ULONG inputBuffer;
    ULONG outputBufferSize;
    ULONG outputBuffer[9];
    
    extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    
    if ((*(PUCHAR)((PUCHAR)extension + 0x30) & 4) == 0) {
        return STATUS_NOT_SUPPORTED;
    }
    
    evalBuffer[0] = 0x42696541;
    evalBuffer[1] = 0x5452475f;
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    inputBuffer = 1;
    outputBufferSize = 0x24;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        &inputBuffer,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    if (outputBuffer[0] < 0x14 || outputBuffer[1] != 0x426f6541 || outputBuffer[3] != 1 || outputBuffer[4] != 2 || outputBuffer[5] != 0x10) {
        return STATUS_UNSUCCESSFUL;
    }
    
    // Copy to RealTime structure (assuming ACPI_REAL_TIME layout)
    *(PULONG)((PUCHAR)RealTime + 0) = outputBuffer[6];
    *(PULONG)((PUCHAR)RealTime + 4) = outputBuffer[7];
    *(PULONG)((PUCHAR)RealTime + 8) = outputBuffer[8];
    *(PULONG)((PUCHAR)RealTime + 12) = outputBuffer[9];
    
    if ((*(PUCHAR)((PUCHAR)extension + 0x30) & 8) == 0) {
        *(PUSHORT)((PUCHAR)RealTime + 10) = 500;
    }
    
    return STATUS_SUCCESS;
}

// Handle ACPI set real time
NTSTATUS WaHandleAcpiSetRealTime(
    WDFDEVICE Device,
    PVOID RealTime,
    PULONG Result
)
{
    PVOID extension;
    NTSTATUS status;
    ULONG evalBuffer[4];
    UCHAR dataBuffer[20];
    ULONG inputBufferSize;
    ULONG outputBufferSize;
    ULONG outputBuffer[4];
    
    extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    
    if ((*(PUCHAR)((PUCHAR)extension + 0x30) & 4) == 0) {
        return STATUS_NOT_SUPPORTED;
    }
    
    evalBuffer[0] = 0x43696541;
    evalBuffer[1] = 0x5452535f;
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    // Build data buffer
    *(USHORT*)dataBuffer = 2;
    *(USHORT*)(dataBuffer + 2) = 0x10;
    memcpy(dataBuffer + 4, RealTime, 0x10);
    
    inputBufferSize = 4 + 4 + 0x10 + 4;
    outputBufferSize = 0x14;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        &inputBufferSize,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (NT_SUCCESS(status)) {
        if (outputBuffer[0] != 0x426f6541 || outputBuffer[1] != 1 || (SHORT)outputBuffer[2] != 0 || outputBuffer[3] != 0) {
            return STATUS_UNSUCCESSFUL;
        }
        *Result = 0;
    }
    
    return status;
}

// Handle wake alarm control
NTSTATUS WaHandleWakeAlarmControl(
    WDFDEVICE Device,
    ULONG ControlCode,
    PVOID InputBuffer,
    PVOID OutputBuffer,
    PULONG Result
)
{
    PVOID extension;
    NTSTATUS status;
    ULONG evalBuffer[4];
    ULONG inputBufferData[4];
    ULONG outputBufferSize;
    ULONG outputBuffer[4];
    ULONG dataSize;
    
    extension = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_WA_EXTENSION_TYPE_INFO.UniqueType);
    
    evalBuffer[0] = 0x43696541;
    
    if (ControlCode == 0x298200) {
        evalBuffer[1] = 0x5654535f;
    } else if (ControlCode == 0x298204) {
        evalBuffer[1] = 0x5054535f;
    } else if (ControlCode == 0x29c208) {
        evalBuffer[1] = 0x5649545f;
    } else {
        evalBuffer[1] = 0x5049545f;
    }
    
    evalBuffer[2] = 0;
    evalBuffer[3] = 0;
    
    if (*(PULONG)((PUCHAR)extension + 0x30) == 0) {
        return STATUS_NOT_SUPPORTED;
    }
    
    if (ControlCode == 0x298200 || ControlCode == 0x298204) {
        dataSize = 4;
        inputBufferData[0] = 4;
        inputBufferData[1] = 0;
        inputBufferData[2] = (ULONG)Device;
        inputBufferData[3] = 0;
    }
    
    outputBufferSize = 0x14;
    
    status = (*(long (__cdecl *)(void *, ULONG, int, int, ULONG *, ULONG *, int, ULONG *))WdfFunctions[0xba])(
        WdfDriverGlobals,
        (*(long (__cdecl *)(void *))WdfFunctions[0x2a])(WdfDriverGlobals),
        0,
        0x32c004,
        inputBufferData,
        &outputBufferSize,
        0,
        outputBuffer
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    if (outputBuffer[0] != 0x14 || outputBuffer[1] != 1 || (SHORT)outputBuffer[2] != 0) {
        return STATUS_UNSUCCESSFUL;
    }
    
    if (ControlCode == 0x29c208 || ControlCode == 0x29c20c) {
        *(PULONG)Result = *(PULONG)((PUCHAR)extension + 0x30);
        *((PULONG)Result + 1) = outputBuffer[3];
        *(PULONG)((PUCHAR)OutputBuffer + 0) = 8;
    } else {
        if (outputBuffer[3] != 0) {
            return STATUS_NOT_SUPPORTED;
        }
        *(PULONG)((PUCHAR)OutputBuffer + 0) = 0;
        if (ControlCode == 0x298200) {
            if (*(PULONG)((PUCHAR)extension + 0x30) != 0) {
                *(PULONG)((PUCHAR)extension + 0x2c) |= (*(PULONG)((PUCHAR)extension + 0x30) != 0) ? 8 : 0;
            } else {
                *(PULONG)((PUCHAR)extension + 0x2c) &= ~((*(PULONG)((PUCHAR)extension + 0x30) != 0) ? 8 : 0);
            }
        }
    }
    
    return STATUS_SUCCESS;
}

// IO device control
void WaIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    NTSTATUS status;
    WDFDEVICE device;
    PVOID outputBuffer;
    size_t outputBufferSize;
    PVOID inputBuffer;
    size_t inputBufferSize;
    ULONG result;
    
    status = WdfRequestRetrieveOutputBuffer(Request, 0, &outputBuffer, &outputBufferSize);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }
    
    status = WdfRequestRetrieveInputBuffer(Request, 0, &inputBuffer, &inputBufferSize);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }
    
    device = WdfIoQueueGetDevice(Queue);
    
    switch (IoControlCode) {
        case 0x294210:
            if (outputBufferSize >= 0x10) {
                status = WaHandleAcpiGetRealTime(device, outputBuffer, &result);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
            
        case 0x294218:
            if (outputBufferSize >= sizeof(ULONG)) {
                *(PULONG)outputBuffer = g_DeepestWakeSystemState;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
            
        case 0x298200:
        case 0x298204:
        case 0x298214:
		    if (InputBufferLength >= 0x10) {
				status = WaHandleAcpiSetRealTime(device, inputBuffer, &result);
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
			}
			break;
        case 0x29c208:
        case 0x29c20c:
            if (InputBufferLength >= 8) {
                status = WaHandleWakeAlarmControl(device, IoControlCode, inputBuffer, outputBuffer, &result);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
            
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    WdfRequestComplete(Request, status);
}

// Check if alarm expired
NTSTATUS WaIsAlarmExpired(
    WDFDEVICE Device,
    ULONG Param2,
    PULONG Result
)
{
    UCHAR alarmInfo[8];
    NTSTATUS status;
    
    status = WaHandleWakeAlarmControl(Device, 0x29c208, alarmInfo, alarmInfo, Result);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    if (*(PULONG)(alarmInfo + 0) == 0xffffffff) {
        status = WaHandleWakeAlarmControl(Device, 0x29c20c, alarmInfo, alarmInfo, Result);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        if (*(PULONG)(alarmInfo + 0) == 0xffffffff) {
            Device = (WDFDEVICE)1;
            return STATUS_SUCCESS;
        }
    }
    
    Device = 0;
    return STATUS_SUCCESS;
}

