/*
 * IoTUpdateConfig.c
 *
 * Manages Windows Insider flight configuration for Windows 10 IoT.
 * Supports -set, -reset, -get, and -? options.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

// ------------------------------------------------------------------
// Registry key structures
// ------------------------------------------------------------------
typedef struct ProvisionRegKey {
    HKEY key;
    LPCWSTR lpSubKey;
    LPCWSTR lpValueName;
    DWORD dwType;
    LPVOID lpProvisionData;
    DWORD cbData;
} ProvisionRegKey;

// ------------------------------------------------------------------
// Ring to SSRK mapping entry (as per decompiled data)
// ------------------------------------------------------------------
typedef struct RingSSRKMapping {
    LPCWSTR lpSSRK;          // TestTarget value
    LPCWSTR lpCategoryID;    // GuidOfCategoryToScan
    DWORD dwStudyID;         // StudyId (numeric)
    LPCWSTR lpRingID;        // Ring name (e.g., "Canary", "MSIT", "WIF")
    DWORD dwUnused;          // unused (flight disable flag)
} RingSSRKMapping;

// ------------------------------------------------------------------
// Global registry key structures (as per decompiled data)
// ------------------------------------------------------------------
ProvisionRegKey s_categoryKey = {
    HKEY_LOCAL_MACHINE,
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceUpdate\\Agent\\Settings",
    L"GuidOfCategoryToScan",
    REG_SZ,
    NULL,
    0
};

ProvisionRegKey s_FlightDisableIdKey = {
    HKEY_LOCAL_MACHINE,
    L"SOFTWARE\\Microsoft\\WindowsSelfhost\\Applicability",
    L"ThresholdFlightsDisabled",
    REG_DWORD,
    NULL,
    0
};

ProvisionRegKey s_ringIdKey = {
    HKEY_LOCAL_MACHINE,
    L"SOFTWARE\\Microsoft\\WindowsSelfhost\\Applicability",
    L"Ring",
    REG_SZ,
    NULL,
    0
};

ProvisionRegKey s_studyIdKey = {
    HKEY_LOCAL_MACHINE,
    L"Software\\Policies\\Microsoft\\SQMClient\\WindowsPhone",
    L"StudyId",
    REG_DWORD,
    NULL,
    0
};

ProvisionRegKey s_SSRKKey = {
    HKEY_LOCAL_MACHINE,
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceUpdate\\Agent\\Protocol",
    L"TestTarget",
    REG_SZ,
    NULL,
    0
};

// ------------------------------------------------------------------
// Ring to SSRK mapping table – exact data from decompiled binary
// ------------------------------------------------------------------
RingSSRKMapping s_ringSSRKMappings[] = {
    // Canary ring
    { L"931E9127-67E2-4FF2-B3BC-3CBC802EBDF0",
      L"98D9C14A-31A5-480E-96A1-047BD4E12B88",
      0x5E6, L"Canary", 0 },
    // MSIT ring
    { L"8C923C83-3E2C-4522-8E51-27E963CA3F6A",
      L"98D9C14A-31A5-480E-96A1-047BD4E12B88",
      0x528, L"MSIT", 0 },
    // WIF ring
    { L"3D73E726-9CE9-4207-832E-2B973E5A10D5",
      L"98D9C14A-31A5-480E-96A1-047BD4E12B88",
      0x136, L"WIF", 0 },
    // Retail (default) – empty ring name and SSRK
    { L"", L"BD237E0E-611D-40B8-B4AC-712E230C1F37", 0, L"", 0 },
    // Sentinel
    { NULL, NULL, 0, NULL, 0 }
};

// ------------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------------
void AssignRing(LPCWSTR ring);
void DumpSSRK(void);
void DumpCategory(void);
void DumpStudyID(void);
void DumpRingID(void);
void DumpDisableFlightID(void);
void SetProvisioningRegValue(ProvisionRegKey* pKey);

// ------------------------------------------------------------------
// AssignRing – map ring name to SSRK/Category/Study ID and set category GUID
// ------------------------------------------------------------------
void AssignRing(LPCWSTR ring)
{
    if (ring == NULL) ring = L"";   // treat NULL as empty

    for (int i = 0; s_ringSSRKMappings[i].lpSSRK != NULL; i++) {
        const RingSSRKMapping* pMapping = &s_ringSSRKMappings[i];

        // Compare ring name with lpRingID (case-sensitive as in original)
        if (wcscmp(pMapping->lpRingID, ring) == 0) {
            wprintf(L"SSRK: %s\n", pMapping->lpSSRK);
            wprintf(L"Study ID: %d\n", pMapping->dwStudyID);
            wprintf(L"Category: %s\n", pMapping->lpCategoryID);

            if (pMapping->lpCategoryID != NULL) {
                wprintf(L"Setting Category ID: %s\n", pMapping->lpCategoryID);
                s_categoryKey.lpProvisionData = (LPVOID)pMapping->lpCategoryID;
                // cbData will be computed by SetProvisioningRegValue based on type
                SetProvisioningRegValue(&s_categoryKey);
            }
            return;
        }
    }

    wprintf(L"Failed to find a matching setting for ring %s\n", ring);
}

// ------------------------------------------------------------------
// Dump functions – read and display registry values
// ------------------------------------------------------------------
void DumpSSRK(void)
{
    wchar_t buffer[2048];
    DWORD dwSize = sizeof(buffer);
    DWORD dwType;
    LONG lResult = RegGetValueW(s_SSRKKey.key,
                                s_SSRKKey.lpSubKey,
                                s_SSRKKey.lpValueName,
                                RRF_RT_REG_SZ,
                                &dwType,
                                buffer,
                                &dwSize);

    if (lResult == ERROR_SUCCESS) {
        wprintf(L"\tGetting Provisioning Sub-Key: %s\n", s_SSRKKey.lpSubKey);
        wprintf(L"\tGetting Provisioning Value: %s\n", s_SSRKKey.lpValueName);
        wprintf(L"\tGetting Provisioning Type: %d\n", REG_SZ);
        wprintf(L"\tGetting Provisioning Data: %s\n", buffer);
        wprintf(L"\n");
    } else {
        wprintf(L"Failed to dump ring info.\n");
    }
}

void DumpCategory(void)
{
    wchar_t buffer[2048];
    DWORD dwSize = sizeof(buffer);
    DWORD dwType;
    LONG lResult = RegGetValueW(s_categoryKey.key,
                                s_categoryKey.lpSubKey,
                                s_categoryKey.lpValueName,
                                RRF_RT_REG_SZ,
                                &dwType,
                                buffer,
                                &dwSize);

    if (lResult == ERROR_SUCCESS) {
        wprintf(L"\tGetting Provisioning Sub-Key: %s\n", s_categoryKey.lpSubKey);
        wprintf(L"\tGetting Provisioning Value: %s\n", s_categoryKey.lpValueName);
        wprintf(L"\tGetting Provisioning Type: %d\n", REG_SZ);
        wprintf(L"\tGetting Provisioning Data: %s\n", buffer);
        wprintf(L"\n");
    } else {
        wprintf(L"Failed to dump category info.\n");
    }
}

void DumpStudyID(void)
{
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwType;
    LONG lResult = RegGetValueW(s_studyIdKey.key,
                                s_studyIdKey.lpSubKey,
                                s_studyIdKey.lpValueName,
                                RRF_RT_REG_DWORD,
                                &dwType,
                                &dwValue,
                                &dwSize);

    if (lResult == ERROR_SUCCESS) {
        wprintf(L"\tGetting Provisioning Sub-Key: %s\n", s_studyIdKey.lpSubKey);
        wprintf(L"\tGetting Provisioning Value: %s\n", s_studyIdKey.lpValueName);
        wprintf(L"\tGetting Provisioning Type: %d\n", REG_DWORD);
        wprintf(L"\tGetting Provisioning Data: %d\n", dwValue);
        wprintf(L"\n");
    } else {
        wprintf(L"Failed to dump study id info.\n");
    }
}

void DumpRingID(void)
{
    wchar_t buffer[2048];
    DWORD dwSize = sizeof(buffer);
    DWORD dwType;
    LONG lResult = RegGetValueW(s_ringIdKey.key,
                                s_ringIdKey.lpSubKey,
                                s_ringIdKey.lpValueName,
                                RRF_RT_REG_SZ,
                                &dwType,
                                buffer,
                                &dwSize);

    if (lResult == ERROR_SUCCESS) {
        wprintf(L"\tGetting Provisioning Sub-Key: %s\n", s_ringIdKey.lpSubKey);
        wprintf(L"\tGetting Provisioning Value: %s\n", s_ringIdKey.lpValueName);
        wprintf(L"\tGetting Provisioning Type: %d\n", REG_SZ);
        wprintf(L"\tGetting Provisioning Data: %s\n", buffer);
        wprintf(L"\n");
    } else {
        wprintf(L"Failed to dump category info.\n");
    }
}

void DumpDisableFlightID(void)
{
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwType;
    LONG lResult = RegGetValueW(s_FlightDisableIdKey.key,
                                s_FlightDisableIdKey.lpSubKey,
                                s_FlightDisableIdKey.lpValueName,
                                RRF_RT_REG_DWORD,
                                &dwType,
                                &dwValue,
                                &dwSize);

    if (lResult == ERROR_SUCCESS) {
        wprintf(L"\tGetting Provisioning Sub-Key: %s\n", s_FlightDisableIdKey.lpSubKey);
        wprintf(L"\tGetting Provisioning Value: %s\n", s_FlightDisableIdKey.lpValueName);
        wprintf(L"\tGetting Provisioning Type: %d\n", REG_DWORD);
        wprintf(L"\tGetting Provisioning Data: %d\n", dwValue);
        wprintf(L"\n");
    } else {
        wprintf(L"Failed to dump flightdisable id info.\n");
    }
}

// ------------------------------------------------------------------
// SetProvisioningRegValue – write a registry value with verbose output
// ------------------------------------------------------------------
void SetProvisioningRegValue(ProvisionRegKey* pKey)
{
    // If type is REG_SZ, cbData is wcslen + 1 * sizeof(wchar_t)
    if (pKey->dwType == REG_SZ && pKey->lpProvisionData != NULL) {
        LPCWSTR str = (LPCWSTR)pKey->lpProvisionData;
        pKey->cbData = (DWORD)(wcslen(str) + 1) * sizeof(wchar_t);
    } else if (pKey->dwType == REG_DWORD) {
        pKey->cbData = sizeof(DWORD);
    }

    LONG lResult = RegSetKeyValueW(pKey->key,
                                   pKey->lpSubKey,
                                   pKey->lpValueName,
                                   pKey->dwType,
                                   pKey->lpProvisionData,
                                   pKey->cbData);

    if (lResult != ERROR_SUCCESS) {
        wprintf(L"Failed to set %s\\%s with %d\n", pKey->lpSubKey, pKey->lpValueName, lResult);
        return;
    }

    wprintf(L"\tSetting Provisioning Sub-Key: %s\n", pKey->lpSubKey);
    wprintf(L"\tSetting Provisioning Value: %s\n", pKey->lpValueName);
    wprintf(L"\tSetting Provisioning Type: %d\n", pKey->dwType);

    if (pKey->dwType == REG_DWORD) {
        DWORD dwValue = *(DWORD*)pKey->lpProvisionData;
        wprintf(L"\tSetting Provisioning Data: %d\n", dwValue);
    } else if (pKey->dwType == REG_SZ) {
        wprintf(L"\tSetting Provisioning Data: %s\n", (LPCWSTR)pKey->lpProvisionData);
    }
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    if (argc == 1) {
        // No output (original behaves this way)
        return 0;
    }

    BOOL bSet = FALSE, bReset = FALSE, bGet = FALSE, bHelp = FALSE;

    for (int i = 1; i < argc; i++) {
        LPCWSTR arg = argv[i];

        if (wcscmp(arg, L"-set") == 0 || wcscmp(arg, L"/set") == 0) {
            bSet = TRUE;
        } else if (wcscmp(arg, L"-reset") == 0 || wcscmp(arg, L"/reset") == 0) {
            bReset = TRUE;
        } else if (wcscmp(arg, L"-get") == 0 || wcscmp(arg, L"/get") == 0) {
            bGet = TRUE;
        } else if (wcscmp(arg, L"-?") == 0 || wcscmp(arg, L"/?") == 0) {
            bHelp = TRUE;
        } else {
            // Unknown argument – ignore as per original
        }
    }

    if (bHelp) {
        wprintf(L"-set   : set flight configurations \n");
        wprintf(L"-reset : reset retail configurations \n");
        wprintf(L"-get   : display flight configurations \n");
        return 0;
    }

    if (bSet) {
        wprintf(L"Setting the flight configuration \n");

        // Read current ring from registry
        wchar_t ringBuffer[2048] = {0};
        DWORD dwSize = sizeof(ringBuffer);
        DWORD dwType;
        LONG lResult = RegGetValueW(s_ringIdKey.key,
                                    s_ringIdKey.lpSubKey,
                                    s_ringIdKey.lpValueName,
                                    RRF_RT_REG_SZ,
                                    &dwType,
                                    ringBuffer,
                                    &dwSize);

        if (lResult == ERROR_SUCCESS) {
            AssignRing(ringBuffer);
        } else {
            // If ring value missing, default to retail
            AssignRing(L"");
        }
        return 0;
    }

    if (bReset) {
        wprintf(L"Setting the retail configuration \n");
        AssignRing(L"");   // matches retail entry (empty ring name)
        return 0;
    }

    if (bGet) {
        wprintf(L"Getting the flighting/retail configuration \n");
        DumpSSRK();
        DumpCategory();
        DumpStudyID();
        DumpRingID();
        DumpDisableFlightID();
        return 0;
    }

    // If none matched, do nothing
    return 0;
}

