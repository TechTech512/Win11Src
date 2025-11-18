#define WPP_INIT_TRACING
#define WPP_MACRO_USE_KM_VERSION_FOR_UM
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        NsiServiceTraceGuid, \
        (a1f8b440,7b2b,4e29,8a17,90a4d2d4c9c1), \
        WPP_DEFINE_BIT(TRACE_FLAG_DEFAULT)    \
        WPP_DEFINE_BIT(TRACE_FLAG_RPC)        \
        WPP_DEFINE_BIT(TRACE_FLAG_SERVICE)    \
    )


#include "nsiserver.tmh"
#include <rpc.h>
#include <rpcdce.h>
#include <winsvc.h>
#include <rpcasync.h>
#include <evntrace.h>

// Critical section locks
CRITICAL_SECTION DeregisterListLock;
CRITICAL_SECTION NsiKernelClientNotificationListLock;
CRITICAL_SECTION NsiRpcSubscribeLock;

// Global lists
LIST_ENTRY DeregisterList;
LIST_ENTRY NsiKernelClientNotificationList;

// Global handles and variables
HANDLE HeapHandle;
HANDLE NotificationCleanupEvent;
HANDLE NotificationWaitHandle;
BOOL RpcServerInitialized = FALSE;

// Service-related globals
SERVICE_STATUS_HANDLE NsiServiceHandle;
SERVICE_STATUS NsiServiceStatus;
HANDLE NsiServiceStopEvent;
HANDLE NsiServiceWaitObject;

// RPC interface handle for NSI
RPC_IF_HANDLE WINNSI___RpcServerInterface = NULL;

typedef struct _SVCHOST_GLOBAL_DATA {
    DWORD dwSize;
    DWORD dwServerApiVersion;
    PVOID ServiceMain;
    PVOID RegisterStopCallback;
    PVOID RegisterShutdownCallback;
    PVOID RegisterThreadCallback;
    PVOID GetConfigData;
    PVOID ReportStatus;
    PVOID SetServiceStatus;
    PVOID CoGetContextToken;
    PVOID GetSharedServiceDirectory;
    PVOID GetSharedServiceIdentity;
    PVOID LogEvent;
    PVOID SetServiceBits;
    PVOID AddServiceToSd;
} SVCHOST_GLOBAL_DATA;

SVCHOST_GLOBAL_DATA* NsiSvcHostGlobalData;

// NSI-specific types
typedef struct _NSI_MODULE_DESC {
    DWORD NsiModule;
    void* ModuleId;
    DWORD ObjectIndex;
} NSI_MODULE_DESC;

typedef struct _NSI_KEYSTRUCT_DESC {
    BYTE* KeyStruct;
    DWORD KeyStructLength;
} NSI_KEYSTRUCT_DESC;

typedef struct _NSI_SINGLE_PARAM_DESC {
    DWORD StructType;
    BYTE* Parameter;
    DWORD ParameterLength;
    DWORD ParameterOffset;
} NSI_SINGLE_PARAM_DESC;

typedef struct _NSI_PARAM_STRUCT_DESC {
    BYTE* RwParameterStruct;
    DWORD RwParameterStructLength;
    BYTE* RoDynamicParameterStruct;
    DWORD RoDynamicParameterStructLength;
    BYTE* RoStaticParameterStruct;
    DWORD RoStaticParameterStructLength;
} NSI_PARAM_STRUCT_DESC;

typedef struct _NSI_KEYSTRUCT_DESC_ENUM {
    DWORD KeyStructLength;
    DWORD TotalKeyStructLength;
} NSI_KEYSTRUCT_DESC_ENUM;

typedef struct _NSI_PARAM_STRUCT_DESC_ENUM {
    DWORD RwParameterStructLength;
    DWORD TotalRwLength;
    DWORD RoDynamicParameterStructLength;
    DWORD TotalRoDynamicLength;
    DWORD RoStaticParameterStructLength;
    DWORD TotalRoStaticLength;
} NSI_PARAM_STRUCT_DESC_ENUM;

typedef struct _NSI_RW_PARAM_STRUCT_DESC {
    BYTE* RwParameterStruct;
    DWORD RwParameterStructLength;
} NSI_RW_PARAM_STRUCT_DESC;

typedef struct _NSI_NOTIFICATION_DESC {
    DWORD CompartmentScope;
    DWORD CompartmentId;
} NSI_NOTIFICATION_DESC;

typedef struct _NSI_NOTIFICATION_REGISTRATION_DESC {
    DWORD CompartmentScope;
    DWORD CompartmentId;
} NSI_NOTIFICATION_REGISTRATION_DESC;

typedef DWORD NSI_NOTIFICATION;
typedef DWORD NSI_STORE;
typedef DWORD NSI_GET_ACTION;
typedef DWORD NSI_SET_ACTION;
typedef DWORD NSI_STRUCT_TYPE;

// NSI constants
#define NsiBoth 0
#define NsiActive 1
#define NsiCurrent 2
#define NsiGetExact 0
#define NsiGetFirst 1
#define NsiGetNext 2

// Kernel notification context
typedef struct _KERNEL_NOTIFICATION_CONTEXT {
    LIST_ENTRY ListEntry;
    BYTE Data[80];
} KERNEL_NOTIFICATION_CONTEXT;

extern const GUID NsiServiceTraceGuid;

// External function declarations
extern DWORD NsiEnumerateObjectsAllParametersEx(void* params);
extern DWORD NsiGetAllParametersEx(void* params);
extern DWORD NsiGetParameterEx(void* params);
extern DWORD NsiSetAllParametersEx(void* params);
extern DWORD NsiSetParameterEx(void* params);
extern DWORD NsiRegisterChangeNotificationEx(void* params);
extern DWORD NsiDeregisterChangeNotificationEx(void* params);
extern DWORD __cdecl NsiServiceInitialize(void);
extern void NsiServiceUpdateStatus(void);
extern GUID* WPP_REGISTRATION_GUIDS[];
extern void WPP_SF_(void* logger, USHORT id, const GUID* guid);
extern void WPP_SF_D(void* logger, USHORT id, const GUID* guid, DWORD data);
extern ULONG WppControlCallback(WMIDPREQUESTCODE RequestCode, void* Context, ULONG* BufferSize, void* Buffer);
extern void WppInitUm(wchar_t* context);

void NsipFreePendingChangeList(LIST_ENTRY* param1);

void CleanupNotificationThread(void* param1, unsigned char param2)
{
    LIST_ENTRY* currentEntry;
    LIST_ENTRY* nextEntry;
    LIST_ENTRY* listHead;
    
    do {
        do {
            EnterCriticalSection(&DeregisterListLock);
            if (DeregisterList.Flink == &DeregisterList) {
                LeaveCriticalSection(&DeregisterListLock);
                return;
            }
            
            listHead = DeregisterList.Flink;
            DeregisterList.Flink->Blink = (LIST_ENTRY*)&listHead;
            DeregisterList.Blink->Flink = (LIST_ENTRY*)&listHead;
            DeregisterList.Blink = &DeregisterList;
            DeregisterList.Flink = &DeregisterList;
            LeaveCriticalSection(&DeregisterListLock);
            currentEntry = (LIST_ENTRY*)listHead;
        } while (currentEntry == (LIST_ENTRY*)&listHead);
        
        do {
            nextEntry = currentEntry->Flink;
            if (currentEntry[10].Blink != NULL) {
                LIST_ENTRY deregParams[4];
                deregParams[0] = *currentEntry[5].Flink;
                deregParams[1] = *currentEntry[5].Blink;
                deregParams[2] = *currentEntry[6].Flink;
                deregParams[3] = *currentEntry[10].Blink;
                NsiDeregisterChangeNotificationEx(deregParams);
            }
            
            LIST_ENTRY** listPtr = (LIST_ENTRY**)&currentEntry[1].Blink;
            while ((listHead = *listPtr) != (LIST_ENTRY*)listPtr) {
                LIST_ENTRY* entry = listHead->Flink;
                if ((listHead->Blink != (LIST_ENTRY*)listPtr) || (entry->Blink != listHead)) {
                    __debugbreak();
                    USHORT error = (USHORT)RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                    if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
                    }
                    LeaveCriticalSection(&NsiRpcSubscribeLock);
                    int rpcResult = RpcAsyncCompleteCall((RPC_ASYNC_STATE*)currentEntry, NULL);
                    if (((rpcResult != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level)) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
                    }
                    return;
                }
                *listPtr = entry;
                entry->Blink = (LIST_ENTRY*)listPtr;
                if (((LIST_ENTRY*)((BYTE*)listHead + 8))->Flink != NULL) {
                    HeapFree(HeapHandle, 0, ((LIST_ENTRY*)((BYTE*)listHead + 8))->Flink);
                }
                if (((LIST_ENTRY*)((BYTE*)listHead + 0x10))->Blink != NULL) {
                    HeapFree(HeapHandle, 0, ((LIST_ENTRY*)((BYTE*)listHead + 0x10))->Blink);
                }
                HeapFree(HeapHandle, 0, listHead);
            }
            
            if (currentEntry[3].Flink != NULL) {
                HeapFree(HeapHandle, 0, currentEntry[3].Flink);
            }
            HeapFree(HeapHandle, 0, currentEntry);
            currentEntry = nextEntry;
        } while (nextEntry != (LIST_ENTRY*)&listHead);
    } while (TRUE);
}

void NOTIFICATION_HANDLE_rundown(void* param1)
{
    LIST_ENTRY* nextEntry;
    LIST_ENTRY* prevEntry;
    
    EnterCriticalSection(&NsiKernelClientNotificationListLock);
    LIST_ENTRY* listEntry = NsiKernelClientNotificationList.Flink;
    
    if (*(char*)((BYTE*)param1 + 8) == 1) {
        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
        EnterCriticalSection(&DeregisterListLock);
        if ((DeregisterList.Blink)->Flink != &DeregisterList) {
            __debugbreak();
            RtlCaptureStackBackTrace(0, 0, NULL, NULL);
            return;
        }
        *(LIST_ENTRY**)param1 = &DeregisterList;
        *(LIST_ENTRY**)((BYTE*)param1 + 4) = DeregisterList.Blink;
        (DeregisterList.Blink)->Flink = (LIST_ENTRY*)param1;
        DeregisterList.Blink = (LIST_ENTRY*)param1;
        LeaveCriticalSection(&DeregisterListLock);
        SetEvent(NotificationCleanupEvent);
    } else {
        for (; listEntry != &NsiKernelClientNotificationList; listEntry = listEntry->Flink) {
            if (listEntry == (LIST_ENTRY*)param1) {
                if (listEntry != &NsiKernelClientNotificationList) {
                    nextEntry = listEntry->Flink;
                    prevEntry = listEntry->Blink;
                    if ((nextEntry->Blink != listEntry) || (prevEntry->Flink != listEntry)) {
                        __debugbreak();
                        RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                        return;
                    }
                    prevEntry->Flink = nextEntry;
                    nextEntry->Blink = prevEntry;
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                    EnterCriticalSection(&DeregisterListLock);
                    if ((DeregisterList.Blink)->Flink != &DeregisterList) {
                        __debugbreak();
                        RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                        return;
                    }
                    *(LIST_ENTRY**)param1 = &DeregisterList;
                    *(LIST_ENTRY**)((BYTE*)param1 + 4) = DeregisterList.Blink;
                    (DeregisterList.Blink)->Flink = (LIST_ENTRY*)param1;
                    DeregisterList.Blink = (LIST_ENTRY*)param1;
                    LeaveCriticalSection(&DeregisterListLock);
                    SetEvent(NotificationCleanupEvent);
                }
                break;
            }
        }
        USHORT traceId = 0x53e0;
        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
        }
    }
}

void NsipClientDisconnectCallback(RPC_ASYNC_STATE* param1, void* param2, RPC_ASYNC_EVENT param3)
{
    LIST_ENTRY* notificationEntry = (LIST_ENTRY*)param1->UserInfo;
    char cleanupRequired = 0;
    
    EnterCriticalSection(&NsiKernelClientNotificationListLock);
    LIST_ENTRY* currentEntry = NsiKernelClientNotificationList.Flink;
    LIST_ENTRY** entryPtr = NULL;
    
    if (NsiKernelClientNotificationList.Flink != &NsiKernelClientNotificationList) {
        do {
            if (currentEntry == notificationEntry) {
                LIST_ENTRY* contextEntry = notificationEntry[2].Blink;
                *(unsigned char*)&notificationEntry[1].Flink = 1;
                HANDLE heap = HeapHandle;
                
                if (contextEntry != NULL) {
                    LIST_ENTRY* asyncState = notificationEntry[3].Flink;
                    cleanupRequired = 1;
                    notificationEntry[2].Blink = NULL;
                    HeapFree(heap, 0, asyncState);
                    notificationEntry[3].Flink = NULL;
                }
                
                LIST_ENTRY* next = currentEntry->Flink;
                LIST_ENTRY* prev = currentEntry->Blink;
                if ((next->Blink != currentEntry) || (prev->Flink != currentEntry)) {
                    __debugbreak();
                    RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                    return;
                }
                prev->Flink = next;
                next->Blink = prev;
                break;
            }
            entryPtr = &currentEntry->Flink;
            currentEntry = *entryPtr;
        } while (*entryPtr != &NsiKernelClientNotificationList);
    }
    
    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
    
    if (cleanupRequired != 0) {
        EnterCriticalSection(&NsiRpcSubscribeLock);
        int result = RpcServerUnsubscribeForNotification((RPC_BINDING_HANDLE)param1->RuntimeInfo, 1, NULL);
        if (((result != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level)) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
        }
        LeaveCriticalSection(&NsiRpcSubscribeLock);
        
        int completeResult = RpcAsyncCompleteCall(param1, NULL);
        if (((completeResult != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control)) && ((1 < WPP_GLOBAL_Control->Control.Level && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)))) {
        }
    }
}

void NsipDeregisterAndFreeNotificationContext(KERNEL_NOTIFICATION_CONTEXT* param1)
{
    if (*(int*)((BYTE*)param1 + 0x54) != 0) {
        struct {
            DWORD field1;
            DWORD field2;
            DWORD field3;
            int field4;
        } deregParams;
        
        deregParams.field1 = *(DWORD*)((BYTE*)param1 + 0x28);
        deregParams.field2 = *(DWORD*)((BYTE*)param1 + 0x2c);
        deregParams.field3 = *(DWORD*)((BYTE*)param1 + 0x30);
        deregParams.field4 = *(int*)((BYTE*)param1 + 0x54);
        NsiDeregisterChangeNotificationEx(&deregParams);
    }
    
    if (*(int*)((BYTE*)param1 + 0x18) != 0) {
        HeapFree(HeapHandle, 0, *(void**)((BYTE*)param1 + 0x18));
    }
    HeapFree(HeapHandle, 0, param1);
}

void NsipFindPendingNotification(LIST_ENTRY* param1, NSI_KEYSTRUCT_DESC* param2, NSI_SINGLE_PARAM_DESC* param3, NSI_NOTIFICATION param4, LIST_ENTRY* param5)
{
    int* currentItem;
    int* nextItem;
    int* listHead = (int*)param1;
    BYTE* sourceData;
    BYTE* targetData;
    DWORD dataSize;
    BOOL isLess;
    
    currentItem = (int*)*listHead;
    
    while (1) {
        while (1) {
            while (1) {
                nextItem = currentItem;
                if (nextItem == listHead) {
                    return;
                }
                currentItem = (int*)*nextItem;
                if (nextItem[3] == ((int*)param2)[1]) {
                    break;
                }
            }
            
            sourceData = (BYTE*)*((int*)param2);
            targetData = (BYTE*)nextItem[2];
            dataSize = nextItem[3];
            
            while (dataSize > 3) {
                if (*(int*)sourceData != *(int*)targetData) {
                    break;
                }
                sourceData += 4;
                targetData += 4;
                dataSize -= 4;
            }
            
            if (dataSize == 0xFFFFFFFC) {
                dataSize = 0;
            } else {
                isLess = *sourceData < *targetData;
                BOOL allEqual = (*sourceData == *targetData);
                
                if (allEqual) {
                    if (dataSize == 0xFFFFFFFD) {
                        dataSize = 0;
                    } else if (dataSize >= 0xFFFFFFFE) {
                        isLess = sourceData[1] < targetData[1];
                        allEqual = (sourceData[1] == targetData[1]);
                        
                        if (allEqual) {
                            if (dataSize == 0xFFFFFFFE) {
                                dataSize = 0;
                            } else if (dataSize == 0xFFFFFFFF) {
                                isLess = sourceData[2] < targetData[2];
                                allEqual = (sourceData[2] == targetData[2]);
                                
                                if (allEqual) {
                                    isLess = sourceData[3] < targetData[3];
                                    allEqual = (sourceData[3] == targetData[3]);
                                    if (allEqual) {
                                        dataSize = 0;
                                    } else {
                                        dataSize = isLess ? -1 : 1;
                                    }
                                } else {
                                    dataSize = isLess ? -1 : 1;
                                }
                            }
                        } else {
                            dataSize = isLess ? -1 : 1;
                        }
                    }
                } else {
                    dataSize = isLess ? -1 : 1;
                }
            }
            
            if (dataSize == 0) {
                break;
            }
        }
        
        if ((param2 == (NSI_KEYSTRUCT_DESC*)0x2) || (param2 == (NSI_KEYSTRUCT_DESC*)0x1)) {
            int prevItem = *nextItem;
            int* prevPtr = (int*)nextItem[1];
            
            if ((*(int**)(prevItem + 4) != nextItem) || ((int*)*prevPtr != nextItem)) {
                __debugbreak();
                RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                return;
            }
            
            *prevPtr = prevItem;
            *(int**)(prevItem + 4) = prevPtr;
            
            int* paramList = (int*)param3->Parameter;
            if ((NSI_SINGLE_PARAM_DESC*)*paramList != param3) {
                __debugbreak();
                RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                return;
            }
            
            *nextItem = (int)param3;
            nextItem[1] = (int)paramList;
            *paramList = (int)nextItem;
            param3->Parameter = (BYTE*)nextItem;
        } else if ((param2 == (NSI_KEYSTRUCT_DESC*)0x0) &&
                  ((((nextItem[8] == 0) && ((LIST_ENTRY*)nextItem[6] == param1[1].Flink)) &&
                   (((LIST_ENTRY*)nextItem[7] == param1[1].Blink) &&
                    ((LIST_ENTRY*)nextItem[4] == param1->Flink))))) {
            int prevItem = *nextItem;
            int* prevPtr = (int*)nextItem[1];
            
            if ((*(int**)(prevItem + 4) == nextItem) && ((int*)*prevPtr == nextItem)) {
                *prevPtr = prevItem;
                *(int**)(prevItem + 4) = prevPtr;
                
                int* paramList = (int*)param3->Parameter;
                if ((NSI_SINGLE_PARAM_DESC*)*paramList == param3) {
                    *nextItem = (int)param3;
                    nextItem[1] = (int)paramList;
                    *paramList = (int)nextItem;
                    param3->Parameter = (BYTE*)nextItem;
                    return;
                }
            }
            
            __debugbreak();
            RtlCaptureStackBackTrace(0, 0, NULL, NULL);
            return;
        }
    }
}

void NsipFreePendingChangeList(LIST_ENTRY* param1)
{
    int* currentItem;
    int* nextItem;
    int* listHead = (int*)param1;
    
    while (1) {
        currentItem = (int*)*listHead;
        if (currentItem == listHead) {
            return;
        }
        
        nextItem = (int*)*currentItem;
        if (((int*)currentItem[1] != listHead) || ((int*)*(int*)((int)nextItem + 4) != currentItem)) {
            break;
        }
        
        *listHead = (int)nextItem;
        *(int**)((int)nextItem + 4) = listHead;
        
        if (currentItem[2] != 0) {
            HeapFree(HeapHandle, 0, (void*)currentItem[2]);
        }
        
        if (currentItem[5] != 0) {
            HeapFree(HeapHandle, 0, (void*)currentItem[5]);
        }
        
        HeapFree(HeapHandle, 0, currentItem);
    }
    
    __debugbreak();
    RtlCaptureStackBackTrace(0, 0, NULL, NULL);
}

void NsipUnsubscribeAndCompleteAsyncCall(RPC_ASYNC_STATE* param1, unsigned long param2)
{
    EnterCriticalSection(&NsiRpcSubscribeLock);
    int result = RpcServerUnsubscribeForNotification((RPC_BINDING_HANDLE)*(DWORD*)((BYTE*)param1 + 0x18), 1, NULL);
    if (((result != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level)) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
    }
    LeaveCriticalSection(&NsiRpcSubscribeLock);
    
    int completeResult = RpcAsyncCompleteCall(param1, NULL);
    if (((completeResult != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control)) && ((1 < WPP_GLOBAL_Control->Control.Level && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)))) {
    }
}

long NsiSecurityCallBack(RPC_IF_ID* param1, void* param2)
{
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    long result = RpcServerInqCallAttributesW(param2, &callAttributes);
    if ((result == 0) && (callAttributes.AuthenticationLevel < RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)) {
        result = 5;
    }
    return result;
}

long NsiStartRpcServer(void)
{
    SECURITY_DESCRIPTOR* securityDescriptor = NULL;
    RPC_BINDING_VECTOR* bindingVector = NULL;
    DWORD status = 0;
    
    wchar_t* securityString = L"D:(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)(A;;GA;;;BA)(A;;GA;;;OW)(A;;GR;;;AC)";
    
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(securityString, SDDL_REVISION_1, &securityDescriptor, NULL)) {
        DWORD error = GetLastError();
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
        }
        status = error;
    } else {
        status = RpcServerUseProtseqW(L"ncalrpc", 10, securityDescriptor);
        if (status == 0) {
            status = RpcServerRegisterIf3(&WINNSI___RpcServerInterface, 0, 0, RPC_IF_ALLOW_LOCAL_ONLY, 0x4d2, 0, NsiSecurityCallBack, securityDescriptor);
            if ((status == 0) || (status == RPC_S_ALREADY_REGISTERED)) {
                NotificationCleanupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
                if (NotificationCleanupEvent == NULL) {
                    DWORD error = GetLastError();
                    if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
                    }
                    status = RpcServerUnregisterIfEx(&WINNSI___RpcServerInterface, 0, 1);
                } else {
                    NotificationWaitHandle = (HANDLE)RegisterWaitForSingleObject(&NotificationWaitHandle, NotificationCleanupEvent, (WAITORTIMERCALLBACK)CleanupNotificationThread, NULL, INFINITE, WT_EXECUTEINWAITTHREAD);
                    if (NotificationWaitHandle == NULL) {
                        DWORD error = GetLastError();
                        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
                        }
                        status = RpcServerUnregisterIfEx(&WINNSI___RpcServerInterface, 0, 1);
                    } else {
                        status = RpcServerInqBindings(&bindingVector);
                        if (status == 0) {
                            status = RpcEpRegisterW(&WINNSI___RpcServerInterface, bindingVector, NULL, L"NSI server endpoint");
                            if (status == 0) {
                                RpcServerInitialized = TRUE;
                                status = 0;
                            } else {
                                if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                                }
                                status = RpcServerUnregisterIfEx(&WINNSI___RpcServerInterface, 0, 1);
                            }
                            if (bindingVector != NULL) {
                                RpcBindingVectorFree(&bindingVector);
                            }
                            if (securityDescriptor != NULL) {
                                LocalFree(securityDescriptor);
                            }
                        } else {
                            if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                            }
                            status = RpcServerUnregisterIfEx(&WINNSI___RpcServerInterface, 0, 1);
                        }
                    }
                }
            } else if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

            }
        } else if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

        }
    }
    return status;
}

void NsiStopRpcServer(void)
{
    if (RpcServerInitialized) {
        RpcServerInitialized = FALSE;
        
        SLIST_HEADER cleanupList;
        InitializeSListHead(&cleanupList);
        
        RPC_BINDING_VECTOR* bindingVector = NULL;
        int result = RpcServerInqBindings(&bindingVector);
        if (result == 0) {
            RpcEpUnregister(&WINNSI___RpcServerInterface, bindingVector, 0);
            RpcBindingVectorFree(&bindingVector);
        }
        
        EnterCriticalSection(&NsiKernelClientNotificationListLock);
        LIST_ENTRY* currentEntry = NsiKernelClientNotificationList.Flink;
        
        while (currentEntry != &NsiKernelClientNotificationList) {
            LIST_ENTRY* entry = currentEntry;
            currentEntry = currentEntry->Flink;
            
            if (entry[2].Blink != NULL) {
                LIST_ENTRY* asyncState = entry[3].Flink;
                asyncState->Blink = entry[2].Blink;
                InterlockedPushEntrySList(&cleanupList, (PSLIST_ENTRY)asyncState);
                
                LIST_ENTRY* next = entry->Flink;
                LIST_ENTRY* prev = entry->Blink;
                entry[3].Flink = NULL;
                entry[2].Blink = NULL;
                *(unsigned char*)&entry[1].Flink = 1;
                
                if ((next->Blink != entry) || (prev->Flink != entry)) {
                    __debugbreak();
                    RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                    return;
                }
                prev->Flink = next;
                next->Blink = prev;
            }
        }
        
        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
        
        RPC_ASYNC_STATE* asyncState = (RPC_ASYNC_STATE*)InterlockedPopEntrySList(&cleanupList);
        while (asyncState != NULL) {
            NsipUnsubscribeAndCompleteAsyncCall(asyncState, 0);
            HeapFree(HeapHandle, 0, asyncState);
            asyncState = (RPC_ASYNC_STATE*)InterlockedPopEntrySList(&cleanupList);
        }
        
        RpcServerUnregisterIfEx(&WINNSI___RpcServerInterface, 0, 1);
    }
}

DWORD RpcNsiDeregisterChangeNotification(void* param1, NSI_MODULE_DESC param2, void** param3)
{
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    void* moduleInfo = (void*)RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (moduleInfo != NULL) {
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

        }
        return (DWORD)(DWORD_PTR)moduleInfo;
    }
    
    if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
        EnterCriticalSection(&NsiKernelClientNotificationListLock);
        if (NsiKernelClientNotificationList.Flink != &NsiKernelClientNotificationList) {
            LIST_ENTRY* currentEntry = NsiKernelClientNotificationList.Flink;
            do {
                if (currentEntry == (LIST_ENTRY*)*param3) {
                    DWORD moduleSize = (DWORD)((NSI_MODULE_DESC*)param2.ModuleId)->NsiModule;
                    LIST_ENTRY** modulePtr = &currentEntry[6].Blink;
                    BOOL isLess;
                    DWORD compareResult;
                    
                    if ((*(USHORT*)modulePtr == ((NSI_MODULE_DESC*)param2.ModuleId)->NsiModule) &&
                        (currentEntry[7].Flink == (LIST_ENTRY*)((NSI_MODULE_DESC*)param2.ModuleId)->ObjectIndex)) {
                        
                        BYTE* sourceData = (BYTE*)param2.ModuleId;
                        BYTE* targetData = (BYTE*)modulePtr;
                        
                        while (moduleSize > 3) {
                            if (*(DWORD*)sourceData != *(DWORD*)targetData) {
                                break;
                            }
                            sourceData += 4;
                            targetData += 4;
                            moduleSize -= 4;
                        }
                        
                        if (moduleSize == 0xFFFFFFFC) {
                            compareResult = 0;
                        } else {
                            isLess = *sourceData < *targetData;
                            BOOL condition1 = (*sourceData == *targetData);
                            BOOL condition2 = FALSE;
                            BOOL condition3 = FALSE;
                            BOOL condition4 = FALSE;
                            
                            if (moduleSize == 0xFFFFFFFD) {
                                condition2 = TRUE;
                            } else if (moduleSize >= 0xFFFFFFFE) {
                                condition2 = (sourceData[1] == targetData[1]);
                                if (moduleSize == 0xFFFFFFFE) {
                                    condition3 = TRUE;
                                } else if (moduleSize == 0xFFFFFFFF) {
                                    condition3 = (sourceData[2] == targetData[2]);
                                    condition4 = (sourceData[3] == targetData[3]);
                                }
                            }
                            
                            if (condition1 && (moduleSize == 0xFFFFFFFD || (condition2 && (moduleSize == 0xFFFFFFFE || (condition3 && (moduleSize == 0xFFFFFFFF || condition4)))))) {
                                compareResult = 0;
                            } else {
                                compareResult = isLess ? -1 : 1;
                            }
                        }
                        
                        if ((compareResult == 0) && (currentEntry[6].Flink == (LIST_ENTRY*)param2.ObjectIndex)) {
                            if (currentEntry != &NsiKernelClientNotificationList) {
                                LIST_ENTRY* next = currentEntry->Flink;
                                LIST_ENTRY* prev = currentEntry->Blink;
                                
                                if ((next->Blink != currentEntry) || (prev->Flink != currentEntry)) {
                                    __debugbreak();
                                    if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                                    }
                                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                                    return 0;
                                }
                                
                                prev->Flink = next;
                                next->Blink = prev;
                                *(unsigned char*)&currentEntry[1].Flink = 1;
                                LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                                
                                if (currentEntry[2].Blink != NULL) {
                                    LIST_ENTRY* asyncState = currentEntry[3].Flink;
                                    currentEntry[2].Blink = NULL;
                                    HeapFree(HeapHandle, 0, asyncState);
                                    currentEntry[3].Flink = NULL;
                                    NsipUnsubscribeAndCompleteAsyncCall((RPC_ASYNC_STATE*)currentEntry, 0);
                                }
                                
                                LIST_ENTRY** listPtr = &currentEntry[1].Blink;
                                LIST_ENTRY* listEntry = *listPtr;
                                
                                while ((LIST_ENTRY**)listEntry != listPtr) {
                                    LIST_ENTRY* nextEntry = listEntry->Flink;
                                    if ((listEntry->Blink != (LIST_ENTRY*)listPtr) || (nextEntry->Blink != listEntry)) {
                                        __debugbreak();
                                        break;
                                    }
                                    *listPtr = nextEntry;
                                    nextEntry->Blink = (LIST_ENTRY*)listPtr;
                                    
                                    if (((LIST_ENTRY*)((DWORD_PTR)listEntry + 8))->Flink != NULL) {
                                        HeapFree(HeapHandle, 0, ((LIST_ENTRY*)((DWORD_PTR)listEntry + 8))->Flink);
                                    }
                                    
                                    if (((LIST_ENTRY*)((DWORD_PTR)listEntry + 0x10))->Blink != NULL) {
                                        HeapFree(HeapHandle, 0, ((LIST_ENTRY*)((DWORD_PTR)listEntry + 0x10))->Blink);
                                    }
                                    
                                    HeapFree(HeapHandle, 0, listEntry);
                                    listEntry = *listPtr;
                                }
                                
                                DWORD impersonateResult = RpcImpersonateClient(param1);
                                if (impersonateResult == 0) {
                                    DWORD deregResult = NsiDeregisterChangeNotificationEx(NULL);
                                    currentEntry[10].Blink = NULL;
                                    RpcRevertToSelf();
                                    int contextResult = RpcSsContextLockExclusive(param1, *param3);
                                    if (contextResult == 0x460) {
                                        moduleInfo = NULL;
                                    } else {
                                        HeapFree(HeapHandle, 0, currentEntry);
                                        *param3 = NULL;
                                    }
                                    return (DWORD)(DWORD_PTR)moduleInfo;
                                }
                                
                                if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                                }
                                return (DWORD)(DWORD_PTR)moduleInfo;
                            }
                        }
                    }
                }
                currentEntry = currentEntry->Flink;
            } while (currentEntry != &NsiKernelClientNotificationList);
        }
        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
        
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level)) {
            if ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0) {

            }
        }
    } else {
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level)) {
            if ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0) {

            }
        }
    }
    
    return 0x490;
}

DWORD RpcNsiEnumerateObjectsAllParameters(void* param1, NSI_MODULE_DESC param2, NSI_STORE param3, NSI_GET_ACTION param4, NSI_KEYSTRUCT_DESC_ENUM* param5, NSI_PARAM_STRUCT_DESC_ENUM* param6, DWORD* param7)
{
    if ((param3 == NsiBoth) || (param4 == NsiGetExact)) {
        return 0x57;
    } else if (param3 == NsiActive) {
        DWORD count = *param7;
        if (((param5->TotalKeyStructLength == count * param5->KeyStructLength) && 
             (param6->TotalRoDynamicLength == param6->RoDynamicParameterStructLength * count)) &&
            (param6->TotalRoStaticLength == param6->RoStaticParameterStructLength * count) &&
            (param6->TotalRwLength == param6->RwParameterStructLength * count)) {
            
            RPC_CALL_ATTRIBUTES_V2_W callAttributes;
            memset(&callAttributes, 0, sizeof(callAttributes));
            callAttributes.Version = 2;
            callAttributes.Flags = 0;
            
            DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
            if (result == 0) {
                if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
                    DWORD impersonateResult = RpcImpersonateClient(param1);
                    if (impersonateResult == 0) {
                        struct {
                            DWORD field1;
                            DWORD nsiModule;
                            void* moduleId;
                            DWORD objectIndex;
                            DWORD store;
                            DWORD action;
                            DWORD keyStructLength;
                            DWORD totalKeyStructLength;
                            DWORD rwParamStructLength;
                            DWORD totalRwLength;
                            DWORD roDynamicParamStructLength;
                            DWORD totalRoDynamicLength;
                            DWORD roStaticParamStructLength;
                            DWORD totalRoStaticLength;
                        } params;
                        
                        memset(&params, 0, sizeof(params));
                        params.nsiModule = param2.NsiModule;
                        params.moduleId = param2.ModuleId;
                        params.objectIndex = param2.ObjectIndex;
                        params.store = param3;
                        params.action = param4;
                        params.keyStructLength = param5->KeyStructLength;
                        params.totalKeyStructLength = param5->TotalKeyStructLength;
                        params.rwParamStructLength = param6->RwParameterStructLength;
                        params.totalRwLength = param6->TotalRwLength;
                        params.roDynamicParamStructLength = param6->RoDynamicParameterStructLength;
                        params.totalRoDynamicLength = param6->TotalRoDynamicLength;
                        params.roStaticParamStructLength = param6->RoStaticParameterStructLength;
                        params.totalRoStaticLength = param6->TotalRoStaticLength;
                        
                        result = NsiEnumerateObjectsAllParametersEx(&params);
                        *param7 = count;
                        RpcRevertToSelf();
                    }
                    return impersonateResult;
                } else {
                    return 0x490;
                }
            }
            return result;
        } else {
            return 0x6f8;
        }
    } else {
        return 0x78;
    }
}

DWORD RpcNsiGetAllParameters(void* param1, NSI_MODULE_DESC param2, NSI_STORE param3, NSI_GET_ACTION param4, NSI_KEYSTRUCT_DESC* param5, NSI_PARAM_STRUCT_DESC* param6)
{
    if (param3 == NsiBoth) {
        return 0x57;
    }
    
    struct {
        DWORD field1;
        DWORD nsiModule;
        void* moduleId;
        DWORD objectIndex;
        DWORD store;
        DWORD action;
        BYTE* keyStruct;
        DWORD keyStructLength;
        BYTE* rwParamStruct;
        DWORD rwParamStructLength;
        BYTE* roDynamicParamStruct;
        DWORD roDynamicParamStructLength;
        BYTE* roStaticParamStruct;
        DWORD roStaticParamStructLength;
    } params;
    
    memset(&params, 0, sizeof(params));
    params.action = param4;
    params.keyStruct = param5->KeyStruct;
    params.keyStructLength = param5->KeyStructLength;
    params.nsiModule = param2.NsiModule;
    params.moduleId = param2.ModuleId;
    params.objectIndex = param2.ObjectIndex;
    params.rwParamStruct = param6->RwParameterStruct;
    params.rwParamStructLength = param6->RwParameterStructLength;
    params.roDynamicParamStruct = param6->RoDynamicParameterStruct;
    params.roDynamicParamStructLength = param6->RoDynamicParameterStructLength;
    params.roStaticParamStruct = param6->RoStaticParameterStruct;
    params.roStaticParamStructLength = param6->RoStaticParameterStructLength;
    params.store = param3;
    
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            DWORD impersonateResult = RpcImpersonateClient(param1);
            if (impersonateResult == 0) {
                result = NsiGetAllParametersEx(&params);
                RpcRevertToSelf();
            }
            return impersonateResult;
        } else {
            return 0x490;
        }
    }
    return result;
}

DWORD RpcNsiGetParameter(void* param1, NSI_MODULE_DESC param2, NSI_STORE param3, NSI_GET_ACTION param4, NSI_KEYSTRUCT_DESC* param5, NSI_SINGLE_PARAM_DESC* param6)
{
    if (((param3 == NsiBoth) || (NsiCurrent < param3)) || (2 < (int)param4) ||
        (((param5->KeyStructLength != 0 && (param5->KeyStruct == NULL)) || 
         ((param6->ParameterLength != 0 && (param6->Parameter == NULL)))))) {
        return 0x57;
    }
    
    struct {
        DWORD field1;
        DWORD nsiModule;
        void* moduleId;
        DWORD objectIndex;
        DWORD store;
        DWORD action;
        BYTE* keyStruct;
        DWORD keyStructLength;
        DWORD structType;
        BYTE* parameter;
        DWORD parameterLength;
        DWORD parameterOffset;
    } params;
    
    memset(&params, 0, sizeof(params));
    params.action = param4;
    params.keyStruct = param5->KeyStruct;
    params.keyStructLength = param5->KeyStructLength;
    params.nsiModule = param2.NsiModule;
    params.moduleId = param2.ModuleId;
    params.objectIndex = param2.ObjectIndex;
    params.structType = param6->StructType;
    params.parameter = param6->Parameter;
    params.parameterLength = param6->ParameterLength;
    params.parameterOffset = param6->ParameterOffset;
    params.store = param3;
    
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            DWORD impersonateResult = RpcImpersonateClient(param1);
            if (impersonateResult == 0) {
                result = NsiGetParameterEx(&params);
                RpcRevertToSelf();
            }
            return impersonateResult;
        } else {
            return 0x490;
        }
    }
    return result;
}

DWORD RpcNsiParameterChange(void* param1, NSI_MODULE_DESC param2, NSI_KEYSTRUCT_DESC param3, NSI_SINGLE_PARAM_DESC param4, NSI_NOTIFICATION_DESC param5, NSI_NOTIFICATION param6)
{
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

            }
            return 0x57;
        } else {
            SLIST_HEADER cleanupList;
            InitializeSListHead(&cleanupList);
            
            EnterCriticalSection(&NsiKernelClientNotificationListLock);
            LIST_ENTRY* currentEntry = NsiKernelClientNotificationList.Flink;
            LIST_ENTRY* notificationEntry = NULL;
            BOOL isLess;
            DWORD compareResult;
            
            while (currentEntry != &NsiKernelClientNotificationList) {
                LIST_ENTRY* nextEntry = currentEntry->Flink;
                
                LIST_ENTRY** modulePtr = &currentEntry[6].Blink;
                DWORD moduleSize = 0x14;
                BYTE* sourceData = (BYTE*)param2.ModuleId;
                BYTE* targetData = (BYTE*)modulePtr;
                
                compareResult = 0;
                while (moduleSize > 3) {
                    if (*(DWORD*)sourceData != *(DWORD*)targetData) {
                        compareResult = 1;
                        break;
                    }
                    sourceData += 4;
                    targetData += 4;
                    moduleSize -= 4;
                }
                
                if (compareResult == 0 && moduleSize != 0xFFFFFFFC) {
                    isLess = *sourceData < *targetData;
                    if (!(*sourceData == *targetData &&
                         (moduleSize == 0xFFFFFFFD || 
                         (sourceData[1] == targetData[1] &&
                         (moduleSize == 0xFFFFFFFE || 
                         (sourceData[2] == targetData[2] &&
                         (moduleSize == 0xFFFFFFFF || sourceData[3] == targetData[3]))))))) {
                        compareResult = (DWORD)(-(int)isLess | 1);
                    }
                }
                
                if ((compareResult == 0) && ((LIST_ENTRY*)param2.ObjectIndex == currentEntry[6].Flink) &&
                    ((currentEntry[9].Blink == (LIST_ENTRY*)0xFFFFFFFF) ||
                     ((param5.CompartmentId == 0) || 
                      ((LIST_ENTRY*)param5.CompartmentId == currentEntry[10].Flink)))) {
                    
                    if (currentEntry[2].Blink == NULL) {
                        notificationEntry = (LIST_ENTRY*)HeapAlloc(HeapHandle, 0, 0x1000);
                        if (notificationEntry == NULL) {
                            result = 8;
                            break;
                        }
                        
                        memset(notificationEntry, 0, 0x1000);
                        notificationEntry[3].Blink = (LIST_ENTRY*)param4.Parameter;
                        notificationEntry[2].Flink = (LIST_ENTRY*)param2.ModuleId;
                        notificationEntry[4].Flink = (LIST_ENTRY*)param6;
                        
                        LIST_ENTRY searchList;
                        NsipFindPendingNotification(&searchList, (NSI_KEYSTRUCT_DESC*)param6, &param4, (NSI_NOTIFICATION)&searchList, notificationEntry);
                        NsipFreePendingChangeList(&searchList);
                        
                        LIST_ENTRY* listHead = currentEntry[2].Flink;
                        if (listHead->Flink != (LIST_ENTRY*)&currentEntry[1].Blink) {
                            __debugbreak();
                            result = (DWORD)RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                            break;
                        }
                        
                        LIST_ENTRY newEntry;
                        newEntry.Flink = (LIST_ENTRY*)&currentEntry[1].Blink;
                        newEntry.Blink = listHead;
                        listHead->Flink = &newEntry;
                        currentEntry[2].Flink = &newEntry;
                    } else {
                        result = 0;
                        notificationEntry = NULL;
                        LIST_ENTRY* pendingEntry = currentEntry[3].Blink;
                        pendingEntry->Flink = NULL;
                        pendingEntry->Blink = NULL;
                        
                        LIST_ENTRY* changeEntry = currentEntry[4].Flink;
                        changeEntry->Flink = (LIST_ENTRY*)param2.ModuleId;
                        changeEntry->Blink = NULL;
                        changeEntry[1].Flink = NULL;
                        changeEntry[1].Blink = (LIST_ENTRY*)param4.Parameter;
                        (currentEntry[4].Blink)->Flink = (LIST_ENTRY*)param6;
                        (currentEntry[3].Blink)->Flink = NULL;
                        (currentEntry[4].Flink)->Blink = NULL;
                        
                        LIST_ENTRY* asyncState = currentEntry[3].Flink;
                        asyncState->Blink = currentEntry[2].Blink;
                        InterlockedPushEntrySList(&cleanupList, (PSLIST_ENTRY)asyncState);
                        currentEntry[2].Blink = NULL;
                        currentEntry[3].Flink = NULL;
                    }
                }
                
                currentEntry = nextEntry;
            }
            
            LeaveCriticalSection(&NsiKernelClientNotificationListLock);
            
            LIST_ENTRY** cleanupEntry = (LIST_ENTRY**)InterlockedPopEntrySList(&cleanupList);
            while (cleanupEntry != NULL) {
                LIST_ENTRY* asyncHandle = (LIST_ENTRY*)cleanupEntry[1];
                EnterCriticalSection(&NsiRpcSubscribeLock);
                int unsubscribeResult = RpcServerUnsubscribeForNotification((RPC_BINDING_HANDLE)asyncHandle[3].Flink, 1, NULL);
                if (unsubscribeResult != 0) {
                    if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                    }
                }
                LeaveCriticalSection(&NsiRpcSubscribeLock);
                
                int completeResult = RpcAsyncCompleteCall((RPC_ASYNC_STATE*)asyncHandle, NULL);
                if (((completeResult != 0) && (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control)) && 
                    ((1 < WPP_GLOBAL_Control->Control.Level && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)))) {

                }
                
                HeapFree(HeapHandle, 0, cleanupEntry);
                cleanupEntry = (LIST_ENTRY**)InterlockedPopEntrySList(&cleanupList);
            }
            
            if ((result != 0) && (notificationEntry != NULL)) {
                HeapFree(HeapHandle, 0, notificationEntry);
            }
        }
    } else if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

    }
    
    return result;
}

DWORD RpcNsiRegisterChangeNotification(void* param1, NSI_MODULE_DESC param2, NSI_NOTIFICATION_REGISTRATION_DESC param3, void** param4)
{
    *param4 = NULL;
    
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            LIST_ENTRY* notificationEntry = (LIST_ENTRY*)HeapAlloc(HeapHandle, 0, 0x58);
            if (notificationEntry == NULL) {
                SetLastError(8);
                return 8;
            }
            
            memset(notificationEntry, 0, 0x58);
            result = RpcImpersonateClient(param1);
            if (result == 0) {
                struct {
                    DWORD field1;
                    DWORD nsiModule;
                    void* moduleId;
                    DWORD objectIndex;
                    DWORD compartmentScope;
                    DWORD compartmentId;
                } registerParams;
                
                memset(&registerParams, 0, sizeof(registerParams));
                registerParams.nsiModule = param2.NsiModule;
                registerParams.moduleId = param2.ModuleId;
                registerParams.objectIndex = param2.ObjectIndex;
                registerParams.compartmentScope = param3.CompartmentScope;
                registerParams.compartmentId = param3.CompartmentId;
                
                result = NsiRegisterChangeNotificationEx(&registerParams);
                RpcRevertToSelf();
                
                if (result == 0) {
                    LIST_ENTRY** listPtr = &notificationEntry[1].Blink;
                    notificationEntry[2].Flink = (LIST_ENTRY*)listPtr;
                    *listPtr = (LIST_ENTRY*)listPtr;
                    notificationEntry[5].Flink = (LIST_ENTRY*)param2.NsiModule;
                    notificationEntry[5].Blink = (LIST_ENTRY*)param2.ModuleId;
                    notificationEntry[6].Flink = (LIST_ENTRY*)param2.ObjectIndex;
                    notificationEntry[6].Blink = *((LIST_ENTRY**)param2.ModuleId);
                    notificationEntry[7].Flink = (LIST_ENTRY*)((DWORD_PTR)param2.ModuleId + 4);
                    notificationEntry[7].Blink = (LIST_ENTRY*)((DWORD_PTR)param2.ModuleId + 8);
                    notificationEntry[8].Flink = (LIST_ENTRY*)((DWORD_PTR)param2.ModuleId + 12);
                    notificationEntry[8].Blink = (LIST_ENTRY*)((DWORD_PTR)param2.ModuleId + 16);
                    notificationEntry[9].Flink = (LIST_ENTRY*)((DWORD_PTR)param2.ModuleId + 20);
                    notificationEntry[5].Blink = (LIST_ENTRY*)&notificationEntry[6].Blink;
                    notificationEntry[9].Blink = (LIST_ENTRY*)param3.CompartmentScope;
                    notificationEntry[10].Flink = (LIST_ENTRY*)param3.CompartmentId;
                    notificationEntry[10].Blink = NULL;
                    
                    EnterCriticalSection(&NsiKernelClientNotificationListLock);
                    
                    if (RpcServerInitialized == FALSE) {
                        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                        NsipDeregisterAndFreeNotificationContext((KERNEL_NOTIFICATION_CONTEXT*)notificationEntry);
                        result = 0x139f;
                    } else {
                        if ((NsiKernelClientNotificationList.Blink)->Flink != &NsiKernelClientNotificationList) {
                            __debugbreak();
                            RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                            LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                            HeapFree(HeapHandle, 0, notificationEntry);
                            return 0;
                        }
                        
                        notificationEntry->Flink = &NsiKernelClientNotificationList;
                        notificationEntry->Blink = NsiKernelClientNotificationList.Blink;
                        (NsiKernelClientNotificationList.Blink)->Flink = notificationEntry;
                        NsiKernelClientNotificationList.Blink = notificationEntry;
                        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                        
                        *param4 = notificationEntry;
                        result = 0;
                    }
                }
            }
            
            if (result != 0) {
                HeapFree(HeapHandle, 0, notificationEntry);
            }
        } else {
            result = 0x490;
        }
    }
    
    return result;
}

void RpcNsiRequestChangeNotification(RPC_ASYNC_STATE* param1, void* param2, void* param3, NSI_KEYSTRUCT_DESC* param4, NSI_SINGLE_PARAM_DESC* param5, NSI_NOTIFICATION* param6)
{
    DWORD status = 0;
    status = RpcImpersonateClient(param2);
    
    if (status == 0) {
        RpcRevertToSelf();
        
        if (param3 == NULL) {
            if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

            }
            status = 0x57;
            RpcAsyncCompleteCall(param1, &status);
        } else {
            EnterCriticalSection(&NsiKernelClientNotificationListLock);
            
            if (RpcServerInitialized == FALSE) {
                if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

                }
                LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                status = 0x4c7;
                RpcAsyncCompleteCall(param1, &status);
            } else if (*(char*)((BYTE*)param3 + 8) != 1) {
                if (*(int*)((BYTE*)param3 + 0x14) != 0) {
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                    status = 0x57;
                    RpcAsyncCompleteCall(param1, &status);
                    return;
                }
                
                int* listHead = (int*)((BYTE*)param3 + 0xc);
                if ((int*)*listHead != listHead) {
                    int* currentItem = (int*)*listHead;
                    int nextItem = *currentItem;
                    
                    if (((int*)currentItem[1] == listHead) && (*(int**)(nextItem + 4) == currentItem)) {
                        *listHead = nextItem;
                        *(int**)(nextItem + 4) = listHead;
                        LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                        
                        param4->KeyStruct = (BYTE*)currentItem[2];
                        param4->KeyStructLength = currentItem[3];
                        param5->StructType = currentItem[4];
                        param5->Parameter = (BYTE*)currentItem[5];
                        param5->ParameterLength = currentItem[6];
                        param5->ParameterOffset = currentItem[7];
                        *param6 = currentItem[8];
                        
                        RpcAsyncCompleteCall(param1, &status);
                        HeapFree(HeapHandle, 0, currentItem);
                        return;
                    }
                    
                    __debugbreak();
                    RtlCaptureStackBackTrace(0, 0, NULL, NULL);
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                    return;
                }
                
                param1->UserInfo = param3;
                EnterCriticalSection(&NsiRpcSubscribeLock);
                int subscribeResult = RpcServerSubscribeForNotification((RPC_BINDING_HANDLE)param1->RuntimeInfo, 1, 5, NULL);
                LeaveCriticalSection(&NsiRpcSubscribeLock);
                
                if (subscribeResult != 0) {
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                    status = subscribeResult;
                    RpcAsyncCompleteCall(param1, &status);
                    return;
                }
                
                DWORD* contextData = (DWORD*)HeapAlloc(HeapHandle, 0, 8);
                if (contextData != NULL) {
                    *contextData = 0;
                    contextData[1] = 0;
                    *(NSI_KEYSTRUCT_DESC**)((BYTE*)param3 + 0x1c) = param4;
                    *(NSI_SINGLE_PARAM_DESC**)((BYTE*)param3 + 0x20) = param5;
                    *(NSI_NOTIFICATION**)((BYTE*)param3 + 0x24) = param6;
                    *(RPC_ASYNC_STATE**)((BYTE*)param3 + 0x14) = param1;
                    *(DWORD**)((BYTE*)param3 + 0x18) = contextData;
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                } else {
                    SetLastError(8);
                    LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                    NsipUnsubscribeAndCompleteAsyncCall((RPC_ASYNC_STATE*)&NsiKernelClientNotificationListLock, 8);
                }
            } else {
                LeaveCriticalSection(&NsiKernelClientNotificationListLock);
                status = 0x4c7;
                RpcAsyncCompleteCall(param1, &status);
            }
        }
    } else {
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {

        }
        RpcAsyncCompleteCall(param1, &status);
    }
}

DWORD RpcNsiSetAllParameters(void* param1, NSI_MODULE_DESC param2, NSI_STORE param3, NSI_SET_ACTION param4, NSI_KEYSTRUCT_DESC* param5, NSI_RW_PARAM_STRUCT_DESC* param6)
{
    if (param3 == NsiCurrent) {
        return 0x57;
    }
    
    struct {
        DWORD field1;
        DWORD nsiModule;
        void* moduleId;
        DWORD objectIndex;
        DWORD store;
        DWORD action;
        BYTE* keyStruct;
        DWORD keyStructLength;
        BYTE* rwParamStruct;
        DWORD rwParamStructLength;
    } params;
    
    memset(&params, 0, sizeof(params));
    params.nsiModule = param2.NsiModule;
    params.moduleId = param2.ModuleId;
    params.objectIndex = param2.ObjectIndex;
    params.store = param3;
    params.action = param4;
    params.keyStruct = param5->KeyStruct;
    params.keyStructLength = param5->KeyStructLength;
    params.rwParamStruct = param6->RwParameterStruct;
    params.rwParamStructLength = param6->RwParameterStructLength;
    
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            DWORD impersonateResult = RpcImpersonateClient(param1);
            if (impersonateResult == 0) {
                result = NsiSetAllParametersEx(&params);
                RpcRevertToSelf();
            }
            return impersonateResult;
        } else {
            return 0x490;
        }
    }
    return result;
}

DWORD RpcNsiSetParameter(void* param1, NSI_MODULE_DESC param2, NSI_STORE param3, NSI_SET_ACTION param4, NSI_KEYSTRUCT_DESC* param5, NSI_SINGLE_PARAM_DESC* param6)
{
    if (param3 == NsiCurrent) {
        return 0x57;
    }
    
    struct {
        DWORD field1;
        DWORD nsiModule;
        void* moduleId;
        DWORD objectIndex;
        DWORD store;
        DWORD action;
        BYTE* keyStruct;
        DWORD keyStructLength;
        DWORD structType;
        BYTE* parameter;
        DWORD parameterLength;
        DWORD parameterOffset;
    } params;
    
    memset(&params, 0, sizeof(params));
    params.nsiModule = param2.NsiModule;
    params.moduleId = param2.ModuleId;
    params.objectIndex = param2.ObjectIndex;
    params.store = param3;
    params.action = param4;
    params.keyStruct = param5->KeyStruct;
    params.keyStructLength = param5->KeyStructLength;
    params.structType = param6->StructType;
    params.parameter = param6->Parameter;
    params.parameterLength = param6->ParameterLength;
    params.parameterOffset = param6->ParameterOffset;
    
    RPC_CALL_ATTRIBUTES_V2_W callAttributes;
    memset(&callAttributes, 0, sizeof(callAttributes));
    callAttributes.Version = 2;
    callAttributes.Flags = 0;
    
    DWORD result = RpcServerInqCallAttributesW(NULL, &callAttributes);
    if (result == 0) {
        if (callAttributes.AuthenticationLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) {
            DWORD impersonateResult = RpcImpersonateClient(param1);
            if (impersonateResult == 0) {
                result = NsiSetParameterEx(&params);
                RpcRevertToSelf();
            }
            return impersonateResult;
        } else {
            return 0x490;
        }
    }
    return result;
}

DWORD WINAPI NsiServiceControlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch (dwControl) {
        case SERVICE_CONTROL_STOP:
            NsiServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            NsiServiceStatus.dwCheckPoint = 1;
            NsiServiceStatus.dwWaitHint = 5000;
            NsiServiceUpdateStatus();
            
            NsiStopRpcServer();
            SetEvent(NsiServiceStopEvent);
            break;
            
        case SERVICE_CONTROL_INTERROGATE:
            break;
            
        default:
            break;
    }
    
    return NO_ERROR;
}

void NsiServiceCleanup(void* param1, unsigned char param2)
{
    if (NotificationWaitHandle != NULL) {
        UnregisterWait(NotificationWaitHandle);
        NotificationWaitHandle = NULL;
    }
    
    if (NotificationCleanupEvent != NULL) {
        CloseHandle(NotificationCleanupEvent);
        NotificationCleanupEvent = NULL;
    }
    
    if (NsiServiceStopEvent != NULL) {
        CloseHandle(NsiServiceStopEvent);
        NsiServiceStopEvent = NULL;
    }
    
    DeleteCriticalSection(&DeregisterListLock);
    DeleteCriticalSection(&NsiKernelClientNotificationListLock);
    DeleteCriticalSection(&NsiRpcSubscribeLock);
    
}

void NsiServiceUpdateStatus(void)
{
    SetServiceStatus(NsiServiceHandle, &NsiServiceStatus);
}

void ServiceMain(DWORD dwArgc, wchar_t** lpszArgv)
{
    // Initialize WPP control block
    WPP_PROJECT_CONTROL_BLOCK WppControl;
    WppControl.Control.Next = NULL;
    WppControl.Control.Logger = 0;
    WppControl.Control.Flags[0] = 0;
    WppControl.Control.Flags[1] = 0;
    WppControl.Control.Level = 1;
    WPP_GLOBAL_Control = &WppControl;
    
    
    InitializeCriticalSection(&DeregisterListLock);
    InitializeCriticalSection(&NsiKernelClientNotificationListLock);
    InitializeCriticalSection(&NsiRpcSubscribeLock);
    
    // Initialize list heads
    DeregisterList.Flink = &DeregisterList;
    DeregisterList.Blink = &DeregisterList;
    NsiKernelClientNotificationList.Flink = &NsiKernelClientNotificationList;
    NsiKernelClientNotificationList.Blink = &NsiKernelClientNotificationList;
    
    HeapHandle = GetProcessHeap();
    NsiServiceStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    
    NsiServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    NsiServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    NsiServiceStatus.dwControlsAccepted = 0;
    NsiServiceStatus.dwWin32ExitCode = 0;
    NsiServiceStatus.dwServiceSpecificExitCode = 0;
    NsiServiceStatus.dwCheckPoint = 1;
    NsiServiceStatus.dwWaitHint = 5000;
    
    NsiServiceHandle = RegisterServiceCtrlHandlerExW(L"NSI", NsiServiceControlHandler, NULL);
    if (NsiServiceHandle == NULL) {
        if ((WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) && (1 < WPP_GLOBAL_Control->Control.Level) && ((WPP_GLOBAL_Control->Control.Flags[0] & 0x10) != 0)) {
        }
        return;
    }
    
    NsiServiceUpdateStatus();
    
    void* initResult = (void*)NsiServiceInitialize();
    if (initResult == NULL) {
        initResult = (void*)NsiStartRpcServer();
        if (initResult == NULL) {
            if (NsiSvcHostGlobalData != NULL && NsiSvcHostGlobalData->RegisterStopCallback != NULL) {
                typedef BOOL (WINAPI *REGISTER_STOP_CALLBACK)(HANDLE*, const wchar_t*, HANDLE, PVOID, PVOID, DWORD);
                ((REGISTER_STOP_CALLBACK)NsiSvcHostGlobalData->RegisterStopCallback)(&NsiServiceWaitObject, L"NSI", NsiServiceStopEvent, NsiServiceCleanup, NULL, 0x18);
            }
            
            NsiServiceStatus.dwCurrentState = SERVICE_RUNNING;
            NsiServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
            NsiServiceStatus.dwWin32ExitCode = NO_ERROR;
            NsiServiceStatus.dwServiceSpecificExitCode = 0;
            NsiServiceStatus.dwCheckPoint = 0;
            NsiServiceStatus.dwWaitHint = 0;
            NsiServiceUpdateStatus();
            
            WaitForSingleObject(NsiServiceStopEvent, INFINITE);
            
            NsiServiceStatus.dwCurrentState = SERVICE_STOPPED;
            NsiServiceStatus.dwWin32ExitCode = NO_ERROR;
            NsiServiceUpdateStatus();
            return;
        }
    }
    
    NsiServiceStatus.dwCurrentState = SERVICE_STOPPED;
    NsiServiceStatus.dwWin32ExitCode = (DWORD)(DWORD_PTR)initResult;
    NsiServiceStatus.dwCheckPoint = 1;
    NsiServiceStatus.dwWaitHint = 0;
    NsiServiceCleanup(NULL, 0);
    NsiServiceUpdateStatus();
}

void SvchostPushServiceGlobals(SVCHOST_GLOBAL_DATA* param1)
{
    NsiSvcHostGlobalData = param1;
}

const GUID NsiServiceTraceGuid = { 0xa1f8b440, 0x7b2b, 0x4e29, { 0x8a, 0x17, 0x90, 0xa4, 0xd2, 0xd4, 0xc9, 0xc1 } };

