#include "sysboot.h"

#define SystemFirmwareTableInformation 0x4E

int SiGetFirmwareType(void) {
#ifdef _KERNEL_MODE
    // In kernel mode, assume BIOS unless overridden
    return 1; // 1 = BIOS, 2 = UEFI (match FirmwareType enum)
#else
    ULONG type = 0;
    if (NtQuerySystemInformation(SystemFirmwareTableInformation, NULL, 0, &type) == 0) {
        return 2; // assume UEFI if firmware table exists
    }
    return 1; // default to BIOS
#endif
}

long SiTranslateSymbolicLink(wchar_t *path, wchar_t **translated) {
    if (!path || !translated) return STATUS_INVALID_PARAMETER;

#ifdef _KERNEL_MODE
    UNICODE_STRING linkName, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE linkHandle;
    NTSTATUS status;

    RtlInitUnicodeString(&linkName, path);
    InitializeObjectAttributes(&attr, &linkName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &attr);
    if (!NT_SUCCESS(status)) return STATUS_NOT_FOUND;

    target.Buffer = (wchar_t *)MemAlloc(512 * sizeof(wchar_t));
    if (!target.Buffer) {
        ZwClose(linkHandle);
        return STATUS_NO_MEMORY;
    }

    target.Length = 0;
    target.MaximumLength = 512 * sizeof(wchar_t);

    status = ZwQuerySymbolicLinkObject(linkHandle, &target, NULL);
    ZwClose(linkHandle);

    if (!NT_SUCCESS(status)) {
        MemFree(target.Buffer);
        return STATUS_UNSUCCESSFUL;
    }

    target.Buffer[target.Length / sizeof(wchar_t)] = L'\0';
    *translated = target.Buffer;
    return STATUS_SUCCESS;

#else
    UNICODE_STRING linkName, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE linkHandle;
    NTSTATUS status;

    RtlInitUnicodeString(&linkName, path);
    InitializeObjectAttributes(&attr, &linkName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenSymbolicLinkObject(&linkHandle, SYMBOLIC_LINK_QUERY, &attr);
    if (!NT_SUCCESS(status)) return STATUS_NOT_FOUND;

    target.Buffer = (wchar_t *)MemAlloc(512 * sizeof(wchar_t));
    if (!target.Buffer) {
        NtClose(linkHandle);
        return STATUS_NO_MEMORY;
    }

    target.Length = 0;
    target.MaximumLength = 512 * sizeof(wchar_t);

    status = NtQuerySymbolicLinkObject(linkHandle, &target, NULL);
    NtClose(linkHandle);

    if (!NT_SUCCESS(status)) {
        MemFree(target.Buffer);
        return STATUS_UNSUCCESSFUL;
    }

    target.Buffer[target.Length / sizeof(wchar_t)] = L'\0';
    *translated = target.Buffer;
    return STATUS_SUCCESS;
#endif
}

long SiGetFirmwareBootDeviceName(int type, wchar_t **outPath) {
    (void)type;

    if (!outPath) return STATUS_INVALID_PARAMETER;

    wchar_t *buf = (wchar_t *)MemAlloc(128 * sizeof(wchar_t));
    if (!buf) return STATUS_NO_MEMORY;

#ifdef _KERNEL_MODE
    swprintf(buf, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
#else
    swprintf_s(buf, 128, L"\\ArcName\\multi(0)disk(0)rdisk(0)partition(1)");
#endif

    *outPath = buf;
    return STATUS_SUCCESS;
}

uchar SiIsWinPEBoot(void) {
#ifdef _KERNEL_MODE
    return 0; // stubbed for kernel mode (real logic may query registry/memory)
#else
    HKEY hKey;
    DWORD value = 0, size = sizeof(value);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\MiniNT", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"SystemPartition", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return 1;
        }
        RegCloseKey(hKey);
    }
    return 0;
#endif
}

long SiGetSystemPartition(int fwType, wchar_t **outPath) {
    if (!outPath) return STATUS_INVALID_PARAMETER;

    if (SiIsWinPEBoot()) {
        return SiGetFirmwareBootDeviceName(0, outPath);
    }

    long status;
    if (fwType == 1) { // BIOS
        status = SiGetBiosSystemPartition(outPath);
    } else if (fwType == 2) { // UEFI
        status = SiGetEfiSystemDevice(0, 0, outPath);
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    if (status == STATUS_SUCCESS) {
        wchar_t *translated = NULL;
        if (SiTranslateSymbolicLink(*outPath, &translated) == STATUS_SUCCESS) {
            MemFree(*outPath);
            *outPath = translated;
        }
    }

    return status;
}

long SiGetSystemDeviceName(void *buf, wchar_t *output, ulong maxSize, ulong *written) {
    (void)buf;

    wchar_t *name = NULL;
    long status = SiGetSystemPartition(SiGetFirmwareType(), &name);
    if (status != STATUS_SUCCESS) return status;

    size_t len = wcslen(name);
    if (len + 1 > maxSize) {
        MemFree(name);
        return STATUS_BUFFER_TOO_SMALL;
    }

    wcscpy(output, name);
    if (written) *written = (ulong)len;

    MemFree(name);
    return STATUS_SUCCESS;
}

