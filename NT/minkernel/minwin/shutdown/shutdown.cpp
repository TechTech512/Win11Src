/*
 * minshutdown.c
 * 
 * A minimal shutdown utility supporting /s, /r, /p, /f, /t, /c, /sync.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

// Forward declarations
static void usage(void);
static int parse_options(int argc, wchar_t* argv[], int* pShutdown, int* pReboot, 
                         int* pSync, int* pForce, int* pTimeout, wchar_t** pComment);

int __cdecl wmain(int argc, wchar_t* argv[])
{
    int shutdown = 0;       // /s
    int reboot = 0;         // /r
    int sync = 0;           // /sync
    int force = 0;          // /f
    int timeout = 30;       // default timeout (seconds)
    wchar_t* comment = L"User initiated shutdown.";

    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Parse command line
    if (argc == 1) {
        usage();
        return 0;
    }

    if (!parse_options(argc, argv, &shutdown, &reboot, &sync, &force, &timeout, &comment)) {
        // If parse_options returned 0 due to /? it would have already called usage.
        // Otherwise, it's an error.
        // We'll just call usage again if no action was taken.
        if (!shutdown && !reboot && !sync && !force) {
            usage();
        }
        return 1;
    }

    // Must have either shutdown, reboot, or p (shutdown with timeout 0)
    if (!shutdown && !reboot && timeout == 0) {
        // /p case - we treat as shutdown
        shutdown = 1;
    }

    if (!shutdown && !reboot) {
        wprintf(L"Error: You must specify /s, /r, or /p.\n");
        usage();
        return 1;
    }

    // If timeout > 0, force is implied (as per help)
    if (timeout > 0) {
        force = 1;
    }

    // Acquire shutdown privilege
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        wprintf(L"OpenProcessToken failed with error %lu\n", GetLastError());
        return 1;
    }

    if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
        wprintf(L"LookupPrivilegeValue failed with error %lu\n", GetLastError());
        CloseHandle(hToken);
        return 1;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL)) {
        wprintf(L"AdjustTokenPrivileges failed with error %lu\n", GetLastError());
        CloseHandle(hToken);
        return 1;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        wprintf(L"The required privilege is not held by the process.\n");
        CloseHandle(hToken);
        return 1;
    }
    CloseHandle(hToken);

    // Display message
    if (timeout > 0) {
        wprintf(L"System will shutdown in %d seconds...\n", timeout);
    } else {
        wprintf(L"System will shutdown immediately.\n");
    }

    // Close standard input (optional)
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn != INVALID_HANDLE_VALUE) {
        CloseHandle(hStdIn);
    }

    // Initiate shutdown
    BOOL result = InitiateSystemShutdownExW(
        NULL,                   // local machine
        comment,                // message
        (DWORD)timeout,         // timeout
        (BOOL)force,            // force apps closed
        (BOOL)reboot,           // reboot after shutdown
        0                       // reason code
    );

    if (!result) {
        DWORD error = GetLastError();
        wprintf(L"InitiateSystemShutdownEx failed with error %lu\n", error);
        return (int)error;
    }

    // If /sync was specified, wait forever
    if (sync) {
        wprintf(L"Waiting forever...\n");
        Sleep(INFINITE);
    }

    return 0;
}

// Parse command line options
static int parse_options(int argc, wchar_t* argv[], int* pShutdown, int* pReboot, 
                         int* pSync, int* pForce, int* pTimeout, wchar_t** pComment)
{
    int i;
    int has_action = 0;

    for (i = 1; i < argc; i++) {
        wchar_t* arg = argv[i];

        // Check for option prefix
        if (arg[0] != L'/' && arg[0] != L'-') {
            wprintf(L"Invalid option: %s\n", arg);
            return 0;
        }

        wchar_t option = arg[1];
        if (option == L'?') {
            usage();
            return 0; // Indicate we handled help
        }

        // Handle options
        if (option == L's' || option == L'S') {
            // Check if it's /sync
            if (arg[2] == L'y' || arg[2] == L'Y') {
                // /sync
                if (arg[3] != L'n' && arg[3] != L'N') {
                    wprintf(L"Invalid option: %s\n", arg);
                    return 0;
                }
                if (arg[4] != L'c' && arg[4] != L'C') {
                    wprintf(L"Invalid option: %s\n", arg);
                    return 0;
                }
                if (arg[5] != L'\0') {
                    wprintf(L"Invalid option: %s\n", arg);
                    return 0;
                }
                *pSync = 1;
                has_action = 1;
            } else {
                // /s (shutdown)
                if (arg[2] != L'\0') {
                    wprintf(L"Invalid option: %s\n", arg);
                    return 0;
                }
                *pShutdown = 1;
                has_action = 1;
            }
        }
        else if (option == L'r' || option == L'R') {
            if (arg[2] != L'\0') {
                wprintf(L"Invalid option: %s\n", arg);
                return 0;
            }
            *pReboot = 1;
            has_action = 1;
        }
        else if (option == L'p' || option == L'P') {
            if (arg[2] != L'\0') {
                wprintf(L"Invalid option: %s\n", arg);
                return 0;
            }
            *pTimeout = 0;   // /p sets timeout to 0
            *pShutdown = 1;   // /p implies shutdown
            has_action = 1;
        }
        else if (option == L'f' || option == L'F') {
            if (arg[2] != L'\0') {
                wprintf(L"Invalid option: %s\n", arg);
                return 0;
            }
            *pForce = 1;
        }
        else if (option == L't' || option == L'T') {
            // /t requires a numeric argument
            if (arg[2] != L'\0') {
                wprintf(L"Invalid option: %s\n", arg);
                return 0;
            }
            if (i + 1 >= argc) {
                wprintf(L"Error: /t requires a timeout value.\n");
                return 0;
            }
            i++;
            wchar_t* ptr = argv[i];
            wchar_t* endptr;
            long val = wcstol(ptr, &endptr, 10);
            if (*endptr != L'\0' || val < 0 || val > 315360000) {
                wprintf(L"Error: Invalid timeout value. Must be between 0 and 315360000.\n");
                return 0;
            }
            *pTimeout = (int)val;
        }
        else if (option == L'c' || option == L'C') {
            // /c requires a comment argument
            if (arg[2] != L'\0') {
                wprintf(L"Invalid option: %s\n", arg);
                return 0;
            }
            if (i + 1 >= argc) {
                wprintf(L"Error: /c requires a comment string.\n");
                return 0;
            }
            i++;
            wchar_t* ptr = argv[i];
            if (wcslen(ptr) > 512) {
                wprintf(L"Error: Comment exceeds 512 characters.\n");
                return 0;
            }
            *pComment = ptr;
        }
        else {
            wprintf(L"Unknown option: %s\n", arg);
            return 0;
        }
    }

    // Check for conflicting options: /s and /r can't both be set
    if (*pShutdown && *pReboot) {
        wprintf(L"Error: /s and /r are mutually exclusive.\n");
        return 0;
    }

    // If no action specified, but we have /? we would have returned earlier.
    // But if no action at all (e.g., just /f), that's an error.
    if (!has_action) {
        wprintf(L"Error: No shutdown action specified.\n");
        return 0;
    }

    return 1;
}

// Display usage help
static void usage(void)
{
    wprintf(L"Usage: shutdown [/s | /r] [/f] [/t xxx] [/c \"comment\"] [/sync]\n\n");
    wprintf(L"No args      Display help. This is the same as typing /?.\n");
    wprintf(L"/?           Display help. This is the same as not typing any options.\n");
    wprintf(L"/s           Shutdown the computer.\n");
    wprintf(L"/r           Shutdown and restart the computer.\n");
    wprintf(L"/p           Turn off the local computer with no time-out or warning.\n");
    wprintf(L"             Can be used with /f options.\n");
    wprintf(L"/t xxx       Set the time-out period before shutdown to xxx seconds.\n");
    wprintf(L"             The valid range is 0-315360000 (10 years), with a default of 30.\n");
    wprintf(L"             If the timeout period is greater than 0, the /f parameter is\n");
    wprintf(L"             implied.\n");
    wprintf(L"/c \"comment\" Comment on the reason for the restart or shutdown.\n");
    wprintf(L"             Maximum of 512 characters allowed.\n");
    wprintf(L"/f           Force running applications to close without forewarning users.\n");
    wprintf(L"             The /f parameter is implied when a value greater than 0 is\n");
    wprintf(L"             specified for the /t parameter.\n");
    wprintf(L"/sync        Do not exit after successfully issuing shutdown request.\n");
}

