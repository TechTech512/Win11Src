#include "sysboot.h"

long SiDisambiguateSystemDevice(ulong *param1, ulong *param2) {
    // Stub logic for disambiguating a device
    *param1 = 0x1234;
    *param2 = 0x5678;
    return STATUS_SUCCESS;
}

long SiGetEspFromFirmware(wchar_t *device, ulong partIndex) {
    // Stub logic to simulate reading ESP partition from firmware
    (void)device;
    (void)partIndex;
    return STATUS_SUCCESS;
}

uchar SiIsValidDiskDevice(wchar_t *device, wchar_t *query) {
    (void)query;
    // Basic check for stub
    if (device && wcsncmp(device, L"Harddisk", 8) == 0) {
        return 1;
    }
    return 0;
}

long SiGetEfiSystemDevice(SYSPART_DEVICE_TYPE type, ulong arg, wchar_t **outPath) {
    (void)type;
    (void)arg;

    *outPath = (wchar_t *)malloc(128 * sizeof(wchar_t));
    if (!*outPath) return STATUS_NO_MEMORY;

    swprintf(*outPath, 128, L"\\Device\\Harddisk0\\Partition1");
    return STATUS_SUCCESS;
}

