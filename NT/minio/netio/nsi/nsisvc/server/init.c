#define WPP_PRIVATE_ENABLE_CALLBACK
#define WPP_MACRO_USE_KM_VERSION_FOR_UM
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        NsiServiceTraceGuid, \
        (a1f8b440,7b2b,4e29,8a17,90a4d2d4c9c1), \
        WPP_DEFINE_BIT(TRACE_FLAG_DEFAULT)    \
        WPP_DEFINE_BIT(TRACE_FLAG_RPC)        \
        WPP_DEFINE_BIT(TRACE_FLAG_SERVICE)    \
    )

#include "init.tmh"
#include <winsvc.h>

// External declarations for global variables
extern CRITICAL_SECTION DeregisterListLock;
extern CRITICAL_SECTION NsiKernelClientNotificationListLock;
extern CRITICAL_SECTION NsiRpcSubscribeLock;
extern LIST_ENTRY DeregisterList;
extern LIST_ENTRY NsiKernelClientNotificationList;
extern HANDLE HeapHandle;
extern HANDLE NotificationCleanupEvent;
extern HANDLE NotificationWaitHandle;
extern HANDLE NsiServiceStopEvent;
extern HANDLE NsiServiceWaitObject;
extern SERVICE_STATUS_HANDLE NsiServiceHandle;
extern SERVICE_STATUS NsiServiceStatus;
extern BOOL RpcServerInitialized;

// External declarations for functions
extern void NsiStopRpcServer(void);
extern void NsipDeregisterAndFreeNotificationContext(void* context);
extern void WPP_SF_D(void* logger, USHORT id, const GUID* guid, DWORD data);

extern WPP_PROJECT_CONTROL_BLOCK* WPP_GLOBAL_Control;

void __cdecl NsiServiceCleanup(void* param1, unsigned char param2)
{
    void* notificationContext = param1;
    
    NsiStopRpcServer();
    
    if (NsiServiceWaitObject != NULL) {
        UnregisterWaitEx(NsiServiceWaitObject, NULL);
        NsiServiceWaitObject = NULL;
    }
    
    if (NsiServiceStopEvent != NULL) {
        CloseHandle(NsiServiceStopEvent);
        NsiServiceStopEvent = NULL;
    }
    
    if (NotificationWaitHandle != NULL) {
        UnregisterWaitEx(NotificationWaitHandle, (HANDLE)0xFFFFFFFF);
        NotificationWaitHandle = NULL;
    }
    
    if (NotificationCleanupEvent != NULL) {
        CloseHandle(NotificationCleanupEvent);
        NotificationCleanupEvent = NULL;
    }
    
    do {
        if (DeregisterList.Flink == &DeregisterList) {
            NsiServiceStatus.dwCurrentState = SERVICE_STOPPED;
            NsiServiceStatus.dwCheckPoint = 0;
            NsiServiceStatus.dwControlsAccepted = 0;
            NsiServiceStatus.dwWaitHint = 0;
            SetServiceStatus(NsiServiceHandle, &NsiServiceStatus);
            DeleteCriticalSection(&NsiKernelClientNotificationListLock);
            DeleteCriticalSection(&DeregisterListLock);
            DeleteCriticalSection(&NsiRpcSubscribeLock);
            return;
        }
        
        LIST_ENTRY* nextEntry = DeregisterList.Flink->Flink;
        if ((DeregisterList.Flink->Blink != &DeregisterList) || (nextEntry->Blink != DeregisterList.Flink)) {
            __debugbreak();
            NsiServiceStatus.dwCurrentState = SERVICE_STOPPED;
            NsiServiceStatus.dwCheckPoint = 0;
            NsiServiceStatus.dwControlsAccepted = 0;
            NsiServiceStatus.dwWaitHint = 0;
            SetServiceStatus(NsiServiceHandle, &NsiServiceStatus);
            DeleteCriticalSection(&NsiKernelClientNotificationListLock);
            DeleteCriticalSection(&DeregisterListLock);
            DeleteCriticalSection(&NsiRpcSubscribeLock);
            return;
        }
        
        DeregisterList.Flink = nextEntry;
        nextEntry->Blink = &DeregisterList;
        NsipDeregisterAndFreeNotificationContext(notificationContext);
    } while (1);
}

DWORD __cdecl NsiServiceControlHandler(DWORD dwControl, DWORD dwEventType, void* lpEventData, void* lpContext)
{
    DWORD result = 0;
    
    if (dwControl == SERVICE_CONTROL_STOP) {
        if (NsiServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
            NsiServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            NsiServiceStatus.dwCheckPoint = 1;
            NsiServiceStatus.dwWaitHint = 60000;
            NsiServiceStatus.dwControlsAccepted = 0;
            SetServiceStatus(NsiServiceHandle, &NsiServiceStatus);
            SetEvent(NsiServiceStopEvent);
            result = 0;
        }
    } else if (dwControl != SERVICE_CONTROL_INTERROGATE) {
        result = 0x78;
    }
    
    return result;
}

DWORD __cdecl NsiServiceInitialize(void)
{
    DWORD result = 0;
    
    NsiKernelClientNotificationList.Blink = &NsiKernelClientNotificationList;
    NsiKernelClientNotificationList.Flink = &NsiKernelClientNotificationList;
    DeregisterList.Blink = &DeregisterList;
    DeregisterList.Flink = &DeregisterList;
    
    InitializeCriticalSection(&NsiKernelClientNotificationListLock);
    InitializeCriticalSection(&DeregisterListLock);
    InitializeCriticalSection(&NsiRpcSubscribeLock);
    
    HeapHandle = GetProcessHeap();
    if (HeapHandle == NULL) {
        result = GetLastError();
        if ((WPP_GLOBAL_Control != NULL) && 
            (1 < WPP_GLOBAL_Control->Control.Level) && 
            ((WPP_GLOBAL_Control->ReserveSpace[0x1c] & 0x10) != 0)) {
        }
        if (result != 0) {
            return result;
        }
    }
    
    NsiServiceStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if ((NsiServiceStopEvent == NULL) && 
        (WPP_GLOBAL_Control != NULL) && 
        (1 < WPP_GLOBAL_Control->Control.Level) && 
        ((WPP_GLOBAL_Control->ReserveSpace[0x1c] & 0x10) != 0)) {
        result = GetLastError();
    }
    
    return result;
}

DWORD __cdecl NsiServiceUpdateStatus(void)
{
    if (SetServiceStatus(NsiServiceHandle, &NsiServiceStatus) != 0) {
        return 0;
    }
    return GetLastError();
}

