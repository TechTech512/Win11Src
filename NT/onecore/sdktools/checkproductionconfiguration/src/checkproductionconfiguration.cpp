/*
 * checkproductionconfiguration.cpp
 *
 * Checks whether the device is running in a production (locked) configuration
 * on Windows Core OS (WCOS) systems.
 *
 * Returns:
 *   0 if device is locked (production configuration)
 *   -0x7ff8ffeb (0x80070015 = HRESULT_FROM_WIN32(ERROR_NOT_FOUND)? Actually original returns -0x7ff8ffeb if unlocked)
 *   Or a negative HRESULT if the check fails.
 */

#include <windows.h>
#include <wldp.h>
#include <stdio.h>

#pragma comment(lib, "wldp.lib")

int __cdecl wmain(int argc, wchar_t* argv[])
{
    BOOL isLocked = FALSE;
    HRESULT hr = WldpIsWcosProductionConfiguration(&isLocked);

    if (FAILED(hr)) {
        wprintf(L"Failed to determine production lock state.\n");
        return (int)hr;
    }

    if (isLocked) {
        wprintf(L"Device is locked.\n");
        return 0;
    } else {
        wprintf(L"Device is unlocked\n");
        // The original returned -0x7ff8ffeb (which is 0x80070015? Actually -0x7ff8ffeb = 0x80070015? Let's check: 0x7ff8ffeb is 2147549163, its negative is -2147549163 which is not a standard HRESULT. The original likely intended to return a specific code for "unlocked" that the caller might check. We'll return 1 instead to indicate unlocked, but to match the original exactly, we need to compute -0x7ff8ffeb. That is a constant, so we can just return -0x7ff8ffeb.
        return -0x7ff8ffeb; // Matches original behavior
    }
}

