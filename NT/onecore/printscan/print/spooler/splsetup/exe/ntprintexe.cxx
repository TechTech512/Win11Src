#include <windows.h>
#include <strsafe.h>

// Global arrays
const wchar_t* MethodsAllowedToElevate[] = {
    L"bidiui",
    L"publish",
    L"driverui",
    L"document"
};

const char* STRMethodsAllowedToElevate[] = {
    "bidiui",
    "publish", 
    "driverui",
    "document"
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    wchar_t* allocatedCmdLine = NULL;
    wchar_t* guidStart = NULL;
    HMODULE ntprintLibrary = NULL;
    HWND windowHandle = NULL;
    int result = 0;
    DWORD lastError = 0;
    int methodIndex = 4; // Default to invalid index
    WNDCLASSW wc = {0};
    
    // Enable heap termination on corruption
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    // Check if command line is provided
    if (lpCmdLine == NULL || lpCmdLine[0] == L'\0') {
        lastError = 0x57; // ERROR_INVALID_PARAMETER
        goto Cleanup;
    }
    
    // Allocate buffer for command line copy
    size_t cmdLineLength;
    if (FAILED(StringCchLengthW(lpCmdLine, STRSAFE_MAX_CCH, &cmdLineLength))) {
        lastError = 0x57; // ERROR_INVALID_PARAMETER
        goto Cleanup;
    }
    
    allocatedCmdLine = (wchar_t*)LocalAlloc(LMEM_FIXED, (cmdLineLength + 1) * sizeof(wchar_t));
    if (allocatedCmdLine == NULL) {
        lastError = 0x7; // ERROR_OUTOFMEMORY
        goto Cleanup;
    }
    
    // Copy command line
    if (FAILED(StringCchCopyW(allocatedCmdLine, cmdLineLength + 1, lpCmdLine))) {
        lastError = 0x57; // ERROR_INVALID_PARAMETER
        goto Cleanup;
    }
    
    // Find space separator and GUID start
    wchar_t* spacePos = (wchar_t*)wcschr(allocatedCmdLine, L' ');
    if (spacePos != NULL) {
        *spacePos = L'\0'; // Null-terminate the method name
        guidStart = (wchar_t*)wcschr(spacePos + 1, L'{'); // Find GUID start
    }
    
    if (guidStart == NULL) {
        lastError = 0x57; // ERROR_INVALID_PARAMETER
        goto Cleanup;
    }
    
    // Determine which method is being called
    for (int i = 0; i < 4; i++) {
        if (wcscmp(allocatedCmdLine, MethodsAllowedToElevate[i]) == 0) {
            methodIndex = i;
            break;
        }
    }
    
    if (methodIndex == 4) {
        lastError = 0x57; // ERROR_INVALID_PARAMETER
        goto Cleanup;
    }
    
    // Register window class
    wc.lpfnWndProc = (WNDPROC)DefWindowProcW;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"StubNtPrintWindow";
    
    if (!RegisterClassW(&wc)) {
        lastError = GetLastError();
        goto Cleanup;
    }
    
    // Create window
    windowHandle = CreateWindowExW(
        0,
        L"StubNtPrintWindow",
        L"",
        WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );
    
    if (windowHandle == NULL) {
        lastError = GetLastError();
        if (lastError == 0) lastError = 0x57; // Ensure we have an error code
        goto Cleanup;
    }
    
    // Load ntprint.dll
    ntprintLibrary = LoadLibraryW(L"ntprint.dll");
    if (ntprintLibrary == NULL) {
        lastError = GetLastError();
        if (lastError == 0) lastError = 0x57;
        goto Cleanup;
    }
    
    // Get the function address for the requested method
    FARPROC methodFunction = GetProcAddress(ntprintLibrary, STRMethodsAllowedToElevate[methodIndex]);
    if (methodFunction == NULL) {
        lastError = GetLastError();
        if (lastError == 0) lastError = 0x57;
        goto Cleanup;
    }
    
    // Call the method
    result = ((int (__cdecl *)(HWND, HINSTANCE, wchar_t*, int))methodFunction)(
        windowHandle, hInstance, guidStart, nCmdShow
    );
    lastError = result;

Cleanup:
    // Cleanup resources
    if (allocatedCmdLine != NULL) {
        LocalFree(allocatedCmdLine);
    }
    
    if (ntprintLibrary != NULL) {
        FreeLibrary(ntprintLibrary);
    }
    
    if (windowHandle != NULL) {
        DestroyWindow(windowHandle);
    }
    
    return lastError;
}

