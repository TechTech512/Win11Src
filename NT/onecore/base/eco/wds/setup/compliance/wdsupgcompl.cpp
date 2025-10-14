#include <windows.h>

#pragma warning (disable:4996)

typedef struct _CompatCheckContext {
    void *Reserved;
} CompatCheckContext;

unsigned long __cdecl WdsIsComplianceCheckNeeded(int *param_1)
{
    OSVERSIONINFOW versionInfo;
    unsigned long result = 0;
    
    memset(&versionInfo, 0, sizeof(versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    
    if (GetVersionExW(&versionInfo)) {
        if ((versionInfo.dwPlatformId == 1) || (versionInfo.dwMajorVersion != 5) || (versionInfo.dwMinorVersion != 2)) {
            *param_1 = 0;
        } else {
            *param_1 = 1;
        }
    } else {
        result = GetLastError();
    }
    
    return result;
}

unsigned long __cdecl WdsModeComplianceCheck(int *param_1)
{
    HKEY hSetupKey = NULL;
    HKEY hWdsKey = NULL;
    unsigned long result = 0;
    DWORD remInstValue = 0;
    DWORD forceNativeValue = 0;
    DWORD dataSize = sizeof(DWORD);
    BOOL bForceNative = FALSE;
    DWORD dwCompliance = 0;

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup", 0, KEY_READ, &hSetupKey);
    if (result == ERROR_SUCCESS) {
        dataSize = sizeof(DWORD);
        result = RegQueryValueExW(hSetupKey, L"RemInst", NULL, NULL, (LPBYTE)&remInstValue, &dataSize);
        if (result == ERROR_SUCCESS) {
            dwCompliance = (remInstValue != 0) ? 1 : 0;
            
            if (dwCompliance != 0) {
                result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WDS", 0, KEY_READ, &hWdsKey);
                if (result == ERROR_SUCCESS) {
                    dataSize = sizeof(DWORD);
                    result = RegQueryValueExW(hWdsKey, L"ForceNative", NULL, NULL, (LPBYTE)&forceNativeValue, &dataSize);
                    if (result == ERROR_SUCCESS) {
                        bForceNative = (forceNativeValue != 0);
                    } else if (result == ERROR_FILE_NOT_FOUND) {
                        result = ERROR_SUCCESS;
                    }
                } else if (result == ERROR_FILE_NOT_FOUND) {
                    result = ERROR_SUCCESS;
                }
            }
        } else if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    } else if (result == ERROR_FILE_NOT_FOUND) {
        result = ERROR_SUCCESS;
    }

    if (result == ERROR_SUCCESS) {
        if (dwCompliance == 0 || bForceNative) {
            *param_1 = 0;
        } else {
            *param_1 = 1;
        }
    }

    if (hSetupKey != NULL) {
        RegCloseKey(hSetupKey);
    }
    if (hWdsKey != NULL) {
        RegCloseKey(hWdsKey);
    }

    return result;
}

int __cdecl WdsUpgradeComplianceCheck(void (*param_1)(CompatCheckContext*, int, unsigned int), CompatCheckContext *param_2)
{
    unsigned long errorCode;
    int result = 1;
    int complianceCheckNeeded = 0;

    if (param_1 == NULL || param_2 == NULL) {
        errorCode = ERROR_INVALID_PARAMETER;
    } else {
        errorCode = WdsIsComplianceCheckNeeded(&complianceCheckNeeded);
        if (errorCode == ERROR_SUCCESS) {
            if (complianceCheckNeeded) {
                errorCode = WdsModeComplianceCheck(&complianceCheckNeeded);
                if (errorCode == ERROR_SUCCESS) {
                    param_1(param_2, complianceCheckNeeded, 0);
                }
            } else {
                param_1(param_2, 0, 0);
            }
        }
    }

    if (errorCode != ERROR_SUCCESS) {
        SetLastError(errorCode);
        result = 0;
    }

    return result;
}

