/*
 * dllmain.cpp
 *
 * DLL entry point with unusual behavior: terminates the process on attach.
 * Exports IllBeBack.
 */

#include <windows.h>

// ------------------------------------------------------------------
// DllMain – terminates the process on DLL_PROCESS_ATTACH
// ------------------------------------------------------------------
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Terminate the calling process
        HANDLE hProcess = GetCurrentProcess();
        TerminateProcess(hProcess, 0);
        // TerminateProcess does not return, but we still return TRUE to keep the compiler happy.
        // The process will be terminated before this returns.
    }
    return TRUE;
}

// ------------------------------------------------------------------
// IllBeBack – exported function that returns 1
// ------------------------------------------------------------------
BOOL WINAPI IllBeBack(void)
{
    return TRUE;
}

