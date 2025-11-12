#include <windows.h>
#include <strsafe.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    DWORD errorCode = 0;
    HMODULE hPlaLibrary = NULL;
    wchar_t systemDirectory[MAX_PATH];
    wchar_t plaDllPath[MAX_PATH];
    
    // Get system directory
    UINT pathLength = GetSystemDirectoryW(systemDirectory, MAX_PATH);
    if (pathLength == 0) {
        errorCode = GetLastError();
        if (errorCode > 0) {
            errorCode = errorCode & 0xFFFF | 0x80070000;
        }
        return errorCode;
    }
    
    // Check if path fits in buffer
    if (pathLength >= MAX_PATH) {
        return 0x8007007A; // ERROR_INSUFFICIENT_BUFFER
    }
    
    // Build full path to pla.dll
    if (FAILED(StringCchPrintfW(plaDllPath, MAX_PATH, L"%s\\pla.dll", systemDirectory))) {
        return 0x80070000; // Generic error
    }
    
    // Load pla.dll
    hPlaLibrary = LoadLibraryW(plaDllPath);
    if (hPlaLibrary == NULL) {
        errorCode = GetLastError();
        if (errorCode > 0) {
            errorCode = errorCode & 0xFFFF | 0x80070000;
        }
        return errorCode;
    }
    
    // Get PlaServer function address
    FARPROC plaServerFunc = GetProcAddress(hPlaLibrary, "PlaServer");
    if (plaServerFunc == NULL) {
        errorCode = GetLastError();
        if (errorCode > 0) {
            errorCode = errorCode & 0xFFFF | 0x80070000;
        }
        FreeLibrary(hPlaLibrary);
        return errorCode;
    }
    
    // Call PlaServer function
    ((void (__cdecl *)(void))plaServerFunc)();
    
    // Cleanup
    FreeLibrary(hPlaLibrary);
    return 0;
}

