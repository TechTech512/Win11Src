#include "xinputi.h"

// Global variable
static HMODULE g_XInput1_4 = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // Process attach - set to NULL
            g_XInput1_4 = NULL;
            break;
            
        case DLL_PROCESS_DETACH:
            // Process detach - free library if loaded
            if (g_XInput1_4 != NULL)
            {
                FreeLibrary(g_XInput1_4);
                g_XInput1_4 = NULL;
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // Thread attach/detach - nothing to do
            break;
    }
    
    return TRUE;
}

