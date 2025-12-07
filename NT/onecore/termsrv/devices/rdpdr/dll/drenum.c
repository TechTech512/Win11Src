/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drenum.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT

Author:

    Joy Chik 1/20/2000

--*/

#pragma warning (disable:4005)

#include <windows.h>
#include <string.h>
#include <winternl.h>
#include <ntstatus.h>
#include <winioctl.h>

// External function declarations
extern NTSTATUS NtFsControlFile(
    HANDLE FileHandle,
    HANDLE Event,
    PVOID ApcRoutine,
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG FsControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);
extern NTSTATUS NtOpenFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions
);
extern void RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);

typedef NETRESOURCEW* PNETRESOURCEW;  // PNETRESOURCEW is typically a typedef for LPNETRESOURCEW

// Global variables
extern UNICODE_STRING DrProviderName;

// Function prototypes
ULONG DrOpenMiniRdr(PHANDLE FileHandle);

// Structures
typedef struct _RDPDR_ENUMERATION_HANDLE {
    DWORD dwScope;
    DWORD dwType;
    DWORD dwUsage;
    UNICODE_STRING RemoteName;
    DWORD enumType;
    PVOID pEnumBuffer;
    DWORD totalEntries;
    DWORD enumIndex;
} RDPDR_ENUMERATION_HANDLE, *PRDPDR_ENUMERATION_HANDLE;

typedef enum _ENUM_TYPE {
    EnumTypeConnection = 0,
    EnumTypeServer = 1,
    EnumTypeShare = 2
} ENUM_TYPE;

ULONG DrDeviceControlGetInfo(
    PVOID InputBuffer,
    ULONG FsControlCode,
    PVOID* OutputBuffer,
    ULONG OutputBufferLength,
    PUCHAR* ExtraInfo,
    ULONG UnknownParam6,
    ULONG UnknownParam7,
    PULONG BytesReturned
)
{
    NTSTATUS status;
    HANDLE heap;
    PVOID buffer = NULL;
    ULONG bufferSize = 0xC000;
    ULONG errorCode = 0;
    ULONG returnValue = 0;
    
    // Get process heap
    heap = GetProcessHeap();
    
    // Allocate initial buffer
    buffer = HeapAlloc(heap, 0, bufferSize);
    *OutputBuffer = buffer;
    
    if (buffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    // First attempt
    status = NtFsControlFile(
        NULL,           // FileHandle
        NULL,           // Event
        NULL,           // ApcRoutine
        NULL,           // ApcContext
        NULL,           // IoStatusBlock
        FsControlCode,  // FsControlCode
        InputBuffer,    // InputBuffer
        0x18,           // InputBufferLength
        buffer,         // OutputBuffer
        bufferSize      // OutputBufferLength
    );
    
    if (status == STATUS_SUCCESS) {
        if (*BytesReturned != 0) {
            return ERROR_SUCCESS;
        }
        return 0;
    }
    
    // Handle buffer too small case
    if (status == STATUS_BUFFER_TOO_SMALL) {
        ULONG requiredSize = *BytesReturned;
        
        // Check if required size is reasonable
        if (requiredSize < 0xC001) {
            returnValue = ERROR_MORE_DATA;
            goto cleanup;
        }
        
        // Free old buffer
        HeapFree(heap, 0, buffer);
        *OutputBuffer = NULL;
        
        // Calculate new size with extra padding
        ULONG newSize = requiredSize + 0x400;
        if (newSize < requiredSize || newSize < 0x400) {
            // Overflow or invalid size
            *OutputBuffer = NULL;
            returnValue = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        
        // Allocate new buffer
        buffer = HeapAlloc(heap, 0, newSize);
        *OutputBuffer = buffer;
        
        if (buffer == NULL) {
            returnValue = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        
        // Second attempt with larger buffer
        status = NtFsControlFile(
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            FsControlCode,
            InputBuffer,
            0x18,
            buffer,
            newSize
        );
        
        if (status == STATUS_SUCCESS) {
            if (*BytesReturned != 0) {
                return ERROR_SUCCESS;
            }
            return 0;
        }
        
        if (status == STATUS_BUFFER_TOO_SMALL) {
            returnValue = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
    
    // Other error
    returnValue = ERROR_INVALID_PARAMETER;
    
cleanup:
    if (*OutputBuffer != NULL) {
        HeapFree(heap, 0, *OutputBuffer);
        *OutputBuffer = NULL;
    }
    
    if (*BytesReturned != 0) {
        return returnValue;
    }
    
    return ERROR_NO_MORE_ITEMS;
}

ULONG DrEnumConnectionInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
)
{
    ULONG errorCode = 0;
    PULONG buffer = NULL;
    DWORD currentEntry = 0;
    DWORD totalEntries = 0;
    PVOID enumBuffer = NULL;
    PULONG currentPtr;
    PWCHAR destPtr = NULL;
    DWORD remainingSpace;
    DWORD neededSpace;
    
    if (EnumHandle->enumIndex == 0) {
        // First enumeration
        errorCode = DrOpenMiniRdr(&enumBuffer);
        if (errorCode != 0) {
            *EntriesReturned = 0;
            return ERROR_NO_MORE_ITEMS;
        }
        
        IO_STATUS_BLOCK ioStatus;
        ULONG bytesReturned;
        
        errorCode = DrDeviceControlGetInfo(
            enumBuffer,
            0,
            &buffer,
            0,
            NULL,
            0,
            0,
            &bytesReturned
        );
        
        if (errorCode == 0) {
            EnumHandle->totalEntries = bytesReturned;
            EnumHandle->pEnumBuffer = buffer;
            currentPtr = buffer;
        } else {
            currentEntry = 0;
            goto done;
        }
    } else {
        buffer = EnumHandle->pEnumBuffer;
        if (buffer == NULL) {
            *EntriesReturned = 0;
            return ERROR_NO_MORE_ITEMS;
        }
        currentPtr = buffer;
    }
    
    totalEntries = EnumHandle->totalEntries;
    if (EnumHandle->enumIndex == totalEntries || buffer == NULL) {
        errorCode = ERROR_NO_MORE_ITEMS;
        currentEntry = 0;
        goto done;
    }
    
    currentPtr += EnumHandle->enumIndex * 8;
    remainingSpace = *BufferSize;
    
    while (currentEntry < *EntriesReturned && EnumHandle->enumIndex < totalEntries) {
        USHORT nameLength = (USHORT)currentPtr[0];
        USHORT commentLength = (USHORT)currentPtr[2];
        
        neededSpace = DrProviderName.Length + 0x26 + nameLength + commentLength;
        
        if (remainingSpace < neededSpace) {
            if (currentEntry == 0) {
                errorCode = ERROR_MORE_DATA;
                *BufferSize = neededSpace;
            } else {
                errorCode = 0;
            }
            break;
        }
        
        // Fill NETRESOURCE structure
        NetResource[currentEntry].dwScope = EnumHandle->dwScope;
        NetResource[currentEntry].dwType = RESOURCETYPE_DISK;
        NetResource[currentEntry].dwUsage = RESOURCEUSAGE_CONNECTABLE;
        NetResource[currentEntry].lpLocalName = NULL;
        
        // Copy remote name
        if (nameLength > 0) {
            destPtr = (PWCHAR)((PBYTE)NetResource + remainingSpace - nameLength - 2);
            NetResource[currentEntry].lpRemoteName = destPtr;
            memcpy(destPtr, (PBYTE)currentPtr + 4, nameLength);
            destPtr[nameLength / sizeof(WCHAR)] = L'\0';
        } else {
            NetResource[currentEntry].lpRemoteName = NULL;
        }
        
        // Copy provider name
        destPtr = (PWCHAR)((PBYTE)destPtr - DrProviderName.Length - 2);
        NetResource[currentEntry].lpProvider = destPtr;
        memcpy(destPtr, DrProviderName.Buffer, DrProviderName.Length);
        destPtr[DrProviderName.Length / sizeof(WCHAR)] = L'\0';
        
        NetResource[currentEntry].lpComment = NULL;
        
        currentEntry++;
        EnumHandle->enumIndex++;
        currentPtr += 8;
        remainingSpace -= neededSpace;
    }
    
    *EntriesReturned = currentEntry;
    
done:
    if (errorCode != 0 && buffer != NULL) {
        HeapFree(GetProcessHeap(), 0, buffer);
        EnumHandle->pEnumBuffer = NULL;
    }
    
    if (currentEntry != 0) {
        return errorCode;
    }
    
    return ERROR_NO_MORE_ITEMS;
}

ULONG DrEnumServerInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
)
{
    ULONG errorCode = 0;
    PUSHORT buffer = NULL;
    DWORD entries = 0;
    
    *EntriesReturned = 0;
    
    if (EnumHandle->enumIndex == 0) {
        PVOID enumBuffer;
        errorCode = DrOpenMiniRdr(&enumBuffer);
        if (errorCode != 0) {
            *EntriesReturned = 0;
            return ERROR_NO_MORE_ITEMS;
        }
        
        IO_STATUS_BLOCK ioStatus;
        ULONG bytesReturned;
        
        errorCode = DrDeviceControlGetInfo(
            enumBuffer,
            0,
            &buffer,
            0,
            NULL,
            0,
            0,
            &bytesReturned
        );
        
        if (errorCode == 0) {
            EnumHandle->pEnumBuffer = buffer;
            
            USHORT nameLength = *buffer;
            DWORD neededSpace = DrProviderName.Length + 0x24 + nameLength;
            
            if (*BufferSize < neededSpace) {
                *BufferSize = neededSpace;
                errorCode = ERROR_MORE_DATA;
                goto done;
            }
            
            // Fill single NETRESOURCE for server
            NetResource[0].dwScope = EnumHandle->dwScope;
            NetResource[0].dwType = RESOURCETYPE_DISK;
            NetResource[0].dwUsage = RESOURCEUSAGE_CONTAINER;
            NetResource[0].lpLocalName = NULL;
            
            // Copy server name
            PWCHAR destPtr = (PWCHAR)((PBYTE)NetResource + *BufferSize - nameLength - 2);
            NetResource[0].lpRemoteName = destPtr;
            memcpy(destPtr, buffer + 1, nameLength);
            destPtr[nameLength / sizeof(WCHAR)] = L'\0';
            
            // Copy provider name
            destPtr = (PWCHAR)((PBYTE)destPtr - DrProviderName.Length - 2);
            NetResource[0].lpProvider = destPtr;
            memcpy(destPtr, DrProviderName.Buffer, DrProviderName.Length);
            destPtr[DrProviderName.Length / sizeof(WCHAR)] = L'\0';
            
            NetResource[0].lpComment = NULL;
            
            entries = 1;
            EnumHandle->enumIndex++;
            errorCode = 0;
        }
    } else {
        errorCode = ERROR_NO_MORE_ITEMS;
    }
    
done:
    *EntriesReturned = entries;
    return errorCode;
}

ULONG DrEnumShareInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
)
{
    ULONG errorCode = 0;
    PULONG buffer = NULL;
    DWORD currentEntry = 0;
    DWORD totalEntries = 0;
    PULONG currentPtr;
    PWCHAR destPtr;
    DWORD remainingSpace;
    DWORD neededSpace;
    
    if (EnumHandle->dwType == 0 || EnumHandle->pEnumBuffer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (EnumHandle->enumIndex == 0) {
        PVOID enumBuffer;
        errorCode = DrOpenMiniRdr(&enumBuffer);
        if (errorCode != 0) {
            *EntriesReturned = 0;
            return ERROR_NO_MORE_ITEMS;
        }
        
        IO_STATUS_BLOCK ioStatus;
        ULONG bytesReturned;
        
        errorCode = DrDeviceControlGetInfo(
            enumBuffer,
            0,
            &buffer,
            0,
            NULL,
            0,
            0,
            &bytesReturned
        );
        
        if (errorCode == 0) {
            EnumHandle->totalEntries = bytesReturned;
            EnumHandle->pEnumBuffer = buffer;
            currentPtr = buffer;
        } else {
            currentEntry = 0;
            goto done;
        }
    } else {
        buffer = EnumHandle->pEnumBuffer;
        if (buffer == NULL) {
            *EntriesReturned = 0;
            return ERROR_NO_MORE_ITEMS;
        }
        currentPtr = buffer;
    }
    
    totalEntries = EnumHandle->totalEntries;
    if (EnumHandle->enumIndex == totalEntries) {
        errorCode = ERROR_NO_MORE_ITEMS;
        currentEntry = 0;
        goto done;
    }
    
    currentPtr += EnumHandle->enumIndex * 4;
    remainingSpace = *BufferSize;
    
    while (currentEntry < *EntriesReturned && EnumHandle->enumIndex < totalEntries) {
        USHORT shareNameLength = (USHORT)*currentPtr;
        
        neededSpace = DrProviderName.Length + 0x24 + shareNameLength;
        
        if (remainingSpace < neededSpace) {
            if (currentEntry == 0) {
                errorCode = ERROR_MORE_DATA;
                *BufferSize = neededSpace;
            } else {
                errorCode = 0;
            }
            break;
        }
        
        // Fill NETRESOURCE structure for share
        NetResource[currentEntry].dwScope = EnumHandle->dwScope;
        NetResource[currentEntry].dwType = RESOURCETYPE_DISK;
        NetResource[currentEntry].dwUsage = RESOURCEUSAGE_CONNECTABLE;
        NetResource[currentEntry].lpLocalName = NULL;
        
        // Copy share name
        destPtr = (PWCHAR)((PBYTE)NetResource + remainingSpace - shareNameLength - 2);
        NetResource[currentEntry].lpRemoteName = destPtr;
        memcpy(destPtr, (PBYTE)currentPtr + 2, shareNameLength);
        destPtr[shareNameLength / sizeof(WCHAR)] = L'\0';
        
        // Copy provider name
        destPtr = (PWCHAR)((PBYTE)destPtr - DrProviderName.Length - 2);
        NetResource[currentEntry].lpProvider = destPtr;
        memcpy(destPtr, DrProviderName.Buffer, DrProviderName.Length);
        destPtr[DrProviderName.Length / sizeof(WCHAR)] = L'\0';
        
        NetResource[currentEntry].lpComment = NULL;
        
        currentEntry++;
        EnumHandle->enumIndex++;
        currentPtr += 4;
        remainingSpace -= neededSpace;
    }
    
done:
    *EntriesReturned = currentEntry;
    
    if (errorCode != 0 && buffer != NULL) {
        HeapFree(GetProcessHeap(), 0, buffer);
        EnumHandle->pEnumBuffer = NULL;
    }
    
    if (currentEntry != 0) {
        return errorCode;
    }
    
    return ERROR_NO_MORE_ITEMS;
}

ULONG DrOpenMiniRdr(PHANDLE FileHandle)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    
    RtlInitUnicodeString(&deviceName, L"\\Device\\RdpDr");
    
    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );
    
    status = NtOpenFile(
        FileHandle,
        FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
    );
    
    if (status == STATUS_SUCCESS) {
        return 0;
    }
    
    return 5; // ERROR_ACCESS_DENIED
}

