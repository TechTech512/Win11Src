#include <windows.h>
#include <winternl.h>

// External function declarations
extern int _DllMainCRTStartupForGS2(HINSTANCE hInstance, DWORD dwReason, void *lpReserved);

// Global variables
HANDLE g_NsiDeviceHandle = (HANDLE)-1;
extern HANDLE g_NsiAsyncDeviceHandle;
extern HANDLE g_NsiHeap;

// External functions that need to be implemented elsewhere
extern ULONG NsiIoctl(
    ULONG controlCode,
    void *inputBuffer,
    ULONG inputBufferLength,
    void *outputBuffer,
    ULONG *outputBufferLength,
    OVERLAPPED *overlapped
);

int _DllInitialize(HINSTANCE hInstance, ULONG dwReason, void *lpReserved)
{
    if (dwReason == 0) {
        // DLL_PROCESS_DETACH
        if (g_NsiDeviceHandle != (HANDLE)-1) {
            CloseHandle(g_NsiDeviceHandle);
            g_NsiDeviceHandle = (HANDLE)-1;
        }
        if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
            CloseHandle(g_NsiAsyncDeviceHandle);
            g_NsiAsyncDeviceHandle = (HANDLE)-1;
        }
    } else if (dwReason == 1) {
        // DLL_PROCESS_ATTACH
        DisableThreadLibraryCalls(hInstance);
        g_NsiHeap = GetProcessHeap();
    }
    return 1;
}

int DllInitialize(HINSTANCE hInstance, ULONG dwReason, void *lpReserved)
{
    int result;
    
    if (dwReason == 1) {
        // DLL_PROCESS_ATTACH
        void *lpReservedLocal = (void*)1;
        _DllMainCRTStartupForGS2(hInstance, 1, lpReserved);
    }
    
    result = _DllInitialize(hInstance, dwReason, lpReserved);
    return result;
}

ULONG NsiCancelChangeNotification(void *notificationHandle)
{
    ULONG bufferSize = 0;
    return NsiIoctl(4, NULL, bufferSize, NULL, NULL, (OVERLAPPED*)notificationHandle);
}

ULONG NsiDeregisterChangeNotificationEx(void *request)
{
    ULONG outputBufferSize = 16;
    return NsiIoctl(0x10, request, 16, NULL, &outputBufferSize, NULL);
}

ULONG NsiEnumerateObjectsAllParametersEx(void *request)
{
    ULONG outputBufferSize = 60;
    return NsiIoctl(0x3C, request, 60, NULL, &outputBufferSize, NULL);
}

ULONG NsiGetAllParametersEx(void *request)
{
    ULONG outputBufferSize = 56;
    return NsiIoctl(0x38, request, 56, NULL, &outputBufferSize, NULL);
}

ULONG NsiGetObjectSecurity(void *request)
{
    ULONG outputBufferSize = 24;
    return NsiIoctl(0x18, request, 24, NULL, &outputBufferSize, NULL);
}

ULONG NsiGetParameterEx(void *request)
{
    ULONG *parameterLength = &((ULONG*)request)[40]; // ParamDesc.ParameterLength offset
    return NsiIoctl(0x30, request, 48, NULL, parameterLength, NULL);
}

ULONG NsiIoctl(
    ULONG controlCode,
    void *inputBuffer,
    ULONG inputBufferLength,
    void *outputBuffer,
    ULONG *outputBufferLength,
    OVERLAPPED *overlapped
)
{
    HANDLE eventHandle;
    ULONG status;
    ULONG ioStatusBlock;
    DWORD bytesReturned;
    
    if (g_NsiAsyncDeviceHandle == (HANDLE)-1) {
        HANDLE deviceHandle = CreateFileW(
            L"\\\\.\\Nsi", 
            0, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, 
            NULL, 
            OPEN_EXISTING, 
            0x40000000, 
            NULL
        );
        
        if (deviceHandle == (HANDLE)-1) {
            ULONG error = GetLastError();
            if (error != 0) {
                return error;
            }
        } else {
            HANDLE existingHandle = (HANDLE)-1;
            
            // Critical section would be here (LOCK())
            HANDLE currentHandle = deviceHandle;
            if (g_NsiAsyncDeviceHandle != (HANDLE)-1) {
                existingHandle = g_NsiAsyncDeviceHandle;
                currentHandle = g_NsiAsyncDeviceHandle;
            }
            g_NsiAsyncDeviceHandle = currentHandle;
            // End critical section (UNLOCK())
            
            if (existingHandle != (HANDLE)-1) {
                CloseHandle(deviceHandle);
            }
        }
    }
    
    void *localInputBuffer = inputBuffer;
    ULONG localInputBufferLength = inputBufferLength;
    
    if (outputBuffer == NULL) {
        // Synchronous operation
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
            controlCode,
            localInputBuffer,
            localInputBufferLength,
            localInputBuffer,
            localInputBufferLength
        );
        
        if (status == 0x103) { // STATUS_PENDING
            status = NtWaitForSingleObject(eventHandle, FALSE, NULL);
            if ((LONG)status >= 0) {
                status = ioStatusBlock;
            }
        }
        
        CloseHandle(eventHandle);
        
        if ((LONG)status >= 0 && status != 0x105) { // STATUS_BUFFER_OVERFLOW
            if (outputBufferLength != NULL) {
                *outputBufferLength = localInputBufferLength;
            }
            return 0;
        }
        
        if ((status & 0xC0000000) != 0xC0000000) {
            if (outputBufferLength != NULL) {
                *outputBufferLength = localInputBufferLength;
            }
        }
        
        return RtlNtStatusToDosError(status);
    } else {
        // Asynchronous operation
        BOOL result = DeviceIoControl(
            g_NsiAsyncDeviceHandle,
            controlCode,
            localInputBuffer,
            localInputBufferLength,
            outputBuffer,
            outputBufferLength ? *outputBufferLength : 0,
            &bytesReturned,
            overlapped
        );
        
        if (result) {
            return 0;
        }
    }
    
    return GetLastError();
}

ULONG NsiRegisterChangeNotificationEx(void *request)
{
    ULONG outputBufferSize = 36;
    
    // Check if callback is NULL (0x24) or has a valid callback (0x57 - ERROR_INVALID_PARAMETER)
    if (*(void**)((BYTE*)request + 16) == NULL) { // Callback field offset
        return NsiIoctl(0x24, request, 36, NULL, &outputBufferSize, NULL);
    } else {
        return 0x57; // ERROR_INVALID_PARAMETER
    }
}

ULONG NsiRequestChangeNotificationEx(void *request)
{
    ULONG outputBufferSize = 20;
    OVERLAPPED *overlapped = *(OVERLAPPED**)((BYTE*)request + 16); // Overlapped field offset
    void **fileHandle = *(void***)((BYTE*)request + 12); // FileHandle field offset
    
    ULONG status = NsiIoctl(0x14, NULL, 0, overlapped, &outputBufferSize, overlapped);
    
    if (fileHandle != NULL && overlapped != NULL) {
        *fileHandle = g_NsiAsyncDeviceHandle;
    }
    
    return status;
}

ULONG NsiSetAllParametersEx(void *request)
{
    ULONG outputBufferSize = 40;
    return NsiIoctl(0x28, request, 40, NULL, &outputBufferSize, NULL);
}

ULONG NsiSetObjectSecurity(void *request)
{
    ULONG outputBufferSize = 24;
    return NsiIoctl(0x18, request, 24, NULL, &outputBufferSize, NULL);
}

ULONG NsiSetParameterEx(void *request)
{
    ULONG *parameterLength = &((ULONG*)request)[40]; // ParamDesc.ParameterLength offset
    return NsiIoctl(0x30, request, 48, NULL, parameterLength, NULL);
}

