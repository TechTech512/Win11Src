/*
 *  SCRNSAVE.C - default screen saver.
 *
 *  this app makes a IdleWild screen saver compatible with the windows 3.1
 *  screen saver interface.
 *
 *  Usage:      SCRNSAVE.EXE saver.iw [/s] [/c]
 *
 *      the IdleWild screen saver 'saver.iw' will be loaded and told to
 *      screen save.  if '/c' is specifed the savers configure dialog will
 *      be shown.
 *
 *      when the screen saver terminates SCRNSAVE.EXE will terminate too.
 *
 *      if the saver.iw is not specifed or refuses to load then a
 *      builtin 'blackness' screen saver will be used.
 *
 *  Restrictions:
 *
 *      because only one screen saver is loaded, (not all the screen savers
 *      like IdleWild.exe does) the random screen saver will not work correctly
 *
 *  History:
 *      10/15/90        ToddLa      stolen from SOS.C by BradCh
 *       6/17/91        stevecat    ported to NT Windows
 *
 */
 
#pragma warning (disable:4996)

#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include "strings.h"

// External functions from IWLIB.DLL
extern SHORT (*FInitScrSave)(HANDLE, HWND);
extern VOID  (*TermScrSave)(VOID);
extern VOID  (*ScrBlank)(SHORT);
extern VOID  (*ScrSetIgnore)(SHORT);
extern SHORT (*ScrLoadServer)(CHAR *);
extern SHORT (*ScrSetServer)(CHAR *);
extern VOID  (*ScrInvokeDlg)(HANDLE, HWND);

// Global variables
static POINT ptLast = {0, 0};
HWND hwndPreview = NULL;
HINSTANCE hMainInstance = NULL;
wchar_t szAppName[255] = L"";
wchar_t szNoConfigure[255] = L"";
wchar_t szNoConfigureToSet[255] = L"";
int fBlankNow = 0;
HWND hwndActive = NULL;

// Function prototypes
LRESULT CALLBACK DefaultProc(HWND, UINT, WPARAM, LPARAM);
int FInitApp(void);
int FInitDefault(void);
int __cdecl main(USHORT, CHAR **);

LRESULT CALLBACK DefaultProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT ptMouse;
    int dx, dy;

    if (msg < 0x101) {
        if (msg != 0x100) {
            if (msg == 1) {
                GetCursorPos(&ptLast);
                goto def_proc;
            }
            if (msg == 2) {
                PostQuitMessage(0);
                goto def_proc;
            }
            if ((msg != 6) && (msg != 0x1c)) {
                if ((msg == 0x20) && (hwndPreview == NULL)) {
                    SetCursor(NULL);
                    return 0;
                }
                goto def_proc;
            }
            if (wParam != 0) {
                goto def_proc;
            }
        }
        if (hwndPreview != NULL) {
            goto def_proc;
        }
    }
    else {
        if (msg == 0x102) {
            if (hwndPreview != NULL) {
                goto def_proc;
            }
        }
        else if (msg != 0x200) {
            if ((msg != 0x201) && ((msg != 0x204 && (msg != 0x207)))) {
                goto def_proc;
            }
            if (hwndPreview != NULL) {
                goto def_proc;
            }
        }
        else {
            if (hwndPreview != NULL) {
                goto def_proc;
            }
            GetCursorPos(&ptMouse);
            dx = ptMouse.x - ptLast.x;
            dy = ptMouse.y - ptLast.y;
            if (abs(dx) + abs(dy) < 4) {
                goto def_proc;
            }
        }
    }
    PostMessageW(hwnd, 0x10, 0, 0);

def_proc:
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int FInitApp(void)
{
    wchar_t *pwchCmdLine;
    wchar_t *pwchToken;
    int len;

    pwchCmdLine = GetCommandLineW();
    LoadStringW((HINSTANCE)hMainInstance, idsAppName, szAppName, 0xff);
    LoadStringW((HINSTANCE)hMainInstance, IDS_NO_OPTIONS, szNoConfigure, 0xff);
    LoadStringW((HINSTANCE)hMainInstance, IDS_NO_OPTIONS_TO_SET, szNoConfigureToSet, 0xff);

    if (*pwchCmdLine != 0) {
        pwchToken = (wchar_t *)wcschr(pwchCmdLine, L' ');
        if (pwchToken == NULL) {
            len = lstrlenW(pwchCmdLine);
            pwchCmdLine = pwchCmdLine + len;
        }
        else {
            pwchCmdLine = pwchToken;
            while (*pwchCmdLine == L' ') {
                pwchCmdLine = pwchCmdLine + 1;
            }
        }
    }

    while (1) {
        if (*pwchCmdLine == 0) {
            if ((fBlankNow == 0) || (FInitDefault() == 0)) {
                TaskDialog(hwndActive, NULL, szAppName, szNoConfigure, szNoConfigureToSet, TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
                PostQuitMessage(0);
            }
            return 1;
        }
        if ((*pwchCmdLine == L'/') || (*pwchCmdLine == L'-')) {
            if ((pwchCmdLine[1] == L's') || (pwchCmdLine[1] == L'S')) {
                fBlankNow = 1;
            }
            if ((pwchCmdLine[1] == L'c') || (pwchCmdLine[1] == L'C')) {
                hwndActive = GetActiveWindow();
            }
            if ((pwchCmdLine[1] == L'p') || (pwchCmdLine[1] == L'P')) {
                fBlankNow = 1;
                hwndPreview = (HWND)_wtoi(pwchCmdLine + 2);
                pwchCmdLine = pwchCmdLine + 1;
                continue;
            }
            pwchCmdLine[0] = L' ';
            pwchCmdLine[1] = L' ';
        }
        pwchCmdLine = pwchCmdLine + 1;
    }
}

int FInitDefault(void)
{
    WNDCLASSW cls;
    HWND hwnd;
    HDC hdc;
    RECT rc;
    OSVERSIONINFOW osvi;
    BOOL bWin2000 = FALSE;
    DWORD dwStyle;

    cls.style = 0;
    cls.lpfnWndProc = DefaultProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = (HINSTANCE)hMainInstance;
    cls.hIcon = NULL;
    if (hwndPreview == NULL) {
        cls.hCursor = NULL;
    }
    else {
        cls.hCursor = LoadCursorW(NULL, (LPCWSTR)0x7f00);
    }
    cls.hbrBackground = GetStockObject(BLACK_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = szAppName;

    if (RegisterClassW(&cls) == 0) {
        return 0;
    }

    hdc = GetDC(NULL);
    GetClipBox(hdc, &rc);
    ReleaseDC(NULL, hdc);

    if (IsRectEmpty(&rc)) {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        if (GetVersionExW(&osvi)) {
            if (((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5)) &&
                (GetSystemMetrics(SM_REMOTESESSION) != 0)) {
                rc.left = 0;
                rc.top = 0;
                rc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            }
        }
    }

    dwStyle = ((hwndPreview != NULL) ? 0xC0000000 : 0) | 0x80000000 | 0x10000000;

    hwnd = CreateWindowExW(
        0x88,
        szAppName,
        szAppName,
        dwStyle,
        rc.left,
        rc.top,
        rc.right - rc.left,
        rc.bottom - rc.top,
        hwndPreview,
        NULL,
        (HINSTANCE)hMainInstance,
        NULL
    );

    return (hwnd != NULL);
}

int __cdecl main(USHORT argc, CHAR **argv)
{
    MSG msg;
    int bRet;

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    hMainInstance = GetModuleHandleW(NULL);

    if (FInitApp() != 0) {
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return 0;
}

