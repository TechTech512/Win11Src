#include <windows.h>

DWORD __cdecl SqmPrinterInstallInitUIEntry(DWORD param_1)
{
    HKEY hKey;
    DWORD dwResult;
    DWORD dwType;
    DWORD dwData;
    DWORD cbData;
    
    dwResult = 0;
    LONG lResult = RegCreateKeyExW(HKEY_CURRENT_USER,
                                  L"Software\\Microsoft\\Windows\\CurrentVersion\\PrinterInstallation",
                                  0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);
    
    if (lResult != ERROR_SUCCESS) {
        dwResult = GetLastError();
        if (dwResult > 0) {
            dwResult = dwResult & 0xFFFF | 0x80070000;
        }
        if ((int)dwResult < 0) {
            return dwResult;
        }
        return 0x80004005;
    }
    
    cbData = sizeof(DWORD);
    lResult = RegQueryValueExW(hKey, L"UIEntry", NULL, &dwType, (LPBYTE)&dwData, &cbData);
    
    if (lResult == ERROR_SUCCESS) {
        if (dwType == REG_DWORD) {
            if (dwData > 0xC) {
                goto SET_VALUE;
            }
        } else {
DELETE_VALUE:
            RegDeleteValueW(hKey, L"UIEntry");
            dwData = 0;
        }
        if (dwData != 0) goto CLOSE_KEY;
    } else if (lResult != ERROR_FILE_NOT_FOUND) {
        goto DELETE_VALUE;
    }
    
SET_VALUE:
    dwResult = RegSetValueExW(hKey, L"UIEntry", 0, REG_DWORD, (const BYTE*)&param_1, sizeof(DWORD));
    if (dwResult > 0) {
        dwResult = dwResult & 0xFFFF | 0x80070000;
    }
    
CLOSE_KEY:
    RegCloseKey(hKey);
    return dwResult;
}

DWORD __cdecl SqmPrinterInstallReadAndResetUIEntry(DWORD *param_1)
{
    HKEY hKey;
    DWORD dwResult;
    DWORD dwType;
    DWORD dwData;
    DWORD cbData;
    
    dwResult = 0;
    *param_1 = 0;
    
    LONG lResult = RegOpenKeyExW(HKEY_CURRENT_USER,
                                L"Software\\Microsoft\\Windows\\CurrentVersion\\PrinterInstallation",
                                0, KEY_READ | KEY_WRITE, &hKey);
    
    if (lResult == ERROR_SUCCESS) {
        cbData = sizeof(DWORD);
        lResult = RegQueryValueExW(hKey, L"UIEntry", NULL, &dwType, (LPBYTE)&dwData, &cbData);
        
        if ((lResult == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwData < 0xD)) {
            *param_1 = dwData;
        }
        
        RegCloseKey(hKey);
        RegDeleteKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\PrinterInstallation", 0, 0);
    } else {
        dwResult = GetLastError();
        if (dwResult > 0) {
            dwResult = dwResult & 0xFFFF | 0x80070000;
        }
        if ((int)dwResult >= 0) {
            dwResult = 0x80004005;
        }
    }
    
    return dwResult;
}

