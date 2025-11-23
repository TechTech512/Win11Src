#include <windows.h>

CRITICAL_SECTION gcsWinWatchLock;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&gcsWinWatchLock);
            DisableThreadLibraryCalls(hinstDLL);
            break;
            
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&gcsWinWatchLock);
            break;
    }
    
    return TRUE;
}

