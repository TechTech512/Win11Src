#include "sysboot.h"

long SiGetBiosSystemDisk(wchar_t **outPath) {
    if (!outPath) return STATUS_INVALID_PARAMETER;

    *outPath = NULL;

#ifdef _KERNEL_MODE
    UNICODE_STRING arcName, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE linkHandle;
    NTSTATUS status;
    wchar_t *buffer = NULL;

    RtlInitUnicodeString(&arcName, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
    InitializeObjectAttributes(&attr, &arcName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &attr);
    if (!NT_SUCCESS(status)) return STATUS_NOT_FOUND;

    buffer = (wchar_t *)MemAlloc(512 * sizeof(wchar_t));
    if (!buffer) {
        ZwClose(linkHandle);
        return STATUS_NO_MEMORY;
    }

    target.Length = 0;
    target.MaximumLength = 512 * sizeof(wchar_t);
    target.Buffer = buffer;

    status = ZwQuerySymbolicLinkObject(linkHandle, &target, NULL);
    ZwClose(linkHandle);

    if (!NT_SUCCESS(status)) {
        MemFree(buffer);
        return STATUS_UNSUCCESSFUL;
    }

    buffer[target.Length / sizeof(wchar_t)] = L'\0';
    *outPath = buffer;
    return STATUS_SUCCESS;

#else
    UNICODE_STRING arcName, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE linkHandle;
    NTSTATUS status;
    wchar_t *buffer = NULL;

    RtlInitUnicodeString(&arcName, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
    InitializeObjectAttributes(&attr, &arcName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &attr);
    if (!NT_SUCCESS(status)) return STATUS_NOT_FOUND;

    buffer = (wchar_t *)MemAlloc(512 * sizeof(wchar_t));
    if (!buffer) {
        NtClose(linkHandle);
        return STATUS_NO_MEMORY;
    }

    target.Length = 0;
    target.MaximumLength = 512 * sizeof(wchar_t);
    target.Buffer = buffer;

    status = NtQuerySymbolicLinkObject(linkHandle, &target, NULL);
    NtClose(linkHandle);

    if (!NT_SUCCESS(status)) {
        MemFree(buffer);
        return STATUS_UNSUCCESSFUL;
    }

    buffer[target.Length / sizeof(wchar_t)] = L'\0';
    *outPath = buffer;
    return STATUS_SUCCESS;
#endif
}

long SiGetBiosSystemPartition(wchar_t **outPath) {
    if (!outPath) return STATUS_INVALID_PARAMETER;

    wchar_t *diskPath = NULL;
    long status = SiGetBiosSystemDisk(&diskPath);
    if (status != STATUS_SUCCESS) return status;

    size_t len = wcslen(diskPath) + wcslen(L"\\Partition0") + 1;
    *outPath = (wchar_t *)MemAlloc(len * sizeof(wchar_t));
    if (!*outPath) {
        MemFree(diskPath);
        return STATUS_NO_MEMORY;
    }

#ifdef _KERNEL_MODE
    swprintf(*outPath, L"%ws\\Partition0", diskPath);
#else
    swprintf(*outPath, L"%s\\Partition0", diskPath);
#endif

    MemFree(diskPath);
    return STATUS_SUCCESS;
}

