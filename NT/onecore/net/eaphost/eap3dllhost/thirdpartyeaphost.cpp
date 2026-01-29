#include "precompiled.h"

struct SC_HANDLE__ *schSCManager;

static const GUID CLSID_AuthRuntime = 
{ 0xB0E28D63, 0x52F6, 0x4E30, { 0x99, 0x2B, 0x78, 0xEC, 0xF9, 0x72, 0x68, 0xE9 } };

static const GUID CLSID_PeerRuntime = 
{ 0x87BB326B, 0xE4A0, 0x4DE1, { 0x94, 0xF0, 0xB9, 0xF4, 0x1D, 0x0C, 0x60, 0x59 } };

static const GUID IID_AuthRuntime = 
{ 0x9DAA7B9D, 0xCE5B, 0x42CE, { 0xB9, 0x42, 0x32, 0xBB, 0xC2, 0x84, 0xAC, 0x44 } };

static const GUID IID_PeerRuntime = 
{ 0xC48CA462, 0x67FB, 0x4C12, { 0xA2, 0x1A, 0x64, 0x15, 0x46, 0x0F, 0xA8, 0xAE } };

static const GUID IID_IUnknown = 
{ 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

static const GUID IID_IClassFactory = 
{ 0x00000001, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

static const GUID IID_IMarshal = 
{ 0x00000003, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

static const GUID IID_ISurrogate = 
{ 0x00000022, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

class SurrClassFactory;

class CoSurrogate : public ISurrogate
{
public:
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    
    // ISurrogate methods
    virtual HRESULT STDMETHODCALLTYPE LoadDllServer(REFCLSID rclsid) override;
    virtual HRESULT STDMETHODCALLTYPE FreeSurrogate() override;
};

// Define vtable structure
struct CoSurrogateVTable {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(void* This, REFIID riid, void **ppv);
    ULONG (STDMETHODCALLTYPE *AddRef)(void* This);
    ULONG (STDMETHODCALLTYPE *Release)(void* This);
    HRESULT (STDMETHODCALLTYPE *LoadDllServer)(void* This, REFCLSID rclsid);
};

// Static vtable instance
static CoSurrogateVTable CoSurrogate_vftable = {
    NULL, NULL, NULL, NULL
};

// Helper static functions that call the actual class methods
static HRESULT STDMETHODCALLTYPE CoSurrogate_QueryInterface_Thunk(void* This, REFIID riid, void **ppv) {
    return ((CoSurrogate*)This)->QueryInterface(riid, ppv);
}

static ULONG STDMETHODCALLTYPE CoSurrogate_AddRef_Thunk(void* This) {
    return ((CoSurrogate*)This)->AddRef();
}

static ULONG STDMETHODCALLTYPE CoSurrogate_Release_Thunk(void* This) {
    return ((CoSurrogate*)This)->Release();
}

static HRESULT STDMETHODCALLTYPE CoSurrogate_LoadDllServer_Thunk(void* This, REFCLSID rclsid) {
    return ((CoSurrogate*)This)->LoadDllServer(rclsid);
}

// Initialize vtable
void InitCoSurrogateVTable() {
    CoSurrogate_vftable.QueryInterface = CoSurrogate_QueryInterface_Thunk;
    CoSurrogate_vftable.AddRef = CoSurrogate_AddRef_Thunk;
    CoSurrogate_vftable.Release = CoSurrogate_Release_Thunk;
    CoSurrogate_vftable.LoadDllServer = CoSurrogate_LoadDllServer_Thunk;
}

class SurrClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    
    // IClassFactory methods
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) override;
    virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;
};

DWORD getEaphostProcessId(void)
{
    SC_HANDLE hService = OpenServiceW(schSCManager, L"EapHost", SERVICE_QUERY_STATUS);
    if (hService)
    {
        BYTE buffer[0x24];
        DWORD bytesNeeded;
        if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, buffer, sizeof(buffer), &bytesNeeded))
        {
            SERVICE_STATUS_PROCESS *ssp = (SERVICE_STATUS_PROCESS*)buffer;
            DWORD pid = ssp->dwProcessId;
            CloseServiceHandle(hService);
            return pid;
        }
        CloseServiceHandle(hService);
    }
    return 0xFFFFFFFF;
}

void operator_delete(void *ptr)
{
    free(ptr);
}

void *operator_new(size_t size)
{
    return malloc(size);
}

ULONG CoSurrogate::AddRef()
{
    ULONG refCount = *(ULONG*)((BYTE*)this + 0xC);
    *(ULONG*)((BYTE*)this + 0xC) = refCount + 1;
    return refCount + 1;
}

ULONG SurrClassFactory::AddRef()
{
    ULONG refCount = *(ULONG*)((BYTE*)this + 0xC);
    *(ULONG*)((BYTE*)this + 0xC) = refCount + 1;
    return refCount + 1;
}

HRESULT SurrClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, IID_PeerRuntime))
    {
        IUnknown **ppFactory = (IUnknown**)((BYTE*)this + 4);
        if (!*ppFactory)
        {
            HRESULT hr = CoGetClassObject(CLSID_PeerRuntime, CLSCTX_INPROC_SERVER, NULL, IID_IUnknown, (void**)ppFactory);
            if (FAILED(hr))
                return REGDB_E_CLASSNOTREG;
        }
        return (*ppFactory)->QueryInterface(riid, ppv);
    }
    else if (IsEqualGUID(riid, IID_AuthRuntime))
    {
        IUnknown **ppFactory = (IUnknown**)((BYTE*)this + 8);
        if (!*ppFactory)
        {
            HRESULT hr = CoGetClassObject(CLSID_AuthRuntime, CLSCTX_INPROC_SERVER, NULL, IID_IUnknown, (void**)ppFactory);
            if (FAILED(hr))
                return REGDB_E_CLASSNOTREG;
        }
        return (*ppFactory)->QueryInterface(riid, ppv);
    }
    return E_NOINTERFACE;
}

HRESULT CoSurrogate::FreeSurrogate()
{
    HRESULT hr = CoRevokeClassObject(*(DWORD*)((BYTE*)this + 8));
    PostQuitMessage(0);
    SetEvent(*(HANDLE*)((BYTE*)this + 0x10));
    return hr;
}

HRESULT CoSurrogate::LoadDllServer(REFCLSID rclsid)
{
    SurrClassFactory *pFactory = (SurrClassFactory*)operator_new(0x10);
    if (!pFactory)
        return E_OUTOFMEMORY;

    *(void**)pFactory = NULL; // vftable will be set by caller
    *(IUnknown**)((BYTE*)pFactory + 4) = NULL;
    *(IUnknown**)((BYTE*)pFactory + 8) = NULL;
    *(ULONG*)((BYTE*)pFactory + 0xC) = 1;

    DWORD dwReg;
    HRESULT hr = CoRegisterClassObject(rclsid, pFactory, CLSCTX_INPROC_SERVER, REGCLS_SURROGATE, &dwReg);
    *(DWORD*)((BYTE*)this + 8) = dwReg;
    return hr;
}

HRESULT SurrClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

HRESULT CoSurrogate::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IMarshal))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    else if (IsEqualGUID(riid, IID_ISurrogate))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

HRESULT SurrClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IClassFactory))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG CoSurrogate::Release()
{
    ULONG refCount = *(ULONG*)((BYTE*)this + 0xC);
    *(ULONG*)((BYTE*)this + 0xC) = refCount - 1;

    if (refCount == 1)
    {
        operator_delete(this);
    }
    return refCount - 1;
}

ULONG SurrClassFactory::Release()
{
    IUnknown *pPeer = *(IUnknown**)((BYTE*)this + 4);
    if (pPeer)
    {
        pPeer->Release();
        *(IUnknown**)((BYTE*)this + 4) = NULL;
    }
    
    IUnknown *pAuth = *(IUnknown**)((BYTE*)this + 8);
    if (pAuth)
    {
        pAuth->Release();
        *(IUnknown**)((BYTE*)this + 8) = NULL;
    }
    
    ULONG refCount = *(ULONG*)((BYTE*)this + 0xC);
    *(ULONG*)((BYTE*)this + 0xC) = refCount - 1;
    
    if (refCount == 1)
    {
        // Clear vftable
        *(void**)this = NULL;
        operator_delete(this);
    }
    
    return refCount - 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    char *cmdLine = lpCmdLine;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    int strLen = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, NULL, 0);
    int allocSize = strLen * 2 + 2;
    
    LPWSTR wideCmdLine = (LPWSTR)CoTaskMemAlloc(allocSize);
    if (wideCmdLine == NULL) {
        goto cleanup;
    }
    
    schSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        DWORD lastError = GetLastError();
        printf("OpenSCManager failed (%d)\n", lastError);
    } else {
        DWORD eaphostPid = getEaphostProcessId();
        if (eaphostPid != 0xFFFFFFFF) {
            memset(wideCmdLine, 0, allocSize);
            GUID clsid;
            int converted = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, wideCmdLine, strLen);
            if (converted != 0) {
                LPWSTR spacePos = (LPWSTR)wcschr(wideCmdLine, L' ');
                if (spacePos != NULL) {
                    *spacePos = L'\0';
                }
                
                hr = CLSIDFromString(wideCmdLine, &clsid);
                if (FAILED(hr)) {
                    goto cleanup;
                }
            }
            
            hr = CoInitializeSecurity(&clsid, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, 
                                      RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
            if (FAILED(hr)) {
                goto cleanup;
            }
            
            // Initialize vtable
            InitCoSurrogateVTable();
            
            CoSurrogate *pSurrogate = (CoSurrogate*)operator_new(0x14);
            if (pSurrogate == NULL) {
                pSurrogate = NULL;
            } else {
                // Set vtable pointer
                *(CoSurrogateVTable**)pSurrogate = &CoSurrogate_vftable;
                // Initialize ref count to 1
                *(ULONG*)((BYTE*)pSurrogate + 0xC) = 1;
                HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
                *(HANDLE*)((BYTE*)pSurrogate + 0x10) = hEvent;
            }
            
            if (pSurrogate != NULL) {
                hr = CoRegisterSurrogate((ISurrogate*)pSurrogate);
                if (FAILED(hr)) {
                    goto cleanup;
                }
                
                hr = pSurrogate->LoadDllServer(clsid);
                DWORD loadResult = hr;
                
                if (FAILED(hr)) {
                    goto cleanup;
                }
                
                BOOL continueRunning = TRUE;
                HANDLE *pEventHandle = (HANDLE*)((BYTE*)pSurrogate + 0x10);
                
                do {
                    DWORD waitResult = MsgWaitForMultipleObjects(1, pEventHandle, FALSE, 60000, QS_ALLEVENTS);
                    
                    if (waitResult == WAIT_OBJECT_0) {
                        continueRunning = FALSE;
                    } else if (waitResult == WAIT_OBJECT_0 + 1) {
                        MSG msg;
                        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                            if (msg.message == WM_QUIT) {
                                continueRunning = FALSE;
                                break;
                            }
                            TranslateMessage(&msg);
                            DispatchMessageW(&msg);
                        }
                    } else if (waitResult == WAIT_TIMEOUT) {
                        DWORD newPid = getEaphostProcessId();
                        if (eaphostPid != newPid) {
                            continueRunning = FALSE;
                        }
                    }
                } while (continueRunning);
                
                pSurrogate->FreeSurrogate();
                pSurrogate->Release();
            }
            
            CoUninitialize();
            goto cleanup;
        }
    }
    
    hr = S_OK;

cleanup:
    return hr;
}

