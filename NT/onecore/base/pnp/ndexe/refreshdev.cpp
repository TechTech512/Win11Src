#include <windows.h>
#include <initguid.h>
#include <objbase.h>

// Define GUIDs if not already in dsm.h
#ifndef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern "C" const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

typedef enum {
    DSMRT_Normal = 0x0,
    DSMRT_Manual = 0x1,
    DSMRT_FullSetup = 0x2,
    DSMRT_LocalStore = 0x3,
    DSMRT_Postponed = 0x4,
    DSMRT_DriverPolicyChange = 0x5,
    DSMRT_MetadataPolicyChange = 0x6
} DSM_REFRESH_TYPE;

DEFINE_GUID(CLSID_DeviceManager, 0xe4785230, 0x0e43, 0x47dc, 0x82, 0x6a, 0x07, 0xdb, 0xc3, 0xaa, 0x63, 0xd8);
DEFINE_GUID(IID_IDeviceManager, 0x46147e3d, 0x0380, 0x450d, 0xb4, 0x8e, 0xcc, 0xa7, 0xf7, 0x7d, 0x1c, 0x69);

extern "C" void RefreshDevices(DSM_REFRESH_TYPE RefreshType)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return;
    }

    IUnknown* pDeviceManager = NULL;
    hr = CoCreateInstance(CLSID_DeviceManager,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IDeviceManager,
                         (void**)&pDeviceManager);

    if (SUCCEEDED(hr) && pDeviceManager) {
        // Call the Refresh method (vtable offset 0x1C)
        void (__stdcall *pRefreshFunc)(IUnknown*) = (void (__stdcall*)(IUnknown*))(*((void***)pDeviceManager)[7]);
        pRefreshFunc(pDeviceManager);
    }

    if (pDeviceManager) {
        pDeviceManager->Release();
    }

    CoUninitialize();
}

