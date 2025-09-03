#include "pch.h"

float __cdecl GetDPIScalingFactor(void)
{
    int errorCode = 0;
    float scalingFactor = 1.0f;
    
    HDC hdc = GetDC(nullptr);
    if (hdc == nullptr) {
        errorCode = GetLastError();
        if (errorCode == 0) {
            errorCode = 0x1f;
        }
    }
    else {
        int logPixels = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
        scalingFactor = static_cast<float>(logPixels) / 96.0f;
    }
    
    SetLastError(errorCode);
    return scalingFactor;
}

int __cdecl MakeProcessDPIAware(void)
{
    int result = 0;
    int errorCode = 0;
    uintptr_t securityCookie = 0;
    
    const wchar_t* user32Dll = L"user32.dll";
    HMODULE hUser32 = LoadLibraryW(user32Dll);
    
    if (hUser32 == nullptr) {
        errorCode = GetLastError();
        if (errorCode == 0) {
            errorCode = 0x1f;
        }
        goto cleanup;
    }
    
    FARPROC setProcessDPIAwareFunc = GetProcAddress(hUser32, "SetProcessDPIAware");
    if (setProcessDPIAwareFunc == nullptr) {
        errorCode = GetLastError();
        if (errorCode == 0) {
            errorCode = 0x1f;
        }
    }
    else {
        result = reinterpret_cast<int(__stdcall*)()>(setProcessDPIAwareFunc)();
        if (result == 0) {
            errorCode = GetLastError();
            if (errorCode == 0) {
                errorCode = 0x1f;
            }
        }
    }
    
    FreeLibrary(hUser32);

cleanup:
    SetLastError(errorCode);
    return result;
}

