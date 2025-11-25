#include <windows.h>
#include <winternl.h>

extern "C" void DbgPrintEx(ULONG ComponentId, ULONG Level, const char* Format, ...);
extern "C" BOOL WinSqmIsOptedIn(void);
extern "C" void WinSqmIncrementDWORD(DWORD dwSqmType, DWORD dwDatapointId, DWORD dwIncrement);
extern "C" NTSTATUS RtlIntegerToUnicodeString(ULONG Value, ULONG Base, PUNICODE_STRING String);

void __cdecl DeprecatedFunctionalityUseError(void)
{
    HANDLE hEventLog;
    DWORD dwError;
    BOOL bReportResult;
    WCHAR* lpCommandLine;
    WCHAR szProcessId[24];
    UNICODE_STRING ProcessIdString;
    LPCWSTR lpStrings[2];
    
    if (WinSqmIsOptedIn()) {
        WinSqmIncrementDWORD(0, 0x848, 1);
    }
    
    hEventLog = RegisterEventSourceW(NULL, L"RpcNs");
    if (hEventLog == NULL) {
        dwError = GetLastError();
        DbgPrintEx(0x2a, 0, "RPC: RegisterEventSource failed: %x\n", dwError);
    }
    else {
        lpCommandLine = GetCommandLineW();
        ProcessIdString.Buffer = szProcessId;
        ProcessIdString.Length = 0;
        ProcessIdString.MaximumLength = sizeof(szProcessId);
        
        RtlIntegerToUnicodeString(GetCurrentProcessId(), 10, &ProcessIdString);
        
        lpStrings[0] = lpCommandLine;
        lpStrings[1] = ProcessIdString.Buffer;
        
        bReportResult = ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, 0, 0xC0000002, NULL, 2, 0, lpStrings, NULL);
        if (!bReportResult) {
            dwError = GetLastError();
            DbgPrintEx(0x2a, 0, "RPC: ReportEventW failed: %X. Can't log deprecation error for event %d. \n", dwError, 0xC0000002);
        }
        
        DeregisterEventSource(hEventLog);
    }
    
    return;
}

