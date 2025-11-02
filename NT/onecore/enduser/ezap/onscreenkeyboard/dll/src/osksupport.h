// oskupport.h (needed for class definition)
#include <windows.h>

class COskSupport
{
public:
    static LRESULT CALLBACK OSKShellHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK s_hHookShell;
    static HWND s_hWndOSK;
};

