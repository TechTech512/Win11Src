#include <windows.h>

ULONG __cdecl UpdateAttemptCount(void)
{
    HKEY hKey;
    DWORD disposition;
    DWORD dataSize;
    ULONG attemptCount;
    LONG result;
    
    hKey = NULL;
    disposition = 0;
    dataSize = sizeof(ULONG);
    attemptCount = 0;
    
    result = RegCreateKeyExW(HKEY_CURRENT_USER,
                             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SystemReset",
                             0,
                             NULL,
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &disposition);
    
    if ((hKey != NULL) && (result == ERROR_SUCCESS)) {
        result = RegQueryValueExW(hKey,
                                  L"SystemResetAttempts",
                                  NULL,
                                  NULL,
                                  (LPBYTE)&attemptCount,
                                  &dataSize);
        
        if (result != ERROR_SUCCESS) {
            attemptCount = 0;
        }
        
        attemptCount = attemptCount + 1;
        
        result = RegSetValueExW(hKey,
                                L"SystemResetAttempts",
                                0,
                                REG_DWORD,
                                (const BYTE*)&attemptCount,
                                sizeof(ULONG));
        
        if (result != ERROR_SUCCESS) {
            attemptCount = 4;
        }
    } else {
        attemptCount = 4;
    }
    
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    
    return attemptCount;
}

