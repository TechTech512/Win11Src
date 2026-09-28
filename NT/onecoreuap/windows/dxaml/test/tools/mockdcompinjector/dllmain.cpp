// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma warning (disable:4530)

#include <windows.h>
#include <functional>
#include <fstream>
#include <activation.h>
#include <assert.h>
#include <mindebug.h>

// This provides a mechanism to detour DComp to load mockdcomp.dll while loading Microsoft.UI.Xaml.dll.
// It does this via a script that redirects the XAML activation from HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Microsoft.UI.Xaml.Applicaiton
// It stores the original DllPath in a registry key HKEY_LOCAL_MACHINE\[XAML_ROOT_KEY]\MockDCompInjector which is queried and passes through the
// DllGetActivationFactory calls to instantiate the proper object from the original DLL's.
// However, the main injection functionality is done when the DLL is loaded via DllMain which essentially loads Mock10, dcomp.dll and calls to detours the DComp device
// creation so that when the real XAML is loaded afterwards, it will use the mock device instead of the real device.

// Validation macros
#define VERIFY_HR(expr) { HRESULT hr = (expr); if (FAILED(hr)) { return false; } }
#define VERIFY_BOOL(expr) { if (!(expr)) { return false; } }
#define VERIFY_NOTNULL(expr) { if ((expr) == NULL) { return false; } }
#define VERIFY_LONGERROR(expr) { if ((expr) != ERROR_SUCCESS) { return false; } }

// Function typedefs
typedef HRESULT (WINAPI* PfnDllGetActivationFactory)(HSTRING, IActivationFactory**);
typedef HRESULT (*PfnStartDetourMockDCompDevice)();
typedef HRESULT (*PfnStopDetourMockDCompDevice)();

// Global State Variables
bool g_shouldInject = true;
bool g_isInjected = false;
HMODULE g_originalXamlHandle = NULL;
PfnDllGetActivationFactory g_originalDllGetActivationFactory = NULL;

HMODULE g_mockDCompHandle = NULL;
PfnStartDetourMockDCompDevice g_startDetourMockDCompDevice = NULL;
PfnStopDetourMockDCompDevice g_stopDetourMockDCompDevice = NULL;

HMODULE g_dcompHandle = NULL;
HMODULE g_mock10Handle = NULL;

// Helper functions
bool ShouldInject()
{
    // Check if we should load private DLLs, both exe's and dll's can cause exclusion.
    // STL60 has no wide-character stream support, so we read the file as narrow chars
    // and use GetModuleHandleA for the lookup.
    std::ifstream excludedModuleList("C:\\Windows\\System32\\MockDCompInjector\\excluded.txt");

    if (excludedModuleList.is_open())
    {
        char excludedModuleFileName[MAX_PATH];
        while (excludedModuleList.getline(excludedModuleFileName, sizeof(excludedModuleFileName)))
        {
            if (GetModuleHandleA(excludedModuleFileName) != NULL)
            {
                return false;
            }
        }

        excludedModuleList.close();
    }

    return true;
}

bool HookOriginalXaml()
{
    wchar_t origXamlDllPath[MAX_PATH];
    DWORD origXamlDllPathSize = MAX_PATH;
    DWORD origXamlDllPathType = 0;

    // Return may be ERROR_MORE_DATA but it is a path and shouldn't exceed MAX_PATH length.
    if (ERROR_SUCCESS == RegGetValueW(
        HKEY_LOCAL_MACHINE,
        XAML_ROOT_KEY L"\\MockDCompInjector",
        L"DllPath",
        RRF_RT_REG_SZ,
        &origXamlDllPathType,
        origXamlDllPath,
        &origXamlDllPathSize))
    {
        // Load the original XAML and ensure we have activation factory redirection setup.
        VERIFY_NOTNULL(g_originalXamlHandle = LoadLibraryExW(origXamlDllPath, NULL, 0));
    }
    else
    {
        // Error, so load from the system32 location
        VERIFY_NOTNULL(g_originalXamlHandle = LoadLibraryExW(L"C:\\Windows\\System32\\Microsoft.UI.Xaml.dll", NULL, 0));
    }

    VERIFY_NOTNULL(g_originalDllGetActivationFactory = reinterpret_cast<PfnDllGetActivationFactory>(GetProcAddress(g_originalXamlHandle, "DllGetActivationFactory")));

    return true;
}

bool InjectMockDComp()
{
    // Load the dll's required for MockDComp injection
    VERIFY_NOTNULL(g_dcompHandle = LoadLibraryExW(L"C:\\Windows\\System32\\dcomp.dll", NULL, 0));
    VERIFY_NOTNULL(g_mock10Handle = LoadLibraryExW(L"C:\\Windows\\System32\\MockDCompInjector\\mock10.dll", NULL, 0));

    VERIFY_NOTNULL(g_mockDCompHandle = LoadLibraryExW(L"C:\\Windows\\System32\\MockDCompInjector\\MockDComp.dll", NULL, 0));
    VERIFY_NOTNULL(g_startDetourMockDCompDevice = reinterpret_cast<PfnStartDetourMockDCompDevice>(GetProcAddress(g_mockDCompHandle, "StartDetourMockDCompDevice")));
    VERIFY_NOTNULL(g_stopDetourMockDCompDevice = reinterpret_cast<PfnStopDetourMockDCompDevice>(GetProcAddress(g_mockDCompHandle, "StopDetourMockDCompDevice")));

    VERIFY_HR(g_startDetourMockDCompDevice());

    return true;
}

bool Initialize()
{
    VERIFY_BOOL(HookOriginalXaml());

    if (ShouldInject())
    {
        g_isInjected = InjectMockDComp();
        VERIFY_BOOL(g_isInjected);
    }

    return true;
}

bool Deinitialize()
{
    bool result = true;

    if (g_isInjected &&
        (g_stopDetourMockDCompDevice != NULL))
    {
        VERIFY_HR(g_stopDetourMockDCompDevice());
    }

    if (g_originalXamlHandle != NULL)
    {
        FreeLibrary(g_originalXamlHandle);
        g_originalXamlHandle = NULL;
    }

    if (g_mock10Handle != NULL)
    {
        FreeLibrary(g_mock10Handle);
        g_mock10Handle = NULL;
    }

    if (g_dcompHandle != NULL)
    {
        FreeLibrary(g_dcompHandle);
        g_dcompHandle = NULL;
    }

    return result;
}

// Dll Exported Functions
BOOL APIENTRY DllMain(HMODULE, DWORD ul_reason_for_call, LPVOID)
{
    BOOL result = TRUE;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        result = Initialize() ? TRUE : FALSE;
        break;
    case DLL_PROCESS_DETACH:
        result = Deinitialize() ? TRUE : FALSE;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    assert(result);
    return TRUE;
}

// Passthrough function to allow activation from the original DLL to proceed normally (This and DllMain provide the contract
// expected from the original DLL.
extern "C" HRESULT WINAPI DllGetActivationFactory(_In_ HSTRING hstrAcid, _Outptr_ IActivationFactory** factory)
{
    return g_originalDllGetActivationFactory(hstrAcid, factory);
}
