#pragma warning (disable:4530)

#include <windows.h>
#include <ole2.h>
#include <iostream>

// Forward declarations
class ProcessRefCount;

// Global GUID array
static const GUID c_rgSupportedObjects[] = {
    // First GUID: {0x34DEA897, 0x7365, 0x4F60, {0xBA, 0x26, 0x53, 0xDA, 0x4B, 0x89, 0x22, 0x6D}}
    {0x34DEA897, 0x7365, 0x4F60, {0xBA, 0x26, 0x53, 0xDA, 0x4B, 0x89, 0x22, 0x6D}},
    
    // Second GUID: {0x9A4948D9, 0xFC13, 0x4FAC, {0xB6, 0x0A, 0xFB, 0xA6, 0xEE, 0x0F, 0xB1, 0x1C}}
    {0x9A4948D9, 0xFC13, 0x4FAC, {0xB6, 0x0A, 0xFB, 0xA6, 0xEE, 0x0F, 0xB1, 0x1C}}
};

// ProcessRefCount class
class ProcessRefCount
{
public:
    // Virtual function table pointer
    void* vftable;
    
    // Reference count
    LONG _cRef;
    
    // Termination event
    HANDLE _hTerminateEvent;
    
    // Timer for delayed shutdown
    PTP_TIMER _pTimer;
    
    // Constructor
    ProcessRefCount() : vftable(nullptr), _cRef(0), _hTerminateEvent(nullptr), _pTimer(nullptr) {}
    
    // Methods
    HRESULT Initialize();
    void CancelExitTimer();
    ULONG AddRef();
    ULONG Release();
    HRESULT QueryInterface(REFIID riid, void** ppv);
    
    // Static callback
    static VOID CALLBACK s_TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);
};

// Global ProcessRefCount instance
ProcessRefCount* g_pProcessRefCount = nullptr;

void SetProcessReference(ProcessRefCount* pRef)
{
    g_pProcessRefCount = pRef;
}

// Timer callback implementation
VOID CALLBACK ProcessRefCount::s_TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer)
{
    HANDLE hEvent = (HANDLE)Context;
    if (hEvent != nullptr)
    {
        SetEvent(hEvent);
    }
}

// Initialize the ProcessRefCount
HRESULT ProcessRefCount::Initialize()
{
    // Create termination event
    _hTerminateEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (_hTerminateEvent == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    // Store global reference
    SetProcessReference(this);
    
    // Create threadpool timer
    _pTimer = CreateThreadpoolTimer(s_TimerCallback, _hTerminateEvent, nullptr);
    if (_pTimer == nullptr)
    {
        CloseHandle(_hTerminateEvent);
        _hTerminateEvent = nullptr;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    // Set timer for delayed shutdown (10 minutes = 6000000000 nanoseconds)
    // 0xDC3CBA00 = 6000000000 nanoseconds
    ULARGE_INTEGER dueTime;
    dueTime.QuadPart = -6000000000LL; // Negative for relative time
    
    FILETIME ftDueTime;
    ftDueTime.dwHighDateTime = dueTime.HighPart;
    ftDueTime.dwLowDateTime = dueTime.LowPart;
    
    SetThreadpoolTimer(_pTimer, &ftDueTime, 0, 0);
    
    return S_OK;
}

// Cancel the exit timer
void ProcessRefCount::CancelExitTimer()
{
    if (_pTimer != nullptr)
    {
        if (IsThreadpoolTimerSet(_pTimer))
        {
            SetThreadpoolTimer(_pTimer, nullptr, 0, 0);
            WaitForThreadpoolTimerCallbacks(_pTimer, TRUE);
        }
    }
}

// Add reference
ULONG ProcessRefCount::AddRef()
{
    CancelExitTimer();
    
    // Thread-safe increment
    return InterlockedIncrement(&_cRef);
}

// Release reference
ULONG ProcessRefCount::Release()
{
    ULONG newCount = InterlockedDecrement(&_cRef);
    
    if (newCount == 0)
    {
        if (_pTimer == nullptr)
        {
            // No timer, signal event immediately
            if (_hTerminateEvent != nullptr)
            {
                SetEvent(_hTerminateEvent);
            }
        }
        else
        {
            // Set timer for delayed shutdown (30 seconds = 300000000 nanoseconds)
            // 0xFD050F80 = 300000000 nanoseconds
            ULARGE_INTEGER dueTime;
            dueTime.QuadPart = -300000000LL; // Negative for relative time
            
            FILETIME ftDueTime;
            ftDueTime.dwHighDateTime = dueTime.HighPart;
            ftDueTime.dwLowDateTime = dueTime.LowPart;
            
            SetThreadpoolTimer(_pTimer, &ftDueTime, 0, 0);
            
            // Free unused libraries
            CoFreeUnusedLibraries();
        }
    }
    
    return newCount;
}

// QueryInterface implementation
HRESULT ProcessRefCount::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == nullptr)
    {
        return E_POINTER;
    }
    
    *ppv = nullptr;
    
    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IUnknown*>(static_cast<void*>(this));
        AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

// Main entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Enable low fragmentation heap
    HeapSetInformation(nullptr, HeapCompatibilityInformation, nullptr, 0);
    
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        return 1;
    }
    
    ProcessRefCount processRefCount;
    hr = processRefCount.Initialize();
    
    DWORD dwRegCookies[2] = {0};
    int iRegisteredObjects = 0;
    
    if (SUCCEEDED(hr))
    {
        // Register COM objects
        for (int i = 0; i < sizeof(c_rgSupportedObjects) / sizeof(GUID) && i < 2; i++)
        {
            IClassFactory* pClassFactory = nullptr;
            
            hr = CoGetClassObject(
                c_rgSupportedObjects[i],
                CLSCTX_LOCAL_SERVER,
                nullptr,
                IID_IClassFactory,
                (void**)&pClassFactory);
            
            if (SUCCEEDED(hr))
            {
                hr = CoRegisterClassObject(
                    c_rgSupportedObjects[i],
                    pClassFactory,
                    CLSCTX_LOCAL_SERVER,
                    REGCLS_MULTIPLEUSE,
                    &dwRegCookies[iRegisteredObjects]);
                
                pClassFactory->Release();
                
                if (SUCCEEDED(hr))
                {
                    iRegisteredObjects++;
                }
            }
            
            if (FAILED(hr))
            {
                break;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            // Wait for termination event
            HANDLE hEvent = processRefCount._hTerminateEvent;
            CoWaitForMultipleHandles(
                0,
                INFINITE,
                1,
                &hEvent,
                nullptr);
        }
        
        // Revoke all registered objects
        for (int i = 0; i < iRegisteredObjects; i++)
        {
            if (dwRegCookies[i] != 0)
            {
                CoRevokeClassObject(dwRegCookies[i]);
            }
        }
    }
    
    CoUninitialize();
    
    // Cleanup ProcessRefCount
    processRefCount.vftable = nullptr; // Set vftable
    
    if (processRefCount._pTimer != nullptr)
    {
        processRefCount.CancelExitTimer();
        CloseThreadpoolTimer(processRefCount._pTimer);
    }
    
    if (processRefCount._hTerminateEvent != nullptr)
    {
        CloseHandle(processRefCount._hTerminateEvent);
    }
    
    return 0;
}

