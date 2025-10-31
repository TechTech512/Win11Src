#include <Windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")

void _ShowDisabledDpiCPLAlert(HINSTANCE hInstance, HWND hWnd)
{
    wchar_t title[256] = {0};
    wchar_t message[1024] = {0};
    
    // Load the title string
    if (LoadStringW(hInstance, IDS_TITLE, title, sizeof(title)/sizeof(wchar_t)) != 0) {
        // Load the disabled message string
        if (LoadStringW(hInstance, IDS_DISABLED, message, sizeof(message)/sizeof(wchar_t)) != 0) {
            // Show task dialog with information icon, OK button, and proper styling
            TASKDIALOGCONFIG config = {0};
            config.cbSize = sizeof(config);
            config.hwndParent = hWnd;
            config.hInstance = hInstance;
            config.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
            config.dwCommonButtons = TDCBF_OK_BUTTON;
            config.pszWindowTitle = title;
            config.pszMainIcon = TD_INFORMATION_ICON;
            config.pszContent = message;
            
            TaskDialogIndirect(&config, NULL, NULL, NULL);
        }
    }
}

BOOL IsDpiCPLDisabled()
{
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(DWORD);
    
    // Check for policy that disables Display CPL
    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"NoDispCPL", NULL, NULL, (LPBYTE)&dwValue, &dwSize);
        RegCloseKey(hKey);
    }
    
    return (dwValue != 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    HRESULT hr;
    IOpenControlPanel* pOpenControlPanel = NULL;
    
    // Check if DPI CPL is disabled via group policy
    if (IsDpiCPLDisabled() != 0) {
        _ShowDisabledDpiCPLAlert(hInstance, NULL);
        return 0;
    }
    
    // Initialize COM
    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        // Create OpenControlPanel instance
        hr = CoCreateInstance(
            CLSID_OpenControlPanel,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pOpenControlPanel)
        );
        
        if (SUCCEEDED(hr) && pOpenControlPanel) {
            // Open the Display Control Panel
            pOpenControlPanel->Open(L"Microsoft.Display", NULL, NULL);
            pOpenControlPanel->Release();
        }
        
        CoUninitialize();
    }
    
    return 0;
}

