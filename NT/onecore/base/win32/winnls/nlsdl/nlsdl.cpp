#include <windows.h>
#include <winternl.h>

BOOL WINAPI IsSpecialLocaleId(LCID lcid);
BOOL WINAPI IsSupportedLocale(LCID lcid);

// Global variable
static HMODULE hNtDllMod = NULL;

// Function pointer type for RtlLocaleNameToLcid
typedef NTSTATUS (NTAPI *PRtlLocaleNameToLcid)(PCWSTR LocaleName, PLCID Lcid, ULONG Flags);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

ULONG WINAPI DownlevelGetParentLocaleLCID(LCID Locale)
{
    ULONG parentLcid = 0;
    NTSTATUS status;
    
    if (Locale == 0 || IsSpecialLocaleId(Locale) || !IsSupportedLocale(Locale))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (hNtDllMod == NULL)
    {
        hNtDllMod = GetModuleHandleW(L"Ntdll.dll");
    }
    
    PRtlLocaleNameToLcid pRtlLocaleNameToLcid = 
        (PRtlLocaleNameToLcid)GetProcAddress(hNtDllMod, "RtlLocaleNameToLcid");
    
    if (pRtlLocaleNameToLcid == NULL)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }
    
    WCHAR localeName[172];
    int result = GetLocaleInfoW(Locale, LOCALE_SPARENT, localeName, 85);
    
    if (result == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    status = pRtlLocaleNameToLcid(localeName, &parentLcid, 2);
    
    if (status < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    return parentLcid;
}

int WINAPI DownlevelGetParentLocaleName(LCID Locale, LPWSTR lpName, int cchName)
{
    if (Locale == 0 || (lpName == NULL && cchName < 1) || cchName < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (IsSpecialLocaleId(Locale) || !IsSupportedLocale(Locale))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    return GetLocaleInfoW(Locale, LOCALE_SPARENT, lpName, cchName);
}

int WINAPI DownlevelLCIDToLocaleName(LCID Locale, LPWSTR lpName, int cchName, DWORD dwFlags)
{
    if (Locale == 0 || (lpName == NULL && cchName < 1) || cchName < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (IsSpecialLocaleId(Locale))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (dwFlags != 0)
    {
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return 0;
    }
    
    if (!IsSupportedLocale(Locale))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    return GetLocaleInfoW(Locale, LOCALE_SNAME, lpName, cchName);
}

ULONG WINAPI DownlevelLocaleNameToLCID(LPCWSTR lpName, DWORD dwFlags)
{
    if (lpName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    if (dwFlags != 0)
    {
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return 0;
    }
    
    return LocaleNameToLCID(lpName, 0);
}

BOOL WINAPI IsSpecialLocaleId(LCID lcid)
{
    // Check for special locale IDs (0x400, 0x800, 0x1000, 0xC00, 0x1400)
    if (lcid != 0x400 && lcid != 0x800 && lcid != 0x1000 && 
        lcid != 0xC00 && lcid != 0x1400)
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL WINAPI IsSupportedLocale(LCID lcid)
{
    int result = GetLocaleInfoW(lcid, 0, NULL, 0);
    
    if (result < 1 || (lcid & 0xFFFF0000) != 0)
    {
        return FALSE;
    }
    
    return TRUE;
}

