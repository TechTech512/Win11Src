#include "acpi.h"

// External ETW variables
extern ULONG_PTR AdEtwHandle;
extern BOOLEAN AdEtwRegistered;

// Event descriptor for AD_ETW_PUR from hex dump
// Id=1, Version=0, Channel=0, Level=4, Opcode=0, Task=1, Keyword=0
static const EVENT_DESCRIPTOR AD_ETW_PUR = {
    0x01,       // Id
    0x00,       // Version
    0x00,       // Channel
    0x04,       // Level
    0x00,       // Opcode
    0x01,       // Task
    0x0000000000000000ULL  // Keyword
};

extern const GUID AD_ETW_OST;

// External functions
extern NTSTATUS AdAcpiAcquireInterfaces(PAD_EXTENSION Extension);
extern NTSTATUS AdAcpiEval_PUR(PAD_EXTENSION Extension, PULONG Result);
extern NTSTATUS AdAcpiEval_OST(PAD_EXTENSION Extension, ULONG Param2, ULONG Param3);
extern void AdEtwOSTEvent(ULONG Param1, ULONG Param2, ULONG Param3);
extern void AdInsertWorkItem(PAD_EXTENSION Extension, AD_WORK_TYPE WorkType);
extern NTSTATUS AdRegisterEtw(void);

// GUID for thermal cooling interface from hex dump
// {ECBE47A8-C498-4BB9-BD70-E867E0940D22}
EXTERN_C const GUID GUID_THERMAL_COOLING_INTERFACE =
    { 0xecbe47a8, 0xc498, 0x4bb9, { 0xbd, 0x70, 0xe8, 0x67, 0xe0, 0x94, 0x0d, 0x22 } };

// GUID for device interface thermal cooling from hex dump
// {DBE4373D-3C81-40CB-ACE4-E0E5D05F0C9F}
EXTERN_C const GUID GUID_DEVINTERFACE_THERMAL_COOLING =
    { 0xdbe4373d, 0x3c81, 0x40cb, { 0xac, 0xe4, 0xe0, 0xe5, 0xd0, 0x5f, 0x0c, 0x9f } };

typedef void (__cdecl *PFN_WDF_CONTEXT_TYPE_INFO_GET_UNIQUE_CONTEXT)(struct _WDF_OBJECT_CONTEXT_TYPE_INFO *);

// Forward declarations
NTSTATUS AdEvtDeviceAdd(WDFDRIVER *Driver, PWDFDEVICE_INIT DeviceInit);
NTSTATUS AdEvtDevicePrepareHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesRaw, WDFCMRESLIST ResourcesTranslated);
NTSTATUS AdEvtDeviceReleaseHardware(WDFDEVICE Device, WDFCMRESLIST ResourcesTranslated);
void AdEvtDriverUnload(WDFDRIVER Driver);
void AdEventWorker(WDFWORKITEM WorkItem);
void AdThermalNotification(PVOID Context, ULONG Temperature);

// External context type info
WDF_OBJECT_CONTEXT_TYPE_INFO _WDF_AD_EXTENSION_TYPE_INFO = {
    sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO),
    "AD_EXTENSION",
    sizeof(AD_EXTENSION),
    &_WDF_AD_EXTENSION_TYPE_INFO,
    NULL
};

// Event worker routine
void AdEventWorker(
    WDFWORKITEM WorkItem
)
{
    PAD_EXTENSION extension;
    ULONG workItemState;
    ULONG purResult;
    NTSTATUS status;
    ULONG powerInfo;
    ULONG processorCount;
    ULONG targetFrequency;
    ULONG currentFrequency;
    ULONG currentPercentage;
    ULONG newPercentage;
    BOOLEAN shouldUpdate;
    ULONG powerInfoValue;
    int frequencyValue;
    int processorIndex;
    int percentageValue;
    int currentFrequencyValue;
    int currentPercentageValue;
    int thermalCapValue;
    int tempValue;
    unsigned int flags;
    int *eventData;
    
    (*(void (__cdecl *)(void *, WDFWORKITEM))WdfFunctions[0x17d])(WdfDriverGlobals, WorkItem);
    
    extension = (PAD_EXTENSION)_WDF_AD_EXTENSION_TYPE_INFO.UniqueType;
    
    do {
        if (extension->DeviceStopped != 0) break;
        
        KeAcquireSpinLockAtDpcLevel(&extension->DeviceStopLock);
        workItemState = extension->WorkItemState.State;
        extension->WorkItemState.State = 4;
        KeReleaseSpinLockFromDpcLevel(&extension->DeviceStopLock);
        
        processorIndex = extension->AcpiPURNumCpus;
        powerInfoValue = extension->ThermalCap;
        frequencyValue = extension->ThermalIdledCpus;
        flags = (workItemState >> 1) & 1;
        
        if (flags != 0) {
            targetFrequency = 100;
            currentFrequencyValue = extension->ThermalIdledCpus - (extension->AcpiValidNumCpus * extension->ThermalIdledCpus / 100);
            if (currentFrequencyValue != frequencyValue) {
                flags = 1;
            }
        } else {
            targetFrequency = frequencyValue;
        }
        
        percentageValue = processorIndex;
        currentPercentageValue = processorIndex;
        
        if ((workItemState & 1) == 0) {
            shouldUpdate = (flags >> 24) & 1;
        } else {
            AdAcpiEval_PUR(extension, &purResult);
            processorIndex = percentageValue;
            currentPercentageValue = percentageValue;
            currentFrequencyValue = percentageValue;
            
            if (AdEtwRegistered != FALSE) {
                if (EtwEventEnabled(AdEtwHandle, (PCEVENT_DESCRIPTOR)&AD_ETW_PUR)) {
                    eventData = &currentFrequencyValue;
                    EtwWrite(AdEtwHandle, (PCEVENT_DESCRIPTOR)&AD_ETW_PUR, NULL, 1, (PEVENT_DATA_DESCRIPTOR)&eventData);
                }
            }
            
            if (processorIndex == extension->AcpiPURNumCpus) {
                shouldUpdate = (flags >> 24) & 1;
            } else {
                shouldUpdate = 1;
                flags = 1;
            }
        }
        
        if (shouldUpdate == 0) {
            shouldUpdate = 0;
            thermalCapValue = 0;
        } else {
            frequencyValue = targetFrequency;
            currentFrequencyValue = processorIndex;
            status = ZwPowerInformation(0x38, &currentFrequencyValue, 8, &powerInfoValue, 4);
            shouldUpdate = (flags >> 24) & 1;
        }
        
        if ((flags != 0) && (shouldUpdate != 0) && (NT_SUCCESS(status))) {
            extension->ThermalIdledCpus = frequencyValue;
        }
        
        if ((workItemState & 1) != 0) {
            if (NT_SUCCESS(status) && (shouldUpdate != 0)) {
                extension->AcpiPURNumCpus = processorIndex;
                extension->ThermalCap = powerInfoValue;
            }
            powerInfo = powerInfoValue;
            AdEtwOSTEvent(powerInfoValue, (ULONG)extension, (ULONG)WdfDriverGlobals);
            AdAcpiEval_OST(extension, powerInfo, (ULONG)extension);
        }
        
        KeAcquireSpinLockAtDpcLevel(&extension->DeviceStopLock);
        tempValue = extension->WorkItemState.State;
        if (tempValue == 4) {
            extension->WorkItemState.State = 0;
            tempValue = 4;
        }
        KeReleaseSpinLockFromDpcLevel(&extension->DeviceStopLock);
    } while (tempValue != 4);
}

// Device add routine
NTSTATUS AdEvtDeviceAdd(
    WDFDRIVER *Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    PAD_EXTENSION extension;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    ULONG processorCount;
    PVOID context;
    
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = AdEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = AdEvtDeviceReleaseHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);
    
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, AD_EXTENSION);
    
    status = (*(long (__cdecl *)(void *, PWDFDEVICE_INIT *, WDF_OBJECT_ATTRIBUTES *, WDFDEVICE *))WdfFunctions[0x4b])(
        WdfDriverGlobals,
        &DeviceInit,
        &deviceAttributes,
        &device
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    context = (*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_AD_EXTENSION_TYPE_INFO.UniqueType);
    extension = (PAD_EXTENSION)context;
    
    extension->WdfSelf = (WDFDEVICE*)device;
    extension->WdmSelf = WdfDeviceWdmGetDeviceObject(device);
    extension->WdmPdo = WdfDeviceWdmGetPhysicalDevice(device);
    extension->AcpiPURNumCpus = 0;
    extension->AcpiValidNumCpus = 1;
    extension->ThermalCap = 100;
    extension->ProcessorCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
	
	WDF_OBJECT_CONTEXT_TYPE_INFO InterfaceDereference1;
	RtlZeroMemory(&InterfaceDereference1, sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO));
	InterfaceDereference1.EvtDriverGetUniqueContextType = (PFN_GET_UNIQUE_CONTEXT_TYPE)WdfDeviceInterfaceDereferenceNoOp;
	PVOID InterfaceDereference2 = (WDF_DRIVER_GLOBALS*)WdfDeviceInterfaceDereferenceNoOp;
	
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
	(void (__stdcall *)(PVOID,ULONG))queueConfig.EvtIoDeviceControl = AdThermalNotification;
    
    status = (*(long (__cdecl *)(void *, WDFDEVICE, GUID *, WDF_OBJECT_CONTEXT_TYPE_INFO, WDF_IO_QUEUE_CONFIG *, PVOID))WdfFunctions[0xe4])(
        WdfDriverGlobals,
        device,
        (GUID *)&GUID_THERMAL_COOLING_INTERFACE,
        InterfaceDereference1,
        InterfaceDereference2,
        &queueConfig
    );
    
    if (NT_SUCCESS(status)) {
        status = (*(long (__cdecl *)(void *, WDFDEVICE, GUID *, ULONG))WdfFunctions[0x4d])(
            WdfDriverGlobals,
            device,
            (GUID *)&GUID_DEVINTERFACE_THERMAL_COOLING,
            0
        );
    }
    
    return status;
}

// Prepare hardware routine
NTSTATUS AdEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PAD_EXTENSION extension;
    NTSTATUS status;
    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_OBJECT_ATTRIBUTES workItemAttributes;
    
    extension = (PAD_EXTENSION)(*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_AD_EXTENSION_TYPE_INFO.UniqueType);
    
    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, AdEventWorker);
    WDF_OBJECT_ATTRIBUTES_INIT(&workItemAttributes);
    workItemAttributes.ParentObject = Device;
    
    status = WdfWorkItemCreate(&workItemConfig, &workItemAttributes, (WDFWORKITEM*)&extension->WorkItem);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    status = AdAcpiAcquireInterfaces(extension);
    if (!NT_SUCCESS(status)) {
        if (extension->WorkItem) {
            WdfObjectDelete(extension->WorkItem);
            extension->WorkItem = NULL;
        }
        return status;
    }
    
    ExAcquirePushLockExclusiveEx(&extension->DeviceStopLock, 0);
    extension->DeviceStopped = 0;
    ExReleasePushLockExclusiveEx(&extension->DeviceStopLock, 0);
    
    AdInsertWorkItem(extension, AdWorkInitialize);
    
    return status;
}

// Release hardware routine
NTSTATUS AdEvtDeviceReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PAD_EXTENSION extension;
    
    extension = (PAD_EXTENSION)(*(void *(__cdecl *)(void *))WdfFunctions[0xca])((void *)_WDF_AD_EXTENSION_TYPE_INFO.UniqueType);
    
    ExAcquirePushLockExclusiveEx(&extension->DeviceStopLock, 0);
    extension->DeviceStopped = 1;
    ExReleasePushLockExclusiveEx(&extension->DeviceStopLock, 0);
    
    if (extension->AcpiInterfaces.UnregisterForDeviceNotifications) {
        ((void (__cdecl *)(PVOID))extension->AcpiInterfaces.UnregisterForDeviceNotifications)(extension->AcpiInterfaces.Context);
    }
    
    if (extension->AcpiInterfaces.InterfaceDereference) {
        extension->AcpiInterfaces.InterfaceDereference(extension->AcpiInterfaces.Context);
    }
    
    if (extension->WorkItem) {
        WdfWorkItemFlush((WDFWORKITEM)extension->WorkItem);
        WdfObjectDelete(extension->WorkItem);
        extension->WorkItem = NULL;
    }
    
    return STATUS_SUCCESS;
}

// Driver unload routine
void AdEvtDriverUnload(
    WDFDRIVER Driver
)
{
    if (AdEtwRegistered != FALSE) {
        EtwUnregister(AdEtwHandle);
        AdEtwRegistered = FALSE;
    }
}

// Insert work item routine
void AdInsertWorkItem(
    PAD_EXTENSION Extension,
    AD_WORK_TYPE WorkType
)
{
    ULONG newState;
    ULONG oldState = 0;
    ULONG currentState;
    BOOLEAN success;
    
    ExAcquirePushLockSharedEx(&Extension->DeviceStopLock, 0);
    
    newState = 0;
    if (Extension->DeviceStopped == 0) {
        if (WorkType == AdWorkInitialize) {
            newState = 3;
        } else if (WorkType == AdWorkPur) {
            newState = 1;
        } else if (WorkType == AdWorkThermal) {
            newState = 2;
        }
        
        do {
            KeAcquireSpinLockAtDpcLevel(&Extension->DeviceStopLock);
            currentState = Extension->WorkItemState.State;
            success = (oldState == currentState);
            if (success) {
                Extension->WorkItemState.State = currentState | newState | 4;
                currentState = oldState;
            }
            KeReleaseSpinLockFromDpcLevel(&Extension->DeviceStopLock);
            oldState = currentState;
        } while (!success);
        
        if ((currentState & 4) == 0) {
            WdfWorkItemEnqueue((WDFWORKITEM)Extension->WorkItem);
        }
    }
    
    ExReleasePushLockSharedEx(&Extension->DeviceStopLock, 0);
}

// Thermal notification callback
void AdThermalNotification(
    PVOID Context,
    ULONG Temperature
)
{
    PAD_EXTENSION extension = (PAD_EXTENSION)Context;
	AD_WORK_TYPE AdWorkThermal = 2;
    
    if (Temperature != extension->ThermalCap) {
        extension->ThermalCap = Temperature;
        AdInsertWorkItem(extension, AdWorkThermal);
    }
}

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    struct {
        ULONG Size;
        void *EvtDeviceAdd;
        void *EvtDriverUnload;
        ULONG Unknown1;
        ULONG Unknown2;
    } driverConfig;
    
    driverConfig.Size = 0x14;
    driverConfig.EvtDeviceAdd = AdEvtDeviceAdd;
    driverConfig.EvtDriverUnload = AdEvtDriverUnload;
    driverConfig.Unknown1 = 0;
    driverConfig.Unknown2 = 0;
    
    status = (*(long (__cdecl *)(void *, PDRIVER_OBJECT, PUNICODE_STRING, int, void *, int))WdfFunctions[0x74])(
        WdfDriverGlobals,
        DriverObject,
        RegistryPath,
        0,
        &driverConfig,
        0
    );
    
    if (NT_SUCCESS(status)) {
        status = AdRegisterEtw();
    }
    
    return status;
}

