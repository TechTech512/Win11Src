#include <windows.h>
#include <shellapi.h>
#include <objbase.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    STARTUPINFOA startupInfo = {0};
    SHELLEXECUTEINFOA sei = {0};
    char perfmonPath[MAX_PATH] = {0};
    int nShowCmd = SW_SHOWDEFAULT;
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    // Get startup information
    startupInfo.cb = sizeof(startupInfo);
    GetStartupInfoA(&startupInfo);
    
    // Determine show command from startup info
    if (startupInfo.dwFlags & STARTF_USESHOWWINDOW)
    {
        nShowCmd = startupInfo.wShowWindow;
    }
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    // Expand environment variable first
    DWORD expandedLength = ExpandEnvironmentStringsA("%SystemRoot%\\System32\\perfmon.exe", perfmonPath, MAX_PATH);
    if (expandedLength == 0 || expandedLength > MAX_PATH)
    {
        // Fallback to direct path if expansion fails
        lstrcpyA(perfmonPath, "C:\\Windows\\System32\\perfmon.exe");
    }
    
    // Set up ShellExecute structure
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_NO_UI;
    sei.lpFile = perfmonPath;
    sei.lpParameters = "/res";
    sei.nShow = nShowCmd;
    
    ShellExecuteExA(&sei);
    
    CoUninitialize();
    
    return 0;
}

