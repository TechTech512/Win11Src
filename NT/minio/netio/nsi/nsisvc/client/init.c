#include <windows.h>

// Global variables
LIST_ENTRY NotificationListHead;
CRITICAL_SECTION NotificationListLock;
HANDLE HeapHandle = NULL;

// Function declarations
void CleanupNsiRpcClient(void);
ULONG InitializeNsiRpcClient(void);

void CleanupNsiRpcClient(void)
{
    LIST_ENTRY *currentEntry;
    LIST_ENTRY *nextEntry;
    HANDLE waitHandle;
    
    DeleteCriticalSection(&NotificationListLock);
    
    while (1) {
        currentEntry = NotificationListHead.Flink;
        if (currentEntry == &NotificationListHead) {
            return;
        }
        
        nextEntry = currentEntry->Flink;
        
        // Validate list integrity
        if (currentEntry->Blink != &NotificationListHead || nextEntry->Blink != currentEntry) {
            // List corruption detected - break out of loop
            break;
        }
        
        // Remove entry from list
        NotificationListHead.Flink = nextEntry;
        nextEntry->Blink = &NotificationListHead;
        
        // Unregister wait (wait handle is stored at offset 8 in the structure)
        waitHandle = *(HANDLE*)((BYTE*)currentEntry + 32); // 8 * sizeof(LIST_ENTRY) = 32
        UnregisterWaitEx(waitHandle, INVALID_HANDLE_VALUE);
    }
    
    // If we get here, there was list corruption
    // This would typically trigger a debug break or exception
    DebugBreak();
}

int DllMain(void *hInstance, ULONG dwReason, void *lpReserved)
{
    ULONG status = 0;
    
    if (dwReason == 0) {
        // DLL_PROCESS_DETACH
        CleanupNsiRpcClient();
    } else {
        if (dwReason != 1) {
            // DLL_THREAD_ATTACH or DLL_THREAD_DETACH
            return 1;
        }
        
        // DLL_PROCESS_ATTACH
        DisableThreadLibraryCalls((HMODULE)hInstance);
        status = InitializeNsiRpcClient();
    }
    
    if (status == 0) {
        return 1;
    }
    return 0;
}

ULONG InitializeNsiRpcClient(void)
{
    int success;
    ULONG status;
    
    // Initialize the notification list
    NotificationListHead.Blink = &NotificationListHead;
    NotificationListHead.Flink = &NotificationListHead;
    
    // Get the process heap
    HeapHandle = GetProcessHeap();
    if (HeapHandle == NULL) {
        status = GetLastError();
        return status;
    }
    
    // Initialize the critical section with spin count
    success = InitializeCriticalSectionAndSpinCount(&NotificationListLock, 0);
    if (success != 0) {
        return 0;
    }
    
    status = GetLastError();
    return status;
}

