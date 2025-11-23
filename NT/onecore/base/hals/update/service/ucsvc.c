#include <windows.h>
#include <winternl.h>
#include <shellapi.h>

extern NTSTATUS NTAPI NtSetSystemInformation(ULONG, PVOID, ULONG);

int __cdecl UcsAcquirePrivilege(void)
{
    HANDLE hToken = NULL;
    BOOL bResult = FALSE;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        return FALSE;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    if (!LookupPrivilegeValueW(NULL, L"SeLoadDriverPrivilege", &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }
    
    tp.Privileges[0].Luid = luid;
    bResult = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
    
    DWORD dwError = GetLastError();
    CloseHandle(hToken);
    
    if (!bResult)
    {
        SetLastError(dwError);
    }
    
    return bResult;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    int argc = 0;
    wchar_t** argv = NULL;
    int bUninstall = 0;
    int i = 0;
    int bSuccess = 0;
    DWORD dwError = 0;
    NTSTATUS status;
    DWORD systemInformation;
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    if (lpCmdLine && *lpCmdLine != L'\0')
    {
        argv = CommandLineToArgvW(lpCmdLine, &argc);
        if (!argv)
        {
            return GetLastError();
        }
    }
    
    for (i = 0; i < argc; i++)
    {
        wchar_t* arg = argv[i];
        if ((arg[0] != L'-' && arg[0] != L'/') || _wcsicmp(arg + 1, L"uninstall") != 0 || bUninstall)
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        bUninstall = 1;
    }
    
    if (!UcsAcquirePrivilege())
    {
        dwError = GetLastError();
        goto cleanup;
    }
    
    systemInformation = 2;
    status = NtSetSystemInformation(0x68, &systemInformation, sizeof(systemInformation));
    if (status < 0)
    {
        dwError = RtlNtStatusToDosError(status);
        SetLastError(dwError);
        goto cleanup;
    }
    
    if (!bUninstall)
    {
        systemInformation = 1;
        status = NtSetSystemInformation(0x68, &systemInformation, sizeof(systemInformation));
        if (status < 0)
        {
            dwError = RtlNtStatusToDosError(status);
            SetLastError(dwError);
            goto cleanup;
        }
    }
    
    bSuccess = 1;

cleanup:
    if (argv)
    {
        LocalFree(argv);
    }
    
    return bSuccess ? 0 : dwError;
}

