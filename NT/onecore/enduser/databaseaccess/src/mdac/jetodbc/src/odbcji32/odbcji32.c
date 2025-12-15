#include <windows.h>

static HMODULE s_hModule = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        s_hModule = hinstDLL;
    }
    return TRUE;
}

HINSTANCE WINAPI IntlLibHinst(void)
{
    return s_hModule;
}

