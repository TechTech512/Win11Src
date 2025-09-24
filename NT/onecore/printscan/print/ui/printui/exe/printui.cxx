#include <windows.h>
#include <wchar.h>
#include <printsqm.h>

wchar_t* __cdecl SkipProgramName(wchar_t* pszCmdLine)
{
    wchar_t* pszCurrent;
    wchar_t* pszNext;
    wchar_t wch;
    int iResult;
    
    if (pszCmdLine == NULL) {
        return NULL;
    }
    
    pszCurrent = pszCmdLine;
    
    // Skip leading whitespace
    while (*pszCurrent != L'\0') {
        iResult = iswspace(*pszCurrent);
        if (iResult == 0) break;
        pszCurrent++;
    }
    
    if (*pszCurrent == L'\0') {
        return pszCurrent;
    }
    
    wch = *pszCurrent;
    if (wch == L'\"') {
        // Quoted string - find closing quote
        pszNext = pszCurrent + 1;
        while (*pszNext != L'\0' && *pszNext != L'\"') {
            pszNext++;
        }
        
        if (*pszNext == L'\"') {
            pszNext++;
        }
    } else {
        // Unquoted string - find next whitespace
        pszNext = pszCurrent;
        while (*pszNext != L'\0') {
            iResult = iswspace(*pszNext);
            if (iResult != 0) break;
            pszNext++;
        }
    }
    
    // Skip trailing whitespace
    while (*pszNext != L'\0') {
        iResult = iswspace(*pszNext);
        if (iResult == 0) break;
        pszNext++;
    }
    
    return pszNext;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    DWORD dwSqmValue;
    int iResult;
    FARPROC pfnPrintUIEntry;
    HMODULE hModule;
    wchar_t* pszArgs;
    HWND hWnd;
    WNDCLASSW wc;
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    // Initialize window class structure
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = DefWindowProcW;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"StubPrintWindow";
    
    RegisterClassW(&wc);
    
    dwSqmValue = 0; // SQM_PRINTER_INSTALL_UNKNOWN
    
    // Create hidden window
    hWnd = CreateWindowExW(0, L"StubPrintWindow", L"", 0, 
                          CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 
                          NULL, NULL, hInstance, NULL);
    
    SqmPrinterInstallInitUIEntry(dwSqmValue);
    
    hModule = LoadLibraryW(L"printui.dll");
    if (hModule == NULL) {
        iResult = GetLastError();
    } else {
        pfnPrintUIEntry = GetProcAddress(hModule, "PrintUIEntryW");
        if (pfnPrintUIEntry == NULL) {
            iResult = GetLastError();
        } else {
            pszArgs = SkipProgramName(GetCommandLineW());
            iResult = ((int (__cdecl *)(HWND, HINSTANCE, wchar_t*))pfnPrintUIEntry)(hWnd, hInstance, pszArgs);
        }
        FreeLibrary(hModule);
    }
    
    SqmPrinterInstallReadAndResetUIEntry(&dwSqmValue);
    
    if (hWnd != NULL) {
        DestroyWindow(hWnd);
    }
    
    return iResult;
}
