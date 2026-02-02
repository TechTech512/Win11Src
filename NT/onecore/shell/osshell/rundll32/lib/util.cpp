#pragma warning (disable:4995)

#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <strsafe.h>
#include "rc_ids.h"

HINSTANCE g_hInstance = 0;

// Define constants for manifest resource IDs
#define IDR_MANIFEST_PRIMARY    0x7B    // 123
#define IDR_MANIFEST_SECONDARY  0x7C    // 124
#define IDR_MANIFEST_FALLBACK   2       // Fallback manifest
#define IDR_MANIFEST_MODULE     0x88    // 136 - Module manifest

// Typedef for RtlSetSearchPathMode
typedef void (NTAPI *PRTL_SET_SEARCH_PATH_MODE)(ULONG Flags);

// Window procedure declaration
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Helper function to parse a switch from command line
int _GetSwitch(wchar_t* param1, wchar_t** param2)
{
    wchar_t* currentChar = param1;
    while (*currentChar != L'\0')
    {
        currentChar++;
    }

    wchar_t* switchStart = *param2;
    wchar_t* switchEnd = switchStart;
    while (*switchEnd != L'\0')
    {
        switchEnd++;
    }

    int paramLength = (int)(currentChar - param1 - 1) / 2;
    int switchLength = (int)(switchEnd - switchStart - 1) / 2;

    if (paramLength <= switchLength)
    {
        int compareResult = CompareStringW(0x7F, 1, param1, -1, switchStart, -1);
        if (compareResult == 2)
        {
            *param2 = *param2 + paramLength * 2 - 2;
            return 1;
        }
    }
    return 0;
}

// Creates a stub window for RunDLL
HWND RunDLL_CreateStubWindow(HINSTANCE hInstance, wchar_t* param2)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_DEFAULT));
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RunDLLStubClass";

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0x80,
        L"RunDLLStubClass",
        L"",
        WS_POPUP,
        0x80000000,
        0x80000000,
        0,
        0,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    return hWnd;
}

// Helper function to call RtlSetSearchPathMode if available
void SafeSetSearchPathMode(ULONG Flags)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll)
    {
        PRTL_SET_SEARCH_PATH_MODE pRtlSetSearchPathMode = 
            (PRTL_SET_SEARCH_PATH_MODE)GetProcAddress(hNtdll, "RtlSetSearchPathMode");
        if (pRtlSetSearchPathMode)
        {
            pRtlSetSearchPathMode(Flags);
        }
        // If function not found, just continue without it
    }
}

// Initializes activation context for manifest
void* RunDLL_InitActCtx(wchar_t* param1, unsigned long* param2, int param3)
{
    wchar_t manifestPath[MAX_PATH * 2] = {0};
    wchar_t searchPath[MAX_PATH] = {0};
    void* hActCtx = (void*)-1;

    if (PathIsRelativeW(param1) == 0)
    {
        if (StringCopyWorkerW(manifestPath, MAX_PATH * 2, NULL, param1, (UINT)-1) < 0)
        {
            return NULL;
        }
    }
    else
    {
        // Dynamically load and call RtlSetSearchPathMode
        SafeSetSearchPathMode(1);
        
        if (SearchPathW(NULL, param1, L".exe", MAX_PATH, searchPath, NULL) == 0)
        {
            return NULL;
        }
        
        StringCchCatW(searchPath, MAX_PATH, L".manifest");
        wcscpy_s(manifestPath, MAX_PATH * 2, searchPath);
    }

    ACTCTXW actCtx = {0};
    actCtx.cbSize = sizeof(ACTCTXW);
    actCtx.lpSource = manifestPath;
    actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;

    if (GetFileAttributesW(manifestPath) != INVALID_FILE_ATTRIBUTES)
    {
        hActCtx = CreateActCtxW(&actCtx);
    }

    if (hActCtx == (void*)-1)
    {
        // Try different resource IDs
        actCtx.lpResourceName = MAKEINTRESOURCEW(IDR_MANIFEST_PRIMARY);
        hActCtx = CreateActCtxW(&actCtx);

        if (hActCtx == (void*)-1)
        {
            actCtx.lpResourceName = MAKEINTRESOURCEW(IDR_MANIFEST_SECONDARY);
            hActCtx = CreateActCtxW(&actCtx);

            if (hActCtx == (void*)-1)
            {
                actCtx.lpResourceName = MAKEINTRESOURCEW(IDR_MANIFEST_FALLBACK);
                hActCtx = CreateActCtxW(&actCtx);

                if (hActCtx == (void*)-1)
                {
                    // Fallback to module's manifest
                    actCtx.cbSize = sizeof(ACTCTXW);
                    actCtx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
                    actCtx.lpResourceName = MAKEINTRESOURCEW(IDR_MANIFEST_MODULE);
                    actCtx.hModule = GetModuleHandleW(NULL);
                    hActCtx = CreateActCtxW(&actCtx);
                }
            }
        }
    }

    if (hActCtx != (void*)-1 && hActCtx != NULL)
    {
        ActivateActCtx(hActCtx, NULL);
    }

    return hActCtx;
}

typedef enum _SWITCH_FLAGS
{
    SF_NO_FLAGS         = 0,
    SF_COM_STA          = 1,
    SF_COM_LOCAL_SERVER = 2
} SWITCH_FLAGS;

// Parses command line for RunDLL
int RunDLL_ParseCommand(
    wchar_t* param1,
    SWITCH_FLAGS* param2,
    wchar_t** param3,
    wchar_t** param4,
    wchar_t** param5
)
{
    wchar_t* currentPos = *param5;
    wchar_t* pathStart = NULL;
    wchar_t* pathEnd = NULL;
    wchar_t* nextToken = NULL;
    unsigned int switchFlags = 0;

    param1[0] = L'\0';
    *param2 = SF_NO_FLAGS;
    *param3 = NULL;

    while (*currentPos != L'\0')
    {
        // Skip spaces
        while (*currentPos == L' ')
        {
            currentPos++;
        }

        if (*currentPos == L'\0')
        {
            break;
        }

        // Check for switch
        if (*currentPos == L'/' || *currentPos == L'-')
        {
            currentPos++;
            if (*currentPos == L'\0')
            {
                return 0;
            }

            while (*currentPos != L'\0' && *currentPos != L' ')
            {
                wchar_t currentChar = *currentPos;
                if (currentChar == L'S' || currentChar == L's')
                {
                    if (_GetSwitch(L"S", &currentPos))
                    {
                        switchFlags |= 1;
                    }
                }
                else if (currentChar == L'L' || currentChar == L'l')
                {
                    if (_GetSwitch(L"L", &currentPos))
                    {
                        switchFlags |= 2;
                    }
                }
                else
                {
                    currentPos++;
                }
            }
        }
        else
        {
            break;
        }
    }

    if (*currentPos == L'\0')
    {
        return 0;
    }

    // Parse path (quoted or unquoted)
    pathStart = currentPos;
    if (*currentPos == L'\"')
    {
        currentPos++;
        pathStart = currentPos;
        if (*currentPos == L'\0')
        {
            return 0;
        }

        while (*currentPos != L'\0' && *currentPos != L'\"')
        {
            currentPos++;
        }

        if (*currentPos == L'\0')
        {
            return 0;
        }
        *currentPos = L'\0';
        currentPos++;
    }
    else
    {
        while (*currentPos != L'\0' && *currentPos != L' ' && *currentPos != L',')
        {
            currentPos++;
        }

        if (*currentPos == L'\0')
        {
            if ((switchFlags & 3) != 0)
            {
                goto has_switches;
            }
            return 0;
        }
        *currentPos = L'\0';
        currentPos++;
    }

    // Skip spaces after path
    while (*currentPos == L' ' || *currentPos == L',')
    {
        currentPos++;
    }

has_switches:
    pathEnd = currentPos;
    nextToken = pathEnd;

    if ((switchFlags & 3) != 0)
    {
        *(wchar_t**)param1 = pathStart;
        if ((switchFlags & 3) == 0)
        {
            *param2 = (SWITCH_FLAGS)(intptr_t)pathEnd;
            *param3 = nextToken;
        }
        else
        {
            *param2 = SF_NO_FLAGS;
            *param3 = pathEnd;
        }
        return 1;
    }

    if (*pathEnd == L'\0')
    {
        return 0;
    }

    // Parse additional parameters
    while (*nextToken != L'\0' && *nextToken != L' ')
    {
        nextToken++;
    }

    if (*nextToken != L'\0')
    {
        *nextToken = L'\0';
        nextToken++;

        while (*nextToken != L'\0' && *nextToken <= L' ')
        {
            nextToken++;
        }
    }

    // Check for path separators
    wchar_t* pathSeparator = pathEnd;
    while (*pathSeparator != L'\0')
    {
        if (*pathSeparator == L'\\' || *pathSeparator == L'/')
        {
            break;
        }
        pathSeparator = CharNextW(pathSeparator);
    }

    if (pathSeparator == NULL || *pathSeparator == L'\0')
    {
        *(wchar_t**)param1 = pathStart;
        *param2 = SF_NO_FLAGS;
        *param3 = nextToken;
        return 1;
    }

    return 0;
}

// Window procedure for stub window
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_ACTIVATEAPP)
    {
        int windowData = GetWindowLongW(hWnd, 0);
        if (windowData != 0)
        {
            HWND hwndChild = GetWindow(hWnd, GW_CHILD);
            if (hwndChild != NULL)
            {
                wchar_t className[128] = {0};
                memset(className, 0, sizeof(className));
                int classNameLen = GetClassNameW(hwndChild, className, 128);

                if (classNameLen != 0)
                {
                    if (CompareStringW(0x7F, 1, className, -1, L"IME", -1) == 2)
                    {
                        hwndChild = GetWindow(hwndChild, GW_CHILD);
                    }
                }

                HWND hwndParent = GetWindow(hwndChild, GW_OWNER);
                if (hwndParent == hWnd)
                {
                    DWORD style = GetWindowLongW(hwndChild, GWL_EXSTYLE);
                    if ((style & 0x40080) == 0)
                    {
                        DWORD classStyle = GetClassLongW(hwndChild, GCL_STYLE);
                        if (classStyle == 0)
                        {
                            DWORD parentClassStyle = GetClassLongW(hWnd, GCL_STYLE);
                            SetWindowLongW(hwndChild, GWL_EXSTYLE, style | 0x40000);
                            SetClassLongW(hwndChild, GCL_STYLE, parentClassStyle);
                        }
                    }
                }
            }
        }
    }
    else if (uMsg == WM_NOTIFY)
    {
        NMHDR* nmhdr = (NMHDR*)lParam;
        if (nmhdr->code == -500)
        {
            if (nmhdr->hwndFrom != NULL)
            {
                SetClassLongW(hWnd, GCL_STYLE, (LONG)nmhdr->hwndFrom);
            }
            SetWindowLongW(hWnd, 0, 0);
            return 1;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

