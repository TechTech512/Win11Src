#include <windows.h>
#include <winternl.h>

// External function declarations
extern void NsiFreeTable(void *param1, void *param2, void *param3, void *param4);
extern ULONG NsiEnumerateObjectsAllPersistentParametersWithMask(
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *paramStruct,
    void *maskStruct,
    ULONG paramStructLength,
    ULONG *entryCount
);
extern void NsiFreePersistentDataWithMaskTable(void *param1, void *param2, void *param3);
extern ULONG NsiIoctl(
    ULONG controlCode,
    void *inputBuffer,
    ULONG inputBufferLength,
    void *outputBuffer,
    ULONG *outputBufferLength,
    OVERLAPPED *overlapped
);
extern ULONG NsiDeregisterChangeNotificationEx(void *request);
extern ULONG NsiRegisterChangeNotificationEx(void *request);
extern ULONG NsiRequestChangeNotificationEx(void *request);
extern ULONG NsiSetAllParametersEx(void *request);
extern ULONG NsiSetParameterEx(void *request);

// Global variables
HANDLE g_NsiAsyncDeviceHandle = (HANDLE)-1;
HANDLE g_NsiHeap = NULL;

ULONG FUN_100021ae(void)
{
    NsiFreeTable(NULL, NULL, NULL, NULL);
    return 8;
}

int FUN_100021ef(DWORD param1, DWORD param2)
{
    HANDLE eventHandle;
    ULONG status;
    ULONG ioStatusBlock;
    BYTE buffer[56];
    DWORD bufferSize = 56;
    
    CloseHandle((HANDLE)param2);
    
    eventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (eventHandle == NULL) {
        return GetLastError();
    }
    
    status = NtDeviceIoControlFile(
        g_NsiAsyncDeviceHandle,
        eventHandle,
        NULL,
        NULL,
        (PIO_STATUS_BLOCK)&ioStatusBlock,
        0x12000F,
        buffer,
        bufferSize,
        buffer,
        bufferSize
    );
    
    if (status == 0x103) {
        status = NtWaitForSingleObject(eventHandle, FALSE, NULL);
        if ((LONG)status >= 0) {
            status = ioStatusBlock;
        }
    }
    
    CloseHandle(eventHandle);
    
    if ((LONG)status >= 0 && status != 0x105) {
        return 0;
    }
    
    if ((status & 0xC0000000) != 0xC0000000) {
        // Buffer size adjustment would go here
    }
    
    return RtlNtStatusToDosError(status);
}

ULONG NsiAllocateAndGetPersistentDataWithMaskTable(
    void *moduleId,
    ULONG objectIndex,
    void **keyStructTable,
    ULONG keyStructLength,
    void **paramStructTable,
    void **maskStructTable,
    ULONG paramStructLength,
    ULONG *entryCount
)
{
    ULONG status;
    void *localKeyStructTable = NULL;
    void *localParamStructTable = NULL;
    void *localMaskStructTable = NULL;
    ULONG localEntryCount = 0;
    DWORD retryCount = 0;
    
    while (TRUE) {
        localKeyStructTable = NULL;
        localParamStructTable = NULL;
        localMaskStructTable = NULL;
        localEntryCount = 0;
        
        status = NsiEnumerateObjectsAllPersistentParametersWithMask(
            0, // NsiGetFirst
            moduleId,
            objectIndex,
            NULL,
            0,
            NULL,
            NULL,
            0,
            &localEntryCount
        );
        
        if (status != 0 && status != 0xEA) {
            return status;
        }
        
        ULONG requiredSize = localEntryCount + 2;
        localEntryCount = requiredSize;
        
        if (keyStructLength != 0) {
            localKeyStructTable = HeapAlloc(g_NsiHeap, 0, requiredSize * keyStructLength);
            if (localKeyStructTable == NULL) {
                return 8; // ERROR_NOT_ENOUGH_MEMORY
            }
        }
        
        if (paramStructLength != 0) {
            localParamStructTable = HeapAlloc(g_NsiHeap, 0, requiredSize * paramStructLength);
            if (localParamStructTable == NULL) {
                status = 8;
                goto cleanup;
            }
            
            localMaskStructTable = HeapAlloc(g_NsiHeap, 0, requiredSize * paramStructLength);
            if (localMaskStructTable == NULL) {
                status = 8;
                goto cleanup;
            }
            
            memset(localParamStructTable, 0, requiredSize * paramStructLength);
            memset(localMaskStructTable, 0, requiredSize * paramStructLength);
        }
        
        status = NsiEnumerateObjectsAllPersistentParametersWithMask(
            0, // NsiGetFirst
            moduleId,
            objectIndex,
            localKeyStructTable,
            keyStructLength,
            localParamStructTable,
            localMaskStructTable,
            paramStructLength,
            &localEntryCount
        );
        
        if (status != 0xEA) {
            break;
        }
        
        NsiFreePersistentDataWithMaskTable(localKeyStructTable, localParamStructTable, localMaskStructTable);
        localKeyStructTable = NULL;
        localParamStructTable = NULL;
        localMaskStructTable = NULL;
        retryCount++;
        
        if (retryCount > 2) {
            if (retryCount == 3) {
                status = 0xC0000001; // STATUS_UNSUCCESSFUL
            } else {
                *entryCount = localEntryCount;
                if (localKeyStructTable != NULL) {
                    *keyStructTable = localKeyStructTable;
                }
                if (localParamStructTable != NULL) {
                    *paramStructTable = localParamStructTable;
                }
                if (localMaskStructTable != NULL) {
                    *maskStructTable = localMaskStructTable;
                }
                status = 0;
            }
            return status;
        }
    }
    
    if (status != 0) {
        goto cleanup;
    }
    
    if (retryCount == 3) {
        status = 0xC0000001;
    } else {
        *entryCount = localEntryCount;
        if (localKeyStructTable != NULL) {
            *keyStructTable = localKeyStructTable;
        }
        if (localParamStructTable != NULL) {
            *paramStructTable = localParamStructTable;
        }
        if (localMaskStructTable != NULL) {
            *maskStructTable = localMaskStructTable;
        }
        status = 0;
    }
    
    return status;

cleanup:
    NsiFreePersistentDataWithMaskTable(localKeyStructTable, localParamStructTable, localMaskStructTable);
    return status;
}

ULONG NsiAllocateAndGetTable(
    DWORD store,
    void *moduleId,
    ULONG objectIndex,
    void **keyStructTable,
    ULONG keyStructLength,
    void **readWriteStructTable,
    ULONG readWriteStructLength,
    void **readOnlyStructTable,
    ULONG readOnlyStructLength,
    void **dynamicStructTable,
    ULONG dynamicStructLength,
    ULONG *entryCount,
    BYTE unknownFlag
)
{
    void *localKeyStructTable = NULL;
    void *localReadWriteStructTable = NULL;
    void *localReadOnlyStructTable = NULL;
    void *localDynamicStructTable = NULL;
    DWORD retryCount = 0;
    ULONG status;
    
    do {
        localKeyStructTable = NULL;
        localReadWriteStructTable = NULL;
        localReadOnlyStructTable = NULL;
        localDynamicStructTable = NULL;
        
        DWORD bufferSize = 60;
        DWORD returnedBufferSize = 60;
        
        if (g_NsiAsyncDeviceHandle == (HANDLE)-1) {
            HANDLE deviceHandle = CreateFileW(L"\\\\.\\Nsi", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0x40000000, NULL);
            if (deviceHandle != (HANDLE)-1) {
                HANDLE existingHandle = (HANDLE)-1;
                
                // Critical section would be here
                HANDLE currentHandle = deviceHandle;
                if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
                    existingHandle = g_NsiAsyncDeviceHandle;
                    currentHandle = g_NsiAsyncDeviceHandle;
                }
                g_NsiAsyncDeviceHandle = currentHandle;
                // End critical section
                
                if (existingHandle != (HANDLE)-1) {
                    CloseHandle(deviceHandle);
                }
            } else {
                status = GetLastError();
                if (status == 0) {
                    // Continue with existing handle
                } else {
                    if (status != 0 && status != 0xEA) {
                        return status;
                    }
                }
            }
        }
        
        HANDLE eventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (eventHandle == NULL) {
            status = GetLastError();
            if (status != 0 && status != 0xEA) {
                return status;
            }
        } else {
            ULONG ioStatusBlock;
            BYTE inputBuffer[60];
            memset(inputBuffer, 0, sizeof(inputBuffer));
            
            status = NtDeviceIoControlFile(
                g_NsiAsyncDeviceHandle,
                eventHandle,
                NULL,
                NULL,
                (PIO_STATUS_BLOCK)&ioStatusBlock,
                0x12001B,
                inputBuffer,
                bufferSize,
                inputBuffer,
                bufferSize
            );
            
            if (status == 0x103) {
                status = NtWaitForSingleObject(eventHandle, FALSE, NULL);
                if ((LONG)status >= 0) {
                    status = ioStatusBlock;
                }
            }
            
            CloseHandle(eventHandle);
            
            if ((LONG)status < 0 || status == 0x105) {
                if ((status & 0xC0000000) != 0xC0000000) {
                    bufferSize = returnedBufferSize;
                }
                status = RtlNtStatusToDosError(status);
                if (status != 0 && status != 0xEA) {
                    return status;
                }
            }
        }
        
        if (keyStructLength != 0) {
            localKeyStructTable = HeapAlloc(g_NsiHeap, 0, keyStructLength * 2);
            if (localKeyStructTable == NULL) {
                return 8;
            }
        }
        
        if (readWriteStructLength != 0) {
            DWORD allocSize = readWriteStructLength * 2;
            localReadWriteStructTable = HeapAlloc(g_NsiHeap, 0, allocSize);
            if (localReadWriteStructTable == NULL) {
                status = 8;
                goto cleanup;
            }
            memset(localReadWriteStructTable, 0, allocSize);
        }
        
        if (readOnlyStructLength != 0) {
            localReadOnlyStructTable = HeapAlloc(g_NsiHeap, 0, readOnlyStructLength * 2);
            if (localReadOnlyStructTable == NULL) {
                status = FUN_100021ae();
                return status;
            }
        }
        
        if (dynamicStructLength != 0) {
            localDynamicStructTable = HeapAlloc(g_NsiHeap, 0, dynamicStructLength * 2);
            if (localDynamicStructTable == NULL) {
                status = 8;
                goto cleanup;
            }
        }
        
        memset(localDynamicStructTable, 0, dynamicStructLength * 2);
        
        bufferSize = 60;
        if (g_NsiAsyncDeviceHandle == (HANDLE)-1) {
            HANDLE deviceHandle = CreateFileW(L"\\\\.\\Nsi", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0x40000000, NULL);
            if (deviceHandle != (HANDLE)-1) {
                HANDLE existingHandle = (HANDLE)-1;
                
                // Critical section would be here
                HANDLE currentHandle = deviceHandle;
                if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
                    existingHandle = g_NsiAsyncDeviceHandle;
                    currentHandle = g_NsiAsyncDeviceHandle;
                }
                g_NsiAsyncDeviceHandle = currentHandle;
                // End critical section
                
                if (existingHandle != (HANDLE)-1) {
                    CloseHandle(deviceHandle);
                }
            } else {
                status = GetLastError();
                if (status == 0) {
                    // Continue with existing handle
                }
            }
        }
        
        returnedBufferSize = bufferSize;
        HANDLE eventHandle2 = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (eventHandle2 == NULL) {
            status = GetLastError();
        } else {
            ULONG ioStatusBlock2;
            BYTE inputBuffer2[60];
            
            status = NtDeviceIoControlFile(
                g_NsiAsyncDeviceHandle,
                eventHandle2,
                NULL,
                NULL,
                (PIO_STATUS_BLOCK)&ioStatusBlock2,
                0x12001B,
                inputBuffer2,
                bufferSize,
                inputBuffer2,
                bufferSize
            );
            
            if (status == 0x103) {
                status = NtWaitForSingleObject(eventHandle2, FALSE, NULL);
                if ((LONG)status >= 0) {
                    status = ioStatusBlock2;
                }
            }
            
            CloseHandle(eventHandle2);
            
            if ((LONG)status >= 0 && status != 0x105) {
                goto success;
            }
            
            if ((status & 0xC0000000) != 0xC0000000) {
                bufferSize = returnedBufferSize;
            }
            status = RtlNtStatusToDosError(status);
        }
        
        if (status != 0xEA) {
            if (status != 0) {
                goto cleanup;
            }
            goto success;
        }
        
        NsiFreeTable(localKeyStructTable, localReadWriteStructTable, localReadOnlyStructTable, localDynamicStructTable);
        localKeyStructTable = NULL;
        localReadWriteStructTable = NULL;
        localReadOnlyStructTable = NULL;
        localDynamicStructTable = NULL;
        retryCount++;
        
        if (retryCount > 2) {
            break;
        }
    } while (TRUE);

success:
    if (retryCount == 3) {
        status = 0xC0000001;
    } else {
        *entryCount = 2;
        if (localKeyStructTable != NULL) {
            *keyStructTable = localKeyStructTable;
        }
        if (localReadWriteStructTable != NULL) {
            *readWriteStructTable = localReadWriteStructTable;
        }
        if (localReadOnlyStructTable != NULL) {
            *readOnlyStructTable = localReadOnlyStructTable;
        }
        if (localDynamicStructTable != NULL) {
            *dynamicStructTable = localDynamicStructTable;
        }
        status = 0;
    }
    
    return status;

cleanup:
    NsiFreeTable(localKeyStructTable, localReadWriteStructTable, localReadOnlyStructTable, localDynamicStructTable);
    return status;
}

ULONG NsiDeregisterChangeNotification(void *moduleId, ULONG objectIndex, void *notificationHandle)
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
    
    return NsiDeregisterChangeNotificationEx(&request);
}

ULONG NsiEnumerateObjectsAllParameters(
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
    ULONG originalEntryCount = *entryCount;
    DWORD bufferSize = 60;
    
    BYTE inputBuffer[60];
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    ULONG status = NsiIoctl(
        0x3C,
        inputBuffer,
        bufferSize,
        NULL,
        entryCount,
        NULL
    );
    
    *entryCount = originalEntryCount;
    return status;
}

ULONG NsiEnumerateObjectsAllPersistentParametersWithMask(
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *paramStruct,
    void *maskStruct,
    ULONG paramStructLength,
    ULONG *entryCount
)
{
    ULONG originalEntryCount = *entryCount;
    DWORD bufferSize = 44;
    
    BYTE inputBuffer[44];
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    ULONG status = NsiIoctl(
        0x2C,
        inputBuffer,
        bufferSize,
        NULL,
        entryCount,
        NULL
    );
    
    *entryCount = originalEntryCount;
    return status;
}

void NsiFreePersistentDataWithMaskTable(void *keyStructTable, void *paramStructTable, void *maskStructTable)
{
    if (keyStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, keyStructTable);
    }
    if (paramStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, paramStructTable);
    }
    if (maskStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, maskStructTable);
    }
}

void NsiFreeTable(void *keyStructTable, void *readWriteStructTable, void *readOnlyStructTable, void *dynamicStructTable)
{
    if (keyStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, keyStructTable);
    }
    if (readWriteStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, readWriteStructTable);
    }
    if (readOnlyStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, readOnlyStructTable);
    }
    if (dynamicStructTable != NULL) {
        HeapFree(g_NsiHeap, 0, dynamicStructTable);
    }
}

ULONG NsiGetAllParameters(
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
    BYTE inputBuffer[56];
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    DWORD bufferSize = 56;
    
    if (g_NsiAsyncDeviceHandle == (HANDLE)-1) {
        HANDLE deviceHandle = CreateFileW(L"\\\\.\\Nsi", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0x40000000, NULL);
        if (deviceHandle != (HANDLE)-1) {
            HANDLE existingHandle = (HANDLE)-1;
            
            // Critical section would be here
            HANDLE currentHandle = deviceHandle;
            if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
                existingHandle = g_NsiAsyncDeviceHandle;
                currentHandle = g_NsiAsyncDeviceHandle;
            }
            g_NsiAsyncDeviceHandle = currentHandle;
            // End critical section
            
            if (existingHandle != (HANDLE)-1) {
                ULONG status = FUN_100021ef((DWORD)deviceHandle, (DWORD)deviceHandle);
                return status;
            }
        } else {
            ULONG status = GetLastError();
            if (status == 0) {
                // Continue with existing handle
            } else {
                return status;
            }
        }
    }
    
    HANDLE eventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (eventHandle == NULL) {
        return GetLastError();
    }
    
    ULONG ioStatusBlock;
    ULONG status = NtDeviceIoControlFile(
        g_NsiAsyncDeviceHandle,
        eventHandle,
        NULL,
        NULL,
        (PIO_STATUS_BLOCK)&ioStatusBlock,
        0x12000F,
        inputBuffer,
        bufferSize,
        inputBuffer,
        bufferSize
    );
    
    if (status == 0x103) {
        status = NtWaitForSingleObject(eventHandle, FALSE, NULL);
        if ((LONG)status >= 0) {
            status = ioStatusBlock;
        }
    }
    
    CloseHandle(eventHandle);
    
    if ((LONG)status >= 0 && status != 0x105) {
        return 0;
    }
    
    if ((status & 0xC0000000) != 0xC0000000) {
        // Buffer size adjustment
    }
    
    return RtlNtStatusToDosError(status);
}

ULONG NsiGetAllPersistentParametersWithMask(
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *paramStruct,
    void *maskStruct,
    ULONG paramStructLength
)
{
    DWORD bufferSize = 36;
    return NsiIoctl(0x24, &bufferSize, bufferSize, NULL, NULL, NULL);
}

ULONG NsiGetParameter(
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
    BYTE inputBuffer[48];
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    if (g_NsiAsyncDeviceHandle == (HANDLE)-1) {
        HANDLE deviceHandle = CreateFileW(L"\\\\.\\Nsi", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0x40000000, NULL);
        if (deviceHandle == (HANDLE)-1) {
            ULONG status = GetLastError();
            if (status != 0) {
                return status;
            }
        } else {
            HANDLE existingHandle = (HANDLE)-1;
            
            // Critical section would be here
            HANDLE currentHandle = deviceHandle;
            if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
                existingHandle = g_NsiAsyncDeviceHandle;
                currentHandle = g_NsiAsyncDeviceHandle;
            }
            g_NsiAsyncDeviceHandle = currentHandle;
            // End critical section
            
            if (existingHandle != (HANDLE)-1) {
                CloseHandle(deviceHandle);
            }
        }
    }
    
    HANDLE eventHandle = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (eventHandle == NULL) {
        return GetLastError();
    }
    
    ULONG ioStatusBlock;
    ULONG status = NtDeviceIoControlFile(
        g_NsiAsyncDeviceHandle,
        eventHandle,
        NULL,
        NULL,
        (PIO_STATUS_BLOCK)&ioStatusBlock,
        0x120007,
        inputBuffer,
        48,
        inputBuffer,
        48
    );
    
    if (status == 0x103) {
        status = NtWaitForSingleObject(eventHandle, FALSE, NULL);
        if ((LONG)status >= 0) {
            status = ioStatusBlock;
        }
    }
    
    CloseHandle(eventHandle);
    
    if ((LONG)status < 0 || status == 0x105) {
        if ((status & 0xC0000000) != 0xC0000000) {
            // Parameter length adjustment would go here
        }
        status = RtlNtStatusToDosError(status);
    }
    
    return status;
}

ULONG NsiRegisterChangeNotification(
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
    
    return NsiRegisterChangeNotificationEx(&request);
}

ULONG NsiRequestChangeNotification(
    DWORD nsiModule,
    void *moduleId,
    ULONG objectIndex,
    OVERLAPPED *overlapped,
    void **fileHandle
)
{
    struct {
        DWORD nsiModule;
        void *moduleId;
        ULONG objectIndex;
        void **fileHandle;
        OVERLAPPED *overlapped;
    } request;
    
    request.nsiModule = nsiModule;
    request.moduleId = moduleId;
    request.objectIndex = objectIndex;
    request.fileHandle = fileHandle;
    request.overlapped = overlapped;
    
    return NsiRequestChangeNotificationEx(&request);
}

ULONG NsiSetAllParameters(
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
    
    return NsiSetAllParametersEx(&request);
}

ULONG NsiSetAllPersistentParametersWithMask(
    DWORD action,
    void *moduleId,
    ULONG objectIndex,
    void *keyStruct,
    ULONG keyStructLength,
    void *paramStruct,
    void *maskStruct,
    ULONG paramStructLength
)
{
    DWORD bufferSize = 40;
    return NsiIoctl(0x28, &bufferSize, bufferSize, NULL, NULL, NULL);
}

ULONG NsiSetParameter(
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
    BYTE inputBuffer[52];
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
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
    } *request = (void*)inputBuffer;
    
    request->moduleId = moduleId;
    request->objectIndex = objectIndex;
    request->store = store;
    request->action = action;
    request->keyStruct = (BYTE*)keyStruct;
    request->keyStructLength = keyStructLength;
    request->structType = structType;
    request->parameter = (BYTE*)parameter;
    request->parameterLength = parameterLength;
    request->parameterOffset = parameterOffset;
    request->nsiModule = 0;
    
    return NsiSetParameterEx(request);
}

