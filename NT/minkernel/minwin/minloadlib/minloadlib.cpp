/*
 * minloadlib.cpp
 *
 * Minimal Load Library Utility.
 * Loads a module with LoadLibraryEx and optional flags, then unloads it.
 * Supports WinRT/XAML runtime instantiation.
 */

#pragma warning (disable:4083)
#include <windows.h>
#include <roapi.h>
#include <wrl.h>
#include <activation.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "onecore.lib")

using namespace Microsoft::WRL;

void __cdecl operator delete(void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void* ptr, unsigned int)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

// ------------------------------------------------------------------
// Forward declarations of COM interfaces used
// ------------------------------------------------------------------
// IXamlRuntime – interface from Windows.UI.Xaml.Hosting
// The GUID is from the decompiled binary: {c805b0c0-6210-4e4f-b76a-e894e8b1a4ad}
MIDL_INTERFACE("c805b0c0-6210-4e4f-b76a-e894e8b1a4ad")
IXamlRuntime : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE InitializeRuntime(
        /* [in] */ void* pUnknown
    ) = 0;
    // There are more methods, but we only call the one at offset 0x28 (method 10)
    // That corresponds to the 10th method in the vtable (0x28 / 4 = 10)
    // Method 10: HRESULT StartRuntime() or similar? Actually the code calls method at vtable+0x28, which is the 11th method (0-indexed 10).
    // We'll define it as StartRuntime.
    virtual HRESULT STDMETHODCALLTYPE StartRuntime() = 0;
    // We'll not define all methods, just the one we need.
};

// ------------------------------------------------------------------
// Helper: Print help message
// ------------------------------------------------------------------
void PrintHelpMessage(void)
{
    wprintf(L"Loads library on module using LoadLibraryEx(), then unloads using FreeLibrary()\n");
    wprintf(L"Usage:\n");
    wprintf(
        L"  MINLOADLIB [-M Binary Name] [-F LoadLibraryEx() Flags] [-S (Optional <FULLPATH 1> <FULLPATH 2> ...)]\n"
    );
    wprintf(L"  -M: specify binary to be loaded\n");
    wprintf(L"  -F: Optional - LoadLibraryEx() flags\n");
    wprintf(L"      Defaults to 0. Flag values listed in:\n");
    wprintf(L"      msdn.microsoft.com/en-us/library/windows/desktop/ms684179(v=vs.85).aspx\n");
    wprintf(L"  -S: Optional - add additional dependency search paths\n");
    wprintf(L"      -S with no parameter means add the same directory as -M module\n\n");
    wprintf(L"Examples:\n");
    wprintf(L"  MinLoadLib.exe -m foo.dll\n");
    wprintf(L"  MinLoadLib.exe -m c:\\test\\foo.dll -f 0x11000 -s\n");
    wprintf(L"  MinLoadLib.exe -m foo.dll -f 0x400 -s <path1> <path2> <...>\n");
	return;
}

// ------------------------------------------------------------------
// Convert a string to DWORD, handling hex with 0x prefix
// ------------------------------------------------------------------
DWORD StrToDw(LPCWSTR szStr)
{
    DWORD value = 0;
    if (szStr[0] == L'0' && (szStr[1] == L'x' || szStr[1] == L'X')) {
        // Hex
        const wchar_t* p = szStr + 2;
        while (*p) {
            wchar_t c = *p;
            if (c >= L'0' && c <= L'9') {
                value = (value << 4) + (c - L'0');
            } else if (c >= L'A' && c <= L'F') {
                value = (value << 4) + (c - L'A' + 10);
            } else if (c >= L'a' && c <= L'f') {
                value = (value << 4) + (c - L'a' + 10);
            } else {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            p++;
        }
    } else {
        // Decimal
        value = (DWORD)_wtol(szStr);
    }
    return value;
}

// ------------------------------------------------------------------
// Process the module: load, try to activate XAML runtime, unload
// ------------------------------------------------------------------
DWORD ProcessModule(LPCWSTR szModulePath, DWORD dwFlags)
{
    DWORD dwError = 0;
    HMODULE hModule = NULL;
    HRESULT hr;
    HSTRING hstr = NULL;
    IActivationFactory* pFactory = NULL;
    IUnknown* pRuntime = NULL;
    HMODULE hXaml = NULL;
    FARPROC pfnDllGetActivationFactory = NULL;
    HSTRING_HEADER header;

    // Initialize WinRT with multithreaded mode
    hr = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(hr)) {
        wprintf(L"Failed in WinRTInitialize(WINRT_INIT_MULTITHREADED) -- GLE: 0x%x\n", hr);
        return hr;
    }

    // Load the module
    SetLastError(0);
    hModule = LoadLibraryExW(szModulePath, NULL, dwFlags);
    if (hModule == NULL) {
        dwError = GetLastError();
        wprintf(L"Failed to load module: %ws -- GLE: 0x%x\n", szModulePath, dwError);
    } else {
        wprintf(L"Successfully loaded module: %ws\n", szModulePath);
    }

    // If WinRT initialization succeeded, try to activate XAML runtime
    if (SUCCEEDED(hr)) {
        // Check if Windows.UI.Xaml.dll is already loaded
        hXaml = GetModuleHandleW(L"Windows.UI.Xaml.dll");
        if (hXaml != NULL && hXaml != hModule) {
            // Get DllGetActivationFactory from Windows.UI.Xaml.dll
            pfnDllGetActivationFactory = GetProcAddress(hXaml, "DllGetActivationFactory");
            if (pfnDllGetActivationFactory != NULL) {
                const wchar_t* className = L"Windows.UI.Xaml.Hosting.XamlRuntime";
                hr = WindowsCreateStringReference(className, (UINT32)wcslen(className),
                                                  &header, &hstr);
                if (SUCCEEDED(hr)) {
                    // Call DllGetActivationFactory via function pointer
                    typedef HRESULT (WINAPI *PFN_DllGetActivationFactory)(HSTRING, IActivationFactory**);
                    PFN_DllGetActivationFactory pfn = (PFN_DllGetActivationFactory)pfnDllGetActivationFactory;
                    hr = pfn(hstr, &pFactory);
                    if (SUCCEEDED(hr) && pFactory != NULL) {
                        // QueryInterface for IXamlRuntime (GUID from decompiled binary)
                        GUID guid = {0xc805b0c0, 0x6210, 0x4e4f, {0xb7, 0x6a, 0xe8, 0x94, 0xe8, 0xb1, 0xa4, 0xad}};
                        hr = pFactory->QueryInterface(guid, (void**)&pRuntime);
                        if (SUCCEEDED(hr) && pRuntime != NULL) {
                            // Call method at vtable offset 0x28 (method 10)
                            void** vtable = *(void***)pRuntime;
                            typedef HRESULT (WINAPI *PFN_StartRuntime)(IUnknown*);
                            PFN_StartRuntime pfnStart = (PFN_StartRuntime)vtable[10];
                            hr = pfnStart(pRuntime);
                            // Original code ignored the return value
                        }
                        if (pRuntime) pRuntime->Release();
                    }
                    if (pFactory) pFactory->Release();
                    WindowsDeleteString(hstr);
                }
            }
        }
    }

    // Unload the module if it was loaded
    if (hModule != NULL) {
        if (!FreeLibrary(hModule)) {
            dwError = GetLastError();
            wprintf(L"Failed to unload module: %ws -- GLE: 0x%x\n", szModulePath, dwError);
        } else {
            wprintf(L"Successfully unloaded module: %ws\n", szModulePath);
        }
    }

    // Uninitialize WinRT if we initialized it
    RoUninitialize();

    return dwError;
}

// ------------------------------------------------------------------
// wmain – parse arguments and run
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    DWORD dwFlags = 0;
    wchar_t szModulePath[MAX_PATH] = { 0 };
    BOOL bModuleSpecified = FALSE;
    BOOL bSearchPathsAdded = FALSE;
    int i = 1;

    while (i < argc) {
        wchar_t* arg = argv[i];
        if (arg[0] == L'/' || arg[0] == L'-') {
            wchar_t option = arg[1];
            switch (option) {
                case L'?':
                case L'h':
                case L'H':
                    PrintHelpMessage();
                    return 0;

                case L'm':
                case L'M':
                    if (i + 1 >= argc) {
                        wprintf(L"Missing parameter: module name.\n");
                        PrintHelpMessage();
                        return 1;
                    }
                    i++;
                    if (wcslen(argv[i]) >= MAX_PATH) {
                        wprintf(L"Error: Module name too long.\n");
                        return 1;
                    }
                    StringCchCopyW(szModulePath, MAX_PATH, argv[i]);
                    bModuleSpecified = TRUE;
                    break;

                case L'f':
                case L'F':
                    if (i + 1 >= argc) {
                        wprintf(L"Missing parameter: LoadLibraryEx Flags.\n");
                        PrintHelpMessage();
                        return 1;
                    }
                    i++;
                    dwFlags = StrToDw(argv[i]);
                    if (dwFlags == 0 && GetLastError() != ERROR_SUCCESS) {
                        wprintf(L"Incorrect flag parameter: %ws\nERROR: 0x%08x\n", argv[i], GetLastError());
                        return 1;
                    }
                    break;

                case L's':
                case L'S':
                    // If no more arguments or next argument starts with '-' or '/', then add the directory of the module
                    if (i + 1 >= argc || (argv[i+1][0] == L'-' || argv[i+1][0] == L'/')) {
                        // Add the directory of the module
                        if (bModuleSpecified) {
                            wchar_t szDir[MAX_PATH];
                            wcscpy_s(szDir, MAX_PATH, szModulePath);
                            wchar_t* pLastSlash = (wchar_t*)wcsrchr(szDir, L'\\');
                            if (pLastSlash == NULL) pLastSlash = (wchar_t*)wcsrchr(szDir, L'/');
                            if (pLastSlash != NULL) {
                                *pLastSlash = L'\0';
                                if (AddDllDirectory(szDir) == 0) {
                                    wprintf(L"Cannot add search path: %ws; Error Code: 0x%08x\n",
                                            szDir, GetLastError());
                                }
                            }
                        } else {
                            wprintf(L"Invalid option given: -s\n");
                            PrintHelpMessage();
                            return 1;
                        }
                    } else {
                        // Add each subsequent path as a DLL directory until we hit another option
                        i++;
                        while (i < argc && (argv[i][0] != L'-' && argv[i][0] != L'/')) {
                            if (AddDllDirectory(argv[i]) == 0) {
                                wprintf(L"Cannot add search path: %ws; Error Code: 0x%08x\n",
                                        argv[i], GetLastError());
                            }
                            i++;
                        }
                        i--; // step back so loop can process the next option
                    }
                    break;

                default:
                    wprintf(L"Invalid option given: %ws\n", arg);
                    PrintHelpMessage();
                    return 1;
            }
        } else {
            // Not an option; ignore (original code did not handle this)
        }
        i++;
    }

    if (!bModuleSpecified) {
        PrintHelpMessage();
        return 0;
    }

    // Process the module
    DWORD dwResult = ProcessModule(szModulePath, dwFlags);
    return (int)dwResult;
}

