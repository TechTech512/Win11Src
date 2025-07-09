#include "sysboot.h"

// These might still be needed locally if the linker can't resolve them
NTSTATUS NtOpenSymbolicLinkObject(PHANDLE LinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS NtQuerySymbolicLinkObject(HANDLE LinkHandle, PUNICODE_STRING LinkTarget, PULONG ReturnedLength);

#ifndef SYMBOLIC_LINK_QUERY
#define SYMBOLIC_LINK_QUERY 0x0001
#endif

long SiOpenArcNameObject(const wchar_t *arcName, HANDLE *outHandle) {
    UNICODE_STRING arcStr;
    OBJECT_ATTRIBUTES attr;

    RtlInitUnicodeString(&arcStr, arcName);
    InitializeObjectAttributes(&attr, &arcStr, OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS status = NtOpenSymbolicLinkObject(outHandle, SYMBOLIC_LINK_QUERY, &attr);
    return NT_SUCCESS(status) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

uchar SiIsWinPeHardDiskZeroUfdBoot(void) {
    return 0;  // Stubbed for simplification
}

long SiGetBiosSystemDisk(wchar_t **outPath) {
    HANDLE linkHandle;
    wchar_t targetBuffer[512];
    UNICODE_STRING arcName, targetName;
    OBJECT_ATTRIBUTES attr;
    ULONG resultLength = 0;

    *outPath = NULL;

    if (SiIsWinPeHardDiskZeroUfdBoot()) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlInitUnicodeString(&arcName, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
    InitializeObjectAttributes(&attr, &arcName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS status = NtOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &attr);
    if (!NT_SUCCESS(status)) return STATUS_NOT_FOUND;

    targetName.Length = 0;
    targetName.MaximumLength = sizeof(targetBuffer);
    targetName.Buffer = targetBuffer;

    status = NtQuerySymbolicLinkObject(linkHandle, &targetName, &resultLength);
    NtClose(linkHandle);

    if (!NT_SUCCESS(status)) return STATUS_UNSUCCESSFUL;

    targetName.Buffer[targetName.Length / sizeof(wchar_t)] = L'\\0';

    size_t len = wcslen(targetName.Buffer) + 1;
    *outPath = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!*outPath) return STATUS_NO_MEMORY;

    wcscpy(*outPath, targetName.Buffer);
    return STATUS_SUCCESS;
}

long SiGetBiosSystemPartition(wchar_t **outPath) {
    wchar_t *diskName = NULL;
    long status = SiGetBiosSystemDisk(&diskName);
    if (status != STATUS_SUCCESS) return status;

    size_t bufSize = wcslen(diskName) + 20;
    *outPath = (wchar_t *)malloc(bufSize * sizeof(wchar_t));
    if (!*outPath) {
        free(diskName);
        return STATUS_NO_MEMORY;
    }

    swprintf(*outPath, (int)bufSize, L"%s\\Partition0", diskName);
    free(diskName);
    return STATUS_SUCCESS;
}

