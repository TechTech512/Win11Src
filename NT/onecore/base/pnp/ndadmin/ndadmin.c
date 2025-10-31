#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>

// External functions from other modules
extern int pSetupInitializeUtils(void);
extern int pSetupIsUserAdmin(void);
extern void pSetupUninitializeUtils(void);

DWORD pHandleInstallNull(WCHAR* param_1)
{
    DWORD dwError = 0;
    HMODULE hLibrary = LoadLibraryW(L"newdev.dll");
    
    if (hLibrary == NULL) {
        dwError = GetLastError();
    } else {
        FARPROC pFunction = GetProcAddress(hLibrary, "pDiDoNullDriverInstall");
        if (pFunction == NULL) {
            dwError = GetLastError();
        } else {
            dwError = ((DWORD (*)(void))pFunction)();
        }
        FreeLibrary(hLibrary);
    }
    
    return dwError;
}

DWORD pHandleInstallSpecificDriver(WCHAR* param_1, WCHAR* param_2, WCHAR* param_3)
{
    DWORD dwError = 0;
    HMODULE hLibrary = LoadLibraryW(L"newdev.dll");
    
    if (hLibrary == NULL) {
        dwError = GetLastError();
    } else {
        FARPROC pFunction = GetProcAddress(hLibrary, "pDiDoDeviceInstallAsAdmin");
        if (pFunction == NULL) {
            dwError = GetLastError();
        } else {
            ((void (*)(DWORD, DWORD, DWORD, WCHAR*))pFunction)(0, 0, 0, param_1);
        }
        FreeLibrary(hLibrary);
    }
    
    return dwError;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DWORD dwResult = 0;
    int argc = 0;
    LPWSTR* argv = NULL;
    DWORD dwCommand = 0;
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    if (pSetupInitializeUtils() == 0) {
        dwResult = GetLastError();
        goto cleanup;
    }
    
    if (pSetupIsUserAdmin() == 0) {
        dwResult = ERROR_ACCESS_DENIED;
        goto cleanup;
    }
    
    LPWSTR lpCmdLineW = GetCommandLineW();
    if (lpCmdLineW == NULL) {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    argv = CommandLineToArgvW(lpCmdLineW, &argc);
    if (argc < 2 || swscanf(argv[1], L"%lx", &dwCommand) != 1) {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (dwCommand == 2) {
        if (argc != 5) {
            dwResult = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        dwResult = pHandleInstallSpecificDriver(argv[4], (WCHAR*)1, NULL);
    } else if (dwCommand == 4) {
        if (argc != 3) {
            dwResult = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        dwResult = pHandleInstallNull(NULL);
    } else {
        dwResult = ERROR_INVALID_PARAMETER;
    }

cleanup:
    pSetupUninitializeUtils();
    if (argv) {
        LocalFree(argv);
    }
    ExitProcess(dwResult);
    return dwResult;
}

