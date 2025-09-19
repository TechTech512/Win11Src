#include "stdafx.h"

#pragma warning (disable:4302)

ULONG __cdecl RegUpdateLegacyKeyFromMUIKey(HKEY__* param_1, wchar_t* param_2, HKEY__* param_3, wchar_t* param_4, wchar_t* param_5, ULONG param_6);

ULONG __cdecl OnMachineUILanguageSwitch(wchar_t* param_1, wchar_t* param_2, ULONG param_3)
{
    DWORD dwNumLanguages = 0;
    wchar_t* pszLanguages = nullptr;
    BOOL bZeroLanguages = FALSE;
    ULONG ulResult = ERROR_SUCCESS;
    HKEY hTimeZonesKey = nullptr;
    HKEY hSubKey = nullptr;
    wchar_t szSubKeyName[256];
    DWORD dwIndex = 0;

    // Get current thread UI languages
    if (!GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &dwNumLanguages, nullptr, &dwNumLanguages))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pszLanguages = new wchar_t[dwNumLanguages];
            if (pszLanguages)
            {
                if (!GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, &dwNumLanguages, pszLanguages, &dwNumLanguages))
                {
                    delete[] pszLanguages;
                    pszLanguages = nullptr;
                }
            }
        }
    }
    else
    {
        bZeroLanguages = TRUE;
    }

    // Set new thread UI language
    DWORD dwNumLangSet = 1;
    SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, param_1, &dwNumLangSet);

    // Open Time Zones registry key
    ulResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
        0,
        KEY_READ,
        &hTimeZonesKey);

    if (ulResult == ERROR_SUCCESS)
    {
        // Enumerate all time zones
        while (RegEnumKeyW(hTimeZonesKey, dwIndex, szSubKeyName, sizeof(szSubKeyName)/sizeof(wchar_t)) == ERROR_SUCCESS)
        {
            // Open each time zone subkey
            ulResult = RegOpenKeyExW(hTimeZonesKey, szSubKeyName, 0, KEY_READ | KEY_WRITE, &hSubKey);
            if (ulResult == ERROR_SUCCESS)
            {
                // Update legacy keys from MUI keys
                RegUpdateLegacyKeyFromMUIKey(hSubKey, L"MUI_Display", nullptr, nullptr, nullptr, 0);
                RegUpdateLegacyKeyFromMUIKey(hSubKey, L"MUI_Dlt", nullptr, nullptr, nullptr, 0);
                RegUpdateLegacyKeyFromMUIKey(hSubKey, L"MUI_Std", nullptr, nullptr, nullptr, 0);
                
                RegCloseKey(hSubKey);
                hSubKey = nullptr;
            }
            dwIndex++;
        }
        RegCloseKey(hTimeZonesKey);
    }

    // Restore original thread UI languages
    if (pszLanguages)
    {
        SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, pszLanguages, &dwNumLanguages);
        delete[] pszLanguages;
    }
    else if (bZeroLanguages)
    {
        SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, nullptr, nullptr);
    }

    return ulResult;
}

ULONG __cdecl RegUpdateLegacyKeyFromMUIKey(HKEY__* param_1, wchar_t* param_2, HKEY__* param_3, wchar_t* param_4, wchar_t* param_5, ULONG param_6)
{
    if (!param_1 || !param_2)
        return ERROR_INVALID_PARAMETER;

    wchar_t szBuffer[256];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG ulResult;

    // Load MUI string from registry
    ulResult = RegLoadMUIStringW(param_1, param_2, szBuffer, dwBufferSize, nullptr, 0, nullptr);
    if (ulResult == ERROR_SUCCESS)
    {
        // Set the legacy registry value
        ulResult = RegSetValueExW(param_1, param_2, 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer) + 1) * sizeof(wchar_t));
    }

    return ulResult;
}

