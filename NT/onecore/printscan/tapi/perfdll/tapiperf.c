#include <windows.h>
#include <stdlib.h>
#include <winperf.h>
#include <tapi.h>
#include <winsvc.h>

// Global variables
static BOOL bInitOK = FALSE;
static BOOL bTapiSrvRunning = FALSE;
static HINSTANCE ghTapiInst = NULL;
static DWORD dwOpenCount = 0;
static DWORD gdwLineDevs = 0;
static DWORD gdwPhoneDevs = 0;

#define PERF_QUERY_GLOBAL 1
#define PERF_QUERY_ITEM 2
#define PERF_QUERY_FOREIGN 3
#define PERF_QUERY_COSTLY 4

// Performance counter structure from TAPI
typedef struct _TAPI_PERF_COUNTERS {
    DWORD dwSize;
    DWORD dwLines;
    DWORD dwPhones;
    DWORD dwLinesInUse;
    DWORD dwPhonesInUse;
    DWORD dwTotalOutgoingCalls;
    DWORD dwTotalIncomingCalls;
    DWORD dwClientApps;
    DWORD dwCurrentOutgoingCalls;
    DWORD dwCurrentIncomingCalls;
} TAPI_PERF_COUNTERS, *PTAPI_PERF_COUNTERS;

// Function pointer type for internalPerformance
typedef LONG (WINAPI* LPFNINTERNALPERFORMANCE)(TAPI_PERF_COUNTERS* pCounters);

// Global function pointer
static LPFNINTERNALPERFORMANCE glpfnInternalPerformance = NULL;

// Performance data definition structure
typedef struct _TAPI_PERF_DATA_DEFINITION {
    PERF_OBJECT_TYPE TapiObjectType;
    PERF_COUNTER_DEFINITION Lines;
    PERF_COUNTER_DEFINITION Phones;
    PERF_COUNTER_DEFINITION LinesInUse;
    PERF_COUNTER_DEFINITION PhonesInUse;
    PERF_COUNTER_DEFINITION TotalOutgoingCalls;
    PERF_COUNTER_DEFINITION TotalIncomingCalls;
    PERF_COUNTER_DEFINITION ClientApps;
    PERF_COUNTER_DEFINITION CurrentOutgoingCalls;
    PERF_COUNTER_DEFINITION CurrentIncomingCalls;
} TAPI_PERF_DATA_DEFINITION, *PTAPI_PERF_DATA_DEFINITION;

// Static performance data definition
static TAPI_PERF_DATA_DEFINITION TapiDataDefinition = {0};

// String constants for query types
static const WCHAR GLOBAL_STRING[] = L"Global";
static const WCHAR FOREIGN_STRING[] = L"Foreign";
static const WCHAR COSTLY_STRING[] = L"Costly";

void CheckForTapiSrv(void)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    
    hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        bTapiSrvRunning = FALSE;
        
        hService = OpenServiceA(hSCManager, "TAPISRV", SERVICE_QUERY_STATUS);
        if (hService != NULL)
        {
            if (QueryServiceStatus(hService, &ServiceStatus))
            {
                if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
                {
                    bTapiSrvRunning = TRUE;
                }
            }
            
            if (bTapiSrvRunning && ghTapiInst == NULL)
            {
                ghTapiInst = LoadLibraryA("tapi32.dll");
                if (ghTapiInst != NULL)
                {
                    glpfnInternalPerformance = (LPFNINTERNALPERFORMANCE)GetProcAddress(ghTapiInst, "internalPerformance");
                }
            }
            
            CloseServiceHandle(hService);
        }
        
        CloseServiceHandle(hSCManager);
    }
}

DWORD CloseTapiPerformanceData(void)
{
    return ERROR_SUCCESS;
}

DWORD GetQueryType(LPCWSTR lpValue)
{
    if (lpValue == NULL)
    {
        return PERF_QUERY_GLOBAL;
    }
    
    if (lpValue[0] == L'\0')
    {
        return PERF_QUERY_GLOBAL;
    }
    
    if (_wcsicmp(lpValue, GLOBAL_STRING) == 0)
    {
        return PERF_QUERY_GLOBAL;
    }
    
    if (_wcsicmp(lpValue, FOREIGN_STRING) == 0)
    {
        return PERF_QUERY_FOREIGN;
    }
    
    if (_wcsicmp(lpValue, COSTLY_STRING) == 0)
    {
        return PERF_QUERY_COSTLY;
    }
    
    return PERF_QUERY_ITEM;
}

BOOL IsNumberInUnicodeList(DWORD dwNumber, LPCWSTR lpList)
{
    if (lpList == NULL)
    {
        return FALSE;
    }
    
    BOOL bInNumber = FALSE;
    BOOL bFirstDigit = TRUE;
    DWORD dwCurrentNumber = 0;
    
    while (*lpList != L'\0')
    {
        WCHAR wc = *lpList;
        
        if (wc == L' ' || wc == L'\0')
        {
            if (bInNumber)
            {
                if (dwCurrentNumber == dwNumber)
                {
                    return TRUE;
                }
                bInNumber = FALSE;
            }
            
            if (wc == L'\0')
            {
                return FALSE;
            }
            
            bFirstDigit = TRUE;
            dwCurrentNumber = 0;
        }
        else if (wc >= L'0' && wc <= L'9')
        {
            if (bFirstDigit)
            {
                bFirstDigit = FALSE;
                bInNumber = TRUE;
            }
            
            if (bInNumber)
            {
                dwCurrentNumber = dwCurrentNumber * 10 + (wc - L'0');
            }
        }
        else
        {
            bInNumber = FALSE;
        }
        
        lpList++;
    }
    
    if (bInNumber && dwCurrentNumber == dwNumber)
    {
        return TRUE;
    }
    
    return FALSE;
}

DWORD CollectTapiPerformanceData(
    LPWSTR lpValueName,
    LPVOID* lppData,
    LPDWORD lpcbTotalBytes,
    LPDWORD lpNumObjectTypes
)
{
    DWORD dwStatus = ERROR_SUCCESS;
    PTAPI_PERF_COUNTERS pTapiCounters = NULL;
    PBYTE pCurrentData = NULL;
    
    if (!bInitOK)
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }
    
    DWORD dwQueryType = GetQueryType(lpValueName);
    
    if (dwQueryType == PERF_QUERY_FOREIGN)
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }
    
    if (dwQueryType == PERF_QUERY_ITEM)
    {
        // Extract object number from string if needed
        DWORD dwObjectId = 0;
        if (!IsNumberInUnicodeList(dwObjectId, lpValueName))
        {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_SUCCESS;
        }
    }
    
    pCurrentData = (PBYTE)*lppData;
    
    if (*lpcbTotalBytes < 464) // 0x1D0 = 464 bytes
    {
        return ERROR_MORE_DATA;
    }
    
    if (!bTapiSrvRunning)
    {
        CheckForTapiSrv();
    }
    
    pTapiCounters = (PTAPI_PERF_COUNTERS)GlobalAlloc(GMEM_ZEROINIT | GMEM_FIXED, sizeof(TAPI_PERF_COUNTERS));
    
    if (pTapiCounters == NULL)
    {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_SUCCESS;
    }
    
    if (!bTapiSrvRunning || glpfnInternalPerformance == NULL)
    {
        ZeroMemory(pTapiCounters, sizeof(TAPI_PERF_COUNTERS));
        pTapiCounters->dwSize = sizeof(TAPI_PERF_COUNTERS);
        pTapiCounters->dwLines = gdwLineDevs;
        pTapiCounters->dwPhones = gdwPhoneDevs;
    }
    else
    {
        pTapiCounters->dwSize = sizeof(TAPI_PERF_COUNTERS);
        glpfnInternalPerformance(pTapiCounters);
        
        if (pTapiCounters->dwClientApps != 0)
        {
            pTapiCounters->dwClientApps--;
        }
    }
    
    if ((LONG)pTapiCounters->dwTotalOutgoingCalls < 0)
    {
        pTapiCounters->dwTotalOutgoingCalls = 0;
    }
    
    if ((LONG)pTapiCounters->dwTotalIncomingCalls < 0)
    {
        pTapiCounters->dwTotalIncomingCalls = 0;
    }
    
    if ((LONG)pTapiCounters->dwCurrentOutgoingCalls < 0)
    {
        pTapiCounters->dwCurrentOutgoingCalls = 0;
    }
    
    if ((LONG)pTapiCounters->dwCurrentIncomingCalls < 0)
    {
        pTapiCounters->dwCurrentIncomingCalls = 0;
    }
    
    memcpy(pCurrentData, &TapiDataDefinition, 0x1A8);
    
    *(DWORD*)(pCurrentData + 0x1A8) = sizeof(TAPI_PERF_COUNTERS);
    *(DWORD*)(pCurrentData + 0x1AC) = pTapiCounters->dwLines;
    *(DWORD*)(pCurrentData + 0x1B0) = pTapiCounters->dwPhones;
    *(DWORD*)(pCurrentData + 0x1B4) = pTapiCounters->dwLinesInUse;
    *(DWORD*)(pCurrentData + 0x1B8) = pTapiCounters->dwPhonesInUse;
    *(DWORD*)(pCurrentData + 0x1BC) = pTapiCounters->dwTotalOutgoingCalls;
    *(DWORD*)(pCurrentData + 0x1C0) = pTapiCounters->dwTotalIncomingCalls;
    *(DWORD*)(pCurrentData + 0x1C4) = pTapiCounters->dwClientApps;
    *(DWORD*)(pCurrentData + 0x1C8) = pTapiCounters->dwCurrentOutgoingCalls;
    *(DWORD*)(pCurrentData + 0x1CC) = pTapiCounters->dwCurrentIncomingCalls;
    
    *lppData = (LPVOID)(pCurrentData + 0x1D0);
    *lpNumObjectTypes = 1;
    *lpcbTotalBytes = 0x1D0;
    
    GlobalFree(pTapiCounters);
    
    return ERROR_SUCCESS;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

DWORD OpenTapiPerformanceData(LPWSTR lpDeviceNames)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwValue = 0;
    
    if (dwOpenCount == 0)
    {
        bInitOK = TRUE;
        
        TapiDataDefinition.TapiObjectType.ObjectNameTitleIndex += 0x47E;
        TapiDataDefinition.Lines.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.Phones.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.LinesInUse.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.TapiObjectType.ObjectHelpTitleIndex += 0x47F;
        TapiDataDefinition.Lines.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.Phones.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.LinesInUse.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.PhonesInUse.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.PhonesInUse.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.TotalOutgoingCalls.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.TotalOutgoingCalls.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.TotalIncomingCalls.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.TotalIncomingCalls.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.ClientApps.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.ClientApps.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.CurrentOutgoingCalls.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.CurrentOutgoingCalls.CounterHelpTitleIndex += 0x47F;
        TapiDataDefinition.CurrentIncomingCalls.CounterNameTitleIndex += 0x47E;
        TapiDataDefinition.CurrentIncomingCalls.CounterHelpTitleIndex += 0x47F;
    }
    
    dwOpenCount++;
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony", 
                      0, 
                      KEY_READ | KEY_WOW64_64KEY, 
                      &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        if (RegQueryValueExA(hKey, "Perf1", NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            gdwLineDevs = dwValue + 0xAFBAADBA;
        }
        else
        {
            gdwLineDevs = 0;
        }
        
        dwSize = sizeof(DWORD);
        if (RegQueryValueExA(hKey, "Perf2", NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        {
            gdwPhoneDevs = dwValue + 0xAFBAADBA;
        }
        else
        {
            gdwPhoneDevs = 0;
        }
        
        RegCloseKey(hKey);
    }
    else
    {
        gdwLineDevs = 0;
        gdwPhoneDevs = 0;
    }
    
    return ERROR_SUCCESS;
}

