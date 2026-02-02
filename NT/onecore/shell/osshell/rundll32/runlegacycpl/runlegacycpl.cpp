#include <windows.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shellapi.h>
#include "rc_ids.h"

// Define constants for manifest resource IDs
#define IDR_MANIFEST_PRIMARY    0x7B    // 123
#define IDR_MANIFEST_SECONDARY  0x7C    // 124
#define IDR_MANIFEST_FALLBACK   2       // Fallback manifest
#define IDR_MANIFEST_MODULE     0x88    // 136 - Module manifest

// External declarations
extern HINSTANCE g_hInstance;
extern void RtlSetSearchPathMode(DWORD Flags);

// Control_RunDLLW is exported by shell32.dll
typedef void (WINAPI *CONTROL_RUNDLLW)(HWND, HINSTANCE, LPCWSTR, int);
#define CONTROL_RUNDLL_ENTRY "Control_RunDLLW"

// Function declarations from util.cpp
extern int _GetSwitch(wchar_t* param1, wchar_t** param2);
extern HWND RunDLL_CreateStubWindow(HINSTANCE hInstance, wchar_t* param2);
extern void* RunDLL_InitActCtx(wchar_t* param1, unsigned long* param2, int param3);

typedef enum _SWITCH_FLAGS
{
    SF_NO_FLAGS         = 0,
    SF_COM_STA          = 1,
    SF_COM_LOCAL_SERVER = 2
} SWITCH_FLAGS;

int RunDLL_ParseCommand(
    wchar_t* param1,
    SWITCH_FLAGS* param2,
    wchar_t** param3,
    wchar_t** param4,
    wchar_t** param5
);

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Windows entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nShowCmd)
{
    SWITCH_FLAGS switchFlags = SF_NO_FLAGS;
    wchar_t parsedPath[2] = {0};
    wchar_t* remainingArgs = NULL;
    wchar_t* cmdLineCopy = NULL;
    void* hActCtx = (void*)-1;
    HWND hStubWindow = NULL;
    ULONG_PTR actCtxCookie = 0;
    int cmdLineLength = 0;
    
    // Initialize security features
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    // Store instance handle globally
    g_hInstance = hInstance;
    
    // Calculate required buffer size for command line copy
    cmdLineLength = lstrlenW(lpCmdLine);
    if (cmdLineLength == 0)
    {
        return 0;
    }
    
    // Allocate buffer for command line copy
    size_t bufferSize = (cmdLineLength + 1) * sizeof(wchar_t);
    cmdLineCopy = (wchar_t*)LocalAlloc(LMEM_FIXED, bufferSize);
    
    if (cmdLineCopy == NULL)
    {
        // Show error if no memory
        MessageBoxW(NULL, L"Not enough memory to process command line.", 
                   MAKEINTRESOURCEW(IDS_TITLETEXT), MB_ICONERROR | MB_OK);
        return 0;
    }
    
    // Copy command line
    HRESULT hr = StringCchCopyW(cmdLineCopy, cmdLineLength + 1, lpCmdLine);
    if (FAILED(hr))
    {
        LocalFree(cmdLineCopy);
        return 0;
    }
    
    // Parse command line
    if (RunDLL_ParseCommand(parsedPath, &switchFlags, &remainingArgs, 
                           (wchar_t**)&cmdLineCopy, (wchar_t**)&lpCmdLine) != 0)
    {
        // Set error mode to suppress critical error dialogs
        UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        
        // Initialize activation context
        unsigned long initParam = 0;
        hActCtx = RunDLL_InitActCtx((wchar_t*)remainingArgs, &initParam, (int)switchFlags);
        
        // Create stub window
        hStubWindow = RunDLL_CreateStubWindow(hInstance, (wchar_t*)&initParam);
        
        // Load Control_RunDLLW from shell32.dll
        HINSTANCE hShell32 = LoadLibraryW(L"shell32.dll");
        if (hShell32 != NULL)
        {
            CONTROL_RUNDLLW pControlRunDLLW = (CONTROL_RUNDLLW)GetProcAddress(hShell32, CONTROL_RUNDLL_ENTRY);
            if (pControlRunDLLW != NULL)
            {
                // Execute the Control Panel applet or DLL
                pControlRunDLLW(hStubWindow, g_hInstance, remainingArgs, nShowCmd);
            }
            else
            {
                MessageBoxW(NULL, MAKEINTRESOURCEW(IDS_MISSINGENTRY), 
                           MAKEINTRESOURCEW(IDS_TITLETEXT), MB_ICONERROR | MB_OK);
            }
            FreeLibrary(hShell32);
        }
        else
        {
            MessageBoxW(NULL, MAKEINTRESOURCEW(IDS_ERRORLOADINGDLL), 
                       MAKEINTRESOURCEW(IDS_TITLETEXT), MB_ICONERROR | MB_OK);
        }
        
        // Cleanup
        if (hStubWindow != NULL)
        {
            DestroyWindow(hStubWindow);
        }
        
        if (hActCtx != (void*)-1)
        {
            if (actCtxCookie != 0)
            {
                DeactivateActCtx(0, actCtxCookie);
            }
            ReleaseActCtx(hActCtx);
        }
        
        // Restore original error mode
        SetErrorMode(oldErrorMode);
    }
    else
    {
        // Show error for invalid command line
        MessageBoxW(NULL, MAKEINTRESOURCEW(IDS_ERRORILLEGALVALUE), 
                   MAKEINTRESOURCEW(IDS_TITLETEXT), MB_ICONERROR | MB_OK);
    }
    
    // Free allocated memory
    LocalFree(cmdLineCopy);
    
    return 0;
}

