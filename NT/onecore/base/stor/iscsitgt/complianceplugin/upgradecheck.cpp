#include "precomp.h"

enum ServerVersion {
    UnknownOS = -1,
    NoComplianceCheckNeeded = 0,
    Server2008 = 1,
    Server2008R2 = 2,
    Server2012 = 3,
    Server2012R2 = 4
};

typedef void (*COMPAT_CHECK_CALLBACK)(void* context, int isCompliant, unsigned int messageId);

typedef struct _CompatCheckContext {
    void* Reserved;
} CompatCheckContext;

DWORD CheckServer2008FamilyCompliance(void);
ServerVersion GetServerVersion(void);
DWORD IsServiceInstalled(LPCWSTR serviceName, LPCWSTR registryPath);
int ItgtUpgradeComplianceCheck(COMPAT_CHECK_CALLBACK callback, CompatCheckContext* context);
void ItgtUpgradeEnableFireWallRule(void);

DWORD CheckServer2008FamilyCompliance(void)
{
    DWORD result1 = IsServiceInstalled(L"Service1", L"RegistryPath1");
    DWORD result2 = IsServiceInstalled(L"Service2", L"RegistryPath2");
    DWORD result3 = IsServiceInstalled(L"Service3", L"RegistryPath3");
    
    DWORD finalResult = 0;
    
    if ((result1 == 0) || (result2 == 0) || (result3 == 0)) {
        finalResult = 1;
    }
    else if ((result1 == (DWORD)-1) || (result2 == (DWORD)-1) || (result3 == (DWORD)-1)) {
        finalResult = 0x80004005; // E_FAIL
    }
    
    ItgtUpgradeEnableFireWallRule();
    return finalResult;
}

ServerVersion GetServerVersion(void)
{
    OSVERSIONINFOEXW versionInfo;
    ZeroMemory(&versionInfo, sizeof(versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    
    ServerVersion serverVersion = NoComplianceCheckNeeded;
    
    if (!GetVersionExW((OSVERSIONINFOW*)&versionInfo)) {
        serverVersion = UnknownOS;
    }
    else if (versionInfo.wProductType == VER_NT_WORKSTATION || versionInfo.dwMajorVersion < 6) {
        serverVersion = NoComplianceCheckNeeded;
    }
    else if (versionInfo.dwMajorVersion == 6) {
        switch (versionInfo.dwMinorVersion) {
            case 0:
                serverVersion = Server2008;
                break;
            case 1:
                serverVersion = Server2008R2;
                break;
            case 2:
                serverVersion = Server2012;
                break;
            case 3:
                serverVersion = Server2012R2;
                break;
            default:
                serverVersion = NoComplianceCheckNeeded;
                break;
        }
    }
    
    return serverVersion;
}

DWORD IsServiceInstalled(LPCWSTR serviceName, LPCWSTR registryPath)
{
    HKEY hKey;
    DWORD result = 0;
    DWORD regType = 0;
    DWORD regData = 0;
    DWORD dataSize = sizeof(DWORD);
    
    LONG regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, KEY_READ, &hKey);
    
    if (regResult == ERROR_SUCCESS) {
        result = 0;
        
        if (serviceName != NULL) {
            regResult = RegQueryValueExW(hKey, serviceName, NULL, &regType, (LPBYTE)&regData, &dataSize);
            if (regResult != ERROR_SUCCESS || (regType != REG_DWORD && regType != REG_BINARY)) {
                result = 1;
            }
        }
        
        RegCloseKey(hKey);
    }
    else if (regResult == ERROR_FILE_NOT_FOUND) {
        result = 1;
    }
    else if (regResult > 0) {
        result = regResult | 0x80070000;
    }
    
    return result;
}

int ItgtUpgradeComplianceCheck(COMPAT_CHECK_CALLBACK callback, CompatCheckContext* context)
{
    ServerVersion version = GetServerVersion();
    int returnValue = 1;
    
    if (version == UnknownOS) {
        if (callback) {
            callback(context, 1, 0x2EE2);
        }
        return returnValue;
    }
    
    if (version >= Server2008 && version <= Server2008R2) {
        DWORD complianceResult = CheckServer2008FamilyCompliance();
        if (complianceResult == 1) {
            if (callback) {
                callback(context, 1, 0x2EE1);
            }
        }
        else if (complianceResult == 0x80004005) {
            if (callback) {
                callback(context, 1, 0x2EE2);
            }
        }
    }
    else if (version >= Server2012 && version <= Server2012R2) {
        DWORD serviceResult = IsServiceInstalled(L"Service1", L"RegistryPath1");
        DWORD finalResult = 1;
        
        if (serviceResult != 0 && serviceResult != (DWORD)-1) {
            finalResult = 0;
        }
        else if (serviceResult == (DWORD)-1) {
            finalResult = 0x80004005;
        }
        
        ItgtUpgradeEnableFireWallRule();
        
        if (finalResult == 1) {
            if (callback) {
                callback(context, 1, 0x2EE3);
            }
        }
        else if (finalResult == 0x80004005) {
            if (callback) {
                callback(context, 1, 0x2EE2);
            }
        }
    }
    
    return returnValue;
}

void ItgtUpgradeEnableFireWallRule(void)
{
    INetFwPolicy2* fwPolicy = NULL;
    INetFwRules* fwRules = NULL;
    INetFwRule* fwRule1 = NULL;
    INetFwRule* fwRule2 = NULL;
    
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    HRESULT hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, 
                                  __uuidof(INetFwPolicy2), (void**)&fwPolicy);
    
    if (SUCCEEDED(hr)) {
        hr = fwPolicy->get_Rules(&fwRules);
        
        if (SUCCEEDED(hr)) {
            WCHAR systemPath[MAX_PATH];
            WCHAR dllPath[MAX_PATH];
            
            if (GetSystemDirectoryW(systemPath, MAX_PATH)) {
                StringCchPrintfW(dllPath, MAX_PATH, L"%s\\firewallapi.dll", systemPath);
                
                HMODULE hFirewallDll = LoadLibraryExW(dllPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
                
                if (hFirewallDll) {
                    WCHAR ruleName1[256];
                    WCHAR ruleName2[256];
                    
                    if (LoadStringW(hFirewallDll, 0x192, ruleName1, 256)) {
                        BSTR bstrRuleName1 = SysAllocString(ruleName1);
                        
                        if (bstrRuleName1) {
                            hr = fwRules->Item(bstrRuleName1, &fwRule1);
                            
                            if (SUCCEEDED(hr)) {
                                VARIANT_BOOL enabled;
                                hr = fwRule1->get_Enabled(&enabled);
                                
                                if (SUCCEEDED(hr) && enabled != VARIANT_FALSE) {
                                    fwRule1->put_Enabled(VARIANT_FALSE);
                                }
                                
                                fwRule1->Release();
                                fwRule1 = NULL;
                            }
                            
                            SysFreeString(bstrRuleName1);
                        }
                    }
                    
                    if (LoadStringW(hFirewallDll, 0x191, ruleName2, 256)) {
                        BSTR bstrRuleName2 = SysAllocString(ruleName2);
                        
                        if (bstrRuleName2) {
                            hr = fwRules->Item(bstrRuleName2, &fwRule2);
                            
                            if (SUCCEEDED(hr)) {
                                VARIANT_BOOL enabled;
                                hr = fwRule2->get_Enabled(&enabled);
                                
                                if (SUCCEEDED(hr) && enabled == VARIANT_FALSE) {
                                    fwRule2->put_Enabled(VARIANT_TRUE);
                                }
                                
                                fwRule2->Release();
                                fwRule2 = NULL;
                            }
                            
                            SysFreeString(bstrRuleName2);
                        }
                    }
                    
                    FreeLibrary(hFirewallDll);
                }
            }
            
            fwRules->Release();
        }
        
        fwPolicy->Release();
    }
    
    CoUninitialize();
}

