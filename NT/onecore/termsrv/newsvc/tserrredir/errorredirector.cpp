#include <windows.h>
#include "errorredirector.h"

const GUID CErrorRedirector::m_MyCLSID = 
    {0x0EB06E8D, 0x0D0A, 0x11DA, {0xA4, 0x6C, 0x00, 0x40, 0xF4, 0xB3, 0x32, 0x41}};

// External WinStation functions
extern "C" HRESULT WinStationRedirectLogonBeginPainting();
extern "C" HRESULT WinStationRedirectLogonError(
    long param2,
    wchar_t* param3,
    wchar_t* param4,
    UINT param5,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param6,
    UINT* param7
);
extern "C" HRESULT WinStationRedirectLogonMessage(
    wchar_t* param2,
    UINT param3,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param4,
    UINT* param5
);
extern "C" HRESULT WinStationRedirectLogonStatus(
    LOGON_ERROR_REDIRECTOR_RESPONSE* param2,
    UINT* param3
);

STDMETHODIMP_(ULONG) CErrorRedirector::AddRef()
{
    ULONG cRef;
    
    cRef = ++m_cRef;
    
    return cRef;
}

STDMETHODIMP CErrorRedirector::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_INVALIDARG;
    }
    
    *ppvObject = nullptr;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ILogonErrorRedirector))
    {
        *ppvObject = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CErrorRedirector::Release()
{
    ULONG cRef;
    
    cRef = --m_cRef;
    
    if (cRef == 0)
    {
        --g_cRef;
        return 0;
    }
    
    return cRef;
}

HRESULT CErrorRedirector::ConvertResponse(UINT* pResponse)
{
    if (pResponse == nullptr)
    {
        return E_INVALIDARG;
    }

    switch (*pResponse)
    {
        case 0:
            *pResponse = 0;
            break;
        case 1:
            *pResponse = 1;
            break;
        case 2:
            *pResponse = 2;
            break;
        case 3:
            *pResponse = 3;
            break;
        case 4:
            *pResponse = 4;
            break;
        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT CErrorRedirector::GetInstance(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObject = nullptr;

    CErrorRedirector* pRedirector = new CErrorRedirector();
    if (pRedirector == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    pRedirector->m_cRef = 1;

    g_cRef++;

    HRESULT hr = pRedirector->QueryInterface(riid, ppvObject);
    pRedirector->Release();

    return hr;
}

HRESULT CErrorRedirector::IsActive()
{
    return S_FALSE;
}

HRESULT CErrorRedirector::OnBeginPainting()
{
    HRESULT hr = WinStationRedirectLogonBeginPainting();
    if (hr == 0x80004001) // E_NOTIMPL
    {
        return 0;
    }
    return hr;
}

HRESULT CErrorRedirector::RedirectLogonError(
    long param1,
    long param2,
    wchar_t* param3,
    wchar_t* param4,
    UINT param5,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param6,
    UINT* param7
)
{
    HRESULT hr = WinStationRedirectLogonError(param2, param3, param4, param5, param6, param7);
    
    if (hr != 0x80004001) // Not E_NOTIMPL
    {
        if (FAILED(hr))
        {
            return hr;
        }
        
        hr = ConvertResponse(param7);
        if (SUCCEEDED(hr))
        {
            return hr;
        }
    }
    
    *param7 = 1;
    return S_OK;
}

HRESULT CErrorRedirector::RedirectMessage(
    wchar_t* param1,
    wchar_t* param2,
    UINT param3,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param4,
    UINT* param5
)
{
    HRESULT hr = WinStationRedirectLogonMessage(param2, param3, param4, param5);
    
    if (hr != 0x80004001) // Not E_NOTIMPL
    {
        if (FAILED(hr))
        {
            return hr;
        }
        
        hr = ConvertResponse(param5);
        if (SUCCEEDED(hr))
        {
            return hr;
        }
    }
    
    *param5 = 1;
    return S_OK;
}

HRESULT CErrorRedirector::RedirectStatus(
    wchar_t* param1,
    LOGON_ERROR_REDIRECTOR_RESPONSE* param2,
    UINT* param3
)
{
    HRESULT hr = WinStationRedirectLogonStatus(param2, param3);
    
    if (hr != 0x80004001) // Not E_NOTIMPL
    {
        if (FAILED(hr))
        {
            return hr;
        }
        
        hr = ConvertResponse(param3);
        if (SUCCEEDED(hr))
        {
            return hr;
        }
    }
    
    *param3 = 1;
    return S_OK;
}

