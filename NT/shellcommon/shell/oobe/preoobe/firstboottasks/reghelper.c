/*
 * reghelper.c
 *
 * Helper functions for registry operations used by FirstBootTasks.exe.
 * These functions wrap native NT registry APIs.
 */
#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ntdll.lib")

// Status codes
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif
#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif
#ifndef STATUS_OBJECT_NAME_NOT_FOUND
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034L)
#endif
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY ((NTSTATUS)0xC0000017L)
#endif
#ifndef STATUS_NOT_FOUND
#define STATUS_NOT_FOUND ((NTSTATUS)0xC0000225L)
#endif

// Error codes used in the original decompilation
#define STATUS_BUFFER_OVERFLOW ((NTSTATUS)0x80000005L)  // actually STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW_ALT ((NTSTATUS)0x80000005L)

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1]; // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
    MaxKeyValueInfoClass  // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

// NtOpenKey – opens an existing registry key
extern NTSTATUS NtOpenKey(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

// NtQueryValueKey – retrieves data for a value under a registry key
extern NTSTATUS NtQueryValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, // usually KeyValueFullInformation (1)
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
);

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

// RtlGetProcessHeap – returns the handle to the default process heap
NTSYSAPI
ULONG
NTAPI
RtlGetProcessHeaps(
    _In_ ULONG NumberOfHeaps,
    _Out_ PVOID *ProcessHeaps
    );

// RtlAllocateHeap – allocates memory from a heap
extern PVOID RtlAllocateHeap(
    HANDLE HeapHandle,
    ULONG Flags,
    SIZE_T Size
);

// RtlFreeHeap – frees memory allocated from a heap
extern BOOLEAN RtlFreeHeap(
    HANDLE HeapHandle,
    ULONG Flags,
    PVOID BaseAddress
);

#if !defined(NTKERNELAPI)

#if defined(_NTHALLIB_)

#define NTKERNELAPI

#else

#define NTKERNELAPI DECLSPEC_IMPORT     // wdm ntndis ntifs

#endif

#endif

typedef _Enum_is_bitflag_ enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType,

    //
    // Define base types for NonPaged (versus Paged) pool, for use in cracking
    // the underlying pool type.
    //

    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,

    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,

} _Enum_is_bitflag_ POOL_TYPE;

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag (
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    );

// Forward declarations of helper functions
NTSTATUS BasepGetMultiValueAddr(PWSTR MultiString, ULONG Index, PWSTR* OutAddr, PULONG OutTotalLen);
NTSTATUS BasepGetNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PWSTR Buffer, PULONG BufferLen);
NTSTATUS BasepGetValueFromReg(PCWSTR KeyPath, PCWSTR ValueName, PVOID Buffer, PULONG BufferLen);
NTSTATUS BaseRemoveMultiNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PCWSTR NameToRemove);
NTSTATUS BaseRemoveMultiValue(PWSTR MultiString, ULONG Index, PULONG NewTotalLen);
NTSTATUS BaseSetMultiNameInReg(PCWSTR KeyPath, PCWSTR ValueName, PCWSTR NewMultiString, ULONG NewSize);

// ------------------------------------------------------------------
// BasepGetMultiValueAddr
// Finds the address and total length of the multi‑string element at the given index.
// ------------------------------------------------------------------
NTSTATUS BasepGetMultiValueAddr(PWSTR MultiString, ULONG Index, PWSTR* OutAddr, PULONG OutTotalLen)
{
    ULONG curIndex = 0;
    ULONG totalLen = 0;
    ULONG count = 0;

    if (Index != 0) {
        do {
            if (*MultiString == 0) {
                return STATUS_NOT_FOUND;  // 0x490 in original? Actually 0x490 is 1168 decimal, which is ERROR_INVALID_DATA? We'll use STATUS_NOT_FOUND.
            }
            ULONG len = wcslen(MultiString);
            totalLen += len + 1;
            count++;
            MultiString += len + 1;
        } while (count < Index);
    }

    // Check if the next string is empty (end of multi‑string)
    if (*MultiString == 0) {
        return STATUS_NOT_FOUND;
    }

    *OutAddr = MultiString;
    *OutTotalLen = totalLen;
    return STATUS_SUCCESS;
}

// ------------------------------------------------------------------
// BasepGetNameFromReg
// Reads a REG_SZ value from the registry, ensuring it is null‑terminated.
// ------------------------------------------------------------------
NTSTATUS BasepGetNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PWSTR Buffer, PULONG BufferLen)
{
    ULONG dataSize = *BufferLen * sizeof(WCHAR);
    NTSTATUS status = BasepGetValueFromReg(KeyPath, ValueName, Buffer, &dataSize);
    *BufferLen = dataSize / sizeof(WCHAR);

    if (NT_SUCCESS(status)) {
        if (*BufferLen == 0 || Buffer[*BufferLen - 1] != 0) {
            // Not null‑terminated
            return STATUS_BUFFER_TOO_SMALL;
        }
        // Remove the trailing null from count
        *BufferLen = *BufferLen - 1;
    }
    return status;
}

// ------------------------------------------------------------------
// BasepGetValueFromReg
// Reads any registry value type into a caller‑supplied buffer.
// ------------------------------------------------------------------
NTSTATUS BasepGetValueFromReg(PCWSTR KeyPath, PCWSTR ValueName, PVOID Buffer, PULONG BufferLen)
{
    UNICODE_STRING KeyName, ValueNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo = NULL;
    ULONG infoSize = 0;
    BOOL bAllocated = FALSE;

    RtlInitUnicodeString(&KeyName, KeyPath);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&ValueNameU, ValueName);

    // Query the value to get required size
    ULONG resultLen = 0;
    status = NtQueryValueKey(hKey, &ValueNameU, KeyValuePartialInformation, NULL, 0, &resultLen);
    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
        pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap(
			(RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}),
			0,
			resultLen
		);
        if (pInfo == NULL) {
            status = STATUS_NO_MEMORY;
            goto cleanup;
        }
        bAllocated = TRUE;
        status = NtQueryValueKey(hKey, &ValueNameU, KeyValuePartialInformation, pInfo, resultLen, &resultLen);
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }
    } else if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    // Now copy data to caller's buffer, adjusting for type
    ULONG dataLen = pInfo->DataLength;
    ULONG type = pInfo->Type;

    // Handle string types: ensure null termination and remove extra nulls
    if (type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) {
        if (dataLen < 2) {
            dataLen = 0;
        } else {
            // Remove trailing null if present
            PWCHAR pData = (PWCHAR)((PBYTE)pInfo + pInfo->DataLength);
            ULONG charLen = dataLen / sizeof(WCHAR);
            if (charLen > 0 && pData[charLen - 1] == 0) {
                dataLen -= sizeof(WCHAR);
            }
        }
    }

    if (dataLen <= *BufferLen && Buffer != NULL) {
        memcpy(Buffer, (PBYTE)pInfo + pInfo->DataLength, dataLen);
        // Ensure null termination for string types
        if (type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) {
            if (dataLen < *BufferLen) {
                ((PBYTE)Buffer)[dataLen] = 0;
                ((PBYTE)Buffer)[dataLen + 1] = 0; // extra for wide char
            }
        }
        *BufferLen = dataLen;
    } else {
        // Buffer too small
        status = STATUS_BUFFER_TOO_SMALL;
        // Still return required size
        *BufferLen = dataLen;
    }

cleanup:
    if (bAllocated && pInfo) {
        RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pInfo);
    }
    NtClose(hKey);
    return status;
}

// ------------------------------------------------------------------
// BaseRemoveMultiNameFromReg
// Removes a specific string from a REG_MULTI_SZ value.
// ------------------------------------------------------------------
NTSTATUS BaseRemoveMultiNameFromReg(PCWSTR KeyPath, PCWSTR ValueName, PCWSTR NameToRemove)
{
    NTSTATUS status;
    ULONG bufferLen = 0;
    PWSTR pMultiString = NULL;

    // First read the value to get its size
    status = BasepGetNameFromReg(KeyPath, ValueName, NULL, &bufferLen);
    if (status == STATUS_BUFFER_TOO_SMALL) {
        // Allocate buffer
        pMultiString = (PWSTR)RtlAllocateHeap(
			(RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}),
			0, bufferLen);
        if (pMultiString == NULL) {
            return STATUS_NO_MEMORY;
        }
        status = BasepGetNameFromReg(KeyPath, ValueName, pMultiString, &bufferLen);
        if (!NT_SUCCESS(status)) {
            RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pMultiString);
            return status;
        }
    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
        // Value doesn't exist – nothing to remove
        return STATUS_SUCCESS;
    } else if (!NT_SUCCESS(status)) {
        return status;
    } else {
        // Buffer was large enough? Actually we passed NULL, so it should have failed with STATUS_BUFFER_TOO_SMALL.
        // If it succeeded, we need to allocate anyway.
        // This path is unlikely; we'll just allocate.
        pMultiString = (PWSTR)RtlAllocateHeap(
			(RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}),
			0, (bufferLen + 1) * sizeof(WCHAR));
        if (pMultiString == NULL) {
            return STATUS_NO_MEMORY;
        }
        status = BasepGetNameFromReg(KeyPath, ValueName, pMultiString, &bufferLen);
        if (!NT_SUCCESS(status)) {
            RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pMultiString);
            return status;
        }
    }

    // Now we have the multi‑string in pMultiString, bufferLen is the total length in characters (excluding final null?)
    // Actually BasepGetNameFromReg returns length without the final null. But the multi‑string has nulls between strings and double null at end.
    // We'll treat it as a multi‑string.

    ULONG totalLen = bufferLen + 1; // include the final null
    PWSTR p = pMultiString;
    ULONG idx = 0;
    while (*p != 0) {
        if (wcscmp(p, NameToRemove) == 0) {
            // Found the entry – remove it
            status = BaseRemoveMultiValue(pMultiString, idx, &totalLen);
            if (!NT_SUCCESS(status)) {
                RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pMultiString);
                return status;
            }
            // Now write back
            status = BaseSetMultiNameInReg(KeyPath, ValueName, pMultiString, totalLen * sizeof(WCHAR));
            RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pMultiString);
            return status;
        }
        p += wcslen(p) + 1;
        idx++;
    }

    // Not found
    RtlFreeHeap((RtlGetProcessHeaps(1, &(PVOID){0}), (PVOID){0}), 0, pMultiString);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

// ------------------------------------------------------------------
// BaseRemoveMultiValue
// Removes the element at the given index from a multi‑string.
// Updates *NewTotalLen to the new total length in characters (including final null).
// ------------------------------------------------------------------
NTSTATUS BaseRemoveMultiValue(PWSTR MultiString, ULONG Index, PULONG NewTotalLen)
{
    PWSTR pRemoveStart = NULL;
    ULONG totalLen = 0;
    NTSTATUS status = BasepGetMultiValueAddr(MultiString, Index, &pRemoveStart, &totalLen);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Find the next element after the one to remove
    PWSTR pNextStart = NULL;
    ULONG nextTotalLen = 0;
    status = BasepGetMultiValueAddr(MultiString, Index + 1, &pNextStart, &nextTotalLen);
    if (status == STATUS_NOT_FOUND) {
        // There is no next element – we are removing the last one.
        // Just place a null at the start of the element to remove.
        // Actually we need to shorten the whole string.
        ULONG removeLen = wcslen(pRemoveStart) + 1;
        PWSTR pEnd = pRemoveStart + removeLen;
        // Copy the rest (which should be just the final null)
        // But if this is the last element, pEnd points to the double null? Actually the multi‑string ends with two nulls.
        // We can just set the first character to null to terminate the string.
        *pRemoveStart = 0;
        // The total length becomes the offset of pRemoveStart from start.
        ULONG newTotalLen = (ULONG)(pRemoveStart - MultiString) + 1; // +1 for the null
        *NewTotalLen = newTotalLen;
        return STATUS_SUCCESS;
    } else if (!NT_SUCCESS(status)) {
        return status;
    }

    // We have the next start, so we can memmove.
    ULONG removeLen = (ULONG)(pNextStart - pRemoveStart);
    ULONG remainingLen = (ULONG)(wcslen(MultiString) + 1 - (pNextStart - MultiString));
    memmove(pRemoveStart, pNextStart, remainingLen * sizeof(WCHAR));
    *NewTotalLen = totalLen + remainingLen - removeLen; // adjust
    return STATUS_SUCCESS;
}

// ------------------------------------------------------------------
// BaseSetMultiNameInReg
// Writes a REG_MULTI_SZ value to the registry.
// ------------------------------------------------------------------
NTSTATUS BaseSetMultiNameInReg(PCWSTR KeyPath, PCWSTR ValueName, PCWSTR NewMultiString, ULONG NewSize)
{
    UNICODE_STRING KeyName, ValueNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    NTSTATUS status;

    RtlInitUnicodeString(&KeyName, KeyPath);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtCreateKey(&hKey, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, 0, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&ValueNameU, ValueName);
    status = NtSetValueKey(hKey, (PUNICODE_STRING)&ValueNameU, 0, REG_MULTI_SZ, (PVOID)NewMultiString, NewSize);
    if (NT_SUCCESS(status)) {
        NtFlushKey(hKey);
    }
    NtClose(hKey);
    return status;
}

