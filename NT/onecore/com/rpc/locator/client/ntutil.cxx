#include <windows.h>

extern "C" void DbgPrint(const char* Format, ...);
extern "C" int _DllMainCRTStartupForGS2(HINSTANCE hInstance, DWORD dwReason, void *lpReserved);

int __cdecl _InitializeDLL(HINSTANCE hinstDLL, ULONG fdwReason, _CONTEXT *lpvReserved)
{
    DbgPrint("RPC:  Using rpcns4.dll.  The dll is no longer supported.\n");
    return 1;
}

extern "C" int __stdcall InitializeDLL(HINSTANCE hinstDLL, ULONG fdwReason, _CONTEXT *lpvReserved)
{
    int result;
    
    if (fdwReason == 1) {
        _DllMainCRTStartupForGS2(hinstDLL, 1, lpvReserved);
    }
    
    result = _InitializeDLL(hinstDLL, fdwReason, lpvReserved);
    return result;
}

