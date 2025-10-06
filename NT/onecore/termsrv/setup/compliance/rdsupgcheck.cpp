#pragma warning (disable:4996)

#include <windows.h>
#include <wbemidl.h>
#include <dismapi.h>
#include <atlbase.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "dismapi.lib")

typedef struct _CompatCheckContext {
    void *Reserved;
} CompatCheckContext;

DWORD CheckKeyValuePresent(HKEY hKey, wchar_t* pszValueName, wchar_t* pszData, int* pResult)
{
    DWORD dwError;
    HKEY hSubKey = NULL;
    DWORD dwType = 4;
    DWORD dwData = 0;
    DWORD cbData = sizeof(DWORD);

    if (pszValueName)
    {
        pszValueName[0] = L'\0';
        pszValueName[1] = L'\0';
    }

    dwError = RegOpenKeyExW(hKey, NULL, 0, KEY_READ, &hSubKey);
    if (dwError != ERROR_FILE_NOT_FOUND)
    {
        if (dwError != ERROR_SUCCESS)
        {
            if ((int)dwError < 0)
            {
                return dwError;
            }
            return dwError | 0x80070000;
        }

        if (hSubKey == NULL)
        {
            if (pszValueName)
            {
                pszValueName[0] = L'\x01';
                pszValueName[1] = L'\0';
            }
        }
        else
        {
            dwError = RegQueryValueExW(hSubKey, pszValueName, 0, &dwType, (LPBYTE)&dwData, &cbData);
            if ((dwError == ERROR_SUCCESS) && (dwData == 1))
            {
                if (pszValueName)
                {
                    pszValueName[0] = L'\x01';
                    pszValueName[1] = L'\0';
                }
            }
        }
        RegCloseKey(hSubKey);
    }

    return ERROR_SUCCESS;
}

HRESULT GetRdConnectionBrokerCount(ULONG* pCount)
{
    HRESULT hr = E_FAIL;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    DWORD dwCount = 0;
    CComBSTR bstrNamespace;
    CComBSTR bstrQuery;
    CComBSTR bstrQueryLanguage;

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hr))
        return hr;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr))
        return hr;

    bstrNamespace.Attach(SysAllocString(L"root\\cimv2\\rdms"));
    hr = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr))
        goto Cleanup;

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
        goto Cleanup;

    bstrQuery.Attach(SysAllocString(L"SELECT * FROM Win32_RDMSJoinedNode WHERE IsRdcb = 'true'"));
    bstrQueryLanguage.Attach(SysAllocString(L"WQL"));
    hr = pSvc->ExecQuery(bstrQueryLanguage, bstrQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr))
        goto Cleanup;

    while (pEnumerator)
    {
        ULONG uReturned = 0;
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturned);
        if (FAILED(hr))
            break;

        if (hr == WBEM_S_FALSE || uReturned == 0)
            break;

        dwCount += uReturned;
        if (pclsObj)
        {
            pclsObj->Release();
            pclsObj = NULL;
        }
    }

    if (pCount)
        *pCount = dwCount;

Cleanup:
    if (pclsObj)
        pclsObj->Release();
    if (pEnumerator)
        pEnumerator->Release();
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();

    return hr;
}

int IDMUUpgradeComplianceCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    int bValue1 = 0;
    int bValue2 = 0;
    int bValue3 = 0;
    HRESULT hr;

    CheckKeyValuePresent(NULL, (wchar_t*)&bValue1, NULL, NULL);
    CheckKeyValuePresent(NULL, (wchar_t*)&bValue2, NULL, (int*)&bValue1);
    CheckKeyValuePresent(NULL, (wchar_t*)&bValue3, NULL, (int*)&bValue2);

    if (bValue1 != 0 || bValue2 != 0 || bValue3 != 0)
    {
        pCallback(pContext, 0, 0x32CB);
    }

    return 1;
}

int IsTelnetServerInstalled()
{
    int bInstalled = 0;
    DismSession hSession = NULL;
    DismFeatureInfo* pFeatureInfo = NULL;
    BOOL bInitialized = FALSE;
    HRESULT hr;

    hr = DismInitialize((DismLogLevel)2, NULL, NULL);
    if (hr == 0x2e4)  // E_DISM_ALREADY_INITIALIZED
    {
        bInitialized = FALSE;
    }
    else if (FAILED(hr))
    {
        return 0;
    }
    else
    {
        bInitialized = TRUE;
    }

    hr = DismOpenSession(L"DISM_{53BFAE52-B167-4E2F-A258-0A37B57FF845}", NULL, NULL, &hSession);
    if (SUCCEEDED(hr))
    {
        hr = DismGetFeatureInfo(hSession, L"TelnetServer", NULL, (DismPackageIdentifier)0, &pFeatureInfo);
        if (SUCCEEDED(hr) && pFeatureInfo && pFeatureInfo->FeatureState == 4)  // DismInstallStateInstalled
        {
            bInstalled = 1;
        }
    }

    if (hSession != NULL)
    {
        DismCloseSession(hSession);
    }

    if (pFeatureInfo)
    {
        DismDelete(pFeatureInfo);
    }

    if (bInitialized)
    {
        DismShutdown();
    }

    return bInstalled;
}

int IsUSBPolicyNotConfigured()
{
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD cbData = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    BOOL bConfigured = FALSE;
    LONG lResult;

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Policies\\Microsoft\\Windows NT\\Terminal Services", 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegQueryValueExW(hKey, L"fDisablePNPRedir", 0, &dwType, (LPBYTE)&dwValue, &cbData);
        bConfigured = (lResult == ERROR_SUCCESS);
        RegCloseKey(hKey);
    }

    return !bConfigured;
}

int RDConnectionBrokerUpgradeCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    HRESULT hr;
    ULONG ulCount = 0;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        GetRdConnectionBrokerCount(&ulCount);
        CoUninitialize();
    }

    return 1;
}

int RdsUpgradeComplianceCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    int bValue1 = 0;
    int bValue2 = 0;
    int bValue3 = 0;
    int bValue4 = 0;
    OSVERSIONINFOEXW osvi = {0};
    DWORDLONG dwlConditionMask = 0;
    BOOL bResult;

    CheckKeyValuePresent((HKEY)(L"TSAppCompat"), (wchar_t*)&bValue1, NULL, NULL);

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 0;
    osvi.wServicePackMajor = 0;
    osvi.wServicePackMinor = 0;
    osvi.wProductType = VER_NT_WORKSTATION;

    dwlConditionMask = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);
    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    bResult = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR | VER_PRODUCT_TYPE, dwlConditionMask);

    if (!bResult)
    {
        CheckKeyValuePresent(NULL, (wchar_t*)&bValue2, NULL, NULL);
        CheckKeyValuePresent(NULL, (wchar_t*)&bValue3, NULL, (int*)&bValue2);
        CheckKeyValuePresent(NULL, (wchar_t*)&bValue4, NULL, (int*)&bValue3);

        if (bValue1 == 0 && bValue2 == 0 && bValue3 == 0 && bValue4 == 0)
        {
            goto Success;
        }

        pCallback(pContext, 0, 0x32C9);
    }

    if (bValue1 != 0)
    {
        if (IsUSBPolicyNotConfigured())
        {
            pCallback(pContext, 1, 0x32CC);
        }
    }

Success:
    return 1;
}

int TelnetServerUpgradeComplianceCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    if (IsTelnetServerInstalled())
    {
        pCallback(pContext, 0, 0x32CA);
    }

    return 1;
}

