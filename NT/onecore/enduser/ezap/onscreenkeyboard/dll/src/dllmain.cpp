// dllmain.cpp
#include "osksupport.h"

HINSTANCE g_hInstance = NULL;
HHOOK COskSupport::s_hHookShell = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hinstDLL;
    }
    return TRUE;
}

HRESULT InitializeOSKSupport(void)
{
    HRESULT hr = S_OK;
    
    COskSupport::s_hHookShell = SetWindowsHookExW(WH_SHELL, COskSupport::OSKShellHookProc, g_hInstance, 0);
    if (COskSupport::s_hHookShell == NULL) {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    
    return hr;
}

void UninitializeOSKSupport(void)
{
    if (COskSupport::s_hHookShell != NULL) {
        UnhookWindowsHookEx(COskSupport::s_hHookShell);
        COskSupport::s_hHookShell = NULL;
    }
}

