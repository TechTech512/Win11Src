#pragma warning (disable:4717)

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
    }
    
    return TRUE;
}

// Note: These functions as decompiled have bugs - they call themselves recursively
// This would cause infinite recursion and stack overflow
__declspec(dllexport) HRESULT __cdecl SHGetFolderPathA(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    // Original decompiled code: uVar1 = SHGetFolderPathA(param_1,param_2,0,0,param_5);
    // This is wrong - calls itself recursively
    return SHGetFolderPathA(hwndOwner, nFolder, NULL, 0, pszPath);
}

__declspec(dllexport) HRESULT __cdecl SHGetFolderPathW(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath)
{
    // Original decompiled code: SHGetFolderPathW();
    // This is wrong - calls itself with no parameters
    SHGetFolderPathW(hwndOwner, nFolder, NULL, 0, pszPath);
    return S_OK;
}

