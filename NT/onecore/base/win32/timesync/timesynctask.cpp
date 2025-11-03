// timesynctask.cpp
#include <Windows.h>
#include <winsvc.h>
#include <wintask.h>

extern "C" {
    HRESULT DllCanUnloadNow();
    HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
    BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, void* lpReserved);
    HRESULT DllRegisterServer();
    HRESULT DllUnregisterServer();
    
    // External functions
    long TsConditionallyStartService(wchar_t* param_1, DWORD* param_2, wchar_t** param_3, BYTE* param_4);
    long TsStopService(wchar_t* param_1);
}

class CTimeSyncTaskHandler {
private:
    DWORD m_dwState;

public:
    HRESULT ForceSynchronizeTimeTask();
    HRESULT Worker();
};

// Critical section implementation
CRITICAL_SECTION g_csLock;
BOOL g_bCsInitialized = FALSE;

void LOCK() {
    if (!g_bCsInitialized) {
        InitializeCriticalSection(&g_csLock);
        g_bCsInitialized = TRUE;
    }
    EnterCriticalSection(&g_csLock);
}

void UNLOCK() {
    if (g_bCsInitialized) {
        LeaveCriticalSection(&g_csLock);
    }
}

extern "C" HRESULT DllCanUnloadNow()
{
    LOCK();
    BOOL bCanUnload = (CWinTaskHandler::s_cInstances == 0);
    UNLOCK();
    return bCanUnload ? S_OK : S_FALSE;
}

extern "C" HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (ppv == NULL) {
        return E_INVALIDARG;
    }
    *ppv = NULL;
    return E_NOTIMPL;
}

extern "C" BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInstance);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        if (g_bCsInitialized) {
            DeleteCriticalSection(&g_csLock);
            g_bCsInitialized = FALSE;
        }
    }
    return TRUE;
}

extern "C" HRESULT DllRegisterServer()
{
    return E_NOTIMPL;
}

extern "C" HRESULT DllUnregisterServer()
{
    return E_NOTIMPL;
}

HRESULT CTimeSyncTaskHandler::ForceSynchronizeTimeTask()
{
    BOOL bServiceStarted = FALSE;
    HRESULT hr = E_FAIL;
    
    long lResult = TsConditionallyStartService(L"w32time", (DWORD*)&bServiceStarted, NULL, NULL);
    if (FAILED(lResult)) {
        return hr;
    }
    
    LOCK();
    BOOL bCancelRequested = (m_dwState == 1);
    UNLOCK();
    
    if (bCancelRequested) {
        hr = E_ABORT;
        goto cleanup;
    }
    
    HMODULE hW32Time = LoadLibraryExW(L"w32time.dll", NULL, 0);
    if (hW32Time == NULL) {
        hr = E_FAIL;
        goto cleanup;
    }
    
    typedef DWORD (WINAPI* W32TimeSyncNow_t)(DWORD, DWORD, DWORD);
    W32TimeSyncNow_t pW32TimeSyncNow = (W32TimeSyncNow_t)GetProcAddress(hW32Time, "W32TimeSyncNow");
    
    if (pW32TimeSyncNow == NULL) {
        FreeLibrary(hW32Time);
        hr = E_FAIL;
        goto cleanup;
    }
    
    for (DWORD i = 0; i < 11; i++) {
        DWORD dwResult = pW32TimeSyncNow(0, 1, 0x12);
        if (dwResult == 0) {
            hr = S_OK;
            break;
        }
        
        LOCK();
        bCancelRequested = (m_dwState == 1);
        UNLOCK();
        
        if (bCancelRequested) {
            hr = E_ABORT;
            break;
        }
    }
    
    FreeLibrary(hW32Time);

cleanup:
    if (bServiceStarted) {
        TsStopService(L"w32time");
    }
    
    return hr;
}

HRESULT CTimeSyncTaskHandler::Worker()
{
    return ForceSynchronizeTimeTask();
}

