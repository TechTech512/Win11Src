#include <windows.h>
#include <string>

// Error logging function (placeholder - implement as needed)
void AslLogCallPrintf(int level, const char* function, unsigned long line, const char* format, ...) {
    // Implementation would log errors to appropriate channel
}

// Registry utility functions
namespace RegistryUtils {

void AmiUtilityRegCloseKey(HKEY hKey) {
    LONG result = RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        AslLogCallPrintf(1, "AmiUtilityRegCloseKey", 0x100, "Failed to close key [%d]", result);
    }
}

DWORD AmiUtilityRegEnsureKey(HKEY* phKey, HKEY hParentKey, const wchar_t* subKey, DWORD* disposition) {
    DWORD dwDisposition = 0;
    LONG result = RegCreateKeyExW(
        hParentKey,
        subKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        nullptr,
        phKey,
        &dwDisposition);

    if (result == ERROR_SUCCESS) {
        if (disposition != nullptr) {
            *disposition = dwDisposition;
        }
        return ERROR_SUCCESS;
    }
    else if (result != ERROR_FILE_NOT_FOUND && 
             result != ERROR_ACCESS_DENIED && 
             result != ERROR_SHARING_VIOLATION) {
        AslLogCallPrintf(1, "AmiUtilityRegEnsureKey", 0x58, "RegCreateKeyEx failed [%d]", result);
    }
    return result;
}

DWORD AmiUtilityRegGetValue(
    HKEY hKey,
    const wchar_t* subKey,
    const wchar_t* valueName,
    DWORD* type,
    void* data,
    DWORD* dataSize) {

    if (dataSize == nullptr) {
        AslLogCallPrintf(1, "AmiUtilityRegGetValue", 0x17c, "Bad parameter [dataSize is null]");
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwType = 0;
    if (type != nullptr) {
        dwType = *type;
    }
    else {
        // Default types based on hKey (from decompiled code)
        if (hKey == nullptr) {
            dwType = 0;
        }
        else if (hKey == (HKEY)1) {
            dwType = REG_SZ;
        }
        else if (hKey == (HKEY)2) {
            dwType = REG_DWORD;
        }
        else if (hKey == (HKEY)3) {
            dwType = REG_BINARY;
        }
        else if (hKey == (HKEY)4) {
            dwType = REG_MULTI_SZ;
        }
        else {
            AslLogCallPrintf(1, "AmiUtilityRegGetValue", 0x19c, "Bad parameter [invalid key]");
            return ERROR_INVALID_PARAMETER;
        }
    }

    LONG result = RegGetValueW(
        hKey,
        subKey,
        valueName,
        RRF_RT_ANY,
        &dwType,
        data,
        dataSize);

    if (result == ERROR_SUCCESS) {
        if (type != nullptr) {
            *type = dwType;
        }
        return ERROR_SUCCESS;
    }
    else if (result == ERROR_FILE_NOT_FOUND) {
        return ERROR_FILE_NOT_FOUND;
    }
    else if (result == ERROR_MORE_DATA) {
        return ERROR_MORE_DATA;
    }

    AslLogCallPrintf(1, "AmiUtilityRegGetValue", 0x1b1, "RegGetValue failed [%d]", result);
    return result;
}

DWORD AmiUtilityRegOpenKey(HKEY* phKey, HKEY hParentKey, const wchar_t* subKey, REGSAM samDesired) {
    LONG result = RegOpenKeyExW(
        hParentKey,
        subKey,
        0,
        samDesired,
        phKey);

    if (result == ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }
    else if (result != ERROR_FILE_NOT_FOUND && 
             result != ERROR_ACCESS_DENIED && 
             result != ERROR_SHARING_VIOLATION) {
        AslLogCallPrintf(1, "AmiUtilityRegOpenKey", 0xa7, "RegOpenKey failed [%d]", result);
    }
    return result;
}

DWORD AmiUtilityRegSetValue(
    HKEY hKey,
    const wchar_t* valueName,
    DWORD type,
    const void* data,
    DWORD dataSize) {

    LONG result = RegSetValueExW(
        hKey,
        valueName,
        0,
        type,
        static_cast<const BYTE*>(data),
        dataSize);

    if (result == ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }
    else if (result != ERROR_ACCESS_DENIED) {
        AslLogCallPrintf(1, "AmiUtilityRegSetValue", 0x27e, "RegSetValueEx failed [%d]", result);
    }
    return result;
}

} // namespace RegistryUtils

