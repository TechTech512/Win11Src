/*
 * winlogon.cxx
 *
 * Minimal Winlogon replacement (mini winlogon) that sets up environment variables,
 * starts the user shell (explorer or custom command), and handles basic system initialization.
 */

#include <windows.h>
#include <shlobj.h>      // for SHGetFolderPathW
#include <userenv.h>     // for CreateEnvironmentBlock? Not needed
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

typedef enum _PROFILE_FOLDER_ID {
	FOLDER_USERS = 1,
	FOLDER_DEFAULT = 2,
	FOLDER_PUBLIC = 3,
	FOLDER_PROGRAM_DATA = 4,
	FOLDER_USER_PROFILE = 5,
	FOLDER_LOCAL_APPDATA = 6,
	FOLDER_ROAMING_APPDATA = 7,
	FOLDER_LOCAL_APPDATA_NO_APPCONTAINER_REDIRECT = 8
} PROFILE_FOLDER_ID, * PPROFILE_FOLDER_ID;

// ------------------------------------------------------------------
// Debug output macro (replaces DbgPrint)
// ------------------------------------------------------------------
extern "C" {
ULONG
__cdecl
DbgPrint (
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
    );
}

// ------------------------------------------------------------------
// Structure for environment variable mapping
// ------------------------------------------------------------------
struct EnvVarMapping {
    LPCWSTR szRegValue;   // Registry value name (e.g., "ProgramFilesDir")
    LPCWSTR szVariable;   // Environment variable name (e.g., "ProgramFiles")
};

// Global mapping table (from decompiled data)
EnvVarMapping g_EnvProgramFiles[] = {
    { L"ProgramFilesDir",  L"ProgramFiles" },
    { L"CommonFilesDir",   L"CommonProgramFiles" },
    { NULL, NULL } // sentinel
};

// ------------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------------
extern "C" {
    // The exact prototype from profapi.h / onecore.lib
    HRESULT WINAPI GetBasicProfileFolderPath(
        PROFILE_FOLDER_ID FolderId,
        PCWSTR pszSid,
        PWSTR pszPath,
        DWORD cchPath
    );
}
void SetProfilesLocation(void);
int SetupBasicEnvironment(void);
void* StartShell(void);
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

// ------------------------------------------------------------------
// SetProfilesLocation – set USERPROFILE, PUBLIC, ProgramData environment variables
// ------------------------------------------------------------------
void SetProfilesLocation(void)
{
    WCHAR buffer[MAX_PATH];
    int result;

    // USERPROFILE (folderId 5)
    result = GetBasicProfileFolderPath(FOLDER_USER_PROFILE, NULL, buffer, MAX_PATH);
    if (result == 0) {
        SetEnvironmentVariableW(L"USERPROFILE", buffer);
    }

    // PUBLIC (folderId 3 – but PUBLIC is not a CSIDL; use KNOWNFOLDERID)
    // We'll use SHGetKnownFolderPath for PUBLIC (FOLDERID_Public)
    // For simplicity, we'll use CSIDL_COMMON_DOCUMENTS? Actually PUBLIC is FOLDERID_Public.
    // We'll skip PUBLIC if not available.
    // Instead we'll use the original logic: GetBasicProfileFolderPath(3,...)
    result = GetBasicProfileFolderPath(FOLDER_PUBLIC, NULL, buffer, MAX_PATH);
    if (result == 0) {
        SetEnvironmentVariableW(L"PUBLIC", buffer);
    }

    // ProgramData (folderId 4)
    result = GetBasicProfileFolderPath(FOLDER_PROGRAM_DATA, NULL, buffer, MAX_PATH);
    if (result == 0) {
        SetEnvironmentVariableW(L"ProgramData", buffer);
        SetEnvironmentVariableW(L"ALLUSERSPROFILE", buffer);
    }
}

// ------------------------------------------------------------------
// SetupBasicEnvironment – set COMPUTERNAME and expand ProgramFiles/CommonProgramFiles
// ------------------------------------------------------------------
int SetupBasicEnvironment(void)
{
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName) / sizeof(WCHAR);

    // Get and set COMPUTERNAME
    if (GetComputerNameExW(ComputerNameNetBIOS, computerName, &size)) {
        SetEnvironmentVariableW(L"COMPUTERNAME", computerName);
    }

    // Open the registry key for environment variables
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows\\CurrentVersion",
                                0, KEY_QUERY_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        for (int i = 0; g_EnvProgramFiles[i].szRegValue != NULL; i++) {
            WCHAR regValue[2048];
            DWORD valueSize = sizeof(regValue);
            DWORD type = 0;
            result = RegQueryValueExW(hKey,
                                      g_EnvProgramFiles[i].szRegValue,
                                      NULL, &type,
                                      (LPBYTE)regValue,
                                      &valueSize);
            if (result == ERROR_SUCCESS) {
                // Expand environment strings (in case they contain %SYSTEMROOT% etc.)
                WCHAR expanded[2048];
                DWORD expandedSize = ExpandEnvironmentStringsW(regValue, expanded, sizeof(expanded)/sizeof(WCHAR));
                if (expandedSize > 0 && expandedSize < sizeof(expanded)/sizeof(WCHAR)) {
                    SetEnvironmentVariableW(g_EnvProgramFiles[i].szVariable, expanded);
                }
            }
        }
        RegCloseKey(hKey);
    }

    return 1;
}

// ------------------------------------------------------------------
// StartShell – launch the user shell (explorer.exe or custom command)
// ------------------------------------------------------------------
void* StartShell(void)
{
    HKEY hKey;
    WCHAR userInit[1024];
    DWORD size = sizeof(userInit);
    DWORD type;
    LONG result;

    // Open Winlogon key
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                           0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS) {
        DbgPrint((PCSTR)L"Failed to open Winlogon key.\n");
        return NULL;
    }

    result = RegQueryValueExW(hKey, L"USERINIT", NULL, &type, (LPBYTE)userInit, &size);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) {
        DbgPrint((PCSTR)L"Failed to read USERINIT.\n");
        return NULL;
    }

    // USERINIT is a comma-separated list of commands
    WCHAR* context = NULL;
    WCHAR* token = wcstok_s(userInit, L",", &context);
    while (token != NULL) {
        // Trim leading/trailing spaces
        while (*token == L' ') token++;
        WCHAR* end = token + wcslen(token) - 1;
        while (end > token && *end == L' ') end--;
        *(end + 1) = L'\0';

        if (wcslen(token) > 0) {
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi = {0};
            si.lpDesktop = L"Winsta0\\Default";

            BOOL created = CreateProcessW(NULL,          // No application name – use command line
                                         token,          // Command line (with arguments)
                                         NULL, NULL, FALSE,
                                         CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                                         NULL, NULL,
                                         &si, &pi);
            if (created) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                // Only the first successful command is used (original code breaks after first?)
                // The original loop iterates and starts each process, but then returns after all.
                // We'll continue to next token to start all commands.
            } else {
                DbgPrint((PCSTR)L"Failed to start '%s'\n", token);
            }
        }
        token = wcstok_s(NULL, L",", &context);
    }

    return NULL;  // No shell started? The original returns 0xfffffffe on failure? Actually it returns 0x0 on success? We'll return NULL to indicate "no shell running".
}

// ------------------------------------------------------------------
// WinMain – Entry point
// ------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DbgPrint((PCSTR)L"minlogon is started.\n");

    // Set process priority (optional)
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Get computer name and set environment variable
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName) / sizeof(WCHAR);
    if (GetComputerNameExW(ComputerNameNetBIOS, computerName, &size)) {
        SetEnvironmentVariableW(L"COMPUTERNAME", computerName);
    }

    // Set profile locations
    SetProfilesLocation();

    // Setup basic environment (ProgramFiles etc.)
    SetupBasicEnvironment();

    // Create a global event for shell termination (optional)
    HANDLE hShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!hShutdownEvent) {
        DbgPrint((PCSTR)L"CreateEvent failed: 0x%08x\n", GetLastError());
        return 1;
    }

    // Start the shell (USERINIT)
    StartShell();

    // Wait forever (or until system shuts down)
    WaitForSingleObject(hShutdownEvent, INFINITE);

    return 0;
}

