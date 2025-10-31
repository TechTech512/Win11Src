#include <Windows.h>
#include <strsafe.h>
#include <stdlib.h>

int __cdecl wmain(int argc, wchar_t** argv)
{
    HMODULE hDevMgr;
    FARPROC pDevicePropertiesW;
    int result = 0;
    HWND hwnd = NULL;
    wchar_t* deviceInstanceId = NULL;
    wchar_t buffer[520] = {0};
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    if (argc == 2) {
        // Copy device instance ID
        if (FAILED(StringCchCopyW(argv[1], MAX_PATH, L""))) {
            goto cleanup;
        }
        deviceInstanceId = argv[1];
    } else if (argc == 3) {
        // Parse window handle and copy device instance ID
        hwnd = (HWND)_wcstoui64(argv[1], NULL, 0);
        if (FAILED(StringCchCopyW(argv[2], MAX_PATH, L""))) {
            goto cleanup;
        }
        deviceInstanceId = argv[2];
        
        // Validate window handle
        if (!IsWindow(hwnd)) {
            result = 0x80070578; // ERROR_INVALID_WINDOW_HANDLE
            goto cleanup;
        }
    } else {
        goto cleanup;
    }
    
    // Load DEVMGR.DLL and get DevicePropertiesW function
    hDevMgr = LoadLibraryW(L"DEVMGR.DLL");
    if (hDevMgr) {
        pDevicePropertiesW = GetProcAddress(hDevMgr, "DevicePropertiesW");
        if (pDevicePropertiesW) {
            // Call DevicePropertiesW function
            ((void(__stdcall*)(HWND, DWORD, wchar_t*, DWORD))pDevicePropertiesW)(
                hwnd, 0, buffer, 0);
        }
        FreeLibrary(hDevMgr);
    }
    
cleanup:
    return result;
}
