#include <Windows.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int __cdecl wmain(int argc, wchar_t** argv)
{
    wchar_t* psCmdLine;
    int iResult;
    DWORD dwProcessId;
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    int exitCode = 0x57; // 87 decimal
    
    // Initialize startup info and process info structures
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    memset(&processInfo, 0, sizeof(processInfo));
    
    // Enable termination-on-corruption heap option
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    psCmdLine = GetCommandLineW();
    if ((psCmdLine != NULL) && (argc > 2) &&
        (_wcsicmp(argv[2], L"-P") == 0)) {
        
        dwProcessId = _wtoi(argv[1]);
        
        // Skip to the argument after the executable name in command line
        wchar_t* pArgs = psCmdLine;
        while (*pArgs != 0 && *pArgs != L'-') {
            pArgs++;
        }
        
        // Create process with specified process ID and creation flags
        iResult = CreateProcessW(
            NULL,                   // No application name
            pArgs,                  // Command line
            NULL,                   // Process handle not inheritable
            NULL,                   // Thread handle not inheritable
            FALSE,                  // Set handle inheritance to FALSE
            (dwProcessId & 0xFFF7FFF3) | 0x10, // Creation flags
            NULL,                   // Use parent's environment block
            NULL,                   // Use parent's starting directory
            &startupInfo,           // Pointer to STARTUPINFO structure
            &processInfo            // Pointer to PROCESS_INFORMATION structure
        );
        
        if (iResult == 0) {
            exitCode = GetLastError();
        }
        else {
            WaitForSingleObject(processInfo.hProcess, INFINITE);
            GetExitCodeProcess(processInfo.hProcess, (DWORD*)&exitCode);
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
        }
    }
    
    return exitCode;
}

