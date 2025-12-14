#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    wchar_t *cmdLine;
    int windowHandle;
    int value = 0;
    wchar_t currentChar;
    
    cmdLine = GetCommandLineW();
    HeapSetInformation(NULL, HeapCompatibilityInformation, NULL, 0);
    
    // Skip initial whitespace/quotes in command line
    currentChar = *cmdLine;
    if (currentChar != '"') {
        while (currentChar > ' ' && currentChar != 0) {
            cmdLine++;
            currentChar = *cmdLine;
        }
    } else {
        do {
            cmdLine++;
            if (*cmdLine == 0) break;
        } while (*cmdLine != '"');
        
        if (*cmdLine == '"') {
            cmdLine++;
        }
    }
    
    // Skip whitespace
    while (*cmdLine != 0 && *cmdLine <= ' ') {
        cmdLine++;
    }
    
    // Find the system tray window
    windowHandle = (int)FindWindowW(L"SystemTray_Main", NULL);
    
    if (windowHandle != 0) {
        // Parse numeric value from command line
        currentChar = *cmdLine;
        while (currentChar >= '0' && currentChar <= '9') {
            value = value * 10 + (currentChar - '0');
            cmdLine++;
            currentChar = *cmdLine;
        }
        
        // Send message to system tray
        PostMessageW((HWND)windowHandle, 0x4dc, value, 1);
    }
    
    return 0;
}

