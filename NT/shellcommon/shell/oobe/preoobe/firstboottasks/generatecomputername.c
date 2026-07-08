/*
 * generatecomputername.c
 *
 * Implements SetComputerNameTask, which reads the platform name, appends random characters,
 * and writes the new computer name to multiple registry locations using native NT API.
 */
#pragma warning (disable:4005)

#include <windows.h>          // DWORD, WINAPI, GetCurrentProcessId, etc.
#include <winternl.h>         // UNICODE_STRING, OBJECT_ATTRIBUTES, Nt* prototypes
#include <ntstatus.h>         // UNICODE_STRING, OBJECT_ATTRIBUTES, Nt* prototypes
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// Ensure NT_SUCCESS macro is defined (winternl.h usually has it, but just in case)
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Link with ntdll.lib for native API functions
#pragma comment(lib, "ntdll.lib")

// NtCreateKey – creates or opens a registry key
extern NTSTATUS NtCreateKey(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG TitleIndex,
    PUNICODE_STRING Class,
    ULONG CreateOptions,
    PULONG Disposition
);

// NtSetValueKey – sets a value for a registry key
extern NTSTATUS NtSetValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    ULONG TitleIndex,
    ULONG Type,
    PVOID Data,
    ULONG DataSize
);

// NtFlushKey – flushes a registry key’s data to disk
extern NTSTATUS NtFlushKey(HANDLE KeyHandle);

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 ) 

// RtlSetEnvironmentVar – sets an environment variable
NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVar(
    _Inout_opt_ PVOID *Environment,
    _In_reads_(NameLength) PCWSTR Name,
    _In_ SIZE_T NameLength,
    _In_reads_(ValueLength) PCWSTR Value,
    _In_opt_ SIZE_T ValueLength
    );

// Forward declarations from reghelper.c (implemented elsewhere)
NTSTATUS BasepGetNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PWSTR Buffer, PULONG BufferLen);
NTSTATUS GenerateRandomChars(ULONG Seed, PWSTR Buffer, SIZE_T BufferSize);

// ------------------------------------------------------------------
// SetComputerNameTask
// Sets the computer name using a random suffix appended to the platform name.
// ------------------------------------------------------------------
NTSTATUS SetComputerNameTask(void)
{
	UNICODE_STRING name;
	UNICODE_STRING value;
    WCHAR platformName[16];
    ULONG platformLen = sizeof(platformName) / sizeof(WCHAR);
	NTSTATUS status = RtlSetEnvironmentVar(NULL, L"VariableName", wcslen(L"VariableName"), L"VariableValue", wcslen(L"VariableValue"));
    PWSTR pAsterisk = NULL;

    // Read the platform name
    status = BasepGetNameFromReg(
        L"\\Registry\\Machine\\System\\Setup\\Platform",
        L"PlatformName",
        platformName,
        &platformLen
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Find the asterisk (*) placeholder
    pAsterisk = (PWSTR)wcschr(platformName, L'*');
    if (pAsterisk) {
        // Generate random suffix for the part after the asterisk
        SIZE_T suffixLen = (SIZE_T)(pAsterisk - platformName); // characters before '*'
        SIZE_T remaining = 16 - suffixLen - 1; // -1 for the null terminator, but we need to fill the rest
        PROCESS_BASIC_INFORMATION pbi; NtQueryInformationProcess(NtCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
		status = GenerateRandomChars((ULONG)pbi.UniqueProcessId, pAsterisk, remaining + 1);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    // Now write the new computer name to several registry keys using NT native API
    struct {
        PCWSTR KeyPath;
        PCWSTR ValueName;
    } regPaths[] = {
        { L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName", L"ComputerName" },
        { L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName", L"ComputerName" },
        { L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"Hostname" },
        { L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"NV Hostname" },
        { NULL, NULL }
    };

    for (int i = 0; regPaths[i].KeyPath != NULL; i++) {
        UNICODE_STRING KeyName, ValueNameU;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE hKey;
        NTSTATUS status2;

        RtlInitUnicodeString(&KeyName, regPaths[i].KeyPath);
        InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        status2 = NtCreateKey(&hKey, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, 0, NULL);
        if (NT_SUCCESS(status2)) {
            RtlInitUnicodeString(&ValueNameU, regPaths[i].ValueName);
            ULONG dataLen = (ULONG)(wcslen(platformName) + 1) * sizeof(WCHAR);
            status2 = NtSetValueKey(hKey, &ValueNameU, 0, REG_SZ, platformName, dataLen);
            if (NT_SUCCESS(status2)) {
                NtFlushKey(hKey);
            }
            NtClose(hKey);
        }
    }

    // Set the COMPUTERNAME environment variable (RtlSetEnvironmentVar is from ntdll)
    RtlInitUnicodeString(&name, L"COMPUTERNAME");
	RtlInitUnicodeString(&value, platformName); // platformName must be a wchar_t* (L"...")


    return STATUS_SUCCESS;
}

