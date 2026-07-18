/*
 * wlanstorageext.cpp
 *
 * Xbox WLAN Storage Override for Console
 * Faithful reconstruction of the original decompiled code.
 * Exports: GetCustomRegRootPath (ordinal 1), GetCustomStorageRootPath (ordinal 2),
 *          GetCustomWfdSettingsRegPath (ordinal 3)
 */

#include <windows.h>
#include <winreg.h>
#include <stdlib.h>
#include <stdio.h>

// ------------------------------------------------------------------
// PROFILE_FOLDER_ID enum (from profapi.h)
// ------------------------------------------------------------------
typedef enum _PROFILE_FOLDER_ID {
    FOLDER_USERS = 1,
    FOLDER_DEFAULT = 2,
    FOLDER_PUBLIC = 3,
    FOLDER_PROGRAM_DATA = 4,
    FOLDER_USER_PROFILE = 5,
    FOLDER_LOCAL_APPDATA = 6,
    FOLDER_ROAMING_APPDATA = 7,
    FOLDER_LOCAL_APPDATA_NO_APPCONTAINER_REDIRECT = 8
} PROFILE_FOLDER_ID;

// ------------------------------------------------------------------
// External function from profapi.dll / onecore.lib
// ------------------------------------------------------------------
extern "C" HRESULT WINAPI GetBasicProfileFolderPath(
    PROFILE_FOLDER_ID FolderId,
    PCWSTR pszSid,
    PWSTR pszPath,
    DWORD cchPath
);

// ------------------------------------------------------------------
// Internal function: reads RedirectedRegistryRoot, or falls back to copying default path
// ------------------------------------------------------------------
static HRESULT Internal_GetCustomRegPath(
    __out PWSTR pszPath,
    __in DWORD cchPath,
    __in PCWSTR pszRegSubKey
)
{
    if (!pszPath || cchPath == 0)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); // 0x80070057

    // Ensure output is null-terminated even if we fail later
    pszPath[0] = L'\0';

    DWORD cbData = cchPath * sizeof(WCHAR);
    LONG lResult = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        pszRegSubKey,
        L"RedirectedRegistryRoot",
        RRF_RT_REG_SZ,
        NULL,
        (PVOID)pszPath,
        &cbData
    );

    if (lResult == ERROR_SUCCESS) {
        // Success: we have the redirected path in pszPath
        return S_OK;
    }

    // Fallback: copy the registry subkey path itself into the buffer
    // (the original does this with a loop, but we can use wcscpy_s)
    if (wcslen(pszRegSubKey) >= cchPath) {
        // Not enough room; truncate and return 0x8007007a (ERROR_INSUFFICIENT_BUFFER)
        wcsncpy_s(pszPath, cchPath, pszRegSubKey, _TRUNCATE);
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER); // 0x8007007a
    } else {
        wcscpy_s(pszPath, cchPath, pszRegSubKey);
        return S_OK;
    }
}

// ------------------------------------------------------------------
// GetCustomRegRootPath – exported ordinal 1 (simply forwards to Internal_GetCustomRegPath)
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT WINAPI GetCustomRegRootPath(
    __out PWSTR pszPath,
    __in DWORD cchPath,
    __in PCWSTR pszRegSubKey
)
{
    return Internal_GetCustomRegPath(pszPath, cchPath, pszRegSubKey);
}

// ------------------------------------------------------------------
// GetCustomStorageRootPath – exported ordinal 2
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT WINAPI GetCustomStorageRootPath(
    __out PWSTR pszPath,
    __in DWORD cchPath,
    __in BOOL fUseAppData
)
{
    if (!pszPath || cchPath == 0)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    // Original expression: (-(fUseAppData != 0) & 2U) + 4
    // If fUseAppData = TRUE (1): -1 & 2 = 2, +4 = 6 → FOLDER_LOCAL_APPDATA
    // If fUseAppData = FALSE (0): 0 & 2 = 0, +4 = 4 → FOLDER_PROGRAM_DATA
    PROFILE_FOLDER_ID folderId = fUseAppData ? FOLDER_LOCAL_APPDATA : FOLDER_PROGRAM_DATA;

    // Call GetBasicProfileFolderPath with no SID (NULL)
    HRESULT hr = GetBasicProfileFolderPath(folderId, NULL, pszPath, cchPath);
    return hr;
}

// ------------------------------------------------------------------
// GetCustomWfdSettingsRegPath – exported ordinal 3 (forwards to Internal_GetCustomRegPath)
// ------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT WINAPI GetCustomWfdSettingsRegPath(
    __out PWSTR pszPath,
    __in DWORD cchPath,
    __in PCWSTR pszRegSubKey
)
{
    return Internal_GetCustomRegPath(pszPath, cchPath, pszRegSubKey);
}

// ------------------------------------------------------------------
// DllMain – entry point
// ------------------------------------------------------------------
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

