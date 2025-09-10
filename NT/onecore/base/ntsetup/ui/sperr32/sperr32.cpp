#include <windows.h>
#include <intrin.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <muiload.h>
#include <utlstring.h>
#include "resource.h"

int __cdecl s32_IsProcessorAmd64(void)
{
    DWORD cpuInfo[4] = {0};
    HMODULE hKernel32;
    FARPROC pGetNativeSystemInfo;
    SYSTEM_INFO systemInfo;
    int isAmd64 = 0;

    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32 != NULL)
    {
        pGetNativeSystemInfo = GetProcAddress(hKernel32, "GetNativeSystemInfo");
        if (pGetNativeSystemInfo != NULL)
        {
            ZeroMemory(&systemInfo, sizeof(systemInfo));
            ((void (WINAPI*)(LPSYSTEM_INFO))pGetNativeSystemInfo)(&systemInfo);
            
            if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            {
                isAmd64 = 1;
            }
        }
        else
        {
            // Fallback to CPUID check if GetNativeSystemInfo is not available
            __cpuid((int*)cpuInfo, 0x80000001);
            isAmd64 = (cpuInfo[2] >> 29) & 1; // Check the LM bit (bit 29 of ECX)
        }
    }

    return isAmd64;
}

void __cdecl s32_MsgBox(HINSTANCE hInstance, UINT param_2)  // Change 'uint' to 'UINT'
{
    UTLString titleString;
    UTLString messageString;
    
    titleString.m_lpsz = nullptr;
    titleString.m_cch = 0;
    messageString.m_lpsz = nullptr;
    messageString.m_cch = 0;

    titleString.LoadStringW((HINSTANCE)0x65, 101); // Load title string
    messageString.LoadStringW(hInstance, param_2); // Load message string

    if (titleString.m_lpsz != nullptr && titleString.m_lpsz[0] != L'\0' &&
        messageString.m_lpsz != nullptr && messageString.m_lpsz[0] != L'\0')
    {
        MessageBoxW(NULL, messageString.m_lpsz, titleString.m_lpsz, MB_OK | MB_ICONINFORMATION);
    }

    titleString.v_Free();
    messageString.v_Free();
}

// Main logic rewritten cleanly
int RunSperr32() {
    UTLString resourceString;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    resourceString.LoadStringW(hInstance, IDS_WINDOWSSETUP);

    if (resourceString.m_lpsz) {
        MessageBoxW(NULL, resourceString.m_lpsz, L"Sperr32", MB_OK);
        resourceString.v_Free();
        return 0;
    }

    return -1;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int result = 0;
    wchar_t wCmdLine[256];
    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, wCmdLine, 256);

    // Assume arguments passed into LoadMUILibraryW are dynamic; hardcoded here as placeholders
    HINSTANCE hMuiLib = LoadMUILibraryW(wCmdLine, 0, 0);

    if (wCmdLine[0] == L'\0') {
        result = 0x57; // ERROR_INVALID_PARAMETER
    } else if (_wcsicmp(wCmdLine, L"x64") == 0) {
        s32_IsProcessorAmd64();
        s32_MsgBox(hInstance, IDS_32BITDETECTED);
    } else if (_wcsicmp(wCmdLine, L"ia64") != 0) {
        result = 0x57;
    } else {
        s32_MsgBox(hInstance, IDS_CANTRUNVERSION);
    }

    if (hMuiLib != NULL) {
        FreeLibrary(hMuiLib);
    }

    return result;
}



