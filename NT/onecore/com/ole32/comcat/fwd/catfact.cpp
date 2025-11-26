#include <windows.h>

HRESULT __cdecl DllCanUnloadNow(void)
{
    return 1;
}

HRESULT __cdecl DllRegisterServer(void)
{
    return 0;
}

HRESULT __cdecl DllUnregisterServer(void)
{
    return 0;
}

