#pragma warning (disable:4133)

#include <stdlib.h>
#include <wdm.h>
#include <wdf.h>

// Function declarations
long __cdecl S3CapEvtDeviceAdd(
    struct WDFDRIVER__ *Driver,
    struct WDFDEVICE_INIT *DeviceInit
);

// Driver entry point
long DriverEntry(
    struct PDRIVER_OBJECT *DriverObject,
    struct UNICODE_STRING *RegistryPath
)
{
    long status = 0;
    struct {
        ULONG Size;
        void *EvtDeviceAdd;
        ULONG Unknown1;
        ULONG Unknown2;
        ULONG Unknown3;
    } driverConfig;
    
    driverConfig.Size = 0x14;
    driverConfig.EvtDeviceAdd = S3CapEvtDeviceAdd;
    driverConfig.Unknown1 = 0;
    driverConfig.Unknown2 = 0;
    driverConfig.Unknown3 = 0x57335376;
    
    ((long (__cdecl *)(void *, void *, UNICODE_STRING*, void *, void *, void *))WdfFunctions[0x74])(
        WdfDriverGlobals,
        DriverObject,
        RegistryPath,
        0,
        &driverConfig,
        0
    );
    
    return status;
}

// Device add callback
long __cdecl S3CapEvtDeviceAdd(
    struct WDFDRIVER__ *Driver,
    struct WDFDEVICE_INIT *DeviceInit
)
{
    long status = 0;
    unsigned char attributes[4];
    
    ((long (__cdecl *)(void *, void *, int, void *))WdfFunctions[0x4b])(
        WdfDriverGlobals,
        &DeviceInit,
        0,
        attributes
    );
    
    return status;
}

