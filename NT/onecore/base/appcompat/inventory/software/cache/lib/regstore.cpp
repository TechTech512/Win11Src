#include <windows.h>
#include <string.h>
#include <strsafe.h>

// Forward declarations for external functions
extern "C" {
    ULONG AmiUtilityRegOpenKey(HKEY__**, HKEY__*, wchar_t*, int);
    ULONG AmiUtilityRegSetValue(HKEY__*, wchar_t*, wchar_t*, ULONG, int);
    ULONG AmiUtilityRegGetValue(void*, ULONG*, HKEY__*, wchar_t*, int);
    void AmiUtilityRegCloseKey(HKEY__*);
    void AslLogCallPrintf(int, const char*, int, const char*, ...);
}

/**
 * Retrieves or creates a registry key for a given file
 * 
 * @param param_1       [Description needed]
 * @param param_2       File path
 * @param param_3       [Description needed]
 * @param param_4       [Description needed]
 * @param param_5       [Description needed]
 * @param param_6       [Description needed]
 * @return ULONG        Error code (0 for success)
 */
ULONG AmipRegGetFileKey(
    HKEY__** param_1,
    wchar_t* filePath,
    void* param_3,
    wchar_t* param_4,
    int param_5,
    int param_6)
{
    ULONG result = 0;
    HKEY__* hKey1 = nullptr;
    HKEY__* hKey2 = nullptr;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    wchar_t volumePath[MAX_PATH] = {0};
    wchar_t volumeName[60] = {0};  // 0x3c bytes
    wchar_t fileId[49] = {0};      // local_106
    BY_HANDLE_FILE_INFORMATION fileInfo = {0};
    DWORD checksum1 = 0, checksum2 = 0;
    DWORD fileAttributes = 0;
    bool keysOpened = false;

    // Open the file
    hFile = CreateFileW(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x253, "Failed to open file %S [%d]", filePath, result);
        goto cleanup;
    }

    // Get volume path
    if (!GetVolumePathNameW(filePath, volumePath, MAX_PATH)) {
        result = GetLastError();
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x253, "GetVolumePathName %S failed [%d]", filePath, result);
        goto cleanup;
    }

    // Get volume GUID
    if (!GetVolumeNameForVolumeMountPointW(volumePath, volumeName, sizeof(volumeName)/sizeof(wchar_t))) {
        result = GetLastError();
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x25b, "Could not get Volume ID for %S [%d]", filePath, result);
        goto cleanup;
    }

    // Copy file ID (implementation simplified)
    if (FAILED(StringCchCopyNW(fileId, 49, param_4, param_6))) {
        result = 0x6f;  // ERROR_BUFFER_OVERFLOW
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x264, "Buffer overflow");
        goto cleanup;
    }

    // Get file information
    if (!GetFileInformationByHandle(hFile, &fileInfo)) {
        result = GetLastError();
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x26e, "Could not get the file information for %S [%d]", filePath, result);
        goto cleanup;
    }

    // Format file ID (implementation simplified)
    if (FAILED(StringCchPrintfW(fileId, 18, L"%x%x", fileInfo.nFileIndexLow, fileInfo.nFileIndexHigh))) {
        result = 0x6f;  // ERROR_BUFFER_OVERFLOW
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x277, "Failed to format file ID");
        goto cleanup;
    }

    // Open registry keys
    result = AmiUtilityRegOpenKey(&hKey1, nullptr, param_4, param_6);
    if (result != 0) {
        if (result != 2 && result != 5 && result != 0x20 && result != 0x13) {
            AslLogCallPrintf(1, "AmipRegGetFileKey", 0x290, "Failed to open file table key [%d]", result);
        }
        goto cleanup;
    }

    result = AmiUtilityRegOpenKey(&hKey2, hKey1, fileId, param_6);
    if (result != 0) {
        if (result != 2 && result != 5 && result != 0x20) {
            AslLogCallPrintf(1, "AmipRegGetFileKey", 0x2a9, "Failed to open file id key [%d]", result);
        }
        goto cleanup;
    }

    keysOpened = true;

    // Get file attributes
    if (!GetFileAttributesExW(filePath, GetFileExInfoStandard, &fileAttributes)) {
        result = GetLastError();
        AslLogCallPrintf(1, "AmipRegGetFileKey", 0x2b4, "Failed to find file [%d]", result);
        goto cleanup;
    }

    // Calculate checksums (simplified)
    checksum1 = fileInfo.dwVolumeSerialNumber ^ fileInfo.nFileIndexHigh;
    checksum2 = fileInfo.nFileIndexLow ^ fileInfo.ftLastWriteTime.dwLowDateTime;

    // Get existing checksum
    AmiUtilityRegGetValue(hKey2, (ULONG*)L"17", (HKEY__*)0x2, (wchar_t*)&fileAttributes, 0);

    // If checksums don't match, update them
    if (checksum1 != 0 || checksum2 != 0) {
        if (RegDeleteValueW(hKey2, L"101") != ERROR_SUCCESS && GetLastError() != ERROR_FILE_NOT_FOUND) {
            AslLogCallPrintf(2, "AmipRegGetFileKey", 0x2ca, "Failed to delete old checksum %S [%d]", filePath, GetLastError());
        }

        result = AmiUtilityRegSetValue(hKey2, fileId, fileId, checksum1, checksum2);
        if (result != 0) {
            AslLogCallPrintf(2, "AmipRegGetFileKey", 0x2d7, "Failed to set new checksum %S [%d]", filePath, result);
            goto cleanup;
        }
    }

    // Success - return the key
    *param_1 = hKey2;
    hKey2 = nullptr;  // Prevent cleanup from closing it
    result = 0;

cleanup:
    if (keysOpened && hKey1 != nullptr) {
        AmiUtilityRegCloseKey(hKey1);
    }
    if (hKey2 != nullptr) {
        AmiUtilityRegCloseKey(hKey2);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    // Map ERROR_PATH_NOT_FOUND to ERROR_FILE_NOT_FOUND
    if (result == ERROR_PATH_NOT_FOUND) {
        result = ERROR_FILE_NOT_FOUND;
    }

    return result;
}

