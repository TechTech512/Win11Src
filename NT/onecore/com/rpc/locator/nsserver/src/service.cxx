#include "locatorpch.h"

SERVICE_STATUS_HANDLE sshServiceHandle;
SERVICE_STATUS ssServiceStatus;
HANDLE MainThreadEvent;

void __cdecl SetSvcStatus(void);

void __cdecl StopLocator(char* param_1, long param_2)
{
    ssServiceStatus.dwCurrentState = SERVICE_STOPPED;
    if (param_2 == 0) {
        ssServiceStatus.dwWin32ExitCode = GetLastError();
    }
    else {
        ssServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        ssServiceStatus.dwServiceSpecificExitCode = param_2;
    }
    if (sshServiceHandle != NULL) {
        SetSvcStatus();
    }
    ExitProcess(0);
}

void __cdecl SetSvcStatus(void)
{
    BOOL result = SetServiceStatus(sshServiceHandle, &ssServiceStatus);
    if (result == 0) {
        StopLocator((char*)&ssServiceStatus, 0);
    }
    ssServiceStatus.dwCheckPoint = ssServiceStatus.dwCheckPoint + 1;
    return;
}

void __cdecl LocatorControl(ULONG param_1)
{
    if (param_1 == 1) {
        ssServiceStatus.dwCheckPoint = 0;
        ssServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        ssServiceStatus.dwWaitHint = 90000;
        SetSvcStatus();
        SetEvent(MainThreadEvent);
    }
    else {
        SetSvcStatus();
    }
    return;
}

void __cdecl LocatorServiceMain(ULONG param_1, char** param_2)
{
    wchar_t* serviceName = L"rpclocator";
    
    ssServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ssServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ssServiceStatus.dwWin32ExitCode = 0;
    ssServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ssServiceStatus.dwCheckPoint = 0;
    ssServiceStatus.dwWaitHint = 30000;
    
    sshServiceHandle = RegisterServiceCtrlHandlerW(serviceName, (LPHANDLER_FUNCTION)LocatorControl);
    if (sshServiceHandle != NULL) {
        SetSvcStatus();
        MainThreadEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (MainThreadEvent != NULL) {
            ssServiceStatus.dwCurrentState = SERVICE_RUNNING;
            SetSvcStatus();
            WaitForSingleObject(MainThreadEvent, INFINITE);
            ssServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ssServiceStatus.dwCheckPoint = 0;
            SetSvcStatus();
            return;
        }
        GetLastError();
    }
    
    StopLocator((char*)serviceName, (long)LocatorControl);
}

int __cdecl main(int argc, char** argv)
{
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { L"rpclocator", (LPSERVICE_MAIN_FUNCTIONW)LocatorServiceMain },
        { NULL, NULL }
    };
    
    StartServiceCtrlDispatcherW(serviceTable);
    return 0;
}



