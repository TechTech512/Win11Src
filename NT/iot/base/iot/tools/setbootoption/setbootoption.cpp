/*
 * setbootoption.c
 *
 * SetBootOption - Query or change the boot mode (headed/headless) for Windows.
 * Reads/writes the registry value at:
 *   HKLM\SYSTEM\CurrentControlSet\Control\WinInit
 *   Value: headless (REG_DWORD)
 *   0 = headed, 1 = headless
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

// Registry constants
#define REG_VALUE_NAME L"headless"

// Function prototypes
static void Usage(void);
static void ShowCurrentBootSetting(void);
static int IsImageConfiguredHeaded(void);
static void SetBootMode(int mode);

// ------------------------------------------------------------------
// Usage
// ------------------------------------------------------------------
static void Usage(void)
{
    wprintf(L"Usage: SetBootOption [headed | headless]\n");
    wprintf(L"Running the app without any options will show the current setting\n");
    wprintf(L"Note: Changing a setting will require a reboot\n");
    wprintf(L"\n");
}

// ------------------------------------------------------------------
// Show current configuration (headed or headless)
// ------------------------------------------------------------------
static void ShowCurrentBootSetting(void)
{
    wprintf(L"Current Configuration: ");
    if (IsImageConfiguredHeaded()) {
        wprintf(L"headed\n");
    } else {
        wprintf(L"headless\n");
    }
}

// ------------------------------------------------------------------
// Determine if the image is configured as headed (value 0)
// Returns: 1 if headed, 0 if headless (or missing key)
// ------------------------------------------------------------------
static int IsImageConfiguredHeaded(void)
{
    HKEY hKey;
    DWORD dwType, dwSize = sizeof(DWORD);
    DWORD dwValue = 0;
    LONG lResult;

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"system\\currentcontrolset\\control\\wininit", 0, KEY_READ, &hKey);
    if (lResult != ERROR_SUCCESS) {
        // If the key doesn't exist, default to headed.
        return 1;
    }

    lResult = RegQueryValueExW(hKey, REG_VALUE_NAME, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS) {
        // If the value doesn't exist, default to headed (0).
        return 1;
    }

    // Value 0 means headed, non-zero means headless.
    return (dwValue == 0);
}

// ------------------------------------------------------------------
// Set the boot mode
// mode: 0 = headed, 1 = headless
// ------------------------------------------------------------------
static void SetBootMode(int mode)
{
    HKEY hKey;
    DWORD dwValue = (mode == 0) ? 0 : 1;
    LONG lResult;

    // Check if the mode is already set correctly.
    int currentHeaded = IsImageConfiguredHeaded();
    int desiredHeaded = (mode == 0);  // headed if mode == 0

    if ((currentHeaded && desiredHeaded) || (!currentHeaded && !desiredHeaded)) {
        ShowCurrentBootSetting();
        wprintf(L"Boot mode not changed\n");
        return;
    }

    // Open the registry key for write access.
    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"system\\currentcontrolset\\control\\wininit", 0, KEY_SET_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        wprintf(L"Something went wrong, couldn't set boot mode\n");
        return;
    }

    lResult = RegSetValueExW(hKey, REG_VALUE_NAME, 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS) {
        wprintf(L"Something went wrong, couldn't set boot mode\n");
        return;
    }

    wprintf(L"Success - boot mode now set to ");
    if (mode == 0) {
        wprintf(L"headed");
    } else {
        wprintf(L"headless");
    }
    wprintf(L"\n");
    wprintf(L"Don't forget to reboot to get the new value\n");
    wprintf(L"Hint: 'shutdown /r /t 0\n");
    wprintf(L"\n");
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    wprintf(L"Set Boot Options\n");

    // No arguments: show usage and current setting.
    if (argc == 1) {
        Usage();
        ShowCurrentBootSetting();
        return 0;
    }

    // One argument: either "headed" or "headless" (case-insensitive)
    if (argc == 2) {
        wchar_t* arg = argv[1];

        if (_wcsicmp(arg, L"headless") == 0) {
            SetBootMode(1);   // headless = 1
            return 0;
        }
        else if (_wcsicmp(arg, L"headed") == 0) {
            SetBootMode(0);   // headed = 0
            return 0;
        }
        else {
            // Invalid option
            wprintf(L"Invalid option:\n");
            Usage();
            return 1;
        }
    }

    // Too many arguments
    wprintf(L"Invalid option:\n");
    Usage();
    return 1;
}

