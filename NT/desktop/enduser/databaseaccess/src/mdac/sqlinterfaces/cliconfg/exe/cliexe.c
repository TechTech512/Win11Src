#include <windows.h>
#include <strsafe.h>
#include "resource.h"

wchar_t szAppName[] = L"Cliconfiguration Utility";
HINSTANCE hLib = NULL;
HWND hwndApp = NULL;

// Function pointer type declaration
typedef void (__cdecl *PROC_ADDR)();

long __cdecl MainWndProc(HWND hWnd, unsigned int uMsg, unsigned int wParam, long lParam)
{
    long lResult;
    PROC_ADDR pFunc;
    unsigned int uError;
    wchar_t* pwszTitle;
    LPWSTR pwszError;
    wchar_t wszTitle[16];
    
    if (uMsg < 0x402) {
        if (uMsg == 0x401) {  // Original code uses 0x401, not WM_INITDIALOG
            hLib = (HINSTANCE)LoadLibraryExW(L"cliconfg.dll", 0, 0x800);
            if (hLib == NULL) {
                uError = GetLastError();
                FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                               0, uError, 0x400, (LPWSTR)&pwszError, 0, 0);
                StringCbPrintfW(wszTitle, 0x20, L"%d");
                pwszTitle = wszTitle;
            } else {
                pFunc = (PROC_ADDR)GetProcAddress(hLib, "OnInitDialogMain");
                if (pFunc != NULL) {
                    (*pFunc)();
                    lResult = DefWindowProcW(hWnd, 0x401, wParam, lParam);
                    return lResult;
                }
                pwszTitle = L"";
                pwszError = L"No Such Function";
            }
            MessageBoxW(0, pwszError, pwszTitle, 0);
            PostMessageW(hWnd, 2, 0, 0);
            lResult = DefWindowProcW(hWnd, 0x401, wParam, lParam);
            return lResult;
        }
        
        if (uMsg == 1) {  // Original code uses 1, not WM_CREATE
            PostMessageW(hWnd, 0x401, 0, 0);
            lResult = DefWindowProcW(hWnd, 1, wParam, lParam);
            return lResult;
        }
        
        if (uMsg == 2) {  // Original code uses 2, not WM_DESTROY
            PostQuitMessage(0);
            lResult = DefWindowProcW(hWnd, 2, wParam, lParam);
            return lResult;
        }
    } else if (uMsg == 0x1e0a) {
        SetWindowLongW(hWnd, 0, IDD_WINDBVER);
        lResult = IDD_WINDBVER;
        return lResult;
    }
    
    lResult = DefWindowProcW(hWnd, uMsg, wParam, lParam);
    return lResult;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
    short sClassRegistered;
    int iMsgResult;
    WNDCLASSW wc;
    MSG msg;
    
    // Initialize MSG structure to zero
    ZeroMemory(&msg, sizeof(MSG));
    
    if (hPrevInstance == NULL) {
        wc.style = 0;
        wc.lpfnWndProc = (WNDPROC)MainWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0x1e;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_MAIN));
        wc.hCursor = LoadCursorW(0, (LPCWSTR)0x7f00);  // Original has 0x7f00
        wc.hbrBackground = (HBRUSH)0x10;  // Original has 0x10
        wc.lpszMenuName = 0;
        wc.lpszClassName = szAppName;
        
        sClassRegistered = RegisterClassW(&wc);
        if (sClassRegistered == 0) {
            return 0;
        }
    }
    
    hwndApp = CreateWindowExW(
        0, 
        szAppName, 
        L"", 
        0xcf0000,  // Original has 0xcf0000
        0x80000000,  // Original has 0x80000000
        0x80000000,
        0x80000000,
        0x80000000,
        0, 
        0, 
        hInstance, 
        0
    );
    
    if (hwndApp != NULL) {
        ShowWindow(hwndApp, 0);  // Original has 0, not nCmdShow
        iMsgResult = GetMessageW(&msg, 0, 0, 0);
        
        while (iMsgResult != 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            iMsgResult = GetMessageW(&msg, 0, 0, 0);
        }
    }
    
    return (int)msg.wParam;
}

