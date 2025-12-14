#include <windows.h>
#include <winternl.h>

// Global variables
typedef void (__cdecl *IDLE_CALLBACK)(DWORD);
static IDLE_CALLBACK g_pfnCallback = NULL;
static PTP_TIMER g_pIdleTimer = NULL;
static DWORD g_dwIdleMin = 0;
static BOOL g_fIdleNotify = FALSE;
static BOOL g_fBusyNotify = FALSE;
static DWORD g_dwIdleBeginTicks = 0;

#define USER_SHARED_DATA 0x7FFE0000

// Forward declarations
static void SetIdleTimer(void);

// Get the tick count of the last input
DWORD GetLastInputTickCount()
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        return lii.dwTime;
    }
    return 0;
}

// Function: OnIdleTimer
static void CALLBACK OnIdleTimer(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_TIMER Timer
)
{
    IDLE_CALLBACK pCallback;
    DWORD dwCurrentTicks;
    DWORD dwIdleTicks;
    DWORD dwElapsed;
    
    // Get last input time (simplified - in real code this would use GetLastInputInfo)
    // Note: _DAT_7ffe02e4 appears to be a reference to USER_SHARED_DATA.LastInputTime
    dwIdleTicks = GetLastInputTickCount();
    
    if (g_fBusyNotify && dwIdleTicks != g_dwIdleBeginTicks) {
        g_fBusyNotify = FALSE;
        g_fIdleNotify = TRUE;
        SetIdleTimer();
        
        pCallback = g_pfnCallback;
        if (pCallback != NULL) {
            pCallback(2); // Busy notification
        }
    }
    
    if (g_fIdleNotify) {
        dwCurrentTicks = GetTickCount();
        dwElapsed = dwCurrentTicks - dwIdleTicks;
        
        if (g_dwIdleMin * 60000 < dwElapsed) {
            g_fIdleNotify = FALSE;
            g_fBusyNotify = TRUE;
            g_dwIdleBeginTicks = dwIdleTicks;
            SetIdleTimer();
            
            pCallback = g_pfnCallback;
            if (pCallback != NULL) {
                pCallback(1); // Idle notification
            }
        }
    }
}

// Function: SetIdleTimer
static void SetIdleTimer(void)
{
    FILETIME ftDueTime;
    ULARGE_INTEGER ulDueTime;
    DWORD dwPeriod;
    
    if (g_pIdleTimer == NULL) {
        return;
    }
    
    dwPeriod = 60000; // Default: 1 minute
    
    if (g_fBusyNotify) {
        dwPeriod = 4000; // 4 seconds when busy
    }
    
    // Set timer to fire immediately and then periodically
    ulDueTime.QuadPart = (ULONGLONG)-(LONGLONG)dwPeriod * 10000;
    ftDueTime.dwLowDateTime = ulDueTime.LowPart;
    ftDueTime.dwHighDateTime = ulDueTime.HighPart;
    
    SetThreadpoolTimer(g_pIdleTimer, &ftDueTime, dwPeriod, 0);
}

// Function: BeginIdleDetection
DWORD __cdecl BeginIdleDetection(
    IDLE_CALLBACK pfnCallback,
    DWORD dwIdleMinutes,
    DWORD dwFlags
)
{
    DWORD dwError;
    
    if (dwFlags == 0) {
        if (g_pIdleTimer == NULL) {
            g_pIdleTimer = CreateThreadpoolTimer(OnIdleTimer, NULL, NULL);
            if (g_pIdleTimer == NULL) {
                dwError = GetLastError();
                return dwError;
            }
        }
        
        g_pfnCallback = pfnCallback;
        g_dwIdleMin = dwIdleMinutes;
        g_fIdleNotify = TRUE;
        SetIdleTimer();
        dwError = 0;
    } else {
        dwError = 0xD; // ERROR_INVALID_DATA
    }
    
    return dwError;
}

// Function: EndIdleDetection
BOOL __cdecl EndIdleDetection(DWORD dwFlags)
{
    PTP_TIMER pTimer;
    BOOL bResult = FALSE;
    
    if (dwFlags == 0) {
        // Enter critical section
        // LOCK();
        g_pfnCallback = NULL;
        // UNLOCK();
        
        if (g_pIdleTimer != NULL) {
            // Enter critical section
            // LOCK();
            pTimer = g_pIdleTimer;
            g_pIdleTimer = NULL;
            // UNLOCK();
            
            SetThreadpoolTimer(pTimer, NULL, 0, 0);
            WaitForThreadpoolTimerCallbacks(pTimer, TRUE);
            CloseThreadpoolTimer(pTimer);
        }
        bResult = TRUE;
    }
    
    return bResult;
}

// Function: GetIdleMinutes
DWORD __cdecl GetIdleMinutes(DWORD dwFlags)
{
    DWORD dwIdleMinutes = 0;
    DWORD dwCurrentTicks;
    DWORD dwIdleTicks;
    
    if (dwFlags == 0) {
        dwIdleTicks = GetLastInputTickCount();
        dwCurrentTicks = GetTickCount();
        dwIdleMinutes = (dwCurrentTicks - dwIdleTicks) / 60000;
    }
    
    return dwIdleMinutes;
}

// Function: GetIdleSeconds
DWORD __cdecl GetIdleSeconds(DWORD dwFlags)
{
    DWORD dwIdleSeconds = 0;
    DWORD dwCurrentTicks;
    DWORD dwIdleTicks;
    
    if (dwFlags == 0) {
        dwIdleTicks = GetLastInputTickCount();
        dwCurrentTicks = GetTickCount();
        dwIdleSeconds = (dwCurrentTicks - dwIdleTicks) / 1000;
    }
    
    return dwIdleSeconds;
}

// Function: SetBusyNotify
void __cdecl SetBusyNotify(BOOL fBusyNotify, DWORD dwFlags)
{
    g_fBusyNotify = fBusyNotify;
    if (fBusyNotify) {
        g_dwIdleBeginTicks = GetLastInputTickCount();
    }
    SetIdleTimer();
}

// Function: SetIdleNotify
void __cdecl SetIdleNotify(BOOL fIdleNotify, DWORD dwFlags)
{
    g_fIdleNotify = fIdleNotify;
}

// Function: SetIdleTimeout
BOOL __cdecl SetIdleTimeout(DWORD dwIdleMinutes, DWORD dwFlags)
{
    BOOL bResult = FALSE;
    
    if (dwFlags == 0) {
        if (dwIdleMinutes != 0) {
            g_dwIdleMin = dwIdleMinutes;
        }
        bResult = TRUE;
    }
    
    return bResult;
}

// DllMain
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved
)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

