#include <windows.h>

// Throttle management class
class UtcThrottle {
private:
    static LONG RefCount;
    static HANDLE ThrottlingSemaphore;
    static HANDLE ThrottlingTimer;
    static HANDLE ThrottlingCleanupEvent;

public:
    static ULONG Uninitialize();
};

// Initialize static members
LONG UtcThrottle::RefCount = 0;
HANDLE UtcThrottle::ThrottlingSemaphore = NULL;
HANDLE UtcThrottle::ThrottlingTimer = NULL;
HANDLE UtcThrottle::ThrottlingCleanupEvent = NULL;

// Throttle cleanup implementation
ULONG UtcThrottle::Uninitialize()
{
    if (InterlockedDecrement(&RefCount) == 0) {
        if (ThrottlingCleanupEvent) {
            SetEvent(ThrottlingCleanupEvent);
            CloseHandle(ThrottlingCleanupEvent);
            ThrottlingCleanupEvent = NULL;
        }
        if (ThrottlingSemaphore) {
            CloseHandle(ThrottlingSemaphore);
            ThrottlingSemaphore = NULL;
        }
        if (ThrottlingTimer) {
            CloseHandle(ThrottlingTimer);
            ThrottlingTimer = NULL;
        }
    }
    return ERROR_SUCCESS;
}

