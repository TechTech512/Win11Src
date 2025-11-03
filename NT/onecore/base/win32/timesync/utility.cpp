// utility.cpp
#include <Windows.h>
#include <winsvc.h>

extern "C" {
    long TsConditionallyStartService(wchar_t* param_1, DWORD* param_2, wchar_t** param_3, BYTE* param_4);
    int TspWaitToExitServiceState(SC_HANDLE hService, SERVICE_STATUS_PROCESS* pServiceStatus, DWORD dwDesiredState, DWORD* pdwTimeout);
    long TsStopService(wchar_t* param_1);
}

extern "C" long TsConditionallyStartService(wchar_t* param_1, DWORD* param_2, wchar_t** param_3, BYTE* param_4)
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS_PROCESS serviceStatus;
    DWORD dwBytesNeeded;
    long result = 0x80010001; // E_FAIL
    
    hSCM = OpenSCManagerW(NULL, L"ServicesActive", SC_MANAGER_CONNECT);
    if (hSCM == NULL) {
        return result;
    }
    
    hService = OpenServiceW(hSCM, L"w32time", SERVICE_QUERY_STATUS | SERVICE_START);
    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        return result;
    }
    
    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (BYTE*)&serviceStatus,
                             sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        goto cleanup;
    }
    
    if (serviceStatus.dwCurrentState == SERVICE_STOPPED || serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
        if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
            TspWaitToExitServiceState(hService, NULL, SERVICE_STOPPED, NULL);
        }
        
        if (!StartServiceW(hService, 0, NULL)) {
            goto cleanup;
        }
        
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (BYTE*)&serviceStatus,
                                 sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
            goto cleanup;
        }
        
        if (!TspWaitToExitServiceState(hService, &serviceStatus, SERVICE_RUNNING, NULL)) {
            goto cleanup;
        }
        
        if (serviceStatus.dwCurrentState != SERVICE_RUNNING) {
            goto cleanup;
        }
        
        *param_2 = TRUE;
    } else {
        *param_2 = FALSE;
    }
    
    result = S_OK;

cleanup:
    if (hService != NULL) {
        CloseServiceHandle(hService);
    }
    if (hSCM != NULL) {
        CloseServiceHandle(hSCM);
    }
    return result;
}

extern "C" int TspWaitToExitServiceState(SC_HANDLE hService, SERVICE_STATUS_PROCESS* pServiceStatus, DWORD dwDesiredState, DWORD* pdwTimeout)
{
    SERVICE_STATUS_PROCESS tempStatus;
    SERVICE_STATUS_PROCESS* pStatus = pServiceStatus;
    DWORD dwBytesNeeded;
    
    if (pStatus == NULL) {
        pStatus = &tempStatus;
    }
    
    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (BYTE*)pStatus,
                             sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        return FALSE;
    }
    
    if (pStatus->dwCurrentState == dwDesiredState) {
        return TRUE;
    }
    
    DWORD dwTimeout = (pdwTimeout != NULL) ? *pdwTimeout : 30000;
    DWORD dwStartTime = GetTickCount();
    
    while (GetTickCount() - dwStartTime < dwTimeout) {
        Sleep(1000);
        
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (BYTE*)pStatus,
                                 sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
            return FALSE;
        }
        
        if (pStatus->dwCurrentState == dwDesiredState) {
            return TRUE;
        }
    }
    
    return FALSE;
}

extern "C" long TsStopService(wchar_t* param_1)
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS serviceStatus;
    long result = 0x80010001; // E_FAIL
    
    hSCM = OpenSCManagerW(NULL, L"ServicesActive", SC_MANAGER_CONNECT);
    if (hSCM == NULL) {
        return result;
    }
    
    hService = OpenServiceW(hSCM, L"w32time", SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        return result;
    }
    
    if (ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus)) {
        if (serviceStatus.dwCurrentState == SERVICE_STOPPED || serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
            result = S_OK;
        }
    }
    
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    
    return result;
}

