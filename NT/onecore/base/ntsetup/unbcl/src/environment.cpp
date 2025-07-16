// environment.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <atlbase.h>
#include <comdef.h>
#include <wbemidl.h>
#include <strsafe.h>
using namespace ATL;

#pragma comment(lib, "wbemuuid.lib")

HRESULT pExecuteWmiQuery(IEnumWbemClassObject** ppEnumerator, const wchar_t* query)
{
    if (!ppEnumerator || !query)
        return E_INVALIDARG;

    *ppEnumerator = nullptr;

    HRESULT hr;
    CComPtr<IWbemLocator> pLoc;
    CComPtr<IWbemServices> pSvc;

    // Initialize COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return hr;

    // Set general COM security
    hr = CoInitializeSecurity(
        NULL,
        -1,                          // COM negotiates service
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
    );
    if (FAILED(hr) && hr != RPC_E_TOO_LATE) {
        CoUninitialize();
        return hr;
    }

    // Obtain the initial locator
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        CoUninitialize();
        return hr;
    }

    // Connect to WMI namespace
    CComBSTR ns(L"ROOT\\CIMV2");
    hr = pLoc->ConnectServer(ns, NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
    if (FAILED(hr)) {
        CoUninitialize();
        return hr;
    }

    // Set security levels on the WMI proxy
    hr = CoSetProxyBlanket(
        pSvc,                        // The proxy to set
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,                        // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,                        // Client identity
        EOAC_NONE
    );
    if (FAILED(hr)) {
        CoUninitialize();
        return hr;
    }

    // Perform the WQL query
    CComPtr<IEnumWbemClassObject> pEnumerator;
    CComBSTR wqlQuery(query);
    hr = pSvc->ExecQuery(
        CComBSTR(L"WQL"),
        wqlQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (SUCCEEDED(hr)) {
        pEnumerator.p;  // transfer ownership
    }

    CoUninitialize();
    return hr;
}

