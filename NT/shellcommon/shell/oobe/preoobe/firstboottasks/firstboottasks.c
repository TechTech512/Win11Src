/*
 * firstboottasks.c
 *
 * Main entry point for FirstBootTasks.exe.
 * Checks if the system is state‑separated; if so, reads the FirstBootRan flag.
 * If the flag is not present, calls SetComputerNameTask, then removes or creates the flag.
 */
#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ntdll.lib")

// Forward declarations from other modules
NTSTATUS BasepGetValueFromReg(PCWSTR KeyPath, PCWSTR ValueName, PVOID Buffer, PULONG BufferLen);
NTSTATUS BaseRemoveMultiNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PCWSTR NameToRemove);
NTSTATUS SetComputerNameTask(void);

extern BOOLEAN RtlIsStateSeparationEnabled(VOID);

extern NTSTATUS RtlWriteRegistryValue(
    ULONG      RelativeTo,
    PCWSTR     Path,
    PCWSTR     ValueName,
    ULONG      ValueType,
    PVOID      ValueData,
    ULONG      ValueLength
);

#ifndef RTL_REGISTRY_ABSOLUTE
#define RTL_REGISTRY_ABSOLUTE 0
#endif

// ------------------------------------------------------------------
// wmain
// The entry point.
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    BOOL stateSeparation = RtlIsStateSeparationEnabled();
    NTSTATUS status = STATUS_SUCCESS;
    DWORD flagValue = 0;
    ULONG dataLen = sizeof(DWORD);

    if (stateSeparation) {
        // Check if FirstBootRan exists
        status = BasepGetValueFromReg(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName",
            L"FirstBootRan",
            &flagValue,
            &dataLen
        );
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            // Not found – we need to set the computer name
            SetComputerNameTask();

            // Write the flag
            flagValue = 1;
            dataLen = sizeof(DWORD);
            RtlWriteRegistryValue(
                RTL_REGISTRY_ABSOLUTE,
                L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName",
                L"FirstBootRan",
                REG_DWORD,
                &flagValue,
                dataLen
            );
        }
        // If status is success, the flag exists, do nothing.
    } else {
        // Not state‑separated: remove the entry from BootExecute
        SetComputerNameTask();
        BaseRemoveMultiNameFromReg(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager",
            L"BootExecute",
            L"FirstBootRan"
        );
    }

    return;
}
