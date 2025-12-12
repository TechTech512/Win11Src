#include <windows.h>

STDAPI DllCanUnloadNow(void)
{
    return FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (ppv != NULL)
    {
        *ppv = NULL;
    }
    return E_NOTIMPL;
}

STDAPI DllRegisterServer(void)
{
    return FALSE;
}

STDAPI DllUnregisterServer(void)
{
    return FALSE;
}

