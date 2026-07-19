/*
 * ishhost.cpp - Input Service Host
 *
 * This program loads InputService.dll from the system folder,
 * initializes the service, and waits for shutdown events.
 * Reconstructed exactly from decompiled binary.
 */

#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <strsafe.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

// ------------------------------------------------------------------
// Global error variable (as in original)
// ------------------------------------------------------------------
DWORD g_dwLastError = 0;  // _DAT_00000000 in decompiled

// ------------------------------------------------------------------
// Function pointer types for the service
// ------------------------------------------------------------------
typedef void (WINAPI *PFN_InitializeService)(BOOL, LPCWSTR*);
typedef void (WINAPI *PFN_UninitializeService)(void);

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
    HRESULT hr;
    PWSTR pszSystemPath = NULL;
    WCHAR szDllPath[MAX_PATH];
    HMODULE hModule = NULL;
    PFN_InitializeService pfnInitialize = NULL;
    PFN_UninitializeService pfnUninitialize = NULL;
    HANDLE hMutex = NULL;
    HANDLE hShutdownEvent = NULL;
    HANDLE handles[2];
    DWORD dwWaitResult;
    WCHAR szEventName[40];
    DWORD dwProcessId;
    int result = 0;

    // Security descriptor variables (exact names from decompiled)
    PSECURITY_DESCRIPTOR pSelfRelativeSD = NULL;   // local_29c
    PSECURITY_DESCRIPTOR pAbsoluteSD = NULL;       // local_2e4
    DWORD dwAbsoluteSDSize = 0x14;                 // local_2b8 (20 bytes)
    DWORD dwDaclSize = 0x34;                       // local_2b4 (52 bytes)
    DWORD dwSaclSize = 8;                          // local_2b0 (8 bytes)
    DWORD dwOwnerSize = 0xc;                       // local_2ac (12 bytes)
    DWORD dwGroupSize = 0xc;                       // local_2a8 (12 bytes)
    BYTE absoluteSDBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH] = {0};
    BYTE daclBuffer[52] = {0};                     // local_70
    BYTE saclBuffer[8] = {0};                      // local_2ec
    BYTE ownerBuffer[12] = {0};                    // local_290
    BYTE groupBuffer[12] = {0};                    // local_284
    SECURITY_ATTRIBUTES sa;

    // Get the system folder path
    hr = SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &pszSystemPath);
    if (FAILED(hr)) {
        g_dwLastError = 0x23;
        goto cleanup;
    }

    // Build the full path to InputService.dll
    hr = StringCchPrintfW(szDllPath, ARRAYSIZE(szDllPath), L"%s\\InputService.dll", pszSystemPath);
    CoTaskMemFree(pszSystemPath);
    if (FAILED(hr)) {
        g_dwLastError = 0x29;
        goto cleanup;
    }

    // Load the DLL
    hModule = LoadLibraryW(szDllPath);
    if (!hModule) {
        g_dwLastError = 0x2e;
        goto cleanup;
    }

    // Get the function pointers
    pfnInitialize = (PFN_InitializeService)GetProcAddress(hModule, "InitializeService");
    pfnUninitialize = (PFN_UninitializeService)GetProcAddress(hModule, "UninitializeService");
    if (!pfnInitialize) {
        g_dwLastError = 0x31;
        goto cleanup;
    }
    if (!pfnUninitialize) {
        g_dwLastError = 0x34;
        goto cleanup;
    }

    // Convert an empty security descriptor string to a self-relative SD.
    // Original: ConvertStringSecurityDescriptorToSecurityDescriptorW(L"", 1, &local_29c, 0);
    BOOL bConvert = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"",  // empty SDDL string (may be invalid, but matches original)
        SDDL_REVISION_1,
        &pSelfRelativeSD,
        NULL
    );
    if (!bConvert) {
        g_dwLastError = 0x3e;
        goto cleanup;
    }

    // Now make the security descriptor absolute.
    // Original:
    //   iVar1 = MakeAbsoluteSD(local_29c, &local_2e4, &local_2b8, local_70, &local_2b4, local_2ec, &local_2b0, local_290, &local_2ac, local_284, &local_2a8);
    pAbsoluteSD = (PSECURITY_DESCRIPTOR)absoluteSDBuffer;
    BOOL bMakeAbs = MakeAbsoluteSD(
        pSelfRelativeSD,
        pAbsoluteSD,
        &dwAbsoluteSDSize,
        (PACL)daclBuffer,
        &dwDaclSize,
        (PACL)saclBuffer,
        &dwSaclSize,
        ownerBuffer,
        &dwOwnerSize,
        groupBuffer,
        &dwGroupSize
    );
    if (!bMakeAbs) {
        g_dwLastError = 0x55;
        LocalFree(pSelfRelativeSD);
        goto cleanup;
    }

    // Set a NULL DACL on the absolute security descriptor (allow everyone).
    // Original: SetSecurityDescriptorDacl(&local_2e4, 1, 0, 0);
    if (!SetSecurityDescriptorDacl(pAbsoluteSD, TRUE, NULL, FALSE)) {
        g_dwLastError = 0x5d;
        LocalFree(pSelfRelativeSD);
        goto cleanup;
    }

    // Create the mutex with the absolute security descriptor.
    // Original:
    //   local_2c4 = &local_2e4;  // pointer to absolute SD
    //   local_2c8 = 0xc;         // SECURITY_ATTRIBUTES.nLength
    //   CreateMutexW(&local_2c8, 0, L"Local\\InputServiceHostMutex");
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pAbsoluteSD;
    sa.bInheritHandle = FALSE;

    hMutex = CreateMutexW(&sa, FALSE, L"Local\\InputServiceHostMutex");
    if (!hMutex) {
        g_dwLastError = 0x67;
        LocalFree(pSelfRelativeSD);
        goto cleanup;
    }

    // Create the shutdown event (named with the process ID).
    // Original: GetCurrentProcessId(); StringCchPrintfW(local_3c, 0x1a, L"Local\\IshShutdown%08x");
    dwProcessId = GetCurrentProcessId();
    hr = StringCchPrintfW(szEventName, ARRAYSIZE(szEventName), L"Local\\IshShutdown%08x", dwProcessId);
    if (FAILED(hr)) {
        g_dwLastError = 0x6f;  // This error code is set when CreateEvent fails, but here we have a string failure.
        // Actually the original sets it after CreateEvent fails.
        // We'll set it here for consistency, but original might not.
        // We'll still go to cleanup.
        CloseHandle(hMutex);
        LocalFree(pSelfRelativeSD);
        goto cleanup;
    }
    hShutdownEvent = CreateEventW(&sa, FALSE, FALSE, szEventName);
    if (!hShutdownEvent) {
        g_dwLastError = 0x6f;
        CloseHandle(hMutex);
        LocalFree(pSelfRelativeSD);
        goto cleanup;
    }

    // Wait for either the mutex or the event.
    // Original:
    //   local_2d0 = local_294;  // hMutex
    //   local_2cc = iVar3;      // hShutdownEvent
    //   iVar2 = WaitForMultipleObjects(2, &local_2d0, 0, 0xffffffff);
    handles[0] = hMutex;
    handles[1] = hShutdownEvent;
    dwWaitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

    // If the event was signaled (WAIT_OBJECT_0 + 1), we skip initialization.
    if (dwWaitResult != WAIT_OBJECT_0 + 1) {
        // Initialize the service.
        // Original: ppwVar6 = &local_2bc; uVar5 = 1; (*local_2a0)(uVar5, ppwVar6);
        LPCWSTR pszServiceName = L"InputService";
        pfnInitialize(TRUE, &pszServiceName);

        // Wait for the shutdown event to be signaled.
        WaitForSingleObject(hShutdownEvent, INFINITE);

        // Uninitialize the service.
        pfnUninitialize();

        // Release the mutex (though not strictly necessary).
        ReleaseMutex(hMutex);
    }

    // Cleanup
    if (hShutdownEvent) CloseHandle(hShutdownEvent);
    if (hMutex) CloseHandle(hMutex);
    if (pSelfRelativeSD) LocalFree(pSelfRelativeSD);
    if (hModule) FreeLibrary(hModule);

    // Original returns 0 always.
    return 0;

cleanup:
    // Cleanup resources that may have been allocated.
    if (pSelfRelativeSD) LocalFree(pSelfRelativeSD);
    if (hModule) FreeLibrary(hModule);
    if (pszSystemPath) CoTaskMemFree(pszSystemPath);
    // Note: hMutex and hShutdownEvent are not created if we are in cleanup from early errors.
    return 0;
}

