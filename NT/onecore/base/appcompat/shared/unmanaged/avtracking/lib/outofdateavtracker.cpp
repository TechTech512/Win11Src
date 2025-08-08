#include <windows.h>
#include <wbemidl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <comutil.h> // For _bstr_t
#include <rtlarray.h> // RtlArray implementation
#pragma comment(lib, "shlwapi.lib")  // Add this line after the includes

// Forward declarations for external functions
extern "C" {
    int HasRegTimestampPassed(bool*, wchar_t*, wchar_t*, DWORD);
    void AslLogCallPrintf(int, const char*, unsigned long, const char*, ...);
    HRESULT CoInitialize(LPVOID);
    void CoUninitialize();
    void VariantInit(VARIANTARG*);
    int IncrementRegistryValueDword(wchar_t*, wchar_t*);
    int AslStringPatternMatchW(wchar_t*, wchar_t*);
    BOOL HeapFree(HANDLE, DWORD, LPVOID);
    int WriteRegistryValue(wchar_t*, wchar_t*, DWORD, BYTE*, DWORD);
    int ClearRegKeyContents(wchar_t*);
    int ReadRegistryValueDword(wchar_t*, wchar_t*, wchar_t*);
    BOOL RtlFreeHeap(HANDLE, DWORD, LPVOID); // Changed return type to BOOL
    HANDLE GetProcessHeap();
}

namespace Windows {
namespace Compat {
namespace AvTracking {

// Enums and types
enum ProductState {
    ProductState_Off,
    ProductState_Enabled,
    ProductState_Snoozed,
    ProductState_OffByPolicy,
    ProductState_Expired,
    ProductState_InsecureByPolicy
};

enum InferredStatus {
    InferredStatus_Unknown,
    InferredStatus_Active,
    InferredStatus_Expired
};

struct AvDescription {
    wchar_t* Path;
    wchar_t* Guid;
    DWORD WmiProductState;
};

struct OneSettingsQuery {
    // Implementation details would be here
    OneSettingsQuery() {}
    ~OneSettingsQuery() {}
    int Initialize(wchar_t*, wchar_t*, wchar_t*, DWORD) { return 0; }
    int Uninitialize() { return 0; }
    int GetSettingDword(wchar_t*, DWORD*) { return 0; }
};

// Global variables
bool ShouldWriteInferredExpiredAv = false;
bool HaveAnswerForShouldWrite = false;
bool TreatAllExpired = false;
bool HaveResult = false;

// Forward declarations for internal functions
DWORD ReadAvFromWMI(RtlArray<AvDescription>* avList);
DWORD MinimumChecksFromOnesettings();
bool TreatAllAsExpiredImpl();
int UpateAvHistory(AvDescription* avDesc, bool param_2);
DWORD GetOverallInferredStatusForAv(InferredStatus* status, AvDescription* avDesc, DWORD productState);
ProductState GetWscStateFromWmiProductState(DWORD productState);
char* InferOverallStatusFromAvHistory(InferredStatus* status, AvDescription* avDesc, DWORD productState);
void FreeAvList(RtlArray<AvDescription>* avList);
DWORD WriteInferredExpiredRegKeysForAv(AvDescription* avDesc, DWORD* param_2);
bool ShouldWriteInferredExpiredAvImpl();

int CopyStringAlloc(wchar_t** dest, const wchar_t* src) {
    if (!dest || !src) return -1;
    
    size_t len = wcslen(src) + 1;
    *dest = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, len * sizeof(wchar_t));
    if (!*dest) return -1;
    
    StringCchCopyW(*dest, len, src);
    return 0;
}

int CheckIfIntervalElapsedAndUpdate(bool* param_1, wchar_t* path1, wchar_t* path2, DWORD timestamp) {
    int result = HasRegTimestampPassed(param_1, path1, path2, timestamp);
    
    if (result < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::CheckIfIntervalElapsedAndUpdate", 
                         0xC0, "Failed to determine if timestamp has passed: [0x%x].", result);
    } else {
        if (*param_1) {
            FILETIME currentTime;
            GetSystemTimeAsFileTime(&currentTime);
            
            result = WriteRegistryValue((wchar_t*)0xB, (wchar_t*)&currentTime, 8, (BYTE*)&currentTime, timestamp);
            if (result < 0) {
                AslLogCallPrintf(1, "Windows::Compat::AvTracking::CheckIfIntervalElapsedAndUpdate", 
                                 0xC5, "Failed to write current timestamp: [0x%x].", result);
            }
        }
        result = 0;
    }
    return result;
}

int CleanRegistryKeys(wchar_t* keyPath) {
    int result = ClearRegKeyContents(keyPath);
    
    if (result < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::CleanRegistryKeys", 
                         0x23B, "Failed to clear registry keys: [0x%x].", result);
    } else {
        result = ClearRegKeyContents(keyPath);
        if (result >= 0) {
            return 0;
        }
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::CleanRegistryKeys", 
                         0x23E, "Failed to clear registry keys: [0x%x].", result);
    }
    return result;
}

char* DailyUpdateAvStatus() {
    RtlArray<AvDescription> avList;
    AvDescription* minCheckResult = nullptr;
    char* result = nullptr;
    bool treatAllExpired = false;
    
    // Initialize COM
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                         0x309, "CoInitialize failed: [0x%x].", hr);
        goto cleanup;
    }
    
    // Clean registry
    result = (char*)CleanRegistryKeys(nullptr);
    if ((int)result < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                         0x30D, "Failed to clean registry: [0x%x].", result);
        goto com_uninit;
    }
    
    // Check interval and update if needed
    CheckIfIntervalElapsedAndUpdate(nullptr, nullptr, nullptr, 0);
    
    // Read AV data from WMI
    result = (char*)ReadAvFromWMI(&avList);
    if ((int)result < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                         0x316, "Failed to query WMI for AV Data: [0x%x].", result);
        goto com_uninit;
    }
    
    // Get minimum checks and expired status
    minCheckResult = (AvDescription*)MinimumChecksFromOnesettings();
    treatAllExpired = TreatAllAsExpiredImpl();
    
    // Process each AV entry
    for (DWORD i = 0; i < avList.GetCount(); i++) {
        AvDescription* item = avList.GetPtr(i);
        
        // Update AV history
        result = (char*)UpateAvHistory(item, false);
        if ((int)result < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                             0x324, "Failed to update history: [0x%x].", result);
            goto com_uninit;
        }
        
        // Get overall status
        InferredStatus status;
        result = (char*)GetOverallInferredStatusForAv(&status, item, item->WmiProductState);
        if ((int)result < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                             0x32B, "Failed to get inferred status: [0x%x].", result);
            goto com_uninit;
        }
        
        // Write expired keys if needed
        if (status == InferredStatus_Expired || treatAllExpired) {
            result = (char*)WriteInferredExpiredRegKeysForAv(item, (DWORD*)item);
            if ((int)result < 0) {
                AslLogCallPrintf(1, "Windows::Compat::AvTracking::DailyUpdateAvStatus", 
                                 0x330, "Failed to write expired keys: [0x%x].", result);
                goto com_uninit;
            }
        }
    }
    
    result = (char*)0;

com_uninit:
    CoUninitialize();

cleanup:
    FreeAvList(&avList);
    if (avList.Array.Buffer != nullptr) {
        RtlFreeHeap(avList.Array.HeapHandle, 0, avList.Array.Buffer);
    }
    return result;
}

void FreeAvList(RtlArray<AvDescription>* avList) {
    if (avList == nullptr) return;
    
    for (DWORD i = 0; i < avList->GetCount(); i++) {
        AvDescription* item = avList->GetPtr(i);
        
        if (item->Path != nullptr) {
            HeapFree(GetProcessHeap(), 0, item->Path);
            item->Path = nullptr;
        }
        
        if (item->Guid != nullptr) {
            HeapFree(GetProcessHeap(), 0, item->Guid);
            item->Guid = nullptr;
        }
    }
    
    if (avList->Array.Buffer != nullptr) {
        RtlFreeHeap(avList->Array.HeapHandle, 0, avList->Array.Buffer);
    }
    
    memset(avList, 0, sizeof(RtlArray<AvDescription>));
}

DWORD GetOverallInferredStatusForAv(InferredStatus* status, AvDescription* avDesc, DWORD productState) {
    *status = InferredStatus_Unknown;
    
    DWORD stateFlags = avDesc->WmiProductState & 0x10;
    ProductState wscState = GetWscStateFromWmiProductState(productState);
    
    switch (wscState) {
        case ProductState_Off:
            if (stateFlags != 0x10) {
                int result = (int)InferOverallStatusFromAvHistory(status, avDesc, productState);
                if (result < 0) {
                    AslLogCallPrintf(1, "Windows::Compat::AvTracking::GetOverallInferredStatusForAv", 
                                     0x204, "Failed to get Anti Virus History: [0x%x].", result);
                    return result;
                }
                return 0;
            }
            *status = InferredStatus_Expired;
            break;
            
        case ProductState_Enabled:
            if (stateFlags != 0) {
                int result = (int)InferOverallStatusFromAvHistory(status, avDesc, productState);
                if (result < 0) {
                    AslLogCallPrintf(1, "Windows::Compat::AvTracking::GetOverallInferredStatusForAv", 
                                     0x218, "Failed to get Anti Virus History: [0x%x].", result);
                    return result;
                }
                return 0;
            }
            *status = InferredStatus_Active;
            break;
            
        case ProductState_Expired:
            *status = InferredStatus_Expired;
            break;
            
        case ProductState_Snoozed:
        case ProductState_OffByPolicy:
        case ProductState_InsecureByPolicy:
            *status = InferredStatus_Active;
            break;
            
        default:
            break;
    }
    
    return 0;
}

ProductState GetWscStateFromWmiProductState(DWORD productState) {
    ProductState state = (ProductState)(productState & 0x7);
    
    switch (state) {
        case ProductState_Enabled:
        case ProductState_Snoozed:
        case ProductState_OffByPolicy:
        case ProductState_Expired:
        case ProductState_InsecureByPolicy:
            return state;
            
        default:
            return ProductState_Off;
    }
}

char* InferOverallStatusFromAvHistory(InferredStatus* status, AvDescription* avDesc, DWORD productState) {
    HKEY hKey = nullptr;
    wchar_t keyPath[260];
    char* result = nullptr;
    
    *status = InferredStatus_Active;
    
    // Format registry key path
    if (StringCchPrintfW(keyPath, 260, L"%ls\\%ls", L"Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Services", avDesc->Guid) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::InferOverallStatusFromAvHistory", 
                         400, "Failed to format device rating string: [0x%x].", GetLastError());
        goto cleanup;
    }
    
    // Open registry key
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD intervalCount = 0;
        result = (char*)ReadRegistryValueDword((wchar_t*)L"IntervalCount", (wchar_t*)L"IntervalCount", (wchar_t*)&intervalCount);
        
        if ((int)result < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::InferOverallStatusFromAvHistory", 
                             0x1A3, "Failed to read interval count: [0x%x].", result);
            goto cleanup;
        }
        
        if (intervalCount >= 2) {
            *status = InferredStatus_Expired;
        }
    }
    
    result = (char*)0;

cleanup:
    if (hKey != nullptr) {
        RegCloseKey(hKey);
    }
    
    return result;
}

DWORD MinimumChecksFromOnesettings() {
    OneSettingsQuery query;
    DWORD minChecks = 15; // Default value
    
    if (query.Initialize(nullptr, nullptr, L"settings-win.data.microsoft.com", 0) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::MinimumChecksFromOnesettings", 
                         0x394, "Error initializing OneSettingsQuery: [%#x].", GetLastError());
        goto cleanup;
    }
    
    if (query.GetSettingDword(L"MINEXPIREDCHECKS", &minChecks) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::MinimumChecksFromOnesettings", 
                         0x397, "Error getting OneSettingsQuery setting: [%#x].", GetLastError());
        goto cleanup;
    }
    
    if (query.Uninitialize() < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::MinimumChecksFromOnesettings", 
                         0x39A, "Error uninitializing OneSettingsQuery: [%#x].", GetLastError());
    }

cleanup:
    return minChecks;
}

DWORD ReadAvFromWMI(RtlArray<AvDescription>* avList) {
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject* pclsObj = nullptr;
    BSTR className = SysAllocString(L"AntiVirusProduct");
    DWORD result = 0;
    
    if (className == nullptr) {
        result = 0x8007000E; // E_OUTOFMEMORY
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadAvFromWMI", 
                         0x11B, "Failed to initialize ClassName: [0x%x].", result);
        return result;
    }
    
    // Connect to WMI
    result = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, 
                             IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(result)) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadAvFromWMI", 
                         0x126, "WMI CoCreateInstance failed: [0x%x].", result);
        goto cleanup;
    }
    
    result = pLoc->ConnectServer(_bstr_t(L"root\\securitycenter2"), nullptr, nullptr, nullptr, 0, 
                                nullptr, nullptr, &pSvc);
    if (FAILED(result)) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadAvFromWMI", 
                         0x131, "WMI Connect failed: [0x%x].", result);
        goto cleanup;
    }
    
    // Execute query
    result = pSvc->ExecQuery(_bstr_t("WQL"), _bstr_t("SELECT * FROM AntiVirusProduct"), 
                            WBEM_FLAG_FORWARD_ONLY, nullptr, &pEnumerator);
    if (FAILED(result)) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadAvFromWMI", 
                         0x137, "Error getting enumerator for AntiVirusProduct: [0x%x].", result);
        goto cleanup;
    }
    
    // Process results
    ULONG uReturn = 0;
    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
        VARIANT vtProp;
        VariantInit(&vtProp);
        
        AvDescription avDesc = {0};
        
        // Get pathToSignedReportingExe
        if (pclsObj->Get(L"pathToSignedReportingExe", 0, &vtProp, nullptr, nullptr) >= 0) {
            wchar_t expandedPath[MAX_PATH];
            DWORD size = ExpandEnvironmentStringsW(vtProp.bstrVal, expandedPath, MAX_PATH);
            
            if (size == 0 || size > MAX_PATH) {
                result = GetLastError();
                if (result > 0) {
                    result |= 0x80070000;
                }
                if (result != 0) {
                    result = 0x80004005; // E_FAIL
                }
                AslLogCallPrintf(1, "Windows::Compat::AvTracking::ReadAvFromWMI", 
                                 0x147, "Error expanding environment strings: [%d].", result);
                VariantClear(&vtProp);
                goto cleanup;
            }
            
            if (CopyStringAlloc(&avDesc.Path, expandedPath) < 0) {
                VariantClear(&vtProp);
                goto cleanup;
            }
        }
        VariantClear(&vtProp);
        
        // Get instanceGuid
        if (pclsObj->Get(L"instanceGuid", 0, &vtProp, nullptr, nullptr) >= 0) {
            if (CopyStringAlloc(&avDesc.Guid, vtProp.bstrVal) < 0) {
                VariantClear(&vtProp);
                goto cleanup;
            }
        }
        VariantClear(&vtProp);
        
        // Get productState
        if (pclsObj->Get(L"productState", 0, &vtProp, nullptr, nullptr) >= 0) {
            avDesc.WmiProductState = vtProp.lVal;
        }
        VariantClear(&vtProp);
        
        // Add to array
        avList->Append(avDesc);
    }
    
    result = 0;

cleanup:
    if (pclsObj != nullptr) pclsObj->Release();
    if (pEnumerator != nullptr) pEnumerator->Release();
    if (pSvc != nullptr) pSvc->Release();
    if (pLoc != nullptr) pLoc->Release();
    if (className != nullptr) SysFreeString(className);
    
    return result;
}

bool ShouldWriteInferredExpiredAvImpl() {
    if (!HaveAnswerForShouldWrite) {
        OneSettingsQuery query;
        DWORD settingValue = 0;
        
        if (query.Initialize(nullptr, nullptr, L"settings-win.data.microsoft.com", 0) < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::ShouldWriteInferredExpiredAv", 
                             0x360, "Error initializing OneSettingsQuery: [%#x].", GetLastError());
            goto cleanup;
        }
        
        if (query.GetSettingDword(L"WRITEEXPIREDAV", &settingValue) >= 0) {
            if (settingValue == 1) {
                ShouldWriteInferredExpiredAv = true;
            }
            
            if (query.Uninitialize() < 0) {
                AslLogCallPrintf(1, "Windows::Compat::AvTracking::ShouldWriteInferredExpiredAv", 
                                 0x36F, "Error uninitializing OneSettingsQuery: [%#x].", GetLastError());
            }
        } else {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::ShouldWriteInferredExpiredAv", 
                             0x363, "Error getting OneSettingsQuery setting: [%#x].", GetLastError());
        }
        
    cleanup:
        HaveAnswerForShouldWrite = true;
    }
    
    return ShouldWriteInferredExpiredAv;
}

bool TreatAllAsExpiredImpl() {
    if (!HaveResult) {
        DWORD value = 0;
        int result = ReadRegistryValueDword((wchar_t*)L"TreatAllAsExpired", (wchar_t*)L"TreatAllAsExpired", (wchar_t*)&value);
        TreatAllExpired = (result >= 0 && value == 1);
    }
    HaveResult = true;
    return TreatAllExpired;
}

int UpateAvHistory(AvDescription* avDesc, bool param_2) {
    ProductState wscState = GetWscStateFromWmiProductState(avDesc->WmiProductState);
    DWORD stateFlags = avDesc->WmiProductState & 0x10;
    int action = 0;
    
    // Determine action based on state
    for (int i = 0; i < 12; i++) {
        // This would compare against some internal state table to determine the action
        // Simplified for this conversion
        if (wscState == ProductState_Enabled && stateFlags == 0) {
            action = 1;
            break;
        }
    }
    
    wchar_t keyPath[260];
    if (StringCchPrintfW(keyPath, 260, L"%ls\\%ls", L"Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Services", avDesc->Guid) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::UpateAvHistory", 
                         0x2D4, "Failed to format key path: [0x%x].", GetLastError());
        return -1;
    }
    
    if (action == 1) {
        DWORD zero = 0;
        if (WriteRegistryValue((wchar_t*)0x4, keyPath, 4, (BYTE*)&zero, 0) < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::UpateAvHistory", 
                             0x2D4, "Failed to write registry key: [0x%x].", GetLastError());
            return -1;
        }
    } else if (action == 2 && !param_2) {
        if (IncrementRegistryValueDword(avDesc->Guid, keyPath) < 0) {
            return -1;
        }
    }
    
    return 0;
}

DWORD WriteInferredExpiredRegKeysForAv(AvDescription* avDesc, DWORD* param_2) {
    HKEY hKey = nullptr;
    wchar_t keyPath[260];
    DWORD result = 0;
    
    (*param_2)++;
    
    // Format the registry value
    if (StringCchPrintfW(keyPath, 260, L"%d", avDesc->WmiProductState) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteInferredExpiredRegKeysForAv", 
                         0x274, "Failed to format string: [0x%x].", GetLastError());
        goto cleanup;
    }
    
    // Write the main registry value
    size_t pathLen = wcslen(avDesc->Path) * 2 + 2;
    if (WriteRegistryValue((wchar_t*)0x1, avDesc->Path, (DWORD)pathLen, (BYTE*)avDesc->Path, 0) < 0) {
        AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteInferredExpiredRegKeysForAv", 
                         0x27C, "Failed to write registry key: [0x%x].", GetLastError());
        goto cleanup;
    }
    
    // Check if we should write expired AV keys
    if (ShouldWriteInferredExpiredAvImpl() || TreatAllAsExpiredImpl()) {
        // Get the filename from path
        PathFindFileNameW(avDesc->Path);
        
        // Create the registry key path
        if (StringCchPrintfW(keyPath, 260, L"%ls\\%ls", L"Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Services", avDesc->Guid) < 0) {
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteInferredExpiredRegKeysForAv", 
                             0x28D, "Failed to format reporting exe key string: [0x%x].", GetLastError());
            goto cleanup;
        }
        
        // Create/open the registry key
        result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
        if (result != ERROR_SUCCESS) {
            if (result > 0) {
                result |= 0x80070000;
            }
            if (result != 0) {
                result = 0x80004005; // E_FAIL
            }
            AslLogCallPrintf(1, "Windows::Compat::AvTracking::WriteInferredExpiredRegKeysForAv", 
                             0x29A, "Could not open/create regkey %ls: [%d].", keyPath, result);
            goto cleanup;
        }
    }
    
    result = 0;

cleanup:
    if (hKey != nullptr) {
        RegCloseKey(hKey);
    }
    
    return result;
}

} // namespace AvTracking
} // namespace Compat
} // namespace Windows

