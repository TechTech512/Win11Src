#include <windows.h>
#include <winternl.h>

LONG g_lWorkItemGuard = 0;
unsigned long g_dwElapsedSinceLastStart = 0;
extern BOOLEAN __stdcall RtlTimeToSecondsSince1980(FILETIME *Time, DWORD *Seconds);

HANDLE hSCManager = NULL;
HANDLE hService = NULL;

void CloseServiceHandles(void)
{
    if (hSCManager != NULL) {
        CloseServiceHandle(hSCManager);
    }
    if (hService != NULL) {
        CloseServiceHandle(hService);
    }
}

int DecrementGuard(void)
{
    int *pGuard = 0;
    
    if (*pGuard == 1) {
        *pGuard = 0;
        return 1;
    }
    return *pGuard;
}

DWORD __cdecl StartUI0DetectThreadProc(void *param);

LONG __cdecl Session0ViewerWindowProcHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    BOOL bSuccess;
    HWND hWndFound;
    HWND hWndParent;
    FILETIME LocalFileTime;
    DWORD SecondsSince1980;
    LONG lResult;
    
    if ((((nCode == 0) && (*(int *)(lParam + 8) == 0x18)) && (*(int *)(lParam + 4) != 0)) &&
       ((hWndFound = FindWindowW(L"$$$UI0Background", NULL), hWndFound == NULL &&
        (hWndParent = GetParent(*(HWND *)(lParam + 0xc)), hWndParent == NULL)))) {
        if (g_lWorkItemGuard == 0) {
            g_lWorkItemGuard = 1;
            bSuccess = TRUE;
        } else {
            bSuccess = FALSE;
        }
        
        if (bSuccess) {
            GetSystemTimeAsFileTime(&LocalFileTime);
            if (RtlTimeToSecondsSince1980(&LocalFileTime, &SecondsSince1980) &&
                (g_dwElapsedSinceLastStart < SecondsSince1980) &&
                ((299 < SecondsSince1980 - g_dwElapsedSinceLastStart))) {
                
                if (QueueUserWorkItem((LPTHREAD_START_ROUTINE)StartUI0DetectThreadProc, NULL, 0) != 0) {
                    g_dwElapsedSinceLastStart = SecondsSince1980;
                }
            }
        }
    }
    
    DecrementGuard();
    lResult = CallNextHookEx(NULL, nCode, wParam, lParam);
    return lResult;
}

DWORD __cdecl StartUI0DetectThreadProc(void *param)
{
    SC_HANDLE hSCM;
    SC_HANDLE hSvc;
    
    hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL) {
        hSCManager = hSCM;
        hSvc = OpenServiceW(hSCM, L"UI0Detect", SERVICE_START);
        if (hSvc != NULL) {
            hService = hSvc;
            if (StartServiceW(hSvc, 0, NULL)) {
                goto cleanup;
            }
        }
    }
    
    GetLastError();
    
cleanup:
    CloseServiceHandles();
    return 0;
}

