/*
 * postest.c
 *
 * Simple GPS test utility for Windows.
 * Displays "GPS Test" to the console and exits.
 */

#include "precomp.h"

int __cdecl wmain(int argc, wchar_t* argv[])
{
    // Disable stdout buffering (write immediately)
    setvbuf(stdout, NULL, _IONBF, 0);

    wprintf(L"GPS Test\n\n");
    return 0;
}

