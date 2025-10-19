#include <windows.h>
#include <shellapi.h>
#include <winreg.h>

// Forward declarations for external types
struct ISppNamedParamsReadOnly;
struct _tagWS_FEATURE_LICENSE_RESULTS;
struct _tagWS_PRODUCT_LICENSE_RESULTS;

// Global variable
static void* dummy = nullptr;

// Alternative implementation for IsDeveloperModeEnabled
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

// Function implementations
unsigned long __cdecl AcquireDeveloperLicense(HWND__* hWnd, _FILETIME* pFileTime)
{
    ShellExecuteW(hWnd, NULL, L"ms-settings:developers", NULL, NULL, SW_SHOWNORMAL);
    pFileTime->dwLowDateTime = 0xA7568000;
    pFileTime->dwHighDateTime = 0x24C85995;
    return 0;
}

int __cdecl CheckDeveloperLicense(_FILETIME* pFileTime)
{
    int isEnabled = 0;
    
    pFileTime->dwLowDateTime = 0xA7568000;
    pFileTime->dwHighDateTime = 0x24C85995;
    
    int result = IsDeveloperModeEnabled(&isEnabled);
    if ((result >= 0) && (isEnabled == 0)) {
        result = -0x7FF8FB70;
    }
    return result;
}

unsigned long __cdecl GetApplicationURL(wchar_t* param_1, wchar_t** param_2)
{
    *param_2 = NULL;
    return 0;
}

unsigned long __cdecl RemoveDeveloperLicense(HWND__* hWnd)
{
    ShellExecuteW(hWnd, NULL, L"ms-settings:developers", NULL, NULL, SW_SHOWNORMAL);
    return 0;
}

unsigned long __cdecl WSCallServer(void* param_1, wchar_t* param_2, ISppNamedParamsReadOnly* param_3, ISppNamedParamsReadOnly** param_4)
{
    if (param_4 != NULL) {
        *param_4 = NULL;
    }
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSCheckForConsumable(void* param_1, wchar_t* param_2, int* param_3)
{
    *param_3 = 0;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSGetEvaluatePackageAttempted(void* param_1, wchar_t* param_2, int* param_3)
{
    *param_3 = 0;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSEvaluatePackage(void* param_1, wchar_t* param_2, unsigned long param_3)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseCleanUpState(void)
{
    return 0;
}

unsigned long __cdecl WSLicenseClose(void* param_1)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseFilterValidAppCategoryIds(void* param_1, unsigned long param_2, _GUID* param_3, int* param_4)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetAllUserTokens(void* param_1, unsigned int* param_2, wchar_t*** param_3)
{
    *param_2 = 0;
    *param_3 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetAllValidAppCategoryIds(void* param_1, unsigned int* param_2, wchar_t*** param_3)
{
    *param_2 = 0;
    *param_3 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetDevInstalledApps(void* param_1, unsigned int* param_2, wchar_t*** param_3)
{
    *param_2 = 0;
    *param_3 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetExtendedUserInfo(void* param_1, wchar_t** param_2, unsigned int* param_3, wchar_t*** param_4)
{
    *param_2 = NULL;
    *param_3 = 0;
    *param_4 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetFeatureLicenseResults(void* param_1, wchar_t* param_2, _tagWS_FEATURE_LICENSE_RESULTS** param_3, unsigned int* param_4)
{
    *param_4 = 0;
    *param_3 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetLicensesForProducts(void* param_1, unsigned int param_2, _GUID* param_3, wchar_t*** param_4, unsigned int* param_5)
{
    if (param_5 != NULL) {
        *param_5 = 0;
    }
    *param_4 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseGetOAuthServiceTicket(HWND__* param_1, wchar_t* param_2, wchar_t* param_3, wchar_t* param_4, wchar_t* param_5, wchar_t** param_6, wchar_t** param_7)
{
    *param_6 = NULL;
    *param_7 = NULL;
    return 0;
}

unsigned long __cdecl WSLicenseGetProductLicenseResults(void* param_1, _GUID* param_2, wchar_t* param_3, _tagWS_PRODUCT_LICENSE_RESULTS** param_4)
{
    *param_4 = NULL;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseInstallLicense(void* param_1, unsigned long param_2, _GUID* param_3, int* param_4)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseOpen(void** param_1)
{
    *param_1 = &dummy;
    return 0;
}

unsigned long __cdecl WSLicenseRefreshLicense(void* param_1, wchar_t* param_2)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseRevokeLicenses(void* param_1, wchar_t* param_2, unsigned long param_3)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseUninstallLicense(void* param_1, wchar_t* param_2, unsigned long param_3)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSLicenseRetrieveMachineID(void* param_1, _GUID* param_2)
{
    param_2->Data1 = 0;
    param_2->Data2 = 0;
    param_2->Data3 = 0;
    param_2->Data4[0] = 0;
    param_2->Data4[1] = 0;
    param_2->Data4[2] = 0;
    param_2->Data4[3] = 0;
    param_2->Data4[4] = 0;
    param_2->Data4[5] = 0;
    param_2->Data4[6] = 0;
    param_2->Data4[7] = 0;
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSNotifyOOBECompletion(void)
{
    return 0;
}

unsigned long __cdecl WSNotifyPackageInstalled(void* param_1, wchar_t* param_2, unsigned long param_3)
{
    return (param_1 != &dummy) ? 0x80070057 : 0;
}

unsigned long __cdecl WSTriggerOOBEFileValidation(void)
{
    return 0;
}

void __cdecl WSpTLRW(HWND__* param_1, HINSTANCE__* param_2, wchar_t* param_3, unsigned int param_4)
{
    return;
}

void __cdecl RefreshBannedAppsList(HWND__* param_1, HINSTANCE__* param_2, wchar_t* param_3, unsigned int param_4)
{
    return;
}

