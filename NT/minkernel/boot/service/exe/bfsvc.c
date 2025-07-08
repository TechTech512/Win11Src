
#include <windows.h>
#include <winreg.h>
#include <stdbool.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>

int __stdcall BfsServiceBootFilesEx(wchar_t *arg1, unsigned long arg2) {
    return 0; // simulate success
}

// Exported function to check if system setup is in progress
__declspec(dllexport)
bool BfsIsSystemSetupInProgress(void)
{
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD dwType = 0;
    DWORD cbData = sizeof(dwValue);
    LONG status;

    // Open the registry key: HKEY_LOCAL_MACHINE\System\Setup
    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_READ, &hKey);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    // Read the "SystemSetupInProgress" DWORD value
    status = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, &dwType, (LPBYTE)&dwValue, &cbData);
    RegCloseKey(hKey);

    // Check if value is non-zero and type is DWORD
    return (status == ERROR_SUCCESS && dwType == REG_DWORD && cbData == sizeof(DWORD) && dwValue != 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LPWSTR *argv = NULL;
    int argc = 0;
    bool bNoResources = true;
    bool bMUI = false;
    bool bFonts = true;
    wchar_t *versionArg = NULL;
    int setupCheckFlag = 0;
    int result = 0;

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == NULL) {
        return GetLastError();
    }

    for (int i = 1; i < argc; ++i) {
        wchar_t *arg = argv[i];
        if (arg[0] == L'-' || arg[0] == L'/') {
            wchar_t *opt = arg + 1;

            if (_wcsicmp(opt, L"nosetupcheck") == 0) {
                if (setupCheckFlag == 1) {
                    result = 0x57;
                    goto cleanup;
                }
                setupCheckFlag = 1;
            } else if (_wcsicmp(opt, L"nofonts") == 0) {
                if (!bFonts) {
                    result = 0x57;
                    goto cleanup;
                }
                bFonts = false;
            } else if (_wcsicmp(opt, L"mui") == 0) {
                if (bMUI) {
                    result = 0x57;
                    goto cleanup;
                }
                bMUI = true;
            } else if (_wcsicmp(opt, L"noresources") == 0) {
                if (!bNoResources) {
                    result = 0x57;
                    goto cleanup;
                }
                bNoResources = false;
            } else if (_wcsicmp(opt, L"v") == 0) {
                versionArg = arg;
            } else {
                result = 0x57;
                goto cleanup;
            }
        }
    }

    if (versionArg != NULL) {
        if ((setupCheckFlag != 0 || BfsIsSystemSetupInProgress() == 0) &&
            BfsServiceBootFilesEx(NULL, 0) == 0) {
            result = GetLastError();
        } else {
            result = 0x57;
        }
    }

cleanup:
    if (argv) LocalFree(argv);
    return result;
}

