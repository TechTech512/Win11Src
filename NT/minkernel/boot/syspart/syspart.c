#include "sysboot.h"

// Declare functions this file uses (that are implemented elsewhere)
uchar SiIsWinPEBoot(void);
long SiGetBiosSystemPartition(wchar_t **outPath);
long SiGetEfiSystemDevice(SYSPART_DEVICE_TYPE type, ulong arg, wchar_t **outPath);

FIRMWARE_TYPE SiGetFirmwareType(void) {
    return FirmwareTypeBios;  // Stub return for BIOS
}

long SiGetFirmwareBootDeviceName(SYSPART_DEVICE_TYPE type, wchar_t **outPath) {
    (void)type;

    *outPath = (wchar_t *)malloc(128 * sizeof(wchar_t));
    if (!*outPath) return STATUS_NO_MEMORY;

    wcscpy(*outPath, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
    return STATUS_SUCCESS;
}

long SiTranslateSymbolicLink(wchar_t *path, wchar_t **translated) {
    // Stub: simply return the input path as-is
    size_t len = wcslen(path) + 1;
    *translated = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!*translated) return STATUS_NO_MEMORY;

    wcscpy(*translated, path);
    return STATUS_SUCCESS;
}

long SiGetSystemPartition(FIRMWARE_TYPE fwType, wchar_t **outPath) {
    long status;

    if (SiIsWinPEBoot()) {
        return SiGetFirmwareBootDeviceName(SyspartDeviceTypeUnknown, outPath);
    }

    if (fwType == FirmwareTypeBios) {
        status = SiGetBiosSystemPartition(outPath);
    } else if (fwType == FirmwareTypeUefi) {
        status = SiGetEfiSystemDevice(SyspartDeviceTypeUnknown, 0, outPath);
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    if (status == STATUS_SUCCESS) {
        wchar_t *translated = NULL;
        if (SiTranslateSymbolicLink(*outPath, &translated) == STATUS_SUCCESS) {
            free(*outPath);
            *outPath = translated;
        }
    }

    return status;
}

long SiGetSystemDeviceName(void *buf, wchar_t *output, ulong maxSize, ulong *written) {
    wchar_t *deviceName = NULL;
    long status = SiGetSystemPartition(SiGetFirmwareType(), &deviceName);
    if (status < 0) return status;

    size_t nameLen = wcslen(deviceName);
    if (nameLen + 1 > maxSize) {
        free(deviceName);
        return STATUS_BUFFER_TOO_SMALL;
    }

    wcscpy(output, deviceName);
    if (written) *written = (ulong)nameLen;
    free(deviceName);
    return STATUS_SUCCESS;
}

long SiQuerySystemPartitionInformation(wchar_t *buffer, ulong bufferSize, ulong *outLen) {
    const wchar_t *fakeValue = L"\\Device\\Harddisk0\\Partition1";
    size_t len = wcslen(fakeValue);

    if (bufferSize < len + 1) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    wcscpy(buffer, fakeValue);
    if (outLen) *outLen = (ulong)len;
    return STATUS_SUCCESS;
}

long SyspartGetSystemPartition(wchar_t *buffer, ulong size, ulong *written) {
    long status = SiQuerySystemPartitionInformation(buffer, size, written);
    if (status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL) {
        status = SiGetSystemDeviceName(NULL, buffer, size, written);
    }
    return status;
}

