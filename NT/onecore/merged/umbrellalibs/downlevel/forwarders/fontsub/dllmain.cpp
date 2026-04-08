#include <windows.h>
#include <evntprov.h>
#include <traceloggingprovider.h>

// External globals (defined elsewhere)
volatile REGHANDLE g_EventHandle;      // DAT_180004020
volatile REGHANDLE g_ProviderHandle;  // DAT_180004000
void* g_EventDescriptorData;          // PTR_DAT_180004008
volatile ULONG_PTR g_SomeFlag;        // _DAT_180004028
volatile ULONG_PTR g_SomeOtherFlag;   // uRam0000000180004030

// Global critical section for multiprocessor safety
static CRITICAL_SECTION g_DllCriticalSection;
static volatile LONG g_InitializationCount = 0;
static volatile BOOL g_CriticalSectionInitialized = FALSE;

void EnsureCriticalSectionInitialized()
{
    if (InterlockedCompareExchange((LONG*)&g_CriticalSectionInitialized, FALSE, FALSE) == FALSE) {
        EnterCriticalSection(&g_DllCriticalSection);
        if (g_CriticalSectionInitialized == FALSE) {
            InitializeCriticalSection(&g_DllCriticalSection);
            InterlockedExchange((LONG*)&g_CriticalSectionInitialized, TRUE);
        }
        LeaveCriticalSection(&g_DllCriticalSection);
    }
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
    BYTE stackBuffer[32];
    ULONG_PTR securityCookie;
    DWORD regType;
    DWORD regSize;
    HKEY hKey;
    WORD descriptorSize;
    LONG eventRegisterResult;
    REGHANDLE localEventHandle = NULL;
    REGHANDLE localProviderHandle;
    ULONG_PTR stackCookie;
    
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(lpReserved);
    
    securityCookie = __security_cookie ^ (ULONG_PTR)stackBuffer;
    
    if (fdwReason == DLL_PROCESS_DETACH) {
        // Ensure all processors see the update
        MemoryBarrier();
        
        localEventHandle = (REGHANDLE)InterlockedExchangePointer((PVOID*)&g_EventHandle, NULL);
        localProviderHandle = (REGHANDLE)InterlockedExchangePointer((PVOID*)&g_ProviderHandle, NULL);
        
        if (localEventHandle != 0) {
            EventUnregister(localEventHandle);
        }
        
        // Cleanup critical section on last unload
        if (g_CriticalSectionInitialized) {
            EnterCriticalSection(&g_DllCriticalSection);
            DeleteCriticalSection(&g_DllCriticalSection);
            InterlockedExchange((LONG*)&g_CriticalSectionInitialized, FALSE);
            LeaveCriticalSection(&g_DllCriticalSection);
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH) {
        // Ensure multiprocessor-safe initialization
        EnsureCriticalSectionInitialized();
        
        EnterCriticalSection(&g_DllCriticalSection);
        
        // Use interlocked operations for initialization count
        if (InterlockedIncrement(&g_InitializationCount) == 1) {
            DisableThreadLibraryCalls(hInstance);
            
            regType = 0;
            regSize = sizeof(DWORD);
            hKey = (HKEY)0xffffffff80000002;  // HKEY_LOCAL_MACHINE
            
            RegGetValueW(hKey, 
                         L"SYSTEM\\CurrentControlSet\\Control\\OneCore",
                         L"OCFW_Enabled",
                         0x20000018,  // RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY
                         &regType,
                         NULL,
                         &regSize);
            
            // Check if the value exists (bit 2 indicates presence)
            if ((regType & 2) != 0) {
                LeaveCriticalSection(&g_DllCriticalSection);
                __debugbreak();  // Replaces swi 3
                return TRUE;
            }
            
            // Get descriptor size from event data
            descriptorSize = *(WORD*)g_EventDescriptorData;
            
            if (g_EventHandle != 0) {
                LeaveCriticalSection(&g_DllCriticalSection);
                __debugbreak();  // Replaces swi 0x29
                EnterCriticalSection(&g_DllCriticalSection);
            }
            
            // Initialize global flags
            InterlockedExchangePointer((PVOID*)&g_SomeFlag, NULL);
            InterlockedExchangePointer((PVOID*)&g_SomeOtherFlag, NULL);
            
            // Register the event provider
            eventRegisterResult = EventRegister(
                (LPCGUID)(stackBuffer + 0x48),
                (PENABLECALLBACK)_tlgEnableCallback,
                NULL,  // Callback context
                &localProviderHandle);
            
            if (eventRegisterResult == ERROR_SUCCESS) {
                // Store handles with memory barrier for other processors
                InterlockedExchangePointer((PVOID*)&g_ProviderHandle, (PVOID)localProviderHandle);
                InterlockedExchangePointer((PVOID*)&g_EventHandle, (PVOID)localEventHandle);
                
                // Set event information
                EventSetInformation(localEventHandle,
                                    EventProviderSetTraits,  // 2
                                    g_EventDescriptorData,
                                    descriptorSize);
            }
        }
        
        LeaveCriticalSection(&g_DllCriticalSection);
    }
    else if (fdwReason == DLL_THREAD_ATTACH || fdwReason == DLL_THREAD_DETACH) {
        // Thread attach/detach - minimal needed for multiprocessor compatibility
        MemoryBarrier();
    }
    
    // Ensure all pending writes are visible to other processors
    MemoryBarrier();
    
    // Stack cookie check
    stackCookie = *(ULONG_PTR*)(stackBuffer + 0x58) ^ (ULONG_PTR)stackBuffer;
    __security_check_cookie(stackCookie);
    
    return TRUE;
}

