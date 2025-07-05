#include <windows.h>
#include <objbase.h>
#include <unknwn.h>  // For IUnknown
#include <initguid.h> // To allow GUID definition
#pragma comment(lib, "ole32.lib")

// --------------------
// Mock COM GUIDs
// --------------------

DEFINE_GUID(CLSID_WelcomeApp, 
0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

DEFINE_GUID(IID_IWelcomeApp, 
0x87654321, 0x4321, 0x4321, 0x43, 0x21, 0x10, 0xfe, 0xdc, 0xba, 0x98, 0x76);

// --------------------
// IWelcomeApp Interface
// --------------------

struct IWelcomeApp : public IUnknown {
    virtual HRESULT __stdcall ShowWelcome(LPCWSTR appName, int opt1, int opt2) = 0;
};

// --------------------
// Global reference count for the DLL
// --------------------

static LONG g_dllReferenceCount = 0;

// --------------------
// Simulated factory entry
// --------------------

struct FactoryEntry {
    const CLSID* clsid;
    IClassFactory* factory;
};

// Dummy factory for demonstration
IClassFactory* g_pFactory = nullptr;

FactoryEntry g_factoryTable[] = {
    { &CLSID_WelcomeApp, g_pFactory },
    { nullptr, nullptr }
};

// --------------------
// External mock functions
// --------------------

HRESULT InitWelcomeApp()
{
    // Stub for initialization
    return S_OK;
}

void ShutdownWelcomeApp()
{
    // Stub for cleanup
}

// --------------------
// DLL Exported COM Functions
// --------------------

STDAPI DllCanUnloadNow(void)
{
    return (g_dllReferenceCount == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    for (FactoryEntry* entry = g_factoryTable; entry->clsid != nullptr; ++entry) {
        if (IsEqualCLSID(rclsid, *entry->clsid)) {
            if (!entry->factory) {
                return CLASS_E_CLASSNOTAVAILABLE;
            }

            HRESULT hr = entry->factory->QueryInterface(riid, ppv);
            if (SUCCEEDED(hr)) {
                InterlockedIncrement(&g_dllReferenceCount);
            }
            return hr;
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

// --------------------
// Business Logic Entry
// --------------------

void ShowWelcomeCenter()
{
    if (SUCCEEDED(InitWelcomeApp())) {
        IWelcomeApp* pWelcomeApp = nullptr;

        HRESULT hr = CoCreateInstance(
            CLSID_WelcomeApp,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWelcomeApp,
            (void**)&pWelcomeApp
        );

        if (SUCCEEDED(hr) && pWelcomeApp) {
            pWelcomeApp->ShowWelcome(L"Microsoft.GettingStarted", 0, 0);
            pWelcomeApp->Release();
        }

        ShutdownWelcomeApp();
    }
}
