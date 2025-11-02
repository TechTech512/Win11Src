// oskupport.cpp
#include "osksupport.h"

HWND COskSupport::s_hWndOSK = NULL;

LRESULT CALLBACK COskSupport::OSKShellHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (s_hWndOSK == NULL) {
        s_hWndOSK = FindWindowW(L"OSKMainClass", NULL);
        if (s_hWndOSK == NULL) {
            goto CallNextHook;
        }
    }
    
    if (nCode == HSHELL_WINDOWCREATED) {
        PostMessageW(s_hWndOSK, 0x465, wParam, lParam);
    }
    
CallNextHook:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

