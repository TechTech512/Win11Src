#include "regedt32.h"

#pragma comment(lib, "shlwapi.lib")

static const char szFile[] = "regedit.exe";

int WINAPI MyWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    char szPath[MAX_PATH];
    BOOL bSuccess = FALSE;
    
    szPath[0] = '\0';
    
    // Get Windows directory and append regedit.exe
    if (GetWindowsDirectoryA(szPath, MAX_PATH) != 0) {
        bSuccess = PathAppendA(szPath, szFile);
        if (!bSuccess) {
            szPath[0] = '\0';
        }
    }
    
    // If path building failed, just use the filename
    if (szPath[0] == '\0') {
        StringCchCopyA(szPath, sizeof(szPath), szFile);
    }
    
    // Execute regedit
    ShellExecuteA(NULL, NULL, szPath, NULL, NULL, SW_SHOW);
    ExitProcess(0);
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    char* pCmdLine;
    char currentChar;
    STARTUPINFOA startupInfo;
    int nCmdShow;
    HINSTANCE hModule;
    
    HeapSetInformation(NULL, HeapCompatibilityInformation, NULL, 0);
    
    // Parse command line
    pCmdLine = GetCommandLineA();
    currentChar = *pCmdLine;
    
    // Skip initial quote if present
    if (currentChar == '"') {
        do {
            pCmdLine++;
            currentChar = *pCmdLine;
            if (currentChar == '\0') break;
        } while (currentChar != '"');
        
        if (currentChar == '"') {
            pCmdLine++;
        }
    } else {
        // Skip until whitespace or end
        while (currentChar > ' ' && currentChar != '\0') {
            pCmdLine++;
            currentChar = *pCmdLine;
        }
    }
    
    // Skip whitespace
    while (*pCmdLine != '\0' && *pCmdLine <= ' ') {
        pCmdLine++;
    }
    
    // Get startup information
    GetStartupInfoA(&startupInfo);
    
    // Determine show command
    if (startupInfo.dwFlags & STARTF_USESHOWWINDOW) {
        nCmdShow = startupInfo.wShowWindow;
    } else {
        nCmdShow = SW_SHOWDEFAULT;
    }
    
    // Get module handle
    hModule = GetModuleHandleA(NULL);
    
    // Call MyWinMain (does not return)
    return MyWinMain((HINSTANCE)pCmdLine, (HINSTANCE)nCmdShow, NULL, 0);
}

