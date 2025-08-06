#pragma once

#include <windows.h>
#include <ntsecapi.h>
#include <strsafe.h>
#include <ntstatus.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

// Define basic types used in the original code
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned long DWORD;

// Structures matching decompiler output
typedef struct _TelemetryLevel {
    DWORD value;
} TelemetryLevel;

typedef struct _TelemetrySettingAuthority {
    DWORD value;
} TelemetrySettingAuthority;

// Forward declarations
uint TelIsTelemetryTypeAllowed(ulong param_1);
int TelpEvaluateWithoutLicenseCheck(TelemetryLevel* param_1, TelemetrySettingAuthority* param_2);
uint TelpReadRegistryDword(const wchar_t* keyPath, const wchar_t* valueName, wchar_t* param_3, wchar_t* param_4);
uint TelpReadUsersGroupPolicySetting(bool* param_1, TelemetryLevel* param_2);

// Constants from decompiled code
const wchar_t* DAT_1010cd78 = L"TelemetryAllowed";

// Helper macro for array size
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

// NTSTATUS values
#define STATUS_SUCCESS 0x00000000

// Declare NtQueryLicenseValue function
extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryLicenseValue(
    PCWSTR Name,
    ULONG Unknown,
    PVOID Data,
    ULONG DataSize,
    PULONG ResultDataSize
);

uint TelIsTelemetryTypeAllowed(ulong param_1) {
    uint result = 0;
    TelemetryLevel level = {0};
    TelemetrySettingAuthority authority = {0};
    uint evalResult = (uint)TelpEvaluateWithoutLicenseCheck(&level, &authority);
    
    if (evalResult != 0xFFFFFFFF && level.value == 0) {
        DWORD licenseValue = 0;
        DWORD licenseValueSize = sizeof(licenseValue);
        NTSTATUS status = NtQueryLicenseValue(
            DAT_1010cd78, 
            0, 
            &licenseValue, 
            sizeof(licenseValue), 
            &licenseValueSize);
        
        if (NT_SUCCESS(status) && licenseValue != 1) {
            level.value = 1;
        }
    }

    if (evalResult != 0xFFFFFFFF) {
        if (level.value != 0) {
            if (level.value != 1) {
                if (level.value != 2) {
                    result = 4;
                }
                result |= 2;
            }
            result |= 1;
        }
        evalResult = (result & param_1) != param_1;
    }

    return evalResult;
}

int TelpEvaluateWithoutLicenseCheck(TelemetryLevel* param_1, TelemetrySettingAuthority* param_2) {
    int retVal = -0x7ff8fffe; // E_NOT_SET
    bool local_5 = false;
    
    // First check Group Policy setting
    retVal = (int)TelpReadRegistryDword(
        L"Software\\Policies\\Microsoft\\Windows\\DataCollection",
        L"AllowTelemetry",
        nullptr,
        nullptr);

    TelpReadUsersGroupPolicySetting(&local_5, param_1);

    if (!local_5 && retVal == -0x7ff8fffe) {
        // Fall back to Policy Manager setting
        retVal = (int)TelpReadRegistryDword(
            L"Software\\Policies\\Microsoft\\Windows\\DataCollection",
            L"AllowTelemetry_PolicyManager",
            nullptr,
            nullptr);

        if (retVal == -0x7ff8fffe) {
            // Fall back to regular registry setting
            if (param_2) {
                param_2->value = 2; // Registry
            }
            param_1->value = 3; // Enhanced level
            
            retVal = (int)TelpReadRegistryDword(
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection",
                L"AllowTelemetry",
                nullptr,
                nullptr);

            if (retVal == -0x7ff8fffe || retVal == 0) {
                retVal = 0;
            }
        }
        else if (param_2) {
            param_2->value = 1; // PolicyManager
        }
    }
    else {
        param_1->value = 3; // Enhanced level
        retVal = 0;
        if (param_2) {
            param_2->value = 0; // GroupPolicy
        }
    }

    return retVal;
}

uint TelpReadRegistryDword(const wchar_t* keyPath, const wchar_t* valueName, wchar_t* param_3, wchar_t* param_4) {
    DWORD value = 0;
    DWORD dataSize = sizeof(value);
    LONG result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        keyPath,
        valueName,
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &dataSize);

    if (result != ERROR_SUCCESS) {
        return result | 0x80070000;
    }
    return value;
}

uint TelpReadUsersGroupPolicySetting(bool* param_1, TelemetryLevel* param_2) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection\\Users",
        0,
        KEY_READ,
        &hKey);

    if (result != ERROR_SUCCESS) {
        *param_1 = false;
        return result | 0x80070000;
    }

    DWORD subkeyCount = 0;
    DWORD maxSubkeyLen = 0;
    result = RegQueryInfoKeyW(
        hKey,
        nullptr,
        nullptr,
        nullptr,
        &subkeyCount,
        &maxSubkeyLen,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (result != ERROR_SUCCESS || subkeyCount == 0) {
        *param_1 = false;
        RegCloseKey(hKey);
        return result;
    }

    wchar_t* subkeyName = new wchar_t[maxSubkeyLen + 1];
    TelemetryLevel minLevel = {3}; // Start with Enhanced level
    *param_1 = false;

    for (DWORD i = 0; i < subkeyCount; i++) {
        DWORD nameSize = maxSubkeyLen + 1;
        result = RegEnumKeyExW(
            hKey,
            i,
            subkeyName,
            &nameSize,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        if (result == ERROR_SUCCESS) {
            wchar_t fullKeyPath[512];
            if (SUCCEEDED(StringCchPrintfW(
                fullKeyPath, 
                _countof(fullKeyPath),
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection\\Users\\%s",
                subkeyName))) {

                DWORD telemetryValue = (DWORD)TelpReadRegistryDword(
                    fullKeyPath,
                    L"AllowTelemetry",
                    nullptr,
                    nullptr);
                
                if (telemetryValue != 0xFFFFFFFF) {
                    *param_1 = true;
                    if (telemetryValue < minLevel.value) {
                        minLevel.value = telemetryValue;
                    }
                }
            }
        }
    }

    *param_2 = minLevel;
    delete[] subkeyName;
    RegCloseKey(hKey);
    return 0;
}

