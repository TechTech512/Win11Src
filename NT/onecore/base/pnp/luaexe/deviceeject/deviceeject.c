#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>
#include <stdio.h>

// External function declarations
extern int pSetupInitializeUtils(void);
extern void pSetupUninitializeUtils(void);
extern int pSetupIsUserAdmin(void);

typedef BOOL (WINAPI *HOTPLUG_EJECT_DEVICE_EX_PROC)(HWND hwndParent, LPCWSTR pszDeviceInstanceId, DWORD dwFlags);

unsigned long __cdecl pHandleHotPlugEjectDevice(HWND hwndParent, wchar_t* pszDeviceInstanceId, unsigned long dwFlags)
{
    HMODULE hHotPlug;
    HOTPLUG_EJECT_DEVICE_EX_PROC pHotPlugEjectDeviceEx;
    unsigned long result = 0;

    hHotPlug = LoadLibraryW(L"hotplug.dll");
    if (hHotPlug == NULL) {
        result = GetLastError();
    } else {
        pHotPlugEjectDeviceEx = (HOTPLUG_EJECT_DEVICE_EX_PROC)GetProcAddress(hHotPlug, "HotPlugEjectDeviceEx");
        if (pHotPlugEjectDeviceEx == NULL) {
            result = GetLastError();
        } else {
            pHotPlugEjectDeviceEx(hwndParent, pszDeviceInstanceId, dwFlags);
        }
        FreeLibrary(hHotPlug);
    }
    return result;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR* argv;
    HWND hwndParent = NULL;
    wchar_t* deviceInstanceId = NULL;
    unsigned long flags = 0;
    unsigned long result = 0;
    int scanResult;
    ULONG_PTR hwndValue = 0;

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    pSetupInitializeUtils();
    
    if (pSetupIsUserAdmin() == 0) {
        result = ERROR_ACCESS_DENIED;
    } else {
        LPWSTR cmdLine = GetCommandLineW();
        argv = CommandLineToArgvW(cmdLine, &argc);
        if (argv == NULL || argc < 2) {
            result = ERROR_INVALID_PARAMETER;
        } else {
            // Parse device instance ID from first argument
            deviceInstanceId = argv[1];
            
            // Parse optional window handle from second argument
            if (argc >= 3) {
                scanResult = swscanf_s(argv[2], L"%Ix", &hwndValue);
                if (scanResult != 1) {
                    result = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }
                hwndParent = (HWND)hwndValue;
            }
            
            // Parse optional flags from third argument  
            if (argc >= 4) {
                scanResult = swscanf_s(argv[3], L"%lx", &flags);
                if (scanResult != 1) {
                    result = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }
            }
            
            result = pHandleHotPlugEjectDevice(hwndParent, deviceInstanceId, flags);
        }
        
cleanup:
        if (argv != NULL) {
            LocalFree(argv);
        }
    }
    
    pSetupUninitializeUtils();
    ExitProcess(result);
    
    return 0;
}

