#pragma warning (disable:4716)

#include <windows.h>
#include <ws2spi.h>

_Must_inspect_result_
INT
WSAAPI
NSPStartup(
    _In_ LPGUID lpProviderId,
    _Inout_ LPNSP_ROUTINE lpnspRoutines
    ){}
