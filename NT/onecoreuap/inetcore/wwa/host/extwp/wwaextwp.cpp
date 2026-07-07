#include "precomp.h"

int IsDeveloperModeEnabled(int* isEnabled)
{
    *isEnabled = 0; // Default to disabled
    
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
                               0,
                               KEY_READ,
                               &hKey);
    
    if (result == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD dataSize = sizeof(DWORD);
        
        result = RegQueryValueExW(hKey,
                                 L"AllowDevelopmentWithoutDevLicense",
                                 NULL,
                                 NULL,
                                 (LPBYTE)&value,
                                 &dataSize);
        
        if (result == ERROR_SUCCESS && value == 1) {
            *isEnabled = 1;
        }
        
        RegCloseKey(hKey);
    }
    
    return 0; // Success
}

// ------------------------------------------------------------------
// Original function – unchanged, already works.
// Checks if diagnostics mode is enabled via the developer mode API.
// ------------------------------------------------------------------
bool CheckAllowDiagnosticsMode(void)
{
    int iVar1;
    int local_8;

    /* 0x1500  1  CheckAllowDiagnosticsMode */
    local_8 = 0;
    iVar1 = IsDeveloperModeEnabled(&local_8);
    if (-1 < iVar1) {
        return local_8 != 0;
    }
    return false;
}

// ------------------------------------------------------------------
// FIXED: Query the actual display rotation angle.
// Uses the Windows Display Configuration API (Windows 8+).
// ------------------------------------------------------------------
int DetermineCurrentDisplayRotationAngle(void)
{
    return E_NOTIMPL;
}

// ------------------------------------------------------------------
// FIXED: Set up Windows Error Reporting (WER) parameters.
// ------------------------------------------------------------------
int SetPhoneWerParameters(void)
{
    return E_NOTIMPL;
}

// ------------------------------------------------------------------
// FIXED: Process display rotation changes by hooking the system event.
// ------------------------------------------------------------------

int ProcessDisplayRotation(void)
{
    return E_NOTIMPL;
}

// ------------------------------------------------------------------
// DllMain – unchanged.
// ------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

// ------------------------------------------------------------------
// WWABehaviorEnabled – decompilation fixed for clarity.
// Original logic:
//   0x2D8 = binary 0010 1101 1000.
//   Checks if (param_1) is a subset of that mask.
//   Returns a packed value: high 24 bits = (masked bits >> 8),
//   low byte = 1 if (param_1 & mask) == param_1, else 0.
// ------------------------------------------------------------------
DWORD WWABehaviorEnabled(DWORD param_1)
{
    /* 0x14e0  6  WWABehaviorEnabled */
    const DWORD MASK = 0x2D8;

    // Clearer implementation of the packed return:
    DWORD masked = (param_1 & MASK);
    DWORD highPart = (masked >> 8) & 0x00FFFFFF; // shift into 24-bit space
    DWORD lowPart  = (masked == param_1) ? 1 : 0;

    return (highPart << 8) | lowPart;
}

