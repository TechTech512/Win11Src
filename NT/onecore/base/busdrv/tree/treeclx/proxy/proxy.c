#pragma warning (disable:4028)

#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>

// External global variables
extern void (*TrFunctions[])(void);
extern void *TrBindContext;

// Forward declarations
NTSTATUS TreeProxyEvtAddDevice(WDFDRIVER *Driver, PWDFDEVICE_INIT DeviceInit);
void DriverReinitialize(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count);
void DriverUnload(WDFDRIVER Driver);

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
        void *EvtDriverUnload;
        ULONG Unknown1;
        ULONG Unknown2;
    } driverConfig;
    ULONG someFlag;
    
    someFlag = 0;
    driverConfig.Size = 0x14;
    driverConfig.EvtDeviceAdd = TreeProxyEvtAddDevice;
    driverConfig.EvtDriverUnload = DriverUnload;
    driverConfig.Unknown1 = 0;
    driverConfig.Unknown2 = 0;
    
    status = (*(long (__cdecl *)(void *, PDRIVER_OBJECT, PUNICODE_STRING, int, void *, ULONG *))WdfFunctions[0x74])(
        WdfDriverGlobals,
        DriverObject,
        RegistryPath,
        0,
        &driverConfig,
        &someFlag
    );
    
    if (NT_SUCCESS(status)) {
        IoRegisterDriverReinitialization(DriverObject, DriverReinitialize, NULL);
    }
    
    return status;
}

// Driver reinitialize routine
void DriverReinitialize(
    PDRIVER_OBJECT DriverObject,
    PVOID Context,
    ULONG Count
)
{
    return;
}

// Driver unload routine
void DriverUnload(
    WDFDRIVER Driver
)
{
    return;
}

// Device add routine
NTSTATUS TreeProxyEvtAddDevice(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE device;
    PWCHAR deviceId;
    PWCHAR p;
    PWCHAR backslashPos;
    USHORT guidStringLength;
    UNICODE_STRING guidString;
    GUID guid;
    ULONG someValue;
    
    status = (*(long (__cdecl *)(void *, PWDFDEVICE_INIT, int, int, int, ULONG *))WdfFunctions[0x7f])(
        WdfDriverGlobals,
        DeviceInit,
        1,
        0x200,
        0,
        &someValue
    );
    
    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    deviceId = (PWCHAR)(*(void *(__cdecl *)(void *, ULONG))WdfFunctions[0xc2])(WdfDriverGlobals, someValue);
    
    if (deviceId[((someValue & 0xfffffffe) - 2) / 2] == 0) {
        for (p = deviceId; *p != 0; p++) {
            if (*p == L'\\') {
                break;
            }
        }
        
        if (*p == L'\\') {
            backslashPos = p + 1;
            for (p = backslashPos; *p != 0; p++) {
            }
            guidStringLength = (USHORT)((ULONG_PTR)p - (ULONG_PTR)backslashPos);
            guidString.Buffer = backslashPos;
            guidString.Length = guidStringLength;
            guidString.MaximumLength = guidStringLength;
            
            status = RtlGUIDFromString(&guidString, &guid);
            if (NT_SUCCESS(status)) {
                status = (*(long (__cdecl *)(void))TrFunctions[1])();
                return status;
            }
        }
    }
    
    return STATUS_OBJECT_PATH_NOT_FOUND;
}

