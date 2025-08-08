#include "sysboot.h"

// Stub or real implementation: resolves \Device path of ESP (UEFI boot partition)
long SiGetEfiSystemDevice(int type, ulong arg, wchar_t **outPath) {
    (void)type;
    (void)arg;

    if (!outPath) return STATUS_INVALID_PARAMETER;

    wchar_t *device = (wchar_t *)MemAlloc(128 * sizeof(wchar_t));
    if (!device) return STATUS_NO_MEMORY;

#ifdef _KERNEL_MODE
    swprintf(device, L"\\Device\\Harddisk0\\Partition1");
#else
    swprintf(device, 128, L"\\Device\\Harddisk0\\Partition1");
#endif

    *outPath = device;
    return STATUS_SUCCESS;
}

// Not originally named in the decompiled code but used elsewhere — this resolves if a device string looks like a disk
uchar SiIsValidDiskDevice(wchar_t *device, wchar_t *query) {
    (void)query;

    if (!device) return 0;

#ifdef _KERNEL_MODE
    return (wcsncmp(device, L"\\Device\\Harddisk", 17) == 0);
#else
    return (wcsncmp(device, L"\\Device\\Harddisk", 17) == 0);
#endif
}

// Kernel stub for ESP logic (placeholder)
long SiGetEspFromFirmware(wchar_t *device, ulong partIndex) {
    (void)device;
    (void)partIndex;
    return STATUS_SUCCESS;
}

// Kernel stub for disambiguating ESP candidates (placeholder)
long SiDisambiguateSystemDevice(ulong *out1, ulong *out2) {
    if (!out1 || !out2) return STATUS_INVALID_PARAMETER;
    *out1 = 0;
    *out2 = 1;
    return STATUS_SUCCESS;
}

