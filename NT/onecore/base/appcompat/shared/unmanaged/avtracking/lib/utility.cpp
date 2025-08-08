#include <windows.h>
#include <strsafe.h>
#include <limits.h>
#include <shlwapi.h>

// Type definitions
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned long long ulong64;
typedef unsigned int uint;

// Forward declarations for external functions
extern "C" {
    void AslLogCallPrintf(int level, const char* function, int line, const char* format, ...);
    int ReadRegistryValue(uchar* param_1, ulong* param_2, ulong param_3, wchar_t* param_4, wchar_t* param_5);
    int ReadRegistryValueDword(ulong* param_1, wchar_t* param_2, wchar_t* param_3);
    int WriteRegistryValue(wchar_t* param_1, wchar_t* param_2, ulong param_3, uchar* param_4, ulong param_5);
    int CopyStringNonConstAlloc(wchar_t** param_1, wchar_t* param_2);
}

namespace Windows {
namespace Compat {
namespace AvTracking {

/**
 * Clears the contents of a registry key
 * 
 * @param keyPath The registry key path to clear
 * @return Error code (0 for success)
 */
uint ClearRegKeyContents(wchar_t* keyPath)
{
    HKEY hKey = nullptr;
    uint result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        0,
        KEY_ALL_ACCESS,
        &hKey);

    if (result == ERROR_SUCCESS) {
        result = SHDeleteKeyW(hKey, L"");
        if (result == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return ERROR_SUCCESS;
        }

        // Convert error code if needed
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ClearRegKeyContents", 0x173,
                        "Error deleting Device Compat Registry Tree: [%d].", result);
    }
    else {
        if (result == ERROR_PATH_NOT_FOUND || result == ERROR_FILE_NOT_FOUND) {
            return ERROR_SUCCESS; // Treat missing key as success
        }

        // Convert error code if needed
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ClearRegKeyContents", 0x16d,
                        "Error opening the key for Device Compat: [%d].", result);
    }

    if (hKey != nullptr) {
        RegCloseKey(hKey);
    }

    return result;
}

/**
 * Allocates and copies a string
 * 
 * @param dest Where to store the new string pointer
 * @param src The string to copy
 * @return Error code (0 for success)
 */
int CopyStringAlloc(wchar_t** dest, wchar_t* src)
{
    if (dest == nullptr) {
        return E_INVALIDARG;
    }

    *dest = nullptr;
    int result = CopyStringNonConstAlloc(dest, src);
    
    if (result < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::CopyStringAlloc", 0x28b,
                        "Failed to alloc and copy string: [0x%x].", result);
    }

    return result;
}

/**
 * Checks if a registry timestamp has passed
 * 
 * @param result Pointer to store the result (true if passed)
 * @param keyPath Registry key path
 * @param valueName Registry value name
 * @param timeoutMs Timeout in milliseconds
 * @return Error code (0 for success)
 */
int HasRegTimestampPassed(bool* result, wchar_t* keyPath, wchar_t* valueName, ulong64 timeoutMs)
{
    if (result == nullptr) {
        return E_INVALIDARG;
    }

    *result = true; // Default to true if we can't determine

    ulong regValue = 0;
    uint size = sizeof(regValue);
    ulong ulSize = size;  // Add this declaration before the ReadRegistryValue call
    int ret = ReadRegistryValue(nullptr, &ulSize, 0, keyPath, valueName);
    
    if (ret < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::HasRegTimestampPassed", 0x223,
                        "Failed to read registry value: [0x%x].", ret);
        return ret;
    }

    if (ret != 1) { // Value exists
        FILETIME currentTime;
        GetSystemTimeAsFileTime(&currentTime);

        // Calculate threshold time (regValue + timeout)
        ULARGE_INTEGER threshold;
        threshold.QuadPart = regValue + timeoutMs * 10000; // Convert ms to 100ns units

        ULARGE_INTEGER now;
        now.LowPart = currentTime.dwLowDateTime;
        now.HighPart = currentTime.dwHighDateTime;

        *result = (now.QuadPart >= threshold.QuadPart);
    }

    return ERROR_SUCCESS;
}

/**
 * Increments a DWORD registry value
 * 
 * @param keyPath Registry key path
 * @param valueName Registry value name
 * @return Error code (0 for success)
 */
int IncrementRegistryValueDword(wchar_t* keyPath, wchar_t* valueName)
{
    ulong value = 0;
    int ret = ReadRegistryValueDword(&value, keyPath, valueName);
    
    if (ret < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::IncrementRegistryValueDword", 0x1c8,
                        "Failed to read registry value: [0x%x].", ret);
        return ret;
    }

    if (value != ULONG_MAX) { // Don't increment if at max value
        value++;
        ret = WriteRegistryValue(keyPath, valueName, sizeof(value), reinterpret_cast<uchar*>(&value), 0);
        
        if (ret < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::IncrementRegistryValueDword", 0x1d4,
                            "Failed to write registry value: [0x%x].", ret);
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

/**
 * Reads a registry value
 * 
 * @param data Buffer to store the value data
 * @param size Pointer to buffer size (in/out)
 * @param flags Additional flags
 * @param keyPath Registry key path
 * @param valueName Registry value name
 * @return Error code (0 for success, 1 if value doesn't exist)
 */
uint ReadRegistryValue(uchar* data, ulong* size, ulong flags, wchar_t* keyPath, wchar_t* valueName)
{
    HKEY hKey = nullptr;
    uint result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\AvTracking\\History",
        0,
        KEY_READ,
        &hKey);

    if (result != ERROR_SUCCESS) {
        if (result == ERROR_FILE_NOT_FOUND) {
            return 1; // Special "not found" code
        }

        // Convert error code
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadRegistryValue", 0x56,
                        "Could not open regkey %ls: [%d].", keyPath, result);
        return result;
    }

    DWORD type = 0;
    result = RegQueryValueExW(hKey, valueName, nullptr, &type, data, size);

    if (result == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(hKey);
        return 1; // Special "not found" code
    }

    if (result != ERROR_SUCCESS) {
        // Convert error code
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadRegistryValue", 0x6a,
                        "Failed to query registry value: %ls under %ls: [%d].", valueName, keyPath, result);
        RegCloseKey(hKey);
        return result;
    }

    RegCloseKey(hKey);
    return ERROR_SUCCESS;
}

/**
 * Writes a registry value
 * 
 * @param keyPath Registry key path
 * @param valueName Registry value name
 * @param size Size of data
 * @param data The data to write
 * @param flags Additional flags
 * @return Error code (0 for success)
 */
uint WriteRegistryValue(wchar_t* keyPath, wchar_t* valueName, ulong size, uchar* data, ulong flags)
{
    HKEY hKey = nullptr;
    uint result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        nullptr);

    if (result != ERROR_SUCCESS) {
        // Convert error code
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteRegistryValue", 0x127,
                        "Could not open/create regkey %ls: [%d].", keyPath, result);
        return result;
    }

    result = RegSetValueExW(hKey, valueName, 0, REG_BINARY, data, size);

    if (result != ERROR_SUCCESS) {
        // Convert error code
        if (result > 0) {
            result = HRESULT_FROM_WIN32(result);
        }
        if (result == S_OK) {
            result = E_FAIL;
        }

        AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteRegistryValue", 0x136,
                        "Failed to create registry value: %ls under %ls: [%d].", valueName, keyPath, result);
    }

    RegCloseKey(hKey);
    return result;
}

} // namespace AvTracking
} // namespace Compat
} // namespace Windows

