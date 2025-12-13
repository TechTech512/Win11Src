#include <windows.h>
#include <ras.h>

// Declare the function since it might not be in ras.h
BOOL WINAPI RasConfigUserProxySettingsW(DWORD dwFlags, LPWSTR lpszPhonebook);

int __cdecl wmain(int argc, wchar_t* argv[])
{
    RasConfigUserProxySettingsW((DWORD)argc, (LPWSTR)argv);
    return 0;
}

