#include "sysboot.h"

long SiGetDeviceNumberInformation(wchar_t *devicePath, ulong *outDiskNum, ulong *outPartitionNum) {
    if (!devicePath || !outDiskNum || !outPartitionNum) return STATUS_INVALID_PARAMETER;

#ifdef _KERNEL_MODE
    UNICODE_STRING devName;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK ioStatus;
    HANDLE fileHandle;
    NTSTATUS status;

    RtlInitUnicodeString(&devName, devicePath);
    InitializeObjectAttributes(&attr, &devName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateFile(&fileHandle, GENERIC_READ, &attr, &ioStatus, NULL,
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL, 0);
    if (!NT_SUCCESS(status)) return STATUS_UNSUCCESSFUL;

    FILE_FS_DEVICE_INFORMATION deviceInfo;
    status = ZwQueryVolumeInformationFile(fileHandle, &ioStatus, &deviceInfo, sizeof(deviceInfo), FileFsDeviceInformation);

    ZwClose(fileHandle);

    if (!NT_SUCCESS(status)) return STATUS_UNSUCCESSFUL;

    *outDiskNum = deviceInfo.DeviceType;
    *outPartitionNum = deviceInfo.Characteristics; // Simplified
    return STATUS_SUCCESS;

#else
    // User-mode stub
    *outDiskNum = 0;
    *outPartitionNum = 1;
    return STATUS_SUCCESS;
#endif
}

long SiGetDiskPartitionInformation(wchar_t *path, void *info) {
    (void)path;
    (void)info;
    return STATUS_SUCCESS;  // Stubbed for both modes
}

long SiGetDriveLayoutInformation(wchar_t *path, void **layout) {
    if (!path || !layout) return STATUS_INVALID_PARAMETER;

    *layout = MemAlloc(sizeof(int)); // dummy structure
    if (!*layout) return STATUS_NO_MEMORY;

    return STATUS_SUCCESS;
}

long SiValidateSystemPartition(wchar_t *device, void *info) {
    (void)device;
    (void)info;
    return STATUS_SUCCESS;  // Stubbed
}

long SiIssueSynchronousIoctl(wchar_t *device, ulong ctrlCode, void *inBuf, ulong inLen, void *outBuf, ulong outLen) {
#ifdef _KERNEL_MODE
    (void)device;
    (void)ctrlCode;
    (void)inBuf;
    (void)inLen;
    (void)outBuf;
    (void)outLen;
    return STATUS_NOT_IMPLEMENTED;
#else
    HANDLE h = CreateFileW(device, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return STATUS_UNSUCCESSFUL;

    DWORD bytesReturned;
    BOOL result = DeviceIoControl(h, ctrlCode, inBuf, inLen, outBuf, outLen, &bytesReturned, NULL);
    CloseHandle(h);

    return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
#endif
}

