#include "pch.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int result = 0;
    SHELLEXECUTEINFOA sei = {0};
    char taskmgrPath[MAX_PATH] = {0};
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    HANDLE hProcess = GetCurrentProcess();
    SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
    DWORD expandedLength = ExpandEnvironmentStringsA("%WINDIR%\\System32\\Taskmgr.exe", taskmgrPath, MAX_PATH);
    if (expandedLength == 0)
    {
        result = GetLastError();
        goto uninitialize;
    }
    
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "open";
    sei.lpFile = taskmgrPath;
    sei.lpParameters = lpCmdLine;  // This is now correct: LPSTR to LPCSTR
    sei.nShow = SW_SHOWNORMAL;
    
    if (!ShellExecuteExA(&sei))
    {
        result = GetLastError();
    }
    
uninitialize:
    CoUninitialize();
    
cleanup:
    return result;
}

