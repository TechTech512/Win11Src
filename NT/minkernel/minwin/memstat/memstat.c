/*
 * memstat.c
 * 
 * A simple memory statistics utility that displays physical, commit, and virtual
 * memory usage information.
 */

#include <windows.h>
#include <stdio.h>

int __cdecl wmain(void)
{
    MEMORYSTATUSEX memStatus;

    // Initialize structure size
    memStatus.dwLength = sizeof(memStatus);

    // Query memory status
    if (!GlobalMemoryStatusEx(&memStatus)) {
        wprintf(L"Failed to get memory status.\n");
        return 1;
    }

    // Commit in use = total commit - available commit
    wprintf(L"%I64d %sbytes of commit in use.\n",
            (memStatus.ullTotalPageFile - memStatus.ullAvailPageFile) >> 20,
            L"M");

    // Memory load percentage
    wprintf(L"%ld percent of memory is in use.\n", memStatus.dwMemoryLoad);

    // Physical memory
    wprintf(L"There are %*I64d total      %sbytes of physical memory.\n",
            7, memStatus.ullTotalPhys >> 20, L"M");
    wprintf(L"There are %*I64d available  %sbytes of physical memory.\n",
            7, memStatus.ullAvailPhys >> 20, L"M");

    // Page file (commit) memory
    wprintf(L"There are %*I64d total      %sbytes of commit.\n",
            7, memStatus.ullTotalPageFile >> 20, L"M");
    wprintf(L"There are %*I64d available  %sbytes of commit.\n",
            7, memStatus.ullAvailPageFile >> 20, L"M");

    // Virtual memory
    wprintf(L"There are %*I64d total      %sbytes of virtual memory.\n",
            7, memStatus.ullTotalVirtual >> 20, L"M");
    wprintf(L"There are %*I64d available  %sbytes of virtual memory.\n",
            7, memStatus.ullAvailVirtual >> 20, L"M");

    return;
}

