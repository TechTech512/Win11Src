/*
 * getdefaultsystemcodepage.c
 *
 * Displays the system default ANSI code page.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

int __cdecl wmain(void)
{
    wchar_t szCodePage[16];
    int cch;
    int codePage;

    // Get the system default locale ID (or use LOCALE_SYSTEM_DEFAULT directly)
    LCID lcid = GetSystemDefaultLCID();

    // Query the default ANSI code page for the system locale
    cch = GetLocaleInfoW(lcid, LOCALE_IDEFAULTANSICODEPAGE, szCodePage, sizeof(szCodePage)/sizeof(wchar_t));
    if (cch == 0) {
        // Fallback: use GetACP()
        codePage = GetACP();
    } else {
        codePage = _wtoi(szCodePage);
    }

    wprintf(L"Boot System Code Page: %ld\n", (long)codePage);

    return 0;
}

