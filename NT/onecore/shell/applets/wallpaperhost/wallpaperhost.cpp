#include <windows.h>
#include <objbase.h>

// {75048700-EF1F-11D0-9888-006097DEACF9}
const CLSID CLSID_ActiveDesktop = 
{0x75048700, 0xEF1F, 0x11D0, {0x98, 0x88, 0x00, 0x60, 0x97, 0xDE, 0xAC, 0xF9}};

// {7E2EA1C2-809E-4B09-9A0D-7A1B238263DA}
const IID IID_IActiveDesktop = 
{0x7E2EA1C2, 0x809E, 0x4B09, {0x9A, 0x0D, 0x7A, 0x1B, 0x23, 0x82, 0x63, 0xDA}};

// Minimal IActiveDesktop interface with only the methods we need
struct IActiveDesktop : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE ApplyChanges(DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWallpaper(wchar_t* pwszWallpaper, DWORD cchWallpaper, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetWallpaper(const wchar_t* pwszWallpaper, DWORD dwReserved) = 0;
};

// Define the constant needed for ApplyChanges
#define AD_APPLY_ALL 0x00000001

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HRESULT hr;
    IActiveDesktop* pActiveDesktop = NULL;
    int iReturn = 1;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    if (hr == RPC_E_CHANGED_MODE) {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (hr == RPC_E_CHANGED_MODE) {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_ActiveDesktop, 
                             NULL, 
                             CLSCTX_INPROC_SERVER, 
                             IID_IActiveDesktop, 
                             (void**)&pActiveDesktop);
        
        if (SUCCEEDED(hr) && pActiveDesktop) {
            pActiveDesktop->ApplyChanges(AD_APPLY_ALL);
            pActiveDesktop->Release();
        }
        
        CoUninitialize();
    }

    return iReturn;
}

