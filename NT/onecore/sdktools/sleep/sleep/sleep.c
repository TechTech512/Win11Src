/*
 * sleep.c
 *
 * A flexible sleep utility supporting seconds, milliseconds, and memory-based
 * sleep until commit percentage drops below a threshold.
 */

#include <stdio.h>
#include <windows.h>

// ------------------------------------------------------------------
// Function prototypes
// ------------------------------------------------------------------
VOID DisplayUsage(char *ProgramName);
int AsciiToInteger(char *Number);
VOID MySleep(DWORD dwMilliseconds);
VOID SleepUntilCommitBelow(DWORD dwPercent);

// ------------------------------------------------------------------
// Display usage information
// ------------------------------------------------------------------
VOID DisplayUsage(char *ProgramName)
{
    printf("Usage:  %s      time-to-sleep-in-seconds\n", ProgramName);
    printf("        %s [-m] time-to-sleep-in-milliseconds\n", ProgramName);
    printf("        %s [-c] commited-memory ratio (1%%-100%%)\n", ProgramName);
}

// ------------------------------------------------------------------
// Convert ASCII string to integer, return -1 on non-numeric input
// ------------------------------------------------------------------
int AsciiToInteger(char *Number)
{
    int total = 0;

    while (*Number != '\0') {
        if (*Number >= '0' && *Number <= '9') {
            total = total * 10 + (*Number - '0');
            Number++;
        } else {
            return -1;
        }
    }
    return total;
}

// ------------------------------------------------------------------
// Sleep for the specified number of milliseconds.
// Uses a waitable timer for sleeps longer than 600,000 seconds (approx 6.9 days)
// to avoid overflow in Sleep().
// ------------------------------------------------------------------
VOID MySleep(DWORD dwMilliseconds)
{
    // Sleep() takes a DWORD (max ~49.7 days). If requested time is longer,
    // use a waitable timer which can handle arbitrary durations.
    if (dwMilliseconds > 600000000) {  // 600,000 seconds = 600,000,000 ms
        HANDLE hTimer = CreateWaitableTimerExW(
            NULL, NULL, 0, TIMER_ALL_ACCESS
        );
        if (!hTimer) {
            printf("ERROR: Unable to create wakeup timer\n");
            Sleep(dwMilliseconds);  // fallback
            return;
        }

        // Convert milliseconds to 100-nanosecond intervals
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -(LONGLONG)dwMilliseconds * 10000; // negative for relative time

        if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
            printf("ERROR: Unable to set wakeup timer\n");
            CloseHandle(hTimer);
            Sleep(dwMilliseconds);  // fallback
            return;
        }

        // Wait for the timer
        WaitForSingleObject(hTimer, INFINITE);
        CloseHandle(hTimer);
    } else {
        Sleep(dwMilliseconds);
    }
}

// ------------------------------------------------------------------
// Sleep until the system commit memory usage drops below the specified
// percentage. Checks every 10 ms.
// ------------------------------------------------------------------
VOID SleepUntilCommitBelow(DWORD dwPercent)
{
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);

    printf("Waiting for commit memory to drop below %lu%%...\n", dwPercent);

    while (1) {
        if (!GlobalMemoryStatusEx(&memStatus)) {
            printf("GlobalMemoryStatusEx failed\n");
            break;
        }

        // memStatus.dwMemoryLoad is the percentage of memory in use.
        // We want to wait until it is less than dwPercent.
        if (memStatus.dwMemoryLoad < dwPercent) {
            break;
        }

        // Sleep 10 ms before checking again
        Sleep(10);
    }
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl main(int argc, char *argv[])
{
    int timeValue = -1;
    char *programName = argv[0];

    // No arguments: show usage
    if (argc == 1) {
        DisplayUsage(programName);
        return 1;
    }

    // Two arguments: could be seconds, or -m/-c with a value
    if (argc == 2) {
        timeValue = AsciiToInteger(argv[1]);
        if (timeValue == -1) {
            DisplayUsage(programName);
            return 1;
        }
        // Sleep for the number of seconds (convert to milliseconds)
        MySleep((DWORD)timeValue * 1000);
        return 0;
    }

    // Three arguments: option and value
    if (argc == 3) {
        if (strcmp(argv[1], "-m") == 0) {
            timeValue = AsciiToInteger(argv[2]);
            if (timeValue == -1) {
                DisplayUsage(programName);
                return 1;
            }
            // Sleep for milliseconds
            MySleep((DWORD)timeValue);
            return 0;
        }

        if (strcmp(argv[1], "-c") == 0) {
            timeValue = AsciiToInteger(argv[2]);
            if (timeValue == -1 || timeValue < 1 || timeValue > 100) {
                printf("Error: percentage must be between 1 and 100\n");
                DisplayUsage(programName);
                return 1;
            }
            SleepUntilCommitBelow((DWORD)timeValue);
            return 0;
        }

        // Unknown option
        DisplayUsage(programName);
        return 1;
    }

    // Too many arguments
    DisplayUsage(programName);
    return 1;
}

