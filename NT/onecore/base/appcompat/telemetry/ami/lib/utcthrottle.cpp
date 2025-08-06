#include <windows.h>
#include <sddl.h>
#include <strsafe.h>
#include <stdio.h>
#include <accctrl.h>
#include <winternl.h>

// Define the function prototype if not available in headers
#if (_WIN32_WINNT < 0x0502)
typedef LONG (WINAPI *RegDeleteKeyValueWFunc)(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName);
#endif

// Logging function
extern "C" void __cdecl AslLogCallPrintf(int level, const char* function, unsigned long line, const char* format, ...);

// Alternative registry key deletion that works on all Windows versions
bool DeleteRegistryValue(HKEY hKey, LPCWSTR subKey, LPCWSTR valueName)
{
    HKEY hSubKey = NULL;
    if (RegOpenKeyExW(hKey, subKey, 0, KEY_SET_VALUE, &hSubKey) == ERROR_SUCCESS) {
        LSTATUS result = RegDeleteValueW(hSubKey, valueName);
        RegCloseKey(hSubKey);
        return result == ERROR_SUCCESS;
    }
    return false;
}

// Throttle management class
class UtcThrottle {
private:
    static LONG RefCount;
    static HANDLE ThrottlingSemaphore;
    static HANDLE ThrottlingTimer;
    static HANDLE ThrottlingCleanupEvent;

public:
    static ULONG Uninitialize();
};

// Initialize static members
LONG UtcThrottle::RefCount = 0;
HANDLE UtcThrottle::ThrottlingSemaphore = NULL;
HANDLE UtcThrottle::ThrottlingTimer = NULL;
HANDLE UtcThrottle::ThrottlingCleanupEvent = NULL;

// Registry path helper
unsigned long AmiRegGetStoreFullPath(wchar_t* outputPath, unsigned long bufferSize)
{
    wchar_t systemPath[MAX_PATH] = {0};
    wchar_t overridePath[MAX_PATH] = {0};
    const wchar_t* registryPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags";
    const wchar_t* valueName = L"AmiOverridePath";

    // Try to get override path from registry
    DWORD pathSize = sizeof(overridePath);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, valueName, 
                    RRF_RT_REG_SZ, NULL, overridePath, &pathSize) == ERROR_SUCCESS) 
    {
        if (GetFileAttributesW(overridePath) != INVALID_FILE_ATTRIBUTES) {
            if (SUCCEEDED(StringCchCopyW(outputPath, bufferSize, overridePath))) {
                return ERROR_SUCCESS;
            }
            AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 361, "Buffer overflow");
            return ERROR_INSUFFICIENT_BUFFER;
        }
        
        // Path doesn't exist - remove invalid registry entry using our compatible function
        if (!DeleteRegistryValue(HKEY_LOCAL_MACHINE, registryPath, valueName)) {
            AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 347, "Failed to delete registry value");
        }
    }

    // Fall back to system directory
    if (GetSystemWindowsDirectoryW(systemPath, MAX_PATH) == 0) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 335, "GetSystemWindowsDirectory failed [%d]", error);
        return error;
    }

    // Construct default path
    if (FAILED(StringCchPrintfW(outputPath, bufferSize, L"%s\\AppCompat\\Programs", systemPath))) {
        AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 347, "Path construction failed");
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return ERROR_SUCCESS;
}

// Mutex helper
unsigned long AmiUtilityAcquireMutex(HANDLE* mutexHandle, const wchar_t* mutexName)
{
    wchar_t fullName[MAX_PATH];
    swprintf_s(fullName, MAX_PATH, L"Local\\%s_%d", mutexName, GetCurrentProcessId());

    HANDLE mutex = CreateMutexW(NULL, FALSE, fullName);
    if (mutex == NULL) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityAcquireMutex", 613, "Failed to create mutex [%d]", error);
        return error;
    }

    if (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0) {
        DWORD error = GetLastError();
        CloseHandle(mutex);
        AslLogCallPrintf(1, "AmiUtilityAcquireMutex", 622, "Failed to acquire mutex [%d]", error);
        return error ? error : ERROR_ACCESS_DENIED;
    }

    *mutexHandle = mutex;
    return ERROR_SUCCESS;
}

// Quota checker
int AmiUtilityExceedQuota(const wchar_t* filePath)
{
    HANDLE file = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityExceedQuota", 421, "Could not open file [%d]", error);
        return 0;
    }

    LARGE_INTEGER fileSize;
    int result = 0;
    
    if (GetFileSizeEx(file, &fileSize)) {
        if (fileSize.QuadPart < 100 * 1024 * 1024) { // 100MB limit
            result = 1;
        } else {
            AslLogCallPrintf(2, "AmiUtilityExceedQuota", 439, "Store size exceeded the limit");
        }
    } else {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityExceedQuota", 430, "Could not get file size [%d]", error);
    }

    CloseHandle(file);
    return result;
}

// Throttle cleanup implementation
ULONG UtcThrottle::Uninitialize()
{
    if (InterlockedDecrement(&RefCount) == 0) {
        if (ThrottlingCleanupEvent) {
            SetEvent(ThrottlingCleanupEvent);
            CloseHandle(ThrottlingCleanupEvent);
            ThrottlingCleanupEvent = NULL;
        }
        if (ThrottlingSemaphore) {
            CloseHandle(ThrottlingSemaphore);
            ThrottlingSemaphore = NULL;
        }
        if (ThrottlingTimer) {
            CloseHandle(ThrottlingTimer);
            ThrottlingTimer = NULL;
        }
    }
    return ERROR_SUCCESS;
}

