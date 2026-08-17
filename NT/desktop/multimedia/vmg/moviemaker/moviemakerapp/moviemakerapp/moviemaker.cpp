// moviemaker.cpp – Windows Movie Maker launcher
#pragma warning(disable:4005)
#include <windows.h>
#include <winerror.h>
#include <objbase.h>
#include <ole2.h>
#include <stdlib.h>

// ------------------------------------------------------------------
// Global flag – set to 1 to start the movie maker engine
// ------------------------------------------------------------------
int g_bRunMovieMaker = 1;

// ------------------------------------------------------------------
// Class CMMHook – wrapper for moviemk.dll exports
// ------------------------------------------------------------------
class CMMHook
{
public:
    HMODULE m_hModule;
    void*   m_pUnk1;
    typedef long (__stdcall *MMInit_t)(HINSTANCE*, HINSTANCE*, wchar_t*, int, void*, void*);
    MMInit_t m_pMMInit;
    typedef long (__stdcall *MMRun_t)(int, int*);
    MMRun_t m_pMMRun;
    typedef long (__stdcall *MMFinish_t)(void*);
    MMFinish_t m_pMMFinish;
    typedef void (__stdcall *MMPostFinish_t)();
    MMPostFinish_t m_pMMPostFinish;
    typedef void* (__stdcall *MMGetPipeline_t)();
    MMGetPipeline_t m_pMMGetPipeline;
    typedef long (__stdcall *MMGetError_t)();
    MMGetError_t m_pMMGetError;
    int m_bInitialized;
    void* m_pParam;

    CMMHook()
        : m_hModule(NULL), m_pUnk1(NULL),
          m_pMMInit(NULL), m_pMMRun(NULL), m_pMMFinish(NULL),
          m_pMMPostFinish(NULL), m_pMMGetPipeline(NULL), m_pMMGetError(NULL),
          m_bInitialized(0), m_pParam(NULL) {}

    ~CMMHook()
    {
        if (m_bInitialized && m_pMMPostFinish)
            m_pMMPostFinish();
        if (m_hModule)
        {
            FreeLibrary(m_hModule);
            m_hModule = NULL;
        }
    }

    long Init(HINSTANCE* hInstance, HINSTANCE* hPrevInstance, wchar_t* lpCmdLine, int nCmdShow);
    long Run(int nParam, int* pRetCode);
    long Finish();
};

// ------------------------------------------------------------------
// Global instance
// ------------------------------------------------------------------
CMMHook vmm;

// ------------------------------------------------------------------
// Implementation
// ------------------------------------------------------------------
long CMMHook::Init(HINSTANCE* hInstance, HINSTANCE* hPrevInstance,
                   wchar_t* lpCmdLine, int nCmdShow)
{
    HMODULE hDll = LoadLibraryW(L"moviemk.dll");
    m_hModule = hDll;
    if (!hDll)
        return E_POINTER;

    m_pMMInit = (MMInit_t)GetProcAddress(hDll, "MMInit");
    if (!m_pMMInit) return E_POINTER;

    m_pMMRun = (MMRun_t)GetProcAddress(hDll, "MMRun");
    if (!m_pMMRun) return E_POINTER;

    m_pMMFinish = (MMFinish_t)GetProcAddress(hDll, "MMFinish");
    if (!m_pMMFinish) return E_POINTER;

    m_pMMPostFinish = (MMPostFinish_t)GetProcAddress(hDll, "MMPostFinish");
    if (!m_pMMPostFinish) return E_POINTER;

    m_pMMGetPipeline = (MMGetPipeline_t)GetProcAddress(hDll, "MMGetPipeline");
    if (!m_pMMGetPipeline) return E_POINTER;

    m_pMMGetError = (MMGetError_t)GetProcAddress(hDll, "MMGetError");
    if (!m_pMMGetError) return E_POINTER;

    long result = m_pMMInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow,
                            &m_pParam, &m_pUnk1);
    if (result >= 0)
    {
        m_bInitialized = TRUE;
        return S_OK;
    }
    return result;
}

long CMMHook::Run(int nParam, int* pRetCode)
{
    if (!pRetCode || !m_pMMRun)
        return E_POINTER;

    long result = m_pMMRun(nParam, pRetCode);
    return (result >= 0) ? S_OK : result;
}

long CMMHook::Finish()
{
    if (!m_pMMFinish)
        return E_POINTER;

    long result = m_pMMFinish(m_pParam);
    if (result >= 0)
    {
        if (m_bInitialized && m_pMMPostFinish)
            m_pMMPostFinish();
        if (m_hModule)
        {
            FreeLibrary(m_hModule);
            m_hModule = NULL;
        }
        m_bInitialized = TRUE;
        return S_OK;
    }
    return result;
}

// ------------------------------------------------------------------
// WinMain – simple flow: Init → Run (blocks) → Finish
// ------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // Initialise COM and OLE (required for drag‑and‑drop and file dialogs)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
        OleInitialize(NULL);

    // Original setup calls
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    wchar_t* lpCmdLineW = GetCommandLineW();
    AllowSetForegroundWindow(GetCurrentProcessId());

    int nRetCode = 0;
    long lResult = vmm.Init(&hInstance, &hPrevInstance, lpCmdLineW, nCmdShow);

    if (lResult >= 0 && g_bRunMovieMaker)
    {
        // MMRun should block until the user closes the Movie Maker window.
        lResult = vmm.Run(nCmdShow, &nRetCode);
        if (lResult < 0)
            goto cleanup;
    }

    // After Run returns, shut down the engine.
    lResult = vmm.Finish();
    if (lResult >= 0)
        lResult = nRetCode;

cleanup:
    OleUninitialize();
    CoUninitialize();
    return (int)lResult;
}

