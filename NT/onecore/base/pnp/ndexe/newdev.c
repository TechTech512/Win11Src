#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>

// External functions from other modules
extern int pSetupInitializeUtils(void);
extern int pSetupIsUserAdmin(void);
extern void pSetupUninitializeUtils(void);
void RefreshDevices(int RefreshType);

DWORD WINAPI MessagePumpThreadProc(LPVOID lpParameter)
{
    MSG msg;
    
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

DWORD pHandleRunFinishInstallOperations(void)
{
    DWORD dwError = 0;
    HMODULE hLibrary = LoadLibraryW(L"newdev.dll");
    
    if (hLibrary == NULL) {
        dwError = GetLastError();
    } else {
        FARPROC pFunction = GetProcAddress(hLibrary, "pDiRunFinishInstallOperations");
        if (pFunction == NULL) {
            dwError = GetLastError();
        } else {
            dwError = ((DWORD (*)(void))pFunction)();
        }
        FreeLibrary(hLibrary);
    }
    
    return dwError;
}

DWORD pHandleSearchConfig(WCHAR* pszParam1, WCHAR* pszParam2, WCHAR* pszParam3)
{
    DWORD dwError = 0;
    HMODULE hLibrary = LoadLibraryW(L"newdev.dll");
    
    if (hLibrary == NULL) {
        dwError = GetLastError();
    } else {
        FARPROC pFunction = GetProcAddress(hLibrary, "SetInternetPolicies");
        if (pFunction == NULL) {
            dwError = GetLastError();
        } else {
            DWORD dwPolicy1, dwPolicy2, dwFlags;
            
            if (swscanf(pszParam3, L"%lx", &dwPolicy1) == 1 &&
                swscanf(pszParam2, L"%lx", &dwPolicy2) == 1 &&
                swscanf(pszParam1, L"%lx", &dwFlags) == 1) {
                
                ((void (*)(DWORD, DWORD))pFunction)(dwPolicy1, dwPolicy2);
                
                if (dwFlags != 0 || dwPolicy2 == 0) {
                    RefreshDevices(0);
                }
            } else {
                dwError = ERROR_INVALID_DATA;
            }
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
    
    if (dwCommand == 1 || dwCommand == 3) {
        if (argc != 4) {
            dwResult = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        dwResult = 0x78;
    } else if (dwCommand == 5) {
        if (argc == 5) {
            HANDLE hThread = CreateThread(NULL, 0, MessagePumpThreadProc, NULL, 0, NULL);
            if (hThread != NULL) {
                CloseHandle(hThread);
            }
            dwResult = pHandleSearchConfig(argv[4], argv[3], argv[2]);
            PostMessageW(NULL, WM_QUIT, 0, 0);
        } else {
            dwResult = ERROR_INVALID_PARAMETER;
        }
    } else if (dwCommand == 6) {
        if (argc != 2) {
            dwResult = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        dwResult = pHandleRunFinishInstallOperations();
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

