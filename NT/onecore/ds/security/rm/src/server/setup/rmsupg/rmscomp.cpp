#pragma warning (disable:4996)

#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef struct _CompatCheckContext {
    void *Reserved;
} CompatCheckContext;

// Function pointer type for compatibility callback
typedef void (__cdecl *COMPATIBILITY_CALLBACK)(CompatCheckContext*, int, unsigned int);

int checkRegistryADRMSWithWindowsInternalDatabase(void)
{
    HKEY hKey1 = NULL;
    HKEY hKey2 = NULL;
    wchar_t* connectionString = NULL;
    DWORD configStatusType = 0;
    DWORD configStatusSize = sizeof(DWORD);
    DWORD configStatusValue = 0;
    DWORD connectionStringType = 0;
    DWORD connectionStringSize = 0;
    int result = 0;
    OSVERSIONINFOEXW osVersionInfo;
    
    // Initialize version info structure
    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    
    if (!GetVersionExW((OSVERSIONINFOW*)&osVersionInfo)) {
        goto cleanup;
    }
    
    const wchar_t* registryPath = NULL;
    const wchar_t* connectionStringPath = NULL;
    
    if (osVersionInfo.dwMajorVersion == 6) {
        // Windows Vista/Server 2008 or later
        if (osVersionInfo.dwMinorVersion != 0) {
            registryPath = L"SOFTWARE\\Microsoft\\DRMS";
            connectionStringPath = L"SOFTWARE\\Microsoft\\DRMS\\ConnectionString";
        } else {
            registryPath = L"SOFTWARE\\Microsoft\\DRMS\\2.0";
            connectionStringPath = L"SOFTWARE\\Microsoft\\DRMS\\2.0\\ConnectionString";
        }
    } else if (osVersionInfo.dwMajorVersion >= 7) {
        // Windows 7/Server 2008 R2 or later
        registryPath = L"SOFTWARE\\Microsoft\\DRMS";
        connectionStringPath = L"SOFTWARE\\Microsoft\\DRMS\\ConnectionString";
    } else {
        // Unsupported version
        goto cleanup;
    }
    
    // Open registry key
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, KEY_READ, &hKey1) != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    // Query ConfigStatus value
    DWORD valueType = 0;
    DWORD valueSize = sizeof(DWORD);
    DWORD valueData = 0;
    
    if (RegQueryValueExW(hKey1, L"ConfigStatus", NULL, &valueType, 
                        (BYTE*)&valueData, &valueSize) == ERROR_SUCCESS) {
        if (valueType == REG_DWORD && valueData == 3) {
            // Open connection string key
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, connectionStringPath, 0, KEY_READ, &hKey2) == ERROR_SUCCESS) {
                // Get connection string size
                if (RegQueryValueExW(hKey2, L"ConfigDatabaseConnectionString", NULL, NULL, NULL, &connectionStringSize) == ERROR_SUCCESS) {
                    if (connectionStringSize > 0) {
                        // Allocate buffer for connection string
                        connectionString = (wchar_t*)malloc(connectionStringSize + 2);
                        if (connectionString != NULL) {
                            ZeroMemory(connectionString, connectionStringSize + 2);
                            
                            // Read connection string
                            if (RegQueryValueExW(hKey2, L"ConfigDatabaseConnectionString", NULL, &connectionStringType,
                                                (BYTE*)connectionString, &connectionStringSize) == ERROR_SUCCESS) {
                                
                                // Ensure null termination
                                connectionString[connectionStringSize / sizeof(wchar_t)] = L'\0';
                                
                                // Check if it contains the Windows Internal Database path
                                const wchar_t* searchString = L"np:\\\\.\\pipe\\MSSQL$Microsoft##SSEE\\sql\\query";
                                if (wcsstr(connectionString, searchString) != NULL) {
                                    result = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
cleanup:
    if (hKey1 != NULL) {
        RegCloseKey(hKey1);
    }
    if (hKey2 != NULL) {
        RegCloseKey(hKey2);
    }
    if (connectionString != NULL) {
        free(connectionString);
    }
    
    return result;
}

int checkRegistryRMSv1(void)
{
    const wchar_t* registryPaths[] = {
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{E3FF64B7-99F3-4FC9-9A76-389FF31350C3}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{6CBB7D64-60C5-4F7B-B427-7E3972519717}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{E9187259-3A95-4D6A-A92D-74B01DB3F3BF}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{245BEC6D-71B1-4A0D-B217-D9712C8D2F78}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{C2598188-3336-4B5D-991B-262498A61E7F}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{E9412DED-C975-4DF4-9DC1-EBC12703ADF4}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{8684DE7D-E50D-4737-8D0C-7CF89748DF17}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{5581D723-2BEB-4120-A56E-BCFCDE7C7AE5}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{CB785C6D-002D-4F5C-8D6C-6659C61441DE}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{C71EA3DE-D99F-4EAA-BC86-BC4FD138708B}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{AB4B79DB-4981-48F0-8EE5-18592B83A4CD}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A29D22C8-1653-4788-9AE4-313F142E6C4A}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A4F4AA9A-AF0E-49B2-8EBB-3E77B7F326A5}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{5D4B9368-242A-4196-81AD-A64EE291A2E7}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{4AECBEE3-D035-40D6-88A2-2D590DCB2256}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{5FF7E84B-96CD-4C83-8382-7B9DB880fDF4}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{B0E39C9B-0374-44E7-95CF-3B4E1B0C9866}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{746C5112-CEE9-4D0E-AEB6-ECE865461AF8}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{22E2EC28-829B-4626-BAAA-EA3E2EDFA300}",
        L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{7D51E7A5-B67C-4E3A-B648-40D882581491}"
    };
    
    const int pathCount = sizeof(registryPaths) / sizeof(registryPaths[0]);
    
    for (int i = 0; i < pathCount; i++) {
        HKEY hKey = NULL;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, registryPaths[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return 1; // Found RMS v1 installation
        }
    }
    
    return 0; // No RMS v1 installation found
}

int __cdecl RmsUpgradeComplianceCheck(COMPATIBILITY_CALLBACK callback, CompatCheckContext* context)
{
    int hasRMSv1 = checkRegistryRMSv1();
    
    if (hasRMSv1 == 0) {
        int hasADRMS = checkRegistryADRMSWithWindowsInternalDatabase();
        if (hasADRMS == 0) {
            return 1; // No compatibility issues found
        }
        // AD RMS with Windows Internal Database found
        if (callback != NULL) {
            callback(context, 0, 0xFA3); // 4003 in decimal
        }
    } else {
        // RMS v1 found
        if (callback != NULL) {
            callback(context, 0, 0xFA2); // 4002 in decimal
        }
    }
    
    return 1;
}

