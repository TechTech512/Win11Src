#include "precomp.h"
#include "resource.h"

// Forward declare interfaces we need
struct IWiaItem;
struct IWiaPropertyStorage;
struct IWiaDevMgr;
struct IWiaDevMgr2;
struct IStiDevice;
struct IStillImageW;
struct IPhotoAcquireDeviceSelectionDialog;
struct IWiaPropHelp;

// GUID definitions
const CLSID CLSID_WiaDevMgr = 
{0x5eb2502a, 0x8cf1, 0x11d1, {0xbf, 0x92, 0x00, 0x60, 0x08, 0x1e, 0xd8, 0x11}};

const CLSID CLSID_WiaDevMgr2 = 
{0x79c07cf1, 0xcbdd, 0x41ee, {0x8e, 0xc3, 0xf0, 0x00, 0x80, 0xca, 0xda, 0x7a}};

const CLSID CLSID_PhotoAcquireDeviceSelectionDialog = 
{0x00f28837, 0x55dd, 0x4f37, {0xaa, 0xf5, 0x68, 0x55, 0xa9, 0x64, 0x04, 0x67}};

const CLSID CLSID_WiaPropHelp = 
{0x7eed2e9b, 0xacda, 0x11d2, {0x80, 0x80, 0x00, 0x80, 0x5f, 0x65, 0x96, 0xd2}};

const IID IID_IWiaDevMgr = 
{0x5eb2502a, 0x8cf1, 0x11d1, {0xbf, 0x92, 0x00, 0x60, 0x08, 0x1e, 0xd8, 0x11}};

const IID IID_IWiaDevMgr2 = 
{0x79c07cf1, 0xcbdd, 0x41ee, {0x8e, 0xc3, 0xf0, 0x00, 0x80, 0xca, 0xda, 0x7a}};

const IID IID_IWiaPropertyStorage = 
{0x98b5e8a0, 0x29cc, 0x491a, {0xaa, 0xc0, 0xe6, 0xdb, 0x4f, 0xdc, 0xce, 0xb6}};

const IID IID_IStillImageW = 
{0x641BD880, 0x2DC8, 0x11D0, {0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C}};

const IID IID_IPhotoAcquireDeviceSelectionDialog = 
{0x00f28837, 0x55dd, 0x4f37, {0xaa, 0xf5, 0x68, 0x55, 0xa9, 0x64, 0x04, 0x67}};

const IID IID_IWiaPropHelp = 
{0x7062368c, 0x9017, 0x49c6, {0xa0, 0x7b, 0x5c, 0xd3, 0xea, 0xb0, 0xde, 0x7d}};

// STI constants
#define STI_VERSION_FLAG_UNICODE    0x00000002
#define STI_DEVICE_CREATE_STATUS    0x00000001

// Minimal interface definitions with only the methods we actually use
struct IWiaDevMgr : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE EnumDeviceInfo(LONG lFlags, void** ppIEnum) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(BSTR bstrDeviceID, IWiaItem** ppWiaItemRoot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SelectDeviceDlg(HWND hwndParent, LONG lDeviceType, LONG lFlags, BSTR* pbstrDeviceID, IWiaItem** ppWiaItemRoot) = 0;
};

struct IWiaDevMgr2 : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE EnumDeviceInfo(LONG lFlags, void** ppIEnum) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(BSTR bstrDeviceID, IWiaItem** ppWiaItemRoot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SelectDeviceDlgID(HWND hwndParent, LONG lDeviceType, LONG lFlags, BSTR* pbstrDeviceID, IWiaItem** ppWiaItemRoot) = 0;
};

struct IWiaItem : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetItemType(LONG* pItemType) = 0;
    virtual HRESULT STDMETHODCALLTYPE AnalyzeItem(LONG lFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumChildItems(void** ppIEnumWiaItem) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteItem(LONG lFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeviceDlg(HWND hwndParent, LONG lFlags, LONG lIntent, LONG* plItemCount, IWiaItem*** pppIWiaItem) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeviceCommand(LONG lFlags, const GUID* pCmdGUID, IWiaItem** ppIWiaItem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRootItem(IWiaItem** ppIWiaItemRoot) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumDeviceCapabilities(LONG lFlags, void** ppIEnumWIA_DEV_CAP) = 0;
};

struct IWiaPropertyStorage : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgpropvar[]) = 0;
    virtual HRESULT STDMETHODCALLTYPE WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgpropvar[], PROPID propidNameFirst) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[]) = 0;
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Revert() = 0;
    virtual HRESULT STDMETHODCALLTYPE Enum(void** ppenum) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClass(REFCLSID clsid) = 0;
    virtual HRESULT STDMETHODCALLTYPE Stat(void* pstatpsstg) = 0;
};

struct IPhotoAcquireDeviceSelectionDialog : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetTitle(LPCWSTR pszTitle) = 0;
    virtual HRESULT STDMETHODCALLTYPE DoModal(HWND hwndParent) = 0;
};

struct IWiaPropHelp : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetWiaPropertySheetPages(IWiaPropertyStorage* pPropStorage, PROPSHEETHEADERW* ppsh) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertySheetPages(IStiDevice* pStiDevice, PROPSHEETHEADERW* ppsh) = 0;
};

struct IStiDevice : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetCapabilities(void* pDevCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStatus(void* pDevStatus) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeviceReset() = 0;
    virtual HRESULT STDMETHODCALLTYPE Diagnostic(void* pDiag) = 0;
    virtual HRESULT STDMETHODCALLTYPE Escape(DWORD EscapeFunction, LPBYTE lpInData, DWORD cbInDataSize, LPBYTE lpOutData, DWORD dwOutDataSize, LPDWORD lpdwActualData) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLastError(LPDWORD pdwLastDeviceError) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockDevice(DWORD dwTimeOut) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnLockDevice() = 0;
};

struct IStillImageW : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Initialize() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceList(DWORD dwType, DWORD dwFlags, DWORD* pdwItemsReturned, BYTE** ppBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceInfo(LPCWSTR pwszDeviceName, BYTE** ppBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDevice(LPCWSTR pwszDeviceName, DWORD dwMode, IStiDevice** ppDevice, IUnknown* pUnkOuter) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceValue(LPCWSTR pwszDeviceName, LPCWSTR pwszValueName, DWORD* pType, BYTE* pData, DWORD* pcbData) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceValue(LPCWSTR pwszDeviceName, LPCWSTR pwszValueName, DWORD Type, BYTE* pData, DWORD cbData) = 0;
};

// STI function prototype
typedef HRESULT (WINAPI* PFNStiCreateInstanceW)(HINSTANCE hInst, DWORD dwVersion, IStillImageW** ppSti, IUnknown* pUnkOuter);

HINSTANCE g_hInstance;

enum EPropertiesDeviceType
{
    DEVICE_TYPE_SCANNER = 1,
    DEVICE_TYPE_CAMERA = 2,
    DEVICE_TYPE_VIDEO = 3
};

struct DeviceTypeInfo
{
    LPCWSTR pszSwitch;
    EPropertiesDeviceType deviceType;
};

DeviceTypeInfo kaDeviceTypes[] = 
{
    {L"Scanner", DEVICE_TYPE_SCANNER},
    {L"Camera", DEVICE_TYPE_CAMERA},
    {L"Video", DEVICE_TYPE_VIDEO}
};

HRESULT CreateWiaDevice(EPropertiesDeviceType deviceType, wchar_t* deviceId, GUID* pInterfaceID, void** ppDevice)
{
    HRESULT hr = E_FAIL;
    IWiaDevMgr* pWiaDevMgr = NULL;
    IWiaDevMgr2* pWiaDevMgr2 = NULL;
    IWiaItem* pWiaItem = NULL;
    IWiaPropertyStorage* pPropStorage = NULL;

    if (deviceType == DEVICE_TYPE_SCANNER)
    {
        hr = CoCreateInstance(CLSID_WiaDevMgr2, NULL, CLSCTX_INPROC_SERVER, IID_IWiaDevMgr2, (void**)&pWiaDevMgr2);
        if (SUCCEEDED(hr) && pWiaDevMgr2)
        {
            BSTR bstrDeviceId = SysAllocString(deviceId);
            hr = pWiaDevMgr2->CreateDevice(bstrDeviceId, &pWiaItem);
            SysFreeString(bstrDeviceId);
            
            if (SUCCEEDED(hr) && pWiaItem)
            {
                hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**)&pPropStorage);
                pWiaItem->Release();
            }
        }
        
        if (pWiaDevMgr2)
            pWiaDevMgr2->Release();
    }
    else
    {
        hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_INPROC_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr);
        if (SUCCEEDED(hr) && pWiaDevMgr)
        {
            BSTR bstrDeviceId = SysAllocString(deviceId);
            hr = pWiaDevMgr->CreateDevice(bstrDeviceId, &pWiaItem);
            SysFreeString(bstrDeviceId);
            
            if (SUCCEEDED(hr) && pWiaItem)
            {
                hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**)&pPropStorage);
                pWiaItem->Release();
            }
        }
        
        if (pWiaDevMgr)
            pWiaDevMgr->Release();
    }

    if (SUCCEEDED(hr))
    {
        *ppDevice = pPropStorage;
    }

    return hr;
}

HRESULT LaunchControlPanel(HWND hwndParent)
{
    HRESULT hr;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return hr;
    }

    // Try to create the STI device manager and show the device selection dialog
    IStillImageW* pSti = NULL;
    HINSTANCE hSti = LoadLibraryW(L"sti.dll");
    if (hSti)
    {
        PFNStiCreateInstanceW pfnStiCreateInstance = (PFNStiCreateInstanceW)GetProcAddress(hSti, "StiCreateInstanceW");
        if (pfnStiCreateInstance)
        {
            hr = pfnStiCreateInstance(g_hInstance, STI_VERSION_FLAG_UNICODE, &pSti, NULL);
            if (SUCCEEDED(hr) && pSti)
            {
                // This should trigger the Scanners and Cameras control panel
                // by attempting to create a device which typically shows the selection UI
                IStiDevice* pDevice = NULL;
                hr = pSti->CreateDevice(L"", STI_DEVICE_CREATE_STATUS, &pDevice, NULL);
                if (pDevice)
                    pDevice->Release();
                    
                pSti->Release();
            }
        }
        FreeLibrary(hSti);
    }

    CoUninitialize();
    
    // If COM approach failed, fall back to shell execution
    if (FAILED(hr))
    {
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(sei);
        sei.hwnd = hwndParent;
        sei.lpVerb = L"open";
        sei.lpFile = L"control.exe";
        sei.lpParameters = L"sticpl.cpl";
        sei.nShow = SW_SHOW;
        
        if (ShellExecuteExW(&sei))
        {
            return S_OK;
        }
    }
    
    return hr;
}

HRESULT ShowInstallWizard(HWND hwndParent)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hSti = LoadLibraryW(L"sti_ci.dll");
    
    if (hSti)
    {
        typedef HRESULT (WINAPI* PFN_AddDevice)(HWND, LPCSTR, DWORD);
        PFN_AddDevice pfnAddDevice = (PFN_AddDevice)GetProcAddress(hSti, "AddDevice");
        
        if (pfnAddDevice)
        {
            HWND hwndDesktop = GetDesktopWindow();
            hr = pfnAddDevice(hwndDesktop, "", 5);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        
        FreeLibrary(hSti);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    return hr;
}

HRESULT ShowWiaProperties(HWND hwndParent, EPropertiesDeviceType deviceType, wchar_t* deviceId)
{
    HRESULT hr;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return hr;
    }

    if (deviceType == DEVICE_TYPE_VIDEO)
    {
        IStillImageW* pSti = NULL;
        IStiDevice* pStiDevice = NULL;
        IWiaPropHelp* pWiaPropHelp = NULL;
        
        // Load STI and get CreateInstance function
        HINSTANCE hSti = LoadLibraryW(L"sti.dll");
        if (hSti)
        {
            PFNStiCreateInstanceW pfnStiCreateInstance = (PFNStiCreateInstanceW)GetProcAddress(hSti, "StiCreateInstanceW");
            if (pfnStiCreateInstance)
            {
                hr = pfnStiCreateInstance(g_hInstance, STI_VERSION_FLAG_UNICODE, &pSti, NULL);
                if (SUCCEEDED(hr) && pSti)
                {
                    BSTR bstrDeviceName = SysAllocString(deviceId);
                    hr = pSti->CreateDevice(bstrDeviceName, STI_DEVICE_CREATE_STATUS, &pStiDevice, NULL);
                    SysFreeString(bstrDeviceName);
                    
                    if (SUCCEEDED(hr) && pStiDevice)
                    {
                        hr = CoCreateInstance(CLSID_WiaPropHelp, NULL, CLSCTX_INPROC_SERVER, 
                                             IID_IWiaPropHelp, (void**)&pWiaPropHelp);
                        if (SUCCEEDED(hr) && pWiaPropHelp)
                        {
                            PROPSHEETHEADERW psh = {0};
                            psh.dwSize = sizeof(PROPSHEETHEADERW);
                            psh.hInstance = g_hInstance;
                            
                            hr = pWiaPropHelp->GetPropertySheetPages(pStiDevice, &psh);
                            if (SUCCEEDED(hr))
                            {
                                psh.dwFlags |= PSH_PROPTITLE | PSH_USEHICON | PSH_USECALLBACK;
                                psh.hwndParent = hwndParent;
                                int iResult = PropertySheetW(&psh);
                                if (iResult < 0)
                                {
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                }
                            }
                            
                            pWiaPropHelp->Release();
                        }
                        
                        pStiDevice->Release();
                    }
                    
                    pSti->Release();
                }
            }
            FreeLibrary(hSti);
        }
    }
    else
    {
        IWiaPropertyStorage* pPropStorage = NULL;
        hr = CreateWiaDevice(deviceType, deviceId, NULL, (void**)&pPropStorage);
        
        if (SUCCEEDED(hr) && pPropStorage)
        {
            PROPVARIANT pv[1];
            PROPSPEC ps[1];
            
            ps[0].ulKind = PRSPEC_PROPID;
            ps[0].propid = 8;
            
            hr = pPropStorage->ReadMultiple(1, ps, pv);
            if (SUCCEEDED(hr))
            {
                if (pv[0].vt == VT_I4 && pv[0].lVal != 0)
                {
                    IWiaPropHelp* pWiaPropHelp = NULL;
                    hr = CoCreateInstance(CLSID_WiaPropHelp, NULL, CLSCTX_INPROC_SERVER, 
                                         IID_IWiaPropHelp, (void**)&pWiaPropHelp);
                    if (SUCCEEDED(hr) && pWiaPropHelp)
                    {
                        PROPSHEETHEADERW psh = {0};
                        psh.dwSize = sizeof(PROPSHEETHEADERW);
                        
                        hr = pWiaPropHelp->GetWiaPropertySheetPages(pPropStorage, &psh);
                        if (SUCCEEDED(hr))
                        {
                            psh.dwFlags |= PSH_PROPTITLE | PSH_USEHICON | PSH_USECALLBACK;
                            psh.hwndParent = hwndParent;
                            int iResult = PropertySheetW(&psh);
                            if (iResult < 0)
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }
                        
                        pWiaPropHelp->Release();
                    }
                }
                else
                {
                    hr = E_UNEXPECTED;
                }
                
                PropVariantClear(pv);
            }
            
            pPropStorage->Release();
        }
        else if (hr == 0x80210005) // WIA_ERROR_BUSY
        {
            WCHAR szTitle[100];
            WCHAR szMessage[500];
            
            if (LoadStringW(g_hInstance, IDS_DIALOGTITLE, szTitle, 100) && 
                LoadStringW(g_hInstance, IDS_SCANNERUNAVAILABLE, szMessage, 500))
            {
                MessageBoxW(hwndParent, szMessage, szTitle, MB_OK | MB_ICONINFORMATION);
            }
        }
    }
    
    CoUninitialize();
    return hr;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    HWND hwndParent = NULL;
    BOOL bInstallWiaDevice = FALSE;
    HRESULT hr = S_OK;
    EPropertiesDeviceType deviceType = DEVICE_TYPE_SCANNER;
    
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (argv)
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == L'/' || argv[i][0] == L'-')
            {
                if (lstrcmpiW(argv[i] + 1, L"InstallWiaDevice") == 0)
                {
                    bInstallWiaDevice = TRUE;
                }
                else if (lstrcmpiW(argv[i] + 1, L"WindowHandle") == 0)
                {
                    if (i + 1 < argc)
                    {
                        i++;
                        hwndParent = (HWND)(INT_PTR)wcstoul(argv[i], NULL, 10);
                        if (!IsWindow(hwndParent))
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    for (int j = 0; j < 3; j++)
                    {
                        if (lstrcmpiW(argv[i] + 1, kaDeviceTypes[j].pszSwitch) == 0)
                        {
                            if (i + 1 < argc)
                            {
                                i++;
                                deviceType = kaDeviceTypes[j].deviceType;
                            }
                            else
                            {
                                hr = E_INVALIDARG;
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        if (SUCCEEDED(hr))
        {
            if (bInstallWiaDevice)
            {
                hr = ShowInstallWizard(hwndParent);
            }
            else if (hwndParent)
            {
                hr = ShowWiaProperties(hwndParent, deviceType, argc > 1 ? argv[1] : L"");
            }
            else
            {
                hr = LaunchControlPanel(hwndParent);
            }
        }
        
        LocalFree(argv);
    }
    
    return hr;
}

