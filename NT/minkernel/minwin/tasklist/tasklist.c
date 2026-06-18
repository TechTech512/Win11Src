/*
 * mintasklist.c
 * 
 * A simple Windows task listing utility that displays running processes
 * with their image names, PIDs, and CPU time (N/A in this version).
 */

#pragma warning (disable:4005)
#pragma warning (disable:4090)
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "advapi32.lib")

// Process information classes
#ifndef SystemProcessInformation
#define SystemProcessInformation 5
#endif
#ifndef ProcessImageFileName
#define ProcessImageFileName 0x2b
#endif
#ifndef ProcessTimes
#define ProcessTimes 0x26
#endif

__declspec(dllimport) void* __stdcall RtlAllocateHeap(
    HANDLE HeapHandle,
    unsigned long Flags,
    unsigned long Size
);

__declspec(dllimport) int __stdcall RtlFreeHeap(
    HANDLE HeapHandle,
    unsigned long Flags,
    void* BaseAddress
);

// Forward declarations of internal functions
static int EnumProcesses(void* ProcessIds, unsigned int BufferSize, int* BytesReturned);
static int GetModuleBaseNameA(HANDLE ProcessHandle, char* NameBuffer, int* NameLength);

// Main entry point
void __cdecl wmain(void)
{
    unsigned int ProcessCount;
    int BytesReturned;
    int* ProcessIdArray;
    unsigned long ArraySize;
    HANDLE hProcess;
    int QueryStatus;
    unsigned long long KernelTime;
    unsigned long long UserTime;
    char ImageName[256];
    int NameLength;
    int Index;

    ArraySize = 1024;  // Initial array size

    while (1)
    {
        ProcessIdArray = (int*)RtlAllocateHeap(
            GetProcessHeap(),
            0,
            ArraySize * sizeof(int)
        );

        if (ProcessIdArray == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }

        if (EnumProcesses(ProcessIdArray, ArraySize, &BytesReturned))
        {
            ProcessCount = BytesReturned / sizeof(int);
            
            // If we got all processes (or no processes), break out
            if (ProcessCount < ArraySize || ProcessCount == 0)
            {
                // We have all processes, now display them
                wprintf(L"Image Name                   PID Services\n");
                wprintf(L"========================= ====== =============================================\n");

                for (Index = 0; (unsigned int)Index < ProcessCount; Index++)
                {
                    int ProcessId = ProcessIdArray[Index];
                    memset(ImageName, 0, sizeof(ImageName));

                    if (ProcessId == 0)
                    {
                        strcpy_s(ImageName, sizeof(ImageName), "System Idle Process");
                    }
                    else if (ProcessId == 4)
                    {
                        strcpy_s(ImageName, sizeof(ImageName), "System");
                    }
                    else
                    {
                        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
                        if (hProcess == NULL)
                        {
                            strcpy_s(ImageName, sizeof(ImageName), "unknown");
                        }
                        else
                        {
                            NameLength = sizeof(ImageName);
                            if (GetModuleBaseNameA(hProcess, ImageName, &NameLength) != STATUS_SUCCESS)
                            {
                                strcpy_s(ImageName, sizeof(ImageName), "unknown");
                            }

                            // Query CPU time information
                            KernelTime = 0;
                            UserTime = 0;
                            QueryStatus = NtQueryInformationProcess(
                                hProcess,
                                ProcessTimes,
                                &KernelTime,
                                sizeof(KernelTime),
                                NULL
                            );

                            CloseHandle(hProcess);
                        }
                    }

                    wprintf(L"%-25S %6d ", ImageName, ProcessId);
                    wprintf(L"N/A");
                    wprintf(L"\n");
                }

                RtlFreeHeap(GetProcessHeap(), 0, ProcessIdArray);
                break;  // Exit the loop
            }

            // If we didn't get all processes, free and try with larger buffer
            RtlFreeHeap(GetProcessHeap(), 0, ProcessIdArray);
            
            // Double the buffer size for next attempt
            ArraySize *= 2;
        }
        else
        {
            // EnumProcesses failed
            RtlFreeHeap(GetProcessHeap(), 0, ProcessIdArray);
            break;
        }
    }

cleanup:
    return;
}

// Enumerate processes by querying system information
static int EnumProcesses(void* ProcessIds, unsigned int BufferSize, int* BytesReturned)
{
    unsigned long BufferLength;
    void* SystemInformation;
    int Status;
    unsigned long CurrentOffset;
    unsigned long EntryCount;
    unsigned long MaxEntries;
    PSYSTEM_PROCESS_INFORMATION CurrentEntry;  // Use the proper structure type

    BufferLength = 0x8000;  // 32768 bytes

    while (1)
    {
        SystemInformation = LocalAlloc(LMEM_FIXED, BufferLength);
        if (SystemInformation == NULL)
        {
            return 0;
        }

        Status = NtQuerySystemInformation(
            SystemProcessInformation,
            SystemInformation,
            BufferLength,
            NULL
        );

        if (Status != STATUS_INFO_LENGTH_MISMATCH)
            break;

        LocalFree(SystemInformation);
        BufferLength += 0x8000;
    }

    if (Status != STATUS_SUCCESS)
    {
        LocalFree(SystemInformation);
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    MaxEntries = BufferSize / sizeof(int);
    EntryCount = 0;
    CurrentOffset = 0;

    // Better to use the proper structure
    CurrentEntry = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
    
    while (CurrentEntry != NULL && EntryCount < MaxEntries)
    {
        // Extract PID (offset 0x50 in SYSTEM_PROCESS_INFORMATION)
        *(int*)((char*)ProcessIds + EntryCount * sizeof(int)) = 
            (int)CurrentEntry->UniqueProcessId;
        EntryCount++;

        // Move to next entry
        if (CurrentEntry->NextEntryOffset == 0)
            break;
            
        CurrentEntry = (PSYSTEM_PROCESS_INFORMATION)((char*)CurrentEntry + CurrentEntry->NextEntryOffset);
    }

    *BytesReturned = EntryCount * sizeof(int);
    LocalFree(SystemInformation);
    return 1;
}

// Get the base module name of a process
static int GetModuleBaseNameA(HANDLE ProcessHandle, char* NameBuffer, int* NameLength)
{
    unsigned long BufferSize;
    UNICODE_STRING* UnicodeBuffer;
    ANSI_STRING* AnsiString;
    char* LastSlash;
    int Status;
    unsigned long ReturnLength;

    BufferSize = 0x1000;  // 4096 bytes - larger buffer
    
    // Allocate buffer for UNICODE_STRING plus data
    UnicodeBuffer = (UNICODE_STRING*)RtlAllocateHeap(
        GetProcessHeap(),
        0,
        sizeof(UNICODE_STRING) + BufferSize
    );

    if (UnicodeBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    // Set up the UNICODE_STRING buffer
    UnicodeBuffer->Length = 0;
    UnicodeBuffer->MaximumLength = (USHORT)BufferSize;
    UnicodeBuffer->Buffer = (PWSTR)((char*)UnicodeBuffer + sizeof(UNICODE_STRING));

    Status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessImageFileName,
        UnicodeBuffer,
        sizeof(UNICODE_STRING) + BufferSize,
        &ReturnLength
    );

    if (Status == STATUS_SUCCESS && UnicodeBuffer->Buffer != NULL && UnicodeBuffer->Length > 0)
    {
        // Convert Unicode to ANSI
        AnsiString = (ANSI_STRING*)RtlAllocateHeap(
            GetProcessHeap(),
            0,
            sizeof(ANSI_STRING) + 512  // Enough for filename
        );

        if (AnsiString != NULL)
        {
            AnsiString->Length = 0;
            AnsiString->MaximumLength = 512;
            AnsiString->Buffer = (PCHAR)((char*)AnsiString + sizeof(ANSI_STRING));

            Status = RtlUnicodeStringToAnsiString(AnsiString, UnicodeBuffer, FALSE);  // FALSE = don't allocate
            if (Status == STATUS_SUCCESS && AnsiString->Length > 0 && AnsiString->Buffer != NULL)
            {
                // Null-terminate the ANSI string
                if (AnsiString->Length < AnsiString->MaximumLength)
                {
                    AnsiString->Buffer[AnsiString->Length] = '\0';
                }
                else
                {
                    AnsiString->Buffer[AnsiString->MaximumLength - 1] = '\0';
                }

                // Find the last backslash
                LastSlash = strrchr(AnsiString->Buffer, '\\');
                if (LastSlash == NULL)
                {
                    LastSlash = strrchr(AnsiString->Buffer, '/');
                }

                if (LastSlash != NULL)
                {
                    // Copy everything after the last slash
                    LastSlash++;  // Move past the slash
                    strcpy_s(NameBuffer, *NameLength, LastSlash);
                    *NameLength = (int)strlen(NameBuffer);
                }
                else
                {
                    // No slash found, copy the whole string
                    strcpy_s(NameBuffer, *NameLength, AnsiString->Buffer);
                    *NameLength = (int)strlen(NameBuffer);
                }
            }
            else
            {
                // Conversion failed, try to extract from UnicodeBuffer directly
                int WideLen = UnicodeBuffer->Length / sizeof(WCHAR);
                if (WideLen > 0)
                {
                    // Find last backslash in Unicode string
                    WCHAR* WideLastSlash = NULL;
                    for (int i = WideLen - 1; i >= 0; i--)
                    {
                        if (UnicodeBuffer->Buffer[i] == L'\\' || UnicodeBuffer->Buffer[i] == L'/')
                        {
                            WideLastSlash = &UnicodeBuffer->Buffer[i + 1];
                            break;
                        }
                    }

                    if (WideLastSlash != NULL)
                    {
                        // Convert just the filename portion
                        int FilenameLen = (int)((UnicodeBuffer->Buffer + WideLen) - WideLastSlash);
                        int ConvertedLen = WideCharToMultiByte(
                            CP_ACP, 0, WideLastSlash, FilenameLen,
                            NameBuffer, *NameLength - 1, NULL, NULL
                        );
                        if (ConvertedLen > 0)
                        {
                            NameBuffer[ConvertedLen] = '\0';
                            *NameLength = ConvertedLen;
                        }
                    }
                }
            }

            RtlFreeHeap(GetProcessHeap(), 0, AnsiString);
        }
    }
    else
    {
        // Query failed, try alternative method for system processes
        Status = STATUS_UNSUCCESSFUL;
    }

    RtlFreeHeap(GetProcessHeap(), 0, UnicodeBuffer);

    // If we couldn't get the name, set to "unknown"
    if (Status != STATUS_SUCCESS || *NameLength <= 0)
    {
        strcpy_s(NameBuffer, *NameLength, "unknown");
        *NameLength = 7;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

