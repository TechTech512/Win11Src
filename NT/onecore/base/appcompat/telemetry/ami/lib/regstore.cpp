#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <winternl.h>
#include <strsafe.h>

// Forward declarations for utility functions
extern ULONG AmiUtilityInitSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, PACL* ppAcl);
ULONG __cdecl AmiFixKeyPermissionsRecursiveHelper(HKEY hKey, PSECURITY_DESCRIPTOR pSecurityDescriptor);
extern ULONG AmiUtilityGivePrivilegesToServices(HKEY hKey);
extern ULONG AmiUtilitySetPrivilege(LPCWSTR privilege, BOOL enable);
ULONG AmiUtilityRegEnsureKey(HKEY* phKey, HKEY hParent, LPCWSTR subKey, ULONG* reserved);
ULONG AmiUtilityRegOpenKey(HKEY* phKey, HKEY hParent, LPCWSTR subKey, ULONG options);
ULONG AmiUtilityRegGetValue(HKEY hKey, LPCWSTR lpValue, DWORD dwType, PVOID pvData, DWORD* pcbData);
ULONG AmiUtilityRegSetValue(HKEY hKey, LPCWSTR lpValue, DWORD dwType, CONST BYTE* lpData, DWORD cbData);
ULONG AmiUtilityRegCloseKey(HKEY hKey);
extern ULONG AmiUtilityAcquireMutex(void** param1, wchar_t* param2);
extern ULONG AmiUtilityExceedQuota(LPCWSTR param);
extern ULONG AmiUtilitySharedBlockAlloc(wchar_t* heap, ULONG size);
ULONG __cdecl AmiStoreInitialize(void** ppStore, LPCWSTR lpPath, LPCWSTR lpReserved);
ULONG __cdecl AmipStoreInitialize(void** ppStore, LPCWSTR lpPath, LPCWSTR lpReserved);
void AmiStoreClose(void* param);

// Global variables
bool PermissionsCorrect = false;

// Process Environment Block (simplified)
extern PEB* ProcessEnvironmentBlock;

// Stub for logging function
void __cdecl AslLogCallPrintf(int level, const char* function, ULONG line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsprintf_s(buffer, sizeof(buffer), format, args);
    OutputDebugStringA(function);
    OutputDebugStringA(": ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    va_end(args);
}

ULONG __cdecl AmiFixKeyPermissionsRecursive(HKEY hKey)
{
    ULONG uResult;
    PACL pAcl = NULL;
    SECURITY_DESCRIPTOR securityDescriptor;

    uResult = AmiUtilityInitSecurityDescriptor(&securityDescriptor, &pAcl);
    if (uResult == 0) {
        uResult = AmiFixKeyPermissionsRecursiveHelper(hKey, &securityDescriptor);
        if (uResult == 0) {
            return 0;
        }
        AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursive", 0x9AC, "AmiFixKeyPermissionsRecursiveHelper failed [%d]", uResult);
    }
    else {
        AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursive", 0x9A5, "AmiUtilityInitSecurityDescriptor failed [%d]", uResult);
    }
    return uResult;
}

ULONG __cdecl AmiFixKeyPermissionsRecursiveHelper(HKEY hKey, PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    ULONG uResult;
    DWORD dwIndex = 0;
    WCHAR szName[MAX_PATH];
    DWORD cbName = MAX_PATH;
    FILETIME ftLastWriteTime;
    HKEY hSubKey = NULL;

    uResult = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pSecurityDescriptor);
    if (uResult == ERROR_SUCCESS) {
        for (;;) {
            cbName = MAX_PATH;
            uResult = RegEnumKeyExW(hKey, dwIndex, szName, &cbName, NULL, NULL, NULL, &ftLastWriteTime);
            if (uResult == ERROR_NO_MORE_ITEMS) {
                uResult = ERROR_SUCCESS;
                break;
            }
            if (uResult != ERROR_SUCCESS) {
                AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursiveHelper", 0x96C, "RegEnumKeyEx failed [%d]", uResult);
                break;
            }

            uResult = RegOpenKeyExW(hKey, szName, 0, KEY_ALL_ACCESS, &hSubKey);
            if (uResult != ERROR_SUCCESS) {
                AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursiveHelper", 0x977, "RegOpenKeyEx failed [%d]", uResult);
                break;
            }

            uResult = AmiFixKeyPermissionsRecursiveHelper(hSubKey, pSecurityDescriptor);
            RegCloseKey(hSubKey);
            hSubKey = NULL;

            if (uResult != ERROR_SUCCESS) {
                AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursiveHelper", 0x97F, "AmiFixKeyPermissionsRecursiveHelper failed [%d]", uResult);
                break;
            }

            dwIndex++;
        }
    }
    else {
        AslLogCallPrintf(1, "AmiFixKeyPermissionsRecursiveHelper", 0x94F, "RegSetKeySecurity failed [%d]", uResult);
    }

    if (hSubKey) {
        RegCloseKey(hSubKey);
    }
    return uResult;
}

ULONG __cdecl AmiInitialize(void** ppStore)
{
    ULONG uResult;
    WCHAR szPath[MAX_PATH];

    if (ppStore == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *ppStore = NULL;

    uResult = AmiStoreInitialize(ppStore, szPath, NULL);
    if (uResult != ERROR_SUCCESS) {
        AslLogCallPrintf(1, "AmiInitialize", 0x5C, "Failed to initialize Ami store [%d]", uResult);
    }

    return uResult;
}

ULONG __cdecl AmipCreateNewCacheFile(LPCWSTR lpFilePath)
{
    ULONG uResult;
    HKEY hKey = NULL;
    HKEY hFlagsKey = NULL;
    DWORD dwValue = 1;

    // Adjust privileges
    uResult = AmiUtilitySetPrivilege(L"SeBackupPrivilege", TRUE);
    if (uResult == ERROR_SUCCESS) {
        uResult = AmiUtilitySetPrivilege(L"SeRestorePrivilege", TRUE);
    }

    if (uResult == ERROR_SUCCESS) {
        // Create registry key
        uResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\{11517B7C-E79D-4e20-961B-75A811715ADD}",
            0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

        if (uResult == ERROR_SUCCESS) {
            // Give privileges to services
            uResult = AmiUtilityGivePrivilegesToServices(hKey);
            if (uResult == ERROR_SUCCESS) {
                // Set permissions correct flag - CORRECTED VERSION
                uResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags",
                    0, KEY_SET_VALUE, &hFlagsKey);
                
                if (uResult == ERROR_SUCCESS) {
                    RegSetValueExW(hFlagsKey,
                        L"AmiHivePermissionsCorrect",  // Value name
                        0,                            // Reserved
                        REG_DWORD,                    // Value type
                        (const BYTE*)&dwValue,        // Data
                        sizeof(dwValue));             // Data size
                    RegCloseKey(hFlagsKey);
                } else {
                    AslLogCallPrintf(1, "AmipCreateNewCacheFile", 0x5C, "Failed to open AppCompatFlags key [%d]", uResult);
                }

                // Save the key to file
                uResult = RegSaveKeyExW(hKey, lpFilePath, NULL, REG_LATEST_FORMAT);
                if (uResult != ERROR_SUCCESS) {
                    AslLogCallPrintf(1, "AmipCreateNewCacheFile", 0x76, "RegSaveKeyEx failed [%d]", uResult);
                }
            }
            else {
                AslLogCallPrintf(1, "AmipCreateNewCacheFile", 0x5C, "AmiUtilityGivePrivilegesToServices failed [%d]", uResult);
            }
        }
        else {
            AslLogCallPrintf(1, "AmipCreateNewCacheFile", 0x51, "RegCreateKeyEx failed [%d]", uResult);
        }
    }
    else {
        AslLogCallPrintf(1, "AmipCreateNewCacheFile", 0x3E, "You must be logged on as Administrator/System [%d]", uResult);
    }

    // Cleanup
    if (hKey) {
        RegCloseKey(hKey);
        RegDeleteKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\{11517B7C-E79D-4e20-961B-75A811715ADD}",
            0, 0);
    }

    return uResult;
}

bool __cdecl AmipVerifyCachePermissionsCorrect(LPCWSTR lpFilePath)
{
    bool bLoaded = false;
    ULONG uResult;
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);

    if (PermissionsCorrect) {
        return true;
    }

    // Check if permissions are already marked as correct
    uResult = RegGetValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags",
        L"AmiHivePermissionsCorrect", RRF_RT_REG_DWORD, NULL, &dwValue, &cbValue);

    if (uResult == ERROR_SUCCESS && dwValue == 1) {
        PermissionsCorrect = true;
        return true;
    }

    // Adjust privileges
    uResult = AmiUtilitySetPrivilege(L"SeBackupPrivilege", TRUE);
    if (uResult == ERROR_SUCCESS) {
        uResult = AmiUtilitySetPrivilege(L"SeRestorePrivilege", TRUE);
    }

    if (uResult == ERROR_SUCCESS) {
        // Load the registry hive
        uResult = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"{11517B7C-E79D-4e20-961B-75A811715ADD}", lpFilePath);
        if (uResult == ERROR_SUCCESS) {
            bLoaded = true;

            // Open the key
            uResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"{11517B7C-E79D-4e20-961B-75A811715ADD}", 
                0, KEY_ALL_ACCESS, &hKey);

            if (uResult == ERROR_SUCCESS) {
                // Fix permissions recursively
                uResult = AmiFixKeyPermissionsRecursive(hKey);
                if (uResult == ERROR_SUCCESS) {
                    // Mark permissions as correct
                    HKEY hSubKey;
                    dwValue = 1;
                    uResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags",
                        0, KEY_SET_VALUE, &hSubKey);
                    
                    if (uResult == ERROR_SUCCESS) {
                        RegSetValueExW(hSubKey,
                            L"AmiHivePermissionsCorrect",
                            0,
                            REG_DWORD,
                            (const BYTE*)&dwValue,
                            sizeof(dwValue));
                        RegCloseKey(hSubKey);
                        PermissionsCorrect = true;
                    }
                    else {
                        AslLogCallPrintf(1, "AmipVerifyCachePermissionsCorrect", 0x919, 
                            "Failed to open AppCompatFlags key [%d]", uResult);
                    }
                }
                else {
                    AslLogCallPrintf(1, "AmipVerifyCachePermissionsCorrect", 0x919, 
                        "AmiFixKeyPermissionsRecursive failed [%d]", uResult);
                }
            }
            else {
                AslLogCallPrintf(1, "AmipVerifyCachePermissionsCorrect", 0x902, 
                    "RegOpenKeyEx failed [%d]", uResult);
            }
        }
        else {
            AslLogCallPrintf(1, "AmipVerifyCachePermissionsCorrect", 0x8F7, 
                "RegLoadKey failed [%d]", uResult);
        }
    }
    else {
        AslLogCallPrintf(1, "AmipVerifyCachePermissionsCorrect", 0x8E9, 
            "You must be logged on as Administrator/System [%d]", uResult);
    }

    // Cleanup
    if (hKey) {
        RegCloseKey(hKey);
    }

    if (bLoaded) {
        RegUnLoadKeyW(HKEY_LOCAL_MACHINE, L"{11517B7C-E79D-4e20-961B-75A811715ADD}");
    }

    return PermissionsCorrect;
}

ULONG __cdecl AmiStoreInitialize(void** ppStore, LPCWSTR lpPath, LPCWSTR lpReserved)
{
    ULONG uResult;
    WCHAR szTempPath[MAX_PATH];
    HANDLE hMutex = NULL;
    DWORD dwVersion = 0;
    DWORD cbVersion = sizeof(dwVersion);

    *ppStore = NULL;

    uResult = AmiUtilityAcquireMutex(&hMutex, L"AMICacheMutex");
    if (uResult != ERROR_SUCCESS) {
        AslLogCallPrintf(3, "AmiStoreInitialize", 0x193, "Failed to create mutex [%d]", uResult);
        return uResult;
    }

    // Check cache version
    uResult = RegGetValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags",
        L"AmiCacheVersion", RRF_RT_REG_DWORD, NULL, &dwVersion, &cbVersion);

    if (uResult == ERROR_SUCCESS && dwVersion != 2) {
        DeleteFileW(lpPath);
    }

    // Check if file exists
    if (GetFileAttributesW(lpPath) == INVALID_FILE_ATTRIBUTES) {
        // Create new cache file
        uResult = AmipCreateNewCacheFile(lpPath);
        if (uResult != ERROR_SUCCESS) {
            AslLogCallPrintf(1, "AmiStoreInitialize", 0x1C7, "AmipCreateNewCacheFile [%d]", uResult);
            goto cleanup;
        }

        // Set version
        HKEY hSubKey;
        dwVersion = 2;
        uResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags",
            0, KEY_SET_VALUE, &hSubKey);
        
        if (uResult == ERROR_SUCCESS) {
            RegSetValueExW(hSubKey,
                L"AmiCacheVersion",
                0,
                REG_DWORD,
                (const BYTE*)&dwVersion,
                sizeof(dwVersion));
            RegCloseKey(hSubKey);
        }
        else {
            AslLogCallPrintf(1, "AmiStoreInitialize", 0x1C7, "Failed to set cache version [%d]", uResult);
        }
    }
    else {
        // Verify permissions
        if (!AmipVerifyCachePermissionsCorrect(lpPath)) {
            // Create temp path
            StringCchPrintfW(szTempPath, MAX_PATH, L"%s.tmp", lpPath);
            
            if (GetFileAttributesW(szTempPath) == INVALID_FILE_ATTRIBUTES) {
                uResult = AmipCreateNewCacheFile(szTempPath);
                if (uResult != ERROR_SUCCESS) {
                    AslLogCallPrintf(1, "AmiStoreInitialize", 0x1BA, "StringCchPrintfW [%#x]", uResult);
                    goto cleanup;
                }
            }
        }

        // Check quota
        if (AmiUtilityExceedQuota(lpPath) == 0) {
            AslLogCallPrintf(2, "AmiStoreInitialize", 0x1F2, "Quota exceeded");
        }
    }

    // Initialize store
    uResult = AmipStoreInitialize(ppStore, lpPath, lpReserved);
    if (uResult != ERROR_SUCCESS) {
        AslLogCallPrintf(1, "AmiStoreInitialize", 0x1FB, "AmipStoreInitialize [%d]", uResult);
    }

cleanup:
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return uResult;
}

// Stub for Process Environment Block
PEB _PEB;
PEB* ProcessEnvironmentBlock = &_PEB;

