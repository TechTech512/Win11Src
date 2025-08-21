#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <shellapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")

// CLSID and IID definitions for HxHelpPane
const CLSID CLSID_HxHelpPane = {0x8cec5860, 0x07a1, 0x11d9, {0xb1, 0x5e, 0x00, 0x0d, 0x56, 0xbf, 0xe6, 0xee}};
const IID IID_IHxHelpPane = {0x8cec5884, 0x07a1, 0x11d9, {0xb1, 0x5e, 0x00, 0x0d, 0x56, 0xbf, 0xe6, 0xee}};

BOOL TryHelpPaneCOM()
{
    IUnknown* pHelpPane = NULL;
    HRESULT hr = S_OK;
    BSTR bstrUrl = NULL;
    BOOL success = FALSE;

    // Create instance of HxHelpPane
    hr = CoCreateInstance(CLSID_HxHelpPane,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IHxHelpPane,
        (void**)&pHelpPane);

    if (SUCCEEDED(hr) && pHelpPane != NULL)
    {
        // Create BSTR for the help URL
        bstrUrl = SysAllocString(L"mshelp://Windows/?id=e513b1b3-a3c8-44e8-ab5a-d6f8e4fc5851");
        if (bstrUrl != NULL)
        {
            // Direct vtable call at offset 0xC (3rd method)
            typedef HRESULT(__stdcall* METHOD_0C)(IUnknown*, BSTR);
            METHOD_0C pMethod = (METHOD_0C)(*(void***)pHelpPane)[3];

            if (pMethod)
            {
                hr = pMethod(pHelpPane, bstrUrl);
                success = SUCCEEDED(hr);
            }

            SysFreeString(bstrUrl);
        }
        pHelpPane->Release();
    }

    return success;
}

BOOL TryShellExecuteFallback()
{
    // Try opening the help URL directly using ShellExecute
    HINSTANCE result = ShellExecuteW(
        NULL,
        L"open",
        L"mshelp://Windows/?id=e513b1b3-a3c8-44e8-ab5a-d6f8e4fc5851",
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    return ((INT_PTR)result > 32);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        // If COM fails, try ShellExecute directly
        return TryShellExecuteFallback() ? 0 : 1;
    }

    BOOL success = FALSE;

    // First try the COM approach (original method)
    success = TryHelpPaneCOM();

    // If COM approach failed, fall back to ShellExecute
    if (!success)
    {
        success = TryShellExecuteFallback();
    }

    CoUninitialize();
    return success ? 0 : 1;
}

