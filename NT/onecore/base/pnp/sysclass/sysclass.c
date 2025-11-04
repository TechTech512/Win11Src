#include <windows.h>

// Global variables  
HANDLE g_hActCtx = INVALID_HANDLE_VALUE;
HMODULE g_hmod = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_hActCtx != INVALID_HANDLE_VALUE) {
            ReleaseActCtx(g_hActCtx);
            g_hActCtx = INVALID_HANDLE_VALUE;
        }
        if (g_hmod != NULL) {
            g_hmod = NULL;
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        
        // Just skip the fusion initialization - it's only for visual styles
        // and the DLL will still function without it
    }
    return TRUE;
}

