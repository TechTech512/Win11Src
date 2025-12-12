#pragma warning (disable:4098)

#include <windows.h>

extern int _DllMainCRTStartupForGS2(HINSTANCE hinstDLL, DWORD fdwReason, void *lpvReserved);

void SE_DllLoaded(void)
{
	return TRUE;
}

void SE_DllUnloaded(void)
{
	return TRUE;
}

void SE_DynamicShim(void)
{
	return TRUE;
}

void SE_GetHookAPIs(void)
{
	return TRUE;
}

void SE_GetMaxShimCount(void)
{
	return TRUE;
}

void SE_GetProcAddressIgnoreIncExc(void)
{
	return TRUE;
}

void SE_GetShimCount(void)
{
	return TRUE;
}

void SE_InstallAfterInit(void)
{
	return TRUE;
}

void SE_InstallBeforeInit(void)
{
	return TRUE;
}

void SE_IsShimDll(void)
{
	return TRUE;
}

void SE_ProcessDying(void)
{
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        _DllMainCRTStartupForGS2(hinstDLL, 1, lpvReserved);
    }
    return TRUE;
}

