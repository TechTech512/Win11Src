// comserver.cpp
#include "precomp.h"
#include <new>

// Use the standard Windows IIDs
const IID IID_IUnknown = __uuidof(IUnknown);
const IID IID_IClassFactory = __uuidof(IClassFactory);

// Forward declarations
extern "C" {
    long g_cLockCount;
    void LOCK();
    void UNLOCK();
    HRESULT QISearch(void* pThis, REFIID riid, const struct QITAB* pqit, void** ppv);
}

// Define the missing symbols
extern "C" {
    // {AAAD6F87-9F5E-4A20-9CD7-8E2682315FB4}
    const GUID CLSID_DefaultDeviceManager = 
        {0xaaad6f87, 0x9f5e, 0x4a20, {0x9c, 0xd7, 0x8e, 0x26, 0x82, 0x31, 0x5f, 0xb4}};
    
    struct QITAB {
        const IID* piid;
        int offset;
        int flags;
    };
    
    // Define QITAB table for TClassFactory
    const QITAB qitClassFactory[] = {
        {&IID_IUnknown, 0, 0},
        {&IID_IClassFactory, 0, 0},
        {NULL, 0, 0}
    };
    
    // Critical section for thread safety
    CRITICAL_SECTION g_csLock;
    BOOL g_bCsInitialized = FALSE;
    
    void LOCK() {
        if (!g_bCsInitialized) {
            InitializeCriticalSection(&g_csLock);
            g_bCsInitialized = TRUE;
        }
        EnterCriticalSection(&g_csLock);
    }
    
    void UNLOCK() {
        if (g_bCsInitialized) {
            LeaveCriticalSection(&g_csLock);
        }
    }
    
    // QISearch implementation
    HRESULT QISearch(void* pThis, REFIID riid, const QITAB* pqit, void** ppv) {
        if (ppv == NULL) return E_INVALIDARG;
        
        *ppv = NULL;
        
        for (; pqit->piid != NULL; pqit++) {
            if (IsEqualIID(*pqit->piid, riid)) {
                *ppv = (void*)((BYTE*)pThis + pqit->offset);
                ((IUnknown*)*ppv)->AddRef();
                return S_OK;
            }
        }
        
        return E_NOINTERFACE;
    }
}

class TClassFactory : public IClassFactory
{
private:
    ULONG m_refCount;

public:
    TClassFactory() : m_refCount(1) {}
    
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    
    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppv) override;
    STDMETHOD(LockServer)(BOOL fLock) override;
};

class TDefaultsManager : public IUnknown
{
private:
    ULONG m_refCount;

public:
    TDefaultsManager() : m_refCount(1) {}
    
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    
    // Custom methods
    STDMETHOD(CreateDefaultDeviceProvider)(REFGUID rguid, void** ppProvider);
};

extern "C" HRESULT STDAPICALLTYPE DllCanUnloadNow()
{
    return (g_cLockCount != 0) ? S_FALSE : S_OK;
}

extern "C" HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (ppv == NULL) {
        return E_INVALIDARG;
    }
    
    *ppv = NULL;
    
    // Check if the requested CLSID matches our supported class
    if (rclsid != CLSID_DefaultDeviceManager) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    
    // Create class factory
    TClassFactory* pClassFactory = new (std::nothrow) TClassFactory();
    if (pClassFactory == NULL) {
        return E_OUTOFMEMORY;
    }
    
    // Increment lock count
    LOCK();
    g_cLockCount++;
    UNLOCK();
    
    // Query for requested interface
    HRESULT hr = pClassFactory->QueryInterface(riid, ppv);
    pClassFactory->Release();
    
    return hr;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInstance);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        if (g_bCsInitialized) {
            DeleteCriticalSection(&g_csLock);
            g_bCsInitialized = FALSE;
        }
    }
    return TRUE;
}

// TClassFactory implementation
STDMETHODIMP TClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    return QISearch(this, riid, qitClassFactory, ppv);
}

STDMETHODIMP_(ULONG) TClassFactory::AddRef()
{
    LOCK();
    ULONG refCount = ++m_refCount;
    UNLOCK();
    return refCount;
}

STDMETHODIMP_(ULONG) TClassFactory::Release()
{
    LOCK();
    ULONG refCount = --m_refCount;
    UNLOCK();
    
    if (refCount == 0) {
        LOCK();
        g_cLockCount--;
        UNLOCK();
        ::operator delete(this);
    }
    
    return refCount;
}

STDMETHODIMP TClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (ppv == NULL) {
        return E_INVALIDARG;
    }
    
    *ppv = NULL;
    
    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }
    
    // Create defaults manager
    TDefaultsManager* pDefaultsManager = new (std::nothrow) TDefaultsManager();
    if (pDefaultsManager == NULL) {
        return E_OUTOFMEMORY;
    }
    
    // Increment lock count
    LOCK();
    g_cLockCount++;
    UNLOCK();
    
    // Query for requested interface
    HRESULT hr = pDefaultsManager->QueryInterface(riid, ppv);
    pDefaultsManager->Release();
    
    return hr;
}

STDMETHODIMP TClassFactory::LockServer(BOOL fLock)
{
    LOCK();
    if (fLock) {
        g_cLockCount++;
    } else {
        g_cLockCount--;
    }
    UNLOCK();
    return S_OK;
}

