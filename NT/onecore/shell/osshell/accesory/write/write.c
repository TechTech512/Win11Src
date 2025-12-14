#include <windows.h>
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *lpCmdLine, int nShowCmd)
{
    STARTUPINFOW startupInfo;
    DWORD startupFlags;
    WORD showWindowCommand;
    
    HeapSetInformation(NULL, HeapCompatibilityInformation, NULL, 0);
    
    // Get startup information
    GetStartupInfoW(&startupInfo);
    startupFlags = startupInfo.dwFlags;
    
    // Determine how to show the window
    if ((startupFlags & STARTF_USESHOWWINDOW) == 0) {
        // If not specified, default to SW_SHOWDEFAULT
        showWindowCommand = SW_SHOWDEFAULT;
    } else {
        showWindowCommand = startupInfo.wShowWindow;
    }
    
    // Execute wordpad with the command line parameter
    ShellExecuteW(NULL, NULL, L"wordpad.exe", lpCmdLine, NULL, showWindowCommand);
    
    return 0;
}

