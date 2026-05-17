#include <windows.h>
#include <unknwn.h>
#include "errorredirector.h"

// External declarations
extern "C" const IID IID_IUnknown;
extern "C" const IID IID_IClassFactory;

long g_cRef = 0;

class CClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    
    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
    STDMETHOD(LockServer)(BOOL fLock) override;
    long m_cRef;
};

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    ULONG cRef;
    
    m_cRef++;
    cRef = m_cRef;
    
    return cRef;
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_INVALIDARG;
    }
    
    *ppvObject = nullptr;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObject = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    ULONG cRef;
    
    cRef = --m_cRef;
    
    if (cRef == 0)
    {
        g_cRef--;
        return 0;
    }
    
    return cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_INVALIDARG;
    }
    
    *ppvObject = nullptr;
    
    if (pUnkOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }
    
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

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        g_cRef++;
    }
    else
    {
        g_cRef--;
    }
    
    return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
    return (g_cRef > 0) ? S_FALSE : S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (ppv == nullptr)
    {
        return E_INVALIDARG;
    }
    
    *ppv = nullptr;
    
    if (!IsEqualCLSID(rclsid, CErrorRedirector::m_MyCLSID))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    
    CClassFactory* pFactory = new CClassFactory();
    if (pFactory == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    
    pFactory->m_cRef = 1;
    
    g_cRef++;
    
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    
    return hr;
}

