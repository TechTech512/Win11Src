#include <windows.h>
#include <rpc.h>
#include <rpcasync.h>
#include <iphlpapi.h>
#include <winbase.h>
#include <threadpoolapiset.h>
#include "nsi_c_stub.c"

// Global variables
extern LIST_ENTRY NotificationListHead;
extern CRITICAL_SECTION NotificationListLock;
extern HANDLE HeapHandle;

// External function declarations
extern ULONG NsiEnumerateObjectsAllParameters(
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength,
    void *readOnlyStruct,
    ULONG readOnlyStructLength,
    void *dynamicStruct,
    ULONG dynamicStructLength,
    ULONG *entryCount
);
extern ULONG NsiGetParameterEx(void *request);
extern ULONG NsiSetAllParametersEx(void *request);
extern ULONG NsiSetParameterEx(void *request);
extern ULONG NsiGetAllParametersEx(void *request);

// Constants
#define NSI_BOTH 0
#define NSI_CURRENT 1
#define NSI_GET_EXACT 0
#define NPI_MS_NDIS_MODULEID {0}

void * __cdecl NsiConnectToServer(WCHAR *serverName);
void __cdecl NsiDisconnectFromServer(void *bindingHandle);
ULONG __cdecl NsiRpcDeregisterChangeNotification(
    void *bindingHandle,
    void *moduleId,
    ULONG objectIndex,
    void *notificationHandle
);
ULONG __cdecl NsiRpcDeregisterChangeNotificationEx(
    void *bindingHandle,
    void *request
);
ULONG __cdecl NsiRpcEnumerateObjectsAllParameters(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength,
    void *readOnlyStruct,
    ULONG readOnlyStructLength,
    void *dynamicStruct,
    ULONG dynamicStructLength,
    ULONG *entryCount
);
ULONG __cdecl NsiRpcGetAllParameters(
    void *bindingHandle,
    DWORD store,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength,
    void *readOnlyStruct,
    ULONG readOnlyStructLength,
    void *dynamicStruct,
    ULONG dynamicStructLength
);
ULONG __cdecl NsiRpcGetAllParametersEx(void *bindingHandle, void *request);
ULONG __cdecl NsiRpcGetParameterEx(void *bindingHandle, void *request);
ULONG __cdecl NsiRpcRegisterChangeNotification(
    void *bindingHandle,
    void *moduleId,
    ULONG objectIndex,
    void *callback,
    BYTE initialNotification,
    void *callerContext,
    void **notificationHandle
);
ULONG __cdecl NsiRpcRegisterChangeNotificationEx(void *bindingHandle, void *request);
ULONG __cdecl NsiRpcSetAllParameters(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength
);
ULONG __cdecl NsiRpcSetAllParametersEx(void *bindingHandle, void *request);
ULONG __cdecl NsiRpcSetParameter(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    DWORD structType,
    void *parameter,
    ULONG parameterLength,
    ULONG parameterOffset
);
ULONG __cdecl NsiRpcSetParameterEx(void *bindingHandle, void *request);
void __cdecl NsiWorkerThread(void *context, BYTE timerOrWaitFired);
long __cdecl PrepareRpcAsyncState(RPC_ASYNC_STATE *asyncState);
ULONG FUN_1000219d(void);

ULONG FUN_1000219d(void)
{
    return 0;
}

void * __cdecl NsiConnectToServer(WCHAR *serverName)
{
    int result;
    void *bindingHandle = NULL;
    WCHAR *stringBinding = NULL;
    
    if (serverName == NULL) {
        result = RpcStringBindingComposeW(
            NULL,
            L"ncalrpc",
            NULL,
            NULL,
            L"Security=Impersonation Dynamic True",
            &stringBinding
        );
        
        if (result == RPC_S_OK) {
            result = RpcBindingFromStringBindingW(stringBinding, &bindingHandle);
            RpcStringFreeW(&stringBinding);
            
            if (result == RPC_S_OK) {
                result = RpcBindingSetAuthInfoW(
                    bindingHandle,
                    NULL,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    0
                );
                
                if (result == RPC_S_OK) {
                    return bindingHandle;
                }
                
                RpcBindingFree(&bindingHandle);
                FUN_1000219d();
            }
        }
    }
    
    return NULL;
}

void __cdecl NsiDisconnectFromServer(void *bindingHandle)
{
    RpcBindingFree(&bindingHandle);
}

ULONG __cdecl NsiRpcDeregisterChangeNotification(
    void *bindingHandle,
    void *moduleId,
    ULONG objectIndex,
    void *notificationHandle
)
{
    struct {
        void *moduleId;
        ULONG objectIndex;
        void *notificationHandle;
        DWORD nsiModule;
    } request;
    
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.notificationHandle = notificationHandle;
    request.nsiModule = 0;
    
    return NsiRpcDeregisterChangeNotificationEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcDeregisterChangeNotificationEx(
    void *bindingHandle,
    void *request
)
{
    LIST_ENTRY *currentEntry = NULL;
    LIST_ENTRY *nextEntry;
    LIST_ENTRY *prevEntry;
    RPC_ASYNC_STATE *asyncState;
	RPC_ASYNC_STATE *secondaryState = *(RPC_ASYNC_STATE**)((BYTE*)currentEntry + 4);
    ULONG status = 0;
    void *notificationHandle = *(void**)((BYTE*)request + 8);
    
    if (bindingHandle == NULL || request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    EnterCriticalSection(&NotificationListLock);
    
    currentEntry = NotificationListHead.Flink;
    while (currentEntry != &NotificationListHead) {
        if (currentEntry == (LIST_ENTRY*)notificationHandle) {
            nextEntry = currentEntry->Flink;
            prevEntry = currentEntry->Blink;
            
            // Validate list integrity
            if (nextEntry->Blink != currentEntry || prevEntry->Flink != currentEntry) {
                DebugBreak();
                status = RPC_S_INVALID_BOUND;
                break;
            }
            
            // Remove from list
            prevEntry->Flink = nextEntry;
            nextEntry->Blink = prevEntry;
            break;
        }
        currentEntry = currentEntry->Flink;
    }
    
    LeaveCriticalSection(&NotificationListLock);
    
    if (currentEntry != &NotificationListHead) {
        // Unregister wait (offset 32 in structure)
        HANDLE waitHandle = *(HANDLE*)((BYTE*)currentEntry + 32);
        UnregisterWaitEx(waitHandle, INVALID_HANDLE_VALUE);
        
        // Get async state and other handles from structure offsets
        HANDLE eventHandle = *(HANDLE*)((BYTE*)currentEntry + 28);
        asyncState = *(RPC_ASYNC_STATE**)((BYTE*)currentEntry + 36);
        
        RpcNsiDeregisterChangeNotification(
            asyncState,
            bindingHandle,
            *(NSI_MODULE_DESC*)request,
            (void**)((BYTE*)currentEntry + 16)
        );
        
        if (status == 0) {
            WaitForSingleObject(asyncState->u.APC.NotificationRoutine, INFINITE);
            RpcAsyncCompleteCall(asyncState, &status);
            
            if (status == 0) {
                BYTE flag = *(BYTE*)((BYTE*)currentEntry + 34);
                if (flag == 1) {
                    if (status != 0) {
                        RpcAsyncCancelCall(secondaryState, TRUE);
                    }
                    
                    HANDLE secondaryEvent = *(HANDLE*)((BYTE*)secondaryState + 16);
                    WaitForSingleObject(secondaryEvent, INFINITE);
                    RpcAsyncCompleteCall(secondaryState, &status);
                    
                    // Free allocated memory in the structure
                    void *alloc1 = *(void**)((BYTE*)currentEntry + 24);
                    void *alloc2 = *(void**)((BYTE*)currentEntry + 20);
                    
                    if (alloc1 != NULL) LocalFree(alloc1);
                    if (alloc2 != NULL) LocalFree(alloc2);
                    
                    status = 0;
                }
                
                // Cleanup handles and memory
                HANDLE handle1 = *(HANDLE*)((BYTE*)secondaryState + 16);
                CloseHandle(handle1);
                HeapFree(HeapHandle, 0, secondaryState);
                HeapFree(HeapHandle, 0, currentEntry);
            }
        }
        
        if (eventHandle != NULL) {
            CloseHandle(eventHandle);
        }
        
        if (asyncState != NULL) {
            CloseHandle(asyncState->u.APC.NotificationRoutine);
            HeapFree(HeapHandle, 0, asyncState);
        }
    } else {
        status = ERROR_INVALID_PARAMETER;
    }
    
    return status;
}

ULONG __cdecl NsiRpcEnumerateObjectsAllParameters(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength,
    void *readOnlyStruct,
    ULONG readOnlyStructLength,
    void *dynamicStruct,
    ULONG dynamicStructLength,
    ULONG *entryCount
)
{
    ULONG status;
    
    if (bindingHandle == NULL) {
        status = NsiEnumerateObjectsAllParameters(
            store, action, moduleId, objectIndex,
            keyStruct, keyStructLength,
            readWriteStruct, readWriteStructLength,
            readOnlyStruct, readOnlyStructLength,
            dynamicStruct, dynamicStructLength,
            entryCount
        );
        return status;
    }
    
    // Parameter validation
    if (moduleId == NULL || store == NSI_BOTH ||
        (keyStructLength != 0 && keyStruct == NULL) ||
        (readWriteStructLength != 0 && readWriteStruct == NULL) ||
        (readOnlyStructLength != 0 && readOnlyStruct == NULL) ||
        (dynamicStructLength != 0 && dynamicStruct == NULL) ||
        entryCount == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    RPC_ASYNC_STATE asyncState;
    ULONG asyncStatus = 0;
    ULONG finalStatus = 0;
    
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    NSI_KEYSTRUCT_DESC_ENUM keyDesc;
    NSI_PARAM_STRUCT_DESC_ENUM paramDesc;
    
    keyDesc.KeyStruct = (BYTE*)keyStruct;
    keyDesc.KeyStructLength = keyStructLength;
    keyDesc.TotalKeyStructLength = *entryCount * keyStructLength;
    
    paramDesc.RwParameterStruct = (BYTE*)readWriteStruct;
    paramDesc.RwParameterStructLength = readWriteStructLength;
    paramDesc.TotalRwLength = *entryCount * readWriteStructLength;
    
    paramDesc.RoStaticParameterStruct = (BYTE*)readOnlyStruct;
    paramDesc.RoStaticParameterStructLength = readOnlyStructLength;
    paramDesc.TotalRoStaticLength = *entryCount * readOnlyStructLength;
    
    paramDesc.RoDynamicParameterStruct = (BYTE*)dynamicStruct;
    paramDesc.RoDynamicParameterStructLength = dynamicStructLength;
    paramDesc.TotalRoDynamicLength = *entryCount * dynamicStructLength;
    
    // Check for overflow
    if (keyDesc.TotalKeyStructLength / *entryCount != keyStructLength ||
        paramDesc.TotalRwLength / *entryCount != readWriteStructLength ||
        paramDesc.TotalRoStaticLength / *entryCount != readOnlyStructLength ||
        paramDesc.TotalRoDynamicLength / *entryCount != dynamicStructLength) {
        status = ERROR_ARITHMETIC_OVERFLOW;
        goto cleanup;
    }
    
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = objectIndex;
    moduleDesc.NsiModule = 0;
    
    RpcNsiEnumerateObjectsAllParameters(
        &asyncState, bindingHandle, moduleDesc, store, action,
        &keyDesc, &paramDesc, entryCount
    );
    
    if (asyncStatus == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        finalStatus = RpcAsyncCompleteCall(&asyncState, &asyncStatus);
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    
    status = (finalStatus == 0) ? asyncStatus : finalStatus;

cleanup:
    return status;
}

ULONG __cdecl NsiRpcGetAllParameters(
    void *bindingHandle,
    DWORD store,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength,
    void *readOnlyStruct,
    ULONG readOnlyStructLength,
    void *dynamicStruct,
    ULONG dynamicStructLength
)
{
    struct {
        void *moduleId;
        ULONG objectIndex;
        BYTE *keyStruct;
        ULONG keyStructLength;
        BYTE *readWriteStruct;
        ULONG readWriteStructLength;
        BYTE *readOnlyStruct;
        ULONG readOnlyStructLength;
        BYTE *dynamicStruct;
        ULONG dynamicStructLength;
        DWORD store;
        DWORD nsiModule;
        DWORD action;
    } request;
    
    memset(&request, 0, sizeof(request));
    
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.keyStruct = (BYTE*)keyStruct;
    request.keyStructLength = keyStructLength;
    request.readWriteStruct = (BYTE*)readWriteStruct;
    request.readWriteStructLength = readWriteStructLength;
    request.readOnlyStruct = (BYTE*)readOnlyStruct;
    request.readOnlyStructLength = readOnlyStructLength;
    request.dynamicStruct = (BYTE*)dynamicStruct;
    request.dynamicStructLength = dynamicStructLength;
    request.store = store;
    request.nsiModule = 0;
    request.action = NSI_GET_EXACT;
    
    return NsiRpcGetAllParametersEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcGetAllParametersEx(void *bindingHandle, void *request)
{
    ULONG status;
    
    if (bindingHandle == NULL) {
        return NsiGetAllParametersEx(request);
    }
    
    if (request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check for transaction (not supported in RPC)
    if (*(void**)request != NULL) {
        return ERROR_INVALID_FUNCTION;
    }
    
    void *moduleId = *(void**)((BYTE*)request + 4);
    DWORD nsiModule = *(DWORD*)((BYTE*)request + 8);
    DWORD store = *(DWORD*)((BYTE*)request + 44);
    
    if ((moduleId == NULL && nsiModule == 0) || store == NSI_BOTH) {
        return ERROR_INVALID_PARAMETER;
    }
    
    RPC_ASYNC_STATE asyncState;
    ULONG asyncStatus = 0;
    ULONG finalStatus = 0;
    
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = *(ULONG*)((BYTE*)request + 12);
    moduleDesc.NsiModule = nsiModule;
    
    RpcNsiGetAllParameters(
        &asyncState, bindingHandle, moduleDesc, store, *(DWORD*)((BYTE*)request + 48),
        (NSI_KEYSTRUCT_DESC*)((BYTE*)request + 16),
        (NSI_PARAM_STRUCT_DESC*)((BYTE*)request + 24)
    );
    
    if (asyncStatus == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        finalStatus = RpcAsyncCompleteCall(&asyncState, &asyncStatus);
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    
    return (finalStatus == 0) ? asyncStatus : finalStatus;
}

ULONG __cdecl NsiRpcGetParameter(
    void *bindingHandle,
    DWORD store,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    DWORD structType,
    void *parameter,
    ULONG parameterLength,
    ULONG parameterOffset
)
{
    struct {
        void *moduleId;
        ULONG objectIndex;
        BYTE *keyStruct;
        ULONG keyStructLength;
        DWORD structType;
        BYTE *parameter;
        ULONG parameterLength;
        ULONG parameterOffset;
        DWORD store;
        DWORD nsiModule;
        DWORD action;
    } request;
    
    memset(&request, 0, sizeof(request));
    
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.keyStruct = (BYTE*)keyStruct;
    request.keyStructLength = keyStructLength;
    request.structType = structType;
    request.parameter = (BYTE*)parameter;
    request.parameterLength = parameterLength;
    request.parameterOffset = parameterOffset;
    request.store = store;
    request.nsiModule = 0;
    request.action = NSI_GET_EXACT;
    
    return NsiRpcGetParameterEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcGetParameterEx(void *bindingHandle, void *request)
{
    ULONG status;
    
    if (bindingHandle == NULL) {
        return NsiGetParameterEx(request);
    }
    
    if (request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    DWORD nsiModule = *(DWORD*)((BYTE*)request + 36);
    void *moduleId = *(void**)((BYTE*)request + 32);
    
    if ((moduleId == NULL && nsiModule == 0)) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check for transaction (not supported in RPC)
    if (*(void**)request != NULL) {
        return ERROR_INVALID_FUNCTION;
    }
    
    DWORD store = *(DWORD*)((BYTE*)request + 40);
    if (store == NSI_BOTH) {
        return ERROR_INVALID_PARAMETER;
    }
    
    RPC_ASYNC_STATE asyncState;
    ULONG asyncStatus = 0;
    ULONG finalStatus = 0;
    
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = *(ULONG*)((BYTE*)request + 36);
    moduleDesc.NsiModule = nsiModule;
    
    RpcNsiGetParameter(
        &asyncState, bindingHandle, moduleDesc, store, *(DWORD*)((BYTE*)request + 44),
        (NSI_KEYSTRUCT_DESC*)((BYTE*)request + 4),
        (NSI_SINGLE_PARAM_DESC*)((BYTE*)request + 12)
    );
    
    if (asyncStatus == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        finalStatus = RpcAsyncCompleteCall(&asyncState, &asyncStatus);
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    
    return (finalStatus == 0) ? asyncStatus : finalStatus;
}

ULONG __cdecl NsiRpcRegisterChangeNotification(
    void *bindingHandle,
    void *moduleId,
    ULONG objectIndex,
    void *callback,
    BYTE initialNotification,
    void *callerContext,
    void **notificationHandle
)
{
    struct {
        DWORD nsiModule;
        DWORD compartmentScope;
        DWORD compartmentId;
        void *moduleId;
        ULONG objectIndex;
        void *callback;
        BYTE unknown1;
        BYTE unknown2;
        BYTE unknown3;
        BYTE initialNotification;
        void *callerContext;
        void **notificationHandle;
    } request;
    
    request.nsiModule = 0;
    request.compartmentScope = 0;
    request.compartmentId = 0;
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.callback = callback;
    request.unknown1 = 0;
    request.unknown2 = 0;
    request.unknown3 = 0;
    request.initialNotification = initialNotification;
    request.callerContext = callerContext;
    request.notificationHandle = notificationHandle;
    
    return NsiRpcRegisterChangeNotificationEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcRegisterChangeNotificationEx(void *bindingHandle, void *request)
{
    LIST_ENTRY *notificationEntry = NULL;
    RPC_ASYNC_STATE asyncState;
    RPC_ASYNC_STATE *secondaryState = NULL;
    RPC_ASYNC_STATE *tertiaryState = NULL;
    ULONG status = 0;
    NET_IF_COMPARTMENT_ID compartmentId;
    DWORD compartmentScope = 0;
    BYTE initialFlag = 0;
    
    if (bindingHandle == NULL || request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    void *moduleId = *(void**)((BYTE*)request + 12);
    DWORD nsiModule = *(DWORD*)((BYTE*)request + 8);
    
    if ((moduleId == NULL && nsiModule == 0)) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Get compartment information if not provided
    if (*(DWORD*)((BYTE*)request + 4) == 0 && *(DWORD*)((BYTE*)request + 0) == 0) {
        compartmentScope = 1; // Assuming compartment scope
		compartmentId = 0;
    }
    
    // Initialize main async state
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    // Register change notification
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = *(ULONG*)((BYTE*)request + 16);
    moduleDesc.NsiModule = nsiModule;
    
    NSI_NOTIFICATION_REGISTRATION_DESC regDesc;
    regDesc.CompartmentScope = *(DWORD*)((BYTE*)request + 4);
    regDesc.CompartmentId = *(DWORD*)((BYTE*)request + 0);
    
    RpcNsiRegisterChangeNotification(
        &asyncState, bindingHandle, moduleDesc, regDesc,
        *(void***)((BYTE*)request + 32)
    );
    
    if (status == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        status = RpcAsyncCompleteCall(&asyncState, &status);
        
        if (status == 0) {
            initialFlag = 1;
            
            // Allocate notification entry structure
            notificationEntry = (LIST_ENTRY*)HeapAlloc(HeapHandle, 0, 0x4C);
            if (notificationEntry == NULL) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            memset(notificationEntry, 0, 0x4C);
            
            // Allocate secondary async state
            secondaryState = (RPC_ASYNC_STATE*)HeapAlloc(HeapHandle, 0, sizeof(RPC_ASYNC_STATE));
            if (secondaryState == NULL) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            memset(secondaryState, 0, sizeof(RPC_ASYNC_STATE));
            
            status = RpcAsyncInitializeHandle(secondaryState, sizeof(RPC_ASYNC_STATE));
            if (status != RPC_S_OK) {
                goto cleanup;
            }
            
            secondaryState->Flags = 1;
            secondaryState->u.APC.NotificationRoutine = CreateEventW(NULL, FALSE, FALSE, NULL);
            if (secondaryState->u.APC.NotificationRoutine == NULL) {
                status = GetLastError();
                goto cleanup;
            }
            
            // Allocate tertiary async state
            tertiaryState = (RPC_ASYNC_STATE*)HeapAlloc(HeapHandle, 0, sizeof(RPC_ASYNC_STATE));
            if (tertiaryState == NULL) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            memset(tertiaryState, 0, sizeof(RPC_ASYNC_STATE));
            
            status = RpcAsyncInitializeHandle(tertiaryState, sizeof(RPC_ASYNC_STATE));
            if (status != RPC_S_OK) {
                goto cleanup;
            }
            
            tertiaryState->Flags = 1;
            tertiaryState->u.APC.NotificationRoutine = CreateEventW(NULL, FALSE, FALSE, NULL);
            if (tertiaryState->u.APC.NotificationRoutine == NULL) {
                status = GetLastError();
                goto cleanup;
            }
            
            // Set up notification entry structure
            *(RPC_ASYNC_STATE**)((BYTE*)notificationEntry + 36) = tertiaryState;
            HANDLE notificationEvent = CreateEventW(NULL, TRUE, 
                (*(BYTE*)((BYTE*)request + 24) == 0) ? FALSE : TRUE, NULL);
            *(HANDLE*)((BYTE*)notificationEntry + 28) = notificationEvent;
            
            if (notificationEvent == NULL) {
                status = GetLastError();
                goto cleanup;
            }
            
            // Fill notification structure
            *(RPC_ASYNC_STATE**)((BYTE*)notificationEntry + 4) = secondaryState;
            *(void**)((BYTE*)notificationEntry + 12) = *(void**)((BYTE*)request + 20);
            *(void**)((BYTE*)notificationEntry + 16) = *(void***)((BYTE*)request + 32);
            *(BYTE*)((BYTE*)notificationEntry + 24) = *(BYTE*)((BYTE*)request + 24);
            *(void**)((BYTE*)notificationEntry + 8) = bindingHandle;
            *(void**)((BYTE*)notificationEntry + 20) = *(void**)((BYTE*)request + 28);
            
            // Register wait for callback
            HANDLE waitHandle = NULL;
            PTP_WAIT waitResult = CreateThreadpoolWait((PTP_WAIT_CALLBACK)NsiWorkerThread, notificationEntry, NULL);
            
            if (waitResult) {
                *(HANDLE*)((BYTE*)notificationEntry + 32) = waitHandle;
                *(BYTE*)((BYTE*)notificationEntry + 34) = 1;
                
                // Request initial notification
                RpcNsiRequestChangeNotification(
                    secondaryState,
                    *(void**)((BYTE*)notificationEntry + 8),
                    *(void**)((BYTE*)notificationEntry + 16),
                    (NSI_KEYSTRUCT_DESC*)((BYTE*)notificationEntry + 24),
                    (NSI_SINGLE_PARAM_DESC*)((BYTE*)notificationEntry + 16),
                    (NSI_NOTIFICATION*)((BYTE*)notificationEntry + 28)
                );
                
                if (status == 0 && *(BYTE*)((BYTE*)notificationEntry + 24) == 1) {
                    // Trigger initial callback
                    void *callback = *(void**)((BYTE*)notificationEntry + 12);
                    void *context = *(void**)((BYTE*)notificationEntry + 20);
                    NSI_KEYSTRUCT_DESC *keyDesc = (NSI_KEYSTRUCT_DESC*)((BYTE*)notificationEntry + 24);
                    NSI_SINGLE_PARAM_DESC *paramDesc = (NSI_SINGLE_PARAM_DESC*)((BYTE*)notificationEntry + 16);
                    
                    // Call the user callback
                    ((void(__cdecl*)(void*, void*, void*, DWORD))callback)(
                        context, keyDesc, paramDesc, 3
                    );
                    
                    SetEvent(*(HANDLE*)((BYTE*)notificationEntry + 28));
                }
                
                // Add to notification list
                EnterCriticalSection(&NotificationListLock);
                notificationEntry->Flink = &NotificationListHead;
                notificationEntry->Blink = NotificationListHead.Blink;
                NotificationListHead.Blink->Flink = notificationEntry;
                NotificationListHead.Blink = notificationEntry;
                LeaveCriticalSection(&NotificationListLock);
                
                *(void**)((BYTE*)request + 32) = notificationEntry;
            } else {
                status = GetLastError();
            }
        }
    }
    
cleanup:
    if (status != 0) {
        if (initialFlag) {
            // Cleanup registration on failure
            RpcNsiDeregisterChangeNotification(
                &asyncState, bindingHandle, moduleDesc,
                *(void***)((BYTE*)request + 32)
            );
            
            if (status == 0) {
                WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
                RpcAsyncCompleteCall(&asyncState, &status);
            }
        }
        
        // Cleanup allocated resources
        if (notificationEntry != NULL) {
            if (*(BYTE*)((BYTE*)notificationEntry + 34) == 1) {
                WaitForSingleObject(*(HANDLE*)((BYTE*)secondaryState + 16), INFINITE);
                RpcAsyncCompleteCall(secondaryState, &status);
                
                // Free allocated parameter structures
                void *keyStruct = *(void**)((BYTE*)notificationEntry + 24);
                void *paramStruct = *(void**)((BYTE*)notificationEntry + 20);
                
                if (keyStruct != NULL) LocalFree(keyStruct);
                if (paramStruct != NULL) LocalFree(paramStruct);
            }
            
            if (*(HANDLE*)((BYTE*)notificationEntry + 32) != NULL) {
                UnregisterWaitEx(*(HANDLE*)((BYTE*)notificationEntry + 32), NULL);
            }
            
            if (*(HANDLE*)((BYTE*)notificationEntry + 28) != NULL) {
                CloseHandle(*(HANDLE*)((BYTE*)notificationEntry + 28));
            }
            
            HeapFree(HeapHandle, 0, notificationEntry);
        }
        
        if (secondaryState != NULL) {
            if (secondaryState->u.APC.NotificationRoutine != NULL) {
                CloseHandle(secondaryState->u.APC.NotificationRoutine);
            }
            HeapFree(HeapHandle, 0, secondaryState);
        }
        
        if (tertiaryState != NULL) {
            if (tertiaryState->u.APC.NotificationRoutine != NULL) {
                CloseHandle(tertiaryState->u.APC.NotificationRoutine);
            }
            HeapFree(HeapHandle, 0, tertiaryState);
        }
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    return status;
}

ULONG __cdecl NsiRpcSetAllParameters(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *readWriteStruct,
    ULONG readWriteStructLength
)
{
    struct {
        void *transaction;
        DWORD nsiModule;
        void *moduleId;
        ULONG objectIndex;
        DWORD store;
        DWORD action;
        BYTE *keyStruct;
        ULONG keyStructLength;
        BYTE *readWriteStruct;
        ULONG readWriteStructLength;
    } request;
    
    request.transaction = NULL;
    request.nsiModule = 0;
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.store = store;
    request.action = action;
    request.keyStruct = (BYTE*)keyStruct;
    request.keyStructLength = keyStructLength;
    request.readWriteStruct = (BYTE*)readWriteStruct;
    request.readWriteStructLength = readWriteStructLength;
    
    return NsiRpcSetAllParametersEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcSetAllParametersEx(void *bindingHandle, void *request)
{
    ULONG status;
    
    if (bindingHandle == NULL) {
        return NsiSetAllParametersEx(request);
    }
    
    if (request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    void *moduleId = *(void**)((BYTE*)request + 8);
    DWORD nsiModule = *(DWORD*)((BYTE*)request + 4);
    DWORD store = *(DWORD*)((BYTE*)request + 20);
    
    if ((moduleId == NULL && nsiModule == 0) || store == NSI_CURRENT) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check for transaction (not supported in RPC)
    if (*(void**)request != NULL) {
        return ERROR_INVALID_FUNCTION;
    }
    
    RPC_ASYNC_STATE asyncState;
    ULONG asyncStatus = 0;
    ULONG finalStatus = 0;
    
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = *(ULONG*)((BYTE*)request + 12);
    moduleDesc.NsiModule = nsiModule;
    
    RpcNsiSetAllParameters(
        &asyncState, bindingHandle, moduleDesc, store, *(DWORD*)((BYTE*)request + 24),
        (NSI_KEYSTRUCT_DESC*)((BYTE*)request + 28),
        (NSI_RW_PARAM_STRUCT_DESC*)((BYTE*)request + 36)
    );
    
    if (asyncStatus == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        finalStatus = RpcAsyncCompleteCall(&asyncState, &asyncStatus);
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    
    return (finalStatus == 0) ? asyncStatus : finalStatus;
}

ULONG __cdecl NsiRpcSetParameter(
    void *bindingHandle,
    DWORD store,
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    DWORD structType,
    void *parameter,
    ULONG parameterLength,
    ULONG parameterOffset
)
{
    struct {
        void *moduleId;
        ULONG objectIndex;
        DWORD store;
        DWORD action;
        BYTE *keyStruct;
        ULONG keyStructLength;
        DWORD structType;
        BYTE *parameter;
        ULONG parameterLength;
        ULONG parameterOffset;
        DWORD nsiModule;
    } request;
    
    memset(&request, 0, sizeof(request));
    
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.store = store;
    request.action = action;
    request.keyStruct = (BYTE*)keyStruct;
    request.keyStructLength = keyStructLength;
    request.structType = structType;
    request.parameter = (BYTE*)parameter;
    request.parameterLength = parameterLength;
    request.parameterOffset = parameterOffset;
    request.nsiModule = 0;
    
    return NsiRpcSetParameterEx(bindingHandle, &request);
}

ULONG __cdecl NsiRpcSetParameterEx(void *bindingHandle, void *request)
{
    ULONG status;
    
    if (bindingHandle == NULL) {
        return NsiSetParameterEx(request);
    }
    
    if (request == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    void *moduleId = *(void**)((BYTE*)request + 0);
    DWORD nsiModule = *(DWORD*)((BYTE*)request + 40);
    DWORD store = *(DWORD*)((BYTE*)request + 8);
    
    if ((moduleId == NULL && nsiModule == 0) || store == NSI_CURRENT) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Check for transaction (not supported in RPC)
    if (*(void**)request != NULL) {
        return ERROR_INVALID_FUNCTION;
    }
    
    RPC_ASYNC_STATE asyncState;
    ULONG asyncStatus = 0;
    ULONG finalStatus = 0;
    
    status = RpcAsyncInitializeHandle(&asyncState, sizeof(RPC_ASYNC_STATE));
    if (status != RPC_S_OK) {
        return status;
    }
    
    asyncState.NotificationType = RpcNotificationTypeEvent;
    asyncState.u.APC.NotificationRoutine = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (asyncState.u.APC.NotificationRoutine == NULL) {
        status = GetLastError();
        return status;
    }
    
    NSI_MODULE_DESC moduleDesc;
    moduleDesc.ModuleId = moduleId;
    moduleDesc.ObjectIndex = *(ULONG*)((BYTE*)request + 4);
    moduleDesc.NsiModule = nsiModule;
    
    RpcNsiSetParameter(
        &asyncState, bindingHandle, moduleDesc, store, *(DWORD*)((BYTE*)request + 12),
        (NSI_KEYSTRUCT_DESC*)((BYTE*)request + 16),
        (NSI_SINGLE_PARAM_DESC*)((BYTE*)request + 24)
    );
    
    if (asyncStatus == 0) {
        WaitForSingleObject(asyncState.u.APC.NotificationRoutine, INFINITE);
        finalStatus = RpcAsyncCompleteCall(&asyncState, &asyncStatus);
    }
    
    CloseHandle(asyncState.u.APC.NotificationRoutine);
    
    return (finalStatus == 0) ? asyncStatus : finalStatus;
}

void __cdecl NsiWorkerThread(void *context, BYTE timerOrWaitFired)
{
    RPC_ASYNC_STATE *asyncState;
    ULONG status = 0;
    void *callback;
    void *userContext;
    NSI_KEYSTRUCT_DESC *keyDesc;
    NSI_SINGLE_PARAM_DESC *paramDesc;
    DWORD notificationType;
    
    // Extract information from context structure
    asyncState = *(RPC_ASYNC_STATE**)((BYTE*)context + 8);
    callback = *(void**)((BYTE*)context + 12);
    userContext = *(void**)((BYTE*)context + 20);
    keyDesc = (NSI_KEYSTRUCT_DESC*)((BYTE*)context + 24);
    paramDesc = (NSI_SINGLE_PARAM_DESC*)((BYTE*)context + 16);
    notificationType = *(DWORD*)((BYTE*)context + 28);
    
    // Wait for completion if this is an initial notification
    if (*(BYTE*)((BYTE*)context + 24) == 1) {
        WaitForSingleObject(asyncState->u.APC.NotificationRoutine, INFINITE);
    }
    
    // Complete the async call
    status = RpcAsyncCompleteCall(asyncState, &status);
    *(BYTE*)((BYTE*)context + 34) = 0;
    
    if (status == RPC_S_OK && status == 0) {
        // Call user callback
        ((void(__cdecl*)(void*, void*, void*, DWORD))callback)(
            userContext, keyDesc, paramDesc, notificationType
        );
        
        // Cleanup allocated parameter structures
        if (keyDesc->KeyStruct != NULL) {
            LocalFree(keyDesc->KeyStruct);
            keyDesc->KeyStruct = NULL;
        }
        
        if (paramDesc->Parameter != NULL) {
            LocalFree(paramDesc->Parameter);
            paramDesc->Parameter = NULL;
        }
        
        // Re-initialize for next notification
        status = RpcAsyncInitializeHandle(asyncState, sizeof(RPC_ASYNC_STATE));
        if (status == RPC_S_OK) {
            *(BYTE*)((BYTE*)context + 34) = 1;
            
            // Request next notification
            RpcNsiRequestChangeNotification(
                asyncState,
                *(void**)((BYTE*)context + 8),
                *(void**)((BYTE*)context + 16),
                keyDesc,
                paramDesc,
                (NSI_NOTIFICATION*)((BYTE*)context + 28)
            );
            
            if (status != 0) {
                *(BYTE*)((BYTE*)context + 34) = 0;
            }
        }
    }
}

long __cdecl PrepareRpcAsyncState(RPC_ASYNC_STATE *asyncState)
{
    long status;
    
    status = RpcAsyncInitializeHandle(asyncState, sizeof(RPC_ASYNC_STATE));
    if (status == RPC_S_OK) {
        asyncState->Flags = 1;
        asyncState->u.APC.NotificationRoutine = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (asyncState->u.APC.NotificationRoutine == NULL) {
            status = GetLastError();
        } else {
            status = RPC_S_OK;
        }
    }
    
    return status;
}

