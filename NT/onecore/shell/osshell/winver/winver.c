/*---------------------------------------------------------------------------
 |   WINVER.C - Windows Version program
 |
 |   History:
 |  03/08/89 Toddla     Created
 |
 *--------------------------------------------------------------------------*/

#include <windows.h>
#include <port1632.h>
#include <stdio.h>
#include "winverp.h"
#include <shellapi.h>
#include <strsafe.h>
#ifndef ARRAYSIZE
    #define ARRAYSIZE(x)                    (sizeof(x) / sizeof(x[0]))
#endif

BOOL FileTimeToDateTimeString(
    LPFILETIME pft,
    LPTSTR     pszBuf,
    UINT       cchBuf)
{
    SYSTEMTIME st;
    int cch;

    FileTimeToLocalFileTime(pft, pft);
    if (FileTimeToSystemTime(pft, &st))
    {
        cch = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf);
        if (cch > 0)
        {
            cchBuf -= cch;
            pszBuf += cch - 1;

            *pszBuf++ = TEXT(' ');
            *pszBuf = 0;          // (in case GetTimeFormat doesn't add anything)
            cchBuf--;

            GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf);
            return TRUE;
        }
    }    
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                              |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|   hInst       instance handle of this instance of the app                    |
|   hPrev       instance handle of previous instance, NULL if first            |
|       lpszCmdLine     ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
INT
__cdecl
ModuleEntry()
{
    TCHAR szTitle[32];
    LARGE_INTEGER Time = {0};

    // Get expiration date for evaluation/insider builds
    HMODULE hNtDll = GetModuleHandle(TEXT("ntdll.dll"));
    if (hNtDll)
    {
        // Try to access KUSER_SHARED_DATA at known address
        PVOID pUserSharedData = (PVOID)0x7FFE0000;
        Time = *(PLARGE_INTEGER)((PBYTE)pUserSharedData + 0x2C8);
        
        // If that fails, try registry fallback
        if (Time.QuadPart == 0)
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 
                            0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType, cbData = sizeof(Time);
                RegQueryValueEx(hKey, TEXT("SystemExpirationDate"), NULL, 
                               &dwType, (LPBYTE)&Time, &cbData);
                RegCloseKey(hKey);
            }
        }
    }

    if (LoadString(GetModuleHandle(NULL), IDS_APPTITLE, szTitle, ARRAYSIZE(szTitle)))
    {
        if (Time.QuadPart) 
        {
            TCHAR szExtra[128];
            TCHAR szTime[128];

            if (FileTimeToDateTimeString((PFILETIME)&Time, szTime, ARRAYSIZE(szTime))
                    && LoadString(GetModuleHandle(NULL), IDS_EVALUATION, szExtra, ARRAYSIZE(szExtra)))
            {
                StringCchCat(szExtra, ARRAYSIZE(szExtra), szTime);
                ShellAbout(NULL, szTitle, szExtra, NULL);
            }
        } else 
        {
            ShellAbout(NULL, szTitle, NULL, NULL);
        }
    }
    return 0;
}
