#include "sysboot.h"

long SiGetDeviceNumberInformation(wchar_t *devicePath, ulong *outDiskNum, ulong *outPartitionNum) {
    (void)devicePath;
    *outDiskNum = 0;
    *outPartitionNum = 1;
    return STATUS_SUCCESS;
}

long SiGetDiskPartitionInformation(wchar_t *path, PARTITION_INFORMATION_EX *info) {
    (void)path;
    (void)info;
    return STATUS_SUCCESS;
}

long SiGetDriveLayoutInformation(wchar_t *path, DRIVE_LAYOUT_INFORMATION_EX **layout) {
    (void)path;
    *layout = (DRIVE_LAYOUT_INFORMATION_EX *)malloc(sizeof(DRIVE_LAYOUT_INFORMATION_EX));
    if (!*layout) return STATUS_NO_MEMORY;
    (*layout)->dummy = 42;
    return STATUS_SUCCESS;
}

long SiGetFirmwareBootDeviceNameFromRegistry(wchar_t **outName) {
    *outName = (wchar_t *)malloc(128 * sizeof(wchar_t));
    if (!*outName) return STATUS_NO_MEMORY;
    wcscpy(*outName, L"multi(0)disk(0)rdisk(0)partition(1)");
    return STATUS_SUCCESS;
}

long SiGetFirmwareBootDeviceName(SYSPART_DEVICE_TYPE type, wchar_t **outPath) {
    (void)type;
    wchar_t *regValue = NULL;
    long status = SiGetFirmwareBootDeviceNameFromRegistry(&regValue);
    if (status != STATUS_SUCCESS) return status;

    *outPath = (wchar_t *)malloc(256 * sizeof(wchar_t));
    if (!*outPath) {
        free(regValue);
        return STATUS_NO_MEMORY;
    }

    swprintf(*outPath, 256, L"\\ArcName\\%s", regValue);
    free(regValue);
    return STATUS_SUCCESS;
}

long SiValidateSystemPartition(wchar_t *device, PARTITION_INFORMATION_EX *info) {
    (void)device;
    (void)info;
    return STATUS_SUCCESS;
}

long SiTranslateSymbolicLink(wchar_t *path, wchar_t **translated) {
    return SiGetFirmwareBootDeviceNameFromRegistry(translated);  // simulated
}

long SiIssueSynchronousIoctl(wchar_t *device, ulong ctrlCode, void *inBuf, ulong inLen, void *outBuf, ulong outLen) {
    (void)device;
    (void)ctrlCode;
    (void)inBuf;
    (void)inLen;
    (void)outBuf;
    (void)outLen;
    return STATUS_SUCCESS;
}

uchar SiIsWinPEBoot(void) {
    return 0;
}

