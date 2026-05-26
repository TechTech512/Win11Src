#include <windows.h>
#include <objbase.h>

extern "C" void __cdecl __security_init_cookie(void);

struct IPStore;
struct IEnumPStoreProviders;

STDAPI DllCanUnloadNow(void)
{
    return FALSE;
}

STDAPI DllRegisterServer(void)
{
    return FALSE;
}

STDAPI DllUnregisterServer(void)
{
    return FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        __security_init_cookie();
    }
    return TRUE;
}

STDAPI PStoreCreateInstance(IPStore **ppPStore, GUID *pProviderType, void *pReserved, DWORD dwFlags)
{
    if (ppPStore != NULL) {
        *ppPStore = NULL;
    }
    return E_NOTIMPL;
}

STDAPI PStoreEnumProviders(DWORD dwFlags, IEnumPStoreProviders **ppEnum)
{
    if (ppEnum != NULL) {
        *ppEnum = NULL;
    }
    return E_NOTIMPL;
}

