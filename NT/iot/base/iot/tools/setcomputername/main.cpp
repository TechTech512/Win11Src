#pragma warning(disable:4530)
#include <windows.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 24
#define INVALID_CHARS L"/\\[]:|< >+=;,?"

static bool ContainsInvalidChars(const wchar_t* name)
{
    for (const wchar_t* p = name; *p; ++p)
        for (const wchar_t* inv = INVALID_CHARS; *inv; ++inv)
            if (*p == *inv) return true;
    return false;
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
    if (argc != 2)
    {
        wprintf(L"Usage: SetComputerName <Name>\n");
        return 0;
    }

    const wchar_t* newName = argv[1];

    if (wcslen(newName) >= MAX_NAME_LENGTH)
    {
        wprintf(L"Computer names should be less than 24 characters in length\n");
        return 1;
    }

    if (ContainsInvalidChars(newName))
    {
        wprintf(L"Your new name '%s' contains one or more invalid characters\n", newName);
        wprintf(L"Check your new machine name for the following characters \"%s\"\n", INVALID_CHARS);
        return 1;
    }

    // Get current computer name via GetComputerNameEx (updated immediately)
    wchar_t oldName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameExW(ComputerNamePhysicalDnsHostname, oldName, &size))
    {
        DWORD err = GetLastError();
        wprintf(L"Could not get the computer name, error # %d\n", err);
        return 1;
    }

    // Compare case-insensitively
    if (lstrcmpiW(oldName, newName) == 0)
    {
        wprintf(L"Your computer is already named that\n");
        return 0;
    }

    // Set the computer name using ComputerNamePhysicalDnsHostname (changes both DNS and NetBIOS)
    if (!SetComputerNameExW(ComputerNamePhysicalDnsHostname, newName))
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            wprintf(L"Error: Access denied. Please run this program as administrator.\n");
        else
            wprintf(L"Could not set the computer name, error # %d\n", err);
        return 1;
    }

    // Also update Hostname in Tcpip\Parameters for compatibility
    HKEY hKey;
    LONG regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                                   0, KEY_SET_VALUE, &hKey);
    if (regResult != ERROR_SUCCESS)
    {
        wprintf(L"Failed to open Registry - Error %d\n", regResult);
        return 1;
    }

    DWORD dataSize = (DWORD)((wcslen(newName) + 1) * sizeof(wchar_t));
    regResult = RegSetValueExW(hKey, L"Hostname", 0, REG_SZ,
                               (const BYTE*)newName, dataSize);
    RegCloseKey(hKey);

    if (regResult != ERROR_SUCCESS)
    {
        wprintf(L"Failed to open Registry - Error %d\n", regResult);
        return 1;
    }

    // Success
    wprintf(L"Success! - now reboot using 'shutdown /r /t 0'\n");
    wprintf(L"Old Name: %s\n", oldName);
    wprintf(L"New Name: %s\n", newName);

    return 0;
}

