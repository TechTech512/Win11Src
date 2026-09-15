#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdReason, PVOID lpReserved)
{
	if (fdReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hInstance);
	}
	return TRUE;
}
