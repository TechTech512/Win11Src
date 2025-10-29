#include "precomp.h"

#pragma comment(lib, "shlwapi.lib")

BOOL IsServerEdition() 
{
    OSVERSIONINFOEXW osvi = {0};
    DWORDLONG dwlConditionMask = 0;
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    
    // Check for workstation product types
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    
    osvi.wProductType = VER_NT_WORKSTATION;
    
    if (VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask)) {
        return FALSE; // Workstation edition
    }
    
    return TRUE; // Server edition
}

unsigned long LaunchComputerManager(void)
{
    BOOL shellExecuteResult;
    SHELLEXECUTEINFOW shellExecuteInfo;
    
    wprintf(L"Launching Computer Management\n");
    
    memset(&shellExecuteInfo, 0, sizeof(SHELLEXECUTEINFOW));
    shellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    shellExecuteInfo.lpVerb = L"open";
    shellExecuteInfo.lpFile = L"compmgmt.msc";
    shellExecuteInfo.nShow = SW_SHOW;
    
    shellExecuteResult = ShellExecuteExW(&shellExecuteInfo);
    if (!shellExecuteResult) {
        DWORD error = GetLastError();
        wprintf(L"Failed to launch Computer Management. Error: %lu\n", error);
        return error;
    }
    
    wprintf(L"Successfully launched Computer Management\n");
    if (shellExecuteInfo.hProcess != NULL) {
        CloseHandle(shellExecuteInfo.hProcess);
    }
    
    return 0;
}

unsigned long LaunchServerManager(void)
{
    BOOL shellExecuteResult;
    SHELLEXECUTEINFOW shellExecuteInfo;
    
    wprintf(L"Launching Server Manager\n");
    
    memset(&shellExecuteInfo, 0, sizeof(SHELLEXECUTEINFOW));
    shellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    shellExecuteInfo.lpVerb = L"open";
    shellExecuteInfo.lpFile = L"ServerManager.exe";
    shellExecuteInfo.nShow = SW_SHOW;
    
    shellExecuteResult = ShellExecuteExW(&shellExecuteInfo);
    if (!shellExecuteResult) {
        DWORD error = GetLastError();
        wprintf(L"Failed to launch Server Manager. Error: %lu\n", error);
        
        // Fall back to compmgmt.msc if Server Manager fails
        wprintf(L"Falling back to Computer Management\n");
        return LaunchComputerManager();
    }
    
    wprintf(L"Successfully launched Server Manager\n");
    if (shellExecuteInfo.hProcess != NULL) {
        CloseHandle(shellExecuteInfo.hProcess);
    }
    
    return 0;
}

unsigned long LaunchManagementTool(void)
{
    OSVERSIONINFOW versionInfo;
    
    memset(&versionInfo, 0, sizeof(versionInfo));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    
    if (!GetVersionExW(&versionInfo)) {
        return GetLastError();
    }
    
    // Check if this is Windows NT platform
    if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        if (IsServerEdition()) {
            wprintf(L"Detected Server edition, launching Server Manager\n");
            return LaunchServerManager();
        } else {
            wprintf(L"Detected Workstation edition, launching Computer Management\n");
            return LaunchComputerManager();
        }
    } else {
        // Non-NT systems (shouldn't happen these days)
        wprintf(L"Non-NT platform detected, launching Computer Management\n");
        return LaunchComputerManager();
    }
}

int __cdecl main(int argc, char** argv)
{
    unsigned long result;
    
    printf("Launching system management tool...\n");
    
    result = LaunchManagementTool();
    
    if (result != 0) {
        printf("Failed to launch management tool. Error code: %lu\n", result);
    } else {
        printf("Management tool launched successfully!\n");
    }
    
    printf("Press any key to exit...\n");
    getchar();
    
    return (result != 0) ? 1 : 0;
}

