/*
 * bootprepmain.c – exact translation of decompiled wmain
 */
#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "bootpreppriv.h"
#include "bootpreppath.h"

// ------------------------------------------------------------------
// Forward declarations from other files
// ------------------------------------------------------------------
LONG ExtendDataVolume(VOID);
LONG InitializeLoggingKey(VOID);
INT IsFirstBootDone(VOID);
INT IsOsMfgModeEnabled(VOID);
LONG LogResult(ULONG Function, LONG Status);
LONG ProvisionDataPartition(VOID);
LONG RemoveBootPrepLaunchKey(VOID);
LONG NativeCreateFile(PHANDLE phFile, PCWSTR pszPath, ULONG DesiredAccess, ULONG ShareAccess,
                      ULONG Disposition, ULONG Flags, ULONG Attributes);
LONG NativeDeleteFile(PCWSTR pszPath);
LONG SetPrivileges(PTOKEN_PRIVILEGES* ppPrevPrivileges);
LONG OpenVolumeHandle(HANDLE* phVolume, ULONG Flags, PVOID pContext);
NTSYSAPI NTSTATUS NTAPI NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock);
NTSYSAPI
_Success_(return != 0)
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RtlAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    );
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
    );
NTSYSAPI
ULONG
NTAPI
RtlGetProcessHeaps(
    _In_ ULONG NumberOfHeaps,
    _Out_ PVOID *ProcessHeaps
    );

// ------------------------------------------------------------------
// Global strings from decompiled binary
// ------------------------------------------------------------------
wchar_t BootDoneFilePath[27] = L"C:\\Data\\FirstBoot.Complete";
wchar_t CurrentSamPath[31] = L"C:\\Windows\\System32\\Config\\SAM";

// ------------------------------------------------------------------
// wmain – exact from decompiled
// ------------------------------------------------------------------
int __cdecl wmain(VOID)
{
    NTSTATUS status;
    HANDLE hFile = NULL;
    LONG result = 0;
    LONG tempResult;
    HANDLE hVolume = NULL;
    IO_STATUS_BLOCK iosb;

    // Check manufacturing mode – if enabled, create the boot done file and exit
    if (IsOsMfgModeEnabled()) {
        status = NativeCreateFile(&hFile, BootDoneFilePath,
                                  FILE_READ_DATA | FILE_WRITE_DATA,  // DesiredAccess (0x120089? actually 0x120089 = 0x120000 | 0x89? we'll use GENERIC_WRITE)
                                  0,                                 // ShareAccess
                                  FILE_OPEN,                        // Disposition
                                  0,                                // Flags
                                  FILE_ATTRIBUTE_NORMAL);           // Attributes
        if (NT_SUCCESS(status)) {
            NtClose(hFile);
            return 0;
        }
    }

    // Initialize logging
    InitializeLoggingKey();

    // Extend data volume
    status = ExtendDataVolume();
    if (!NT_SUCCESS(status)) {
        LogResult(1, status);
        result = status;
    }

    // Provision data partition
    status = ProvisionDataPartition();
    if (!NT_SUCCESS(status) && result >= 0) {
        LogResult(2, status);
        result = status;
    }

    // If first boot not done, delete SAM file (with privileges)
    if (!IsFirstBootDone()) {
        PTOKEN_PRIVILEGES pPrev = NULL;

        status = SetPrivileges(&pPrev);
        if (NT_SUCCESS(status)) {
            // Delete the SAM file (original used NativeDeleteFile on CurrentSamPath)
            tempResult = NativeDeleteFile(CurrentSamPath);
            // Restore privileges (original called SetPrivileges again with the previous buffer)
            SetPrivileges(&pPrev);  // This should restore or free the previous buffer
            if (!NT_SUCCESS(tempResult) && result >= 0) {
                LogResult(3, tempResult);
                result = tempResult;
            }
        } else {
            if (result >= 0) {
                LogResult(3, status);
                result = status;
            }
        }
        // Free the previous privileges buffer if it was allocated
        if (pPrev) {
            RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pPrev);
        }
    }

    // Create the boot done file (overwrite if exists)
    status = NativeCreateFile(&hFile, BootDoneFilePath,
                              FILE_READ_DATA | FILE_WRITE_DATA,
                              0,
                              FILE_OVERWRITE_IF,
                              0,
                              FILE_ATTRIBUTE_NORMAL);
    if (NT_SUCCESS(status)) {
        NtClose(hFile);
    } else if (result >= 0) {
        LogResult(4, status);
        result = status;
    }

    // Open volume, flush, and close
    status = OpenVolumeHandle(&hVolume, 0, NULL);
    if (NT_SUCCESS(status)) {
        NtFlushBuffersFile(hVolume, &iosb);
        NtClose(hVolume);
    } else if (result >= 0) {
        LogResult(5, status);
        result = status;
    }

    // Remove BootPrep launch key
    status = RemoveBootPrepLaunchKey();
    if (!NT_SUCCESS(status) && result >= 0) {
        LogResult(6, status);
        result = status;
    }

    // Log final success
    if (result >= 0)
        LogResult(7, STATUS_SUCCESS);

    return result;
}

