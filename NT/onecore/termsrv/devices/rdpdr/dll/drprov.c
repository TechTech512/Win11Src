/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drprov.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT for RDP mini-redirector

Author:

    Joy Chik 1/20/2000

--*/

#pragma warning (disable:4005)

#include <windows.h>
#include <winnetwk.h>
#include <winternl.h>
#include <winioctl.h>
#include <ntstatus.h>
#include <string.h>
#include <strsafe.h>

typedef NETRESOURCEW* PNETRESOURCEW;  // PNETRESOURCEW is typically a typedef for LPNETRESOURCEW

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

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
extern NTSTATUS NtClose(HANDLE Handle);
NTSTATUS NtQueryAttributesFile(
    POBJECT_ATTRIBUTES ObjectAttributes,
    PFILE_BASIC_INFORMATION FileInformation
);
extern void RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
extern void RtlAppendUnicodeToString(PUNICODE_STRING Destination, PCWSTR Source);
extern BOOL WinStationIsSessionRemoteable(HANDLE ServerHandle, ULONG SessionId, PBOOL IsRemoteable);

// Function prototypes from other modules
extern ULONG DrOpenMiniRdr(PHANDLE FileHandle);
extern ULONG DrDeviceControlGetInfo(
    PVOID InputBuffer,
    ULONG FsControlCode,
    PVOID* OutputBuffer,
    ULONG OutputBufferLength,
    PUCHAR* ExtraInfo,
    ULONG UnknownParam6,
    ULONG UnknownParam7,
    PULONG BytesReturned
);
ULONG DrEnumServerInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
);
ULONG DrEnumShareInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
);
ULONG DrEnumConnectionInfo(
    PRDPDR_ENUMERATION_HANDLE EnumHandle,
    PULONG BufferSize,
    PNETRESOURCEW NetResource,
    PULONG EntriesReturned
);

// Global variables based on your data section
UNICODE_STRING DrDeviceName = {
    0x1A,
    0x1C,
    L"\\Device\\RdpDr"
};

UNICODE_STRING DrProviderName = {
    0,
    0,
    NULL
};

wchar_t ProviderName[260] = {0};

DWORD WNetValidReturn[6] = {
    0xEA,
    0x1E7,
    0x55,
    0x4C7,
    0x43,
    0x4B0
};

// Structure definitions
typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

// Function prototypes
ULONG CreateConnectionName(PUNICODE_STRING ConnectionName, LPCWSTR LocalName, LPCWSTR RemoteName);
BOOL IsPortDriveName(LPCWSTR DeviceName);
BOOL IsRemoteNameTsclient(LPCWSTR RemoteName);
BOOL IsValidWNetReturn(DWORD ErrorCode);
ULONG ValidateConnectionName(PUNICODE_STRING ConnectionName);
ULONG ValidateRemoteName(LPCWSTR RemoteName);
ULONG OpenConnection(
    PUNICODE_STRING ConnectionName,
    DWORD DesiredAccess,
    DWORD ShareMode,
    PFILE_FULL_EA_INFORMATION FileInfo,
    DWORD CreateOptions,
    PVOID EaBuffer
);

// NPAPI Functions
ULONG NPAddConnection(PNETRESOURCEW lpNetResource, LPWSTR lpPassword, LPWSTR lpUserName);
ULONG NPAddConnection3(HWND hwndOwner, PNETRESOURCEW lpNetResource, LPWSTR lpPassword, 
                       LPWSTR lpUserName, DWORD dwFlags);
ULONG NPCancelConnection(LPWSTR lpName, BOOL fForce);
ULONG NPCloseEnum(HANDLE hEnum);
ULONG NPEnumResource(HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
ULONG NPGetCaps(DWORD nIndex);
ULONG NPGetConnection(LPWSTR lpLocalName, LPWSTR lpRemoteName, LPDWORD lpnBufferLen);
ULONG NPGetConnectionPerformance(LPWSTR lpRemoteName, LPNETCONNECTINFOSTRUCT lpNetConnectInfo);
ULONG NPGetResourceInformation(PNETRESOURCEW lpNetResource, LPVOID lpBuffer, 
                               LPDWORD lpBufferSize, LPWSTR* lplpSystem);
ULONG NPGetResourceParent(PNETRESOURCEW lpNetResource, LPVOID lpBuffer, LPDWORD lpBufferSize);
ULONG NPGetUniversalName(LPWSTR lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, 
                         LPDWORD lpBufferSize);
ULONG NPOpenEnum(DWORD dwScope, DWORD dwType, DWORD dwUsage, PNETRESOURCEW lpNetResource, 
                 LPHANDLE lphEnum);

ULONG CreateConnectionName(PUNICODE_STRING ConnectionName, LPCWSTR LocalName, LPCWSTR RemoteName)
{
    WCHAR Buffer[520] = {0};
    WCHAR PortNumber[32] = {0};
    WCHAR LocalNameBuffer[520] = {0};
    SIZE_T LocalNameLen = 0;
    SIZE_T RemoteNameLen = 0;
    SIZE_T PortNumberLen = 0;
    SIZE_T TotalLength = 0;
    PWSTR CurrentPtr;
    SIZE_T BufferLen = 0;
    HANDLE ProcessHeap;
    
    ProcessHeap = GetProcessHeap();
    
    if (RemoteName)
    {
        CurrentPtr = (PWSTR)RemoteName;
        while (*CurrentPtr != L'\0')
        {
            CurrentPtr++;
        }
        RemoteNameLen = (CurrentPtr - RemoteName) * sizeof(WCHAR);
        
        if (RemoteNameLen > 0x208)
        {
            return ERROR_INVALID_PARAMETER;
        }
        
        if (FAILED(StringCbCopyW(Buffer, sizeof(Buffer), RemoteName)))
        {
            return ERROR_INVALID_PARAMETER;
        }
        
        CurrentPtr = Buffer;
        while (*CurrentPtr != L'\0')
        {
            CurrentPtr++;
        }
        RemoteNameLen = (CurrentPtr - Buffer) * sizeof(WCHAR);
        
        if (RemoteNameLen >= sizeof(WCHAR) && *(CurrentPtr - 1) == L':')
        {
            *(CurrentPtr - 1) = L'\0';
            RemoteNameLen -= sizeof(WCHAR);
        }
    }
    
    if (FAILED(StringCbPrintfW(PortNumber, sizeof(PortNumber), L"%d", 1)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    CurrentPtr = PortNumber;
    while (*CurrentPtr != L'\0')
    {
        CurrentPtr++;
    }
    PortNumberLen = (CurrentPtr - PortNumber) * sizeof(WCHAR);
    
    TotalLength = DrDeviceName.Length + PortNumberLen + RemoteNameLen + 8;
    
    ConnectionName->Length = 0;
    ConnectionName->MaximumLength = (USHORT)TotalLength;
    ConnectionName->Buffer = HeapAlloc(ProcessHeap, 0, TotalLength);
    
    if (!ConnectionName->Buffer)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    ConnectionName->Buffer[0] = L'\0';
    RtlAppendUnicodeToString(ConnectionName, DrDeviceName.Buffer);
    RtlAppendUnicodeToString(ConnectionName, L"\\;");
    
    if (RemoteNameLen > 0)
    {
        RtlAppendUnicodeToString(ConnectionName, Buffer);
    }
    
    RtlAppendUnicodeToString(ConnectionName, L":");
    RtlAppendUnicodeToString(ConnectionName, PortNumber);
    
    if (LocalName)
    {
        SIZE_T LocalPathLen = 0;
        CurrentPtr = (PWSTR)LocalName;
        while (*CurrentPtr != L'\0')
        {
            CurrentPtr++;
        }
        LocalPathLen = (CurrentPtr - LocalName) * sizeof(WCHAR);
        
        if (LocalPathLen > 0)
        {
            RtlAppendUnicodeToString(ConnectionName, LocalName);
        }
    }
    
    return ERROR_SUCCESS;
}

BOOL IsPortDriveName(LPCWSTR DeviceName)
{
    const wchar_t* Current;
    
    if (!DeviceName)
    {
        return FALSE;
    }
    
    Current = DeviceName;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    return (Current - DeviceName) > 2;
}

BOOL IsRemoteNameTsclient(LPCWSTR RemoteName)
{
    const wchar_t* Current;
    const wchar_t* ServerStart;
    const wchar_t* BackslashPos;
    SIZE_T ServerNameLength;
    
    if (!RemoteName)
    {
        return FALSE;
    }
    
    Current = RemoteName;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    if ((Current - RemoteName) <= 2)
    {
        return FALSE;
    }
    
    if (RemoteName[0] != L'\\' || RemoteName[1] != L'\\')
    {
        return FALSE;
    }
    
    ServerStart = RemoteName + 2;
    BackslashPos = wcsstr(ServerStart, L"\\");
    
    if (!BackslashPos)
    {
        BackslashPos = ServerStart;
        while (*BackslashPos != L'\0')
        {
            BackslashPos++;
        }
    }
    
    ServerNameLength = (BackslashPos - ServerStart) * sizeof(WCHAR);
    
    if (ServerNameLength == 0x10)
    {
        return _wcsnicmp(ServerStart, L"tsclient", 8) == 0;
    }
    
    return FALSE;
}

BOOL IsValidWNetReturn(DWORD ErrorCode)
{
    DWORD* Current;
    DWORD* End;
    
    Current = WNetValidReturn;
    End = WNetValidReturn + 6;
    
    while (Current < End)
    {
        if (*Current == ErrorCode)
        {
            return TRUE;
        }
        Current++;
    }
    
    return FALSE;
}

ULONG NPAddConnection(PNETRESOURCEW lpNetResource, LPWSTR lpPassword, LPWSTR lpUserName)
{
    return NPAddConnection3(NULL, lpNetResource, lpPassword, lpUserName, 0);
}

ULONG NPAddConnection3(HWND hwndOwner, PNETRESOURCEW lpNetResource, LPWSTR lpPassword, 
                       LPWSTR lpUserName, DWORD dwFlags)
{
    UNICODE_STRING ConnectionName;
    WCHAR DevicePath[64];
    WCHAR PortNumber[32];
    WCHAR ConnectionBuffer[520];
    WCHAR DosDevicePath[520];
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG ErrorCode = ERROR_SUCCESS;
    HANDLE ProcessHeap;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;
    BOOL IsTsClient = FALSE;
    const wchar_t* RemoteNamePtr;
    const wchar_t* Current;
    SIZE_T RemoteNameLen;
    SIZE_T PortNumberLen;
    SIZE_T TotalLength;
    
    ProcessHeap = GetProcessHeap();
    
    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionName.MaximumLength = 0;
    
    DevicePath[0] = L'\0';
    
    if (lpNetResource->lpLocalName)
    {
        Current = lpNetResource->lpLocalName;
        while (*Current != L'\0')
        {
            Current++;
        }
        
        if ((Current - lpNetResource->lpLocalName) > 2)
        {
            ErrorCode = ERROR_BAD_DEVICE;
            goto Cleanup;
        }
    }
    
    if (!lpNetResource->lpRemoteName || lpNetResource->lpRemoteName[0] != L'\\' || 
        lpNetResource->lpRemoteName[1] != L'\\')
    {
        ErrorCode = ERROR_BAD_NET_NAME;
        goto Cleanup;
    }
    
    RemoteNamePtr = lpNetResource->lpRemoteName;
    Current = RemoteNamePtr;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    if ((Current - RemoteNamePtr) > 2 && RemoteNamePtr[0] == L'\\' && RemoteNamePtr[1] == L'\\')
    {
        const wchar_t* ServerStart = RemoteNamePtr + 2;
        const wchar_t* BackslashPos = wcsstr(ServerStart, L"\\");
        
        if (!BackslashPos)
        {
            BackslashPos = ServerStart;
            while (*BackslashPos != L'\0')
            {
                BackslashPos++;
            }
        }
        
        if (((BackslashPos - ServerStart) * sizeof(WCHAR)) == 0x10)
        {
            if (_wcsnicmp(ServerStart, L"tsclient", 8) == 0)
            {
                IsTsClient = TRUE;
            }
        }
    }
    
    Current = lpNetResource->lpRemoteName + 1;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    RemoteNameLen = (Current - (lpNetResource->lpRemoteName + 1)) * sizeof(WCHAR);
    
    if (FAILED(StringCbPrintfW(PortNumber, sizeof(PortNumber), L"%d", 1)))
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    Current = PortNumber;
    while (*Current != L'\0')
    {
        Current++;
    }
    PortNumberLen = (Current - PortNumber) * sizeof(WCHAR);
    
    TotalLength = DrDeviceName.Length + PortNumberLen + RemoteNameLen + 8;
    
    ConnectionName.MaximumLength = (USHORT)TotalLength;
    ConnectionName.Buffer = HeapAlloc(ProcessHeap, 0, TotalLength);
    
    if (!ConnectionName.Buffer)
    {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    
    ConnectionName.Length = 0;
    ConnectionName.Buffer[0] = L'\0';
    
    RtlAppendUnicodeToString(&ConnectionName, DrDeviceName.Buffer);
    RtlAppendUnicodeToString(&ConnectionName, L"\\;");
    RtlAppendUnicodeToString(&ConnectionName, L":");
    RtlAppendUnicodeToString(&ConnectionName, PortNumber);
    RtlAppendUnicodeToString(&ConnectionName, lpNetResource->lpRemoteName + 1);
    
    if (ConnectionName.Length < 0x1A || _wcsnicmp(ConnectionName.Buffer, L"\\Device\\RdpDr", 13) != 0)
    {
        ErrorCode = ERROR_BAD_NET_NAME;
        goto Cleanup;
    }
    
    RtlInitUnicodeString(&ConnectionName, ConnectionName.Buffer);
    
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = &ConnectionName;
    ObjectAttributes.Attributes = 0x40;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;
    
    if (NtQueryAttributesFile(&ObjectAttributes, &FileInfo) == 0)
    {
        if (FileInfo.FileAttributes & 0x10)
        {
            ErrorCode = ERROR_BAD_NET_NAME;
        }
        else
        {
            ErrorCode = ERROR_SUCCESS;
        }
    }
    else
    {
        ErrorCode = ERROR_BAD_NET_NAME;
    }
    
    if (ErrorCode != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    ErrorCode = CreateConnectionName(&ConnectionName, NULL, lpNetResource->lpRemoteName);
    
    if (ErrorCode != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    
    if (lpNetResource->lpLocalName)
    {
        if (QueryDosDeviceW(lpNetResource->lpLocalName, DosDevicePath, sizeof(DosDevicePath) / sizeof(WCHAR)) == 0)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                // Continue
            }
            else
            {
                ErrorCode = ERROR_SHARING_PAUSED;
                goto Cleanup;
            }
        }
    }
    
    FileHandle = INVALID_HANDLE_VALUE;
    ErrorCode = OpenConnection(&ConnectionName, 0, 0, NULL, 0, NULL);
    
    if (ErrorCode == ERROR_SUCCESS)
    {
        if (DefineDosDeviceW(DDD_RAW_TARGET_PATH, lpNetResource->lpLocalName, ConnectionName.Buffer))
        {
            ErrorCode = ERROR_SUCCESS;
        }
        else
        {
            ErrorCode = GetLastError();
        }
        
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            NtClose(FileHandle);
        }
    }
    
Cleanup:
    if (ConnectionName.Buffer)
    {
        HeapFree(ProcessHeap, 0, ConnectionName.Buffer);
    }
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        NtClose(FileHandle);
    }
    
    if (ErrorCode != ERROR_SUCCESS && IsTsClient && !IsValidWNetReturn(ErrorCode))
    {
        ErrorCode = ERROR_INVALID_ADDRESS;
    }
    
    return ErrorCode;
}

ULONG NPCancelConnection(LPWSTR lpName, BOOL fForce)
{
    UNICODE_STRING ConnectionName;
    WCHAR DosDevicePath[520];
    WCHAR PortNumber[32];
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG ErrorCode = ERROR_SUCCESS;
    HANDLE ProcessHeap;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SIZE_T NameLength;
    SIZE_T PortNumberLen;
    SIZE_T TotalLength;
    const wchar_t* Current;
    BOOL IsDosDevice = FALSE;
    
    ProcessHeap = GetProcessHeap();
    
    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionName.MaximumLength = 0;
    
    if (!lpName)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    Current = lpName;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    NameLength = (Current - lpName) * sizeof(WCHAR);
    
    if (NameLength == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (lpName[0] == L'\\' && lpName[1] == L'\\')
    {
        Current = lpName + 1;
        while (*Current != L'\0')
        {
            Current++;
        }
        
        NameLength = (Current - (lpName + 1)) * sizeof(WCHAR);
        
        if (FAILED(StringCbPrintfW(PortNumber, sizeof(PortNumber), L"%d", 1)))
        {
            return ERROR_INVALID_PARAMETER;
        }
        
        Current = PortNumber;
        while (*Current != L'\0')
        {
            Current++;
        }
        PortNumberLen = (Current - PortNumber) * sizeof(WCHAR);
        
        TotalLength = DrDeviceName.Length + PortNumberLen + NameLength + 8;
        
        ConnectionName.MaximumLength = (USHORT)TotalLength;
        ConnectionName.Buffer = HeapAlloc(ProcessHeap, 0, TotalLength);
        
        if (!ConnectionName.Buffer)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        ConnectionName.Length = 0;
        ConnectionName.Buffer[0] = L'\0';
        
        RtlAppendUnicodeToString(&ConnectionName, DrDeviceName.Buffer);
        RtlAppendUnicodeToString(&ConnectionName, L"\\;");
        RtlAppendUnicodeToString(&ConnectionName, L":");
        RtlAppendUnicodeToString(&ConnectionName, PortNumber);
        RtlAppendUnicodeToString(&ConnectionName, lpName + 1);
    }
    else
    {
        IsDosDevice = TRUE;
        
        if (QueryDosDeviceW(lpName, DosDevicePath, sizeof(DosDevicePath) / sizeof(WCHAR)) == 0)
        {
            return ERROR_BAD_NET_NAME;
        }
        
        Current = DosDevicePath;
        while (*Current != L'\0')
        {
            Current++;
        }
        
        NameLength = (Current - DosDevicePath) * sizeof(WCHAR);
        
        ConnectionName.Length = (USHORT)NameLength;
        ConnectionName.MaximumLength = (USHORT)(NameLength + 2);
        ConnectionName.Buffer = DosDevicePath;
    }
    
    if (ConnectionName.Length < 0x1A || _wcsnicmp(ConnectionName.Buffer, L"\\Device\\RdpDr", 13) != 0)
    {
        ErrorCode = ERROR_BAD_NET_NAME;
        goto Cleanup;
    }
    
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = &ConnectionName;
    ObjectAttributes.Attributes = 0x40;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;
    
    if (NtCreateFile(&FileHandle, 0x100001, &ObjectAttributes, &IoStatusBlock, 0, 0x80, 7, 1, 0xA0, 0, 0) != 0)
    {
        ErrorCode = ERROR_BAD_NET_NAME;
        goto Cleanup;
    }
    
    if (fForce)
    {
        if (NtFsControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x1401AC, 0, 0, 0, 0) != 0)
        {
            ErrorCode = ERROR_BAD_NET_NAME;
            goto Cleanup;
        }
    }
    else
    {
        if (NtFsControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x140198, 0, 0, 0, 0) != 0)
        {
            ErrorCode = ERROR_BAD_NET_NAME;
            goto Cleanup;
        }
    }
    
    if (IsDosDevice)
    {
        if (!DefineDosDeviceW(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, lpName, ConnectionName.Buffer))
        {
            ErrorCode = GetLastError();
        }
        else
        {
            ErrorCode = ERROR_SUCCESS;
        }
    }
    else
    {
        ErrorCode = ERROR_SUCCESS;
    }
    
Cleanup:
    if (!IsDosDevice && ConnectionName.Buffer)
    {
        HeapFree(ProcessHeap, 0, ConnectionName.Buffer);
    }
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        NtClose(FileHandle);
    }
    
    return ErrorCode;
}

ULONG NPCloseEnum(HANDLE hEnum)
{
    HANDLE ProcessHeap;
    
    if (!hEnum)
    {
        return ERROR_SUCCESS;
    }
    
    ProcessHeap = GetProcessHeap();
    
    if (*(PVOID*)((ULONG_PTR)hEnum + 0x20))
    {
        HeapFree(ProcessHeap, 0, *(PVOID*)((ULONG_PTR)hEnum + 0x20));
    }
    
    if (*(PVOID*)((ULONG_PTR)hEnum + 0x10))
    {
        HeapFree(ProcessHeap, 0, *(PVOID*)((ULONG_PTR)hEnum + 0x10));
    }
    
    HeapFree(ProcessHeap, 0, hEnum);
    
    return ERROR_SUCCESS;
}

ULONG NPEnumResource(HANDLE hEnum, LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    DWORD* EnumData;
    DWORD EnumType;
    ULONG ErrorCode;
    
    if (!lpcCount || !lpBuffer || !lpBufferSize)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    EnumData = (DWORD*)hEnum;
    
    if (EnumData[1] == 2)
    {
        return ERROR_NO_MORE_ITEMS;
    }
    
    EnumType = EnumData[5];
    
    if (EnumType == 0)
    {
        ErrorCode = DrEnumServerInfo((PRDPDR_ENUMERATION_HANDLE)lpBuffer, lpBufferSize, 
                                     (PNETRESOURCEW)lpBuffer, lpcCount);
    }
    else if (EnumType == 1)
    {
        ErrorCode = DrEnumShareInfo((PRDPDR_ENUMERATION_HANDLE)lpBuffer, lpBufferSize, 
                                    (PNETRESOURCEW)lpBuffer, lpcCount);
    }
    else if (EnumType == 2)
    {
        ErrorCode = DrEnumConnectionInfo((PRDPDR_ENUMERATION_HANDLE)lpBuffer, lpBufferSize, 
                                         (PNETRESOURCEW)lpBuffer, lpcCount);
    }
    else if (EnumType == 3)
    {
        ErrorCode = ERROR_NO_MORE_ITEMS;
    }
    else
    {
        ErrorCode = ERROR_NO_MORE_ITEMS;
    }
    
    return ErrorCode;
}

ULONG NPGetCaps(DWORD nIndex)
{
    switch (nIndex)
    {
        case 1:
            return 0x50001;
        case 2:
            return 0x360000;
        case 3:
            return 0x1000D;
        case 4:
        case 0xC:
            return 1;
        case 6:
            return 0x4F;
        case 8:
            return 0xA00;
        case 0xB:
            return 0xB;
        default:
            return 0;
    }
}

ULONG NPGetConnection(LPWSTR lpLocalName, LPWSTR lpRemoteName, LPDWORD lpnBufferLen)
{
    WCHAR DosDevicePath[520];
    WCHAR ConnectionBuffer[520];
    UNICODE_STRING ConnectionName;
    FILE_FULL_EA_INFORMATION FileInfo;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG ErrorCode = ERROR_SUCCESS;
    HANDLE ProcessHeap;
    PVOID EnumBuffer = NULL;
    ULONG BytesReturned = 0;
    USHORT RemoteNameLength;
    const wchar_t* RdpDrPos;
    const wchar_t* Current;
    
    ProcessHeap = GetProcessHeap();
    
    memset(&FileInfo, 0, sizeof(FileInfo));
    FileInfo.NextEntryOffset = 0xFFFFFFFF;
    
    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionName.MaximumLength = 0;
    
    if (!lpLocalName || !lpnBufferLen)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (QueryDosDeviceW(lpLocalName, DosDevicePath, sizeof(DosDevicePath) / sizeof(WCHAR)) == 0)
    {
        return GetLastError();
    }
    
    Current = DosDevicePath;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    ConnectionName.Length = (USHORT)((Current - DosDevicePath) * sizeof(WCHAR));
    ConnectionName.MaximumLength = (USHORT)(ConnectionName.Length + 2);
    ConnectionName.Buffer = DosDevicePath;
    
    RdpDrPos = wcsstr(DosDevicePath, L"\\Device\\RdpDr");
    if (!RdpDrPos)
    {
        return ERROR_NO_NETWORK;
    }
    
    ErrorCode = OpenConnection(&ConnectionName, 0, 0, &FileInfo, 0, NULL);
    if (ErrorCode != ERROR_SUCCESS)
    {
        return ERROR_BAD_NET_NAME;
    }
    
    ErrorCode = DrDeviceControlGetInfo(FileInfo.EaName, 0, &EnumBuffer, 0, NULL, 0, 0, &BytesReturned);
    
    if (ErrorCode == ERROR_SUCCESS && EnumBuffer)
    {
        RemoteNameLength = *(USHORT*)((ULONG_PTR)EnumBuffer + 8);
        
        if (RemoteNameLength < *lpnBufferLen && lpRemoteName)
        {
            *lpnBufferLen = RemoteNameLength + 2;
            memcpy(lpRemoteName, (PUCHAR)EnumBuffer + 10, RemoteNameLength);
            lpRemoteName[RemoteNameLength / sizeof(WCHAR)] = L'\0';
            ErrorCode = ERROR_SUCCESS;
        }
        else
        {
            ErrorCode = ERROR_MORE_DATA;
            *lpnBufferLen = RemoteNameLength + 2;
        }
    }
    else
    {
        ErrorCode = ERROR_BAD_NET_NAME;
    }
    
    if (FileInfo.NextEntryOffset != 0xFFFFFFFF)
    {
        NtClose((HANDLE)FileInfo.NextEntryOffset);
    }
    
    if (EnumBuffer)
    {
        HeapFree(ProcessHeap, 0, EnumBuffer);
    }
    
    return ErrorCode;
}

ULONG NPGetConnectionPerformance(LPWSTR lpRemoteName, LPNETCONNECTINFOSTRUCT lpNetConnectInfo)
{
    if (!lpNetConnectInfo)
    {
        return ERROR_INVALID_ADDRESS;
    }
    
    if (IsRemoteNameTsclient(lpRemoteName))
    {
        return 0x495;
    }
    
    return 0x32;
}

ULONG NPGetResourceInformation(PNETRESOURCEW lpNetResource, LPVOID lpBuffer, 
                               LPDWORD lpBufferSize, LPWSTR* lplpSystem)
{
    WCHAR RemoteNameBuffer[520];
    WCHAR ProviderBuffer[520];
    UNICODE_STRING ConnectionName;
    FILE_FULL_EA_INFORMATION FileInfo;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ULONG ErrorCode = ERROR_SUCCESS;
    HANDLE ProcessHeap;
    PVOID EnumBuffer = NULL;
    ULONG BytesReturned = 0;
    SIZE_T RemoteNameLen = 0;
    SIZE_T ProviderNameLen = 0;
    SIZE_T TotalLength = 0;
    const wchar_t* Current = NULL;
    const wchar_t* ServerEnd = NULL;
    USHORT* EnumData;
    BOOL IsTsClient = FALSE;
    BOOL IsServerLevel = FALSE;
    DWORD BackslashCount = 0;
    
    ProcessHeap = GetProcessHeap();
    
    memset(&FileInfo, 0, sizeof(FileInfo));
    FileInfo.NextEntryOffset = 0xFFFFFFFF;
    
    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionName.MaximumLength = 0;
    
    if (!lpBuffer || !lpBufferSize)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (lpNetResource && lpNetResource->lpRemoteName)
    {
        IsTsClient = IsRemoteNameTsclient(lpNetResource->lpRemoteName);
        
        Current = lpNetResource->lpRemoteName;
        while (*Current != L'\0')
        {
            if (*Current == L'\\')
            {
                BackslashCount++;
                if (BackslashCount == 4)
                {
                    ServerEnd = Current;
                }
            }
            Current++;
        }
        
        if (BackslashCount >= 2)
        {
            RemoteNameLen = (ServerEnd - lpNetResource->lpRemoteName) * sizeof(WCHAR);
            IsServerLevel = TRUE;
        }
        else
        {
            Current = lpNetResource->lpRemoteName;
            while (*Current != L'\0')
            {
                Current++;
            }
            RemoteNameLen = (Current - lpNetResource->lpRemoteName) * sizeof(WCHAR);
        }
    }
    
    if (RemoteNameLen == 0)
    {
        if (*lpBufferSize < 0x20)
        {
            *lpBufferSize = 0x20;
            return ERROR_MORE_DATA;
        }
        
        memset(lpBuffer, 0, 0x20);
        ((DWORD*)lpBuffer)[0] = 2;
        ((DWORD*)lpBuffer)[2] = 6;
        ((DWORD*)lpBuffer)[3] = 2;
        *lpBufferSize = 0x20;
        
        if (lplpSystem)
        {
            *lplpSystem = NULL;
        }
        
        return ERROR_SUCCESS;
    }
    
    ConnectionName.Buffer = HeapAlloc(ProcessHeap, 0, RemoteNameLen + 2);
    if (!ConnectionName.Buffer)
    {
        return GetLastError();
    }
    
    ConnectionName.Length = (USHORT)RemoteNameLen;
    ConnectionName.MaximumLength = (USHORT)(RemoteNameLen + 2);
    
    if (IsServerLevel)
    {
        memcpy(ConnectionName.Buffer, lpNetResource->lpRemoteName, RemoteNameLen);
        ConnectionName.Buffer[RemoteNameLen / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        memcpy(ConnectionName.Buffer, lpNetResource->lpRemoteName, RemoteNameLen);
        ConnectionName.Buffer[RemoteNameLen / sizeof(WCHAR)] = L'\0';
        
        ErrorCode = DrOpenMiniRdr(&FileHandle);
        if (ErrorCode == ERROR_SUCCESS)
        {
            ErrorCode = DrDeviceControlGetInfo(&FileHandle, 0, &EnumBuffer, 0, NULL, 0, 0, &BytesReturned);
            
            if (ErrorCode == ERROR_SUCCESS && EnumBuffer)
            {
                EnumData = (USHORT*)EnumBuffer;
                if (RemoteNameLen == *EnumData && 
                    _wcsnicmp(ConnectionName.Buffer, (PWSTR)((ULONG_PTR)EnumBuffer + 4), 
                             RemoteNameLen / sizeof(WCHAR)) == 0)
                {
                    HeapFree(ProcessHeap, 0, EnumBuffer);
                    goto BuildResourceInfo;
                }
                HeapFree(ProcessHeap, 0, EnumBuffer);
            }
        }
        
        ErrorCode = ERROR_BAD_NET_NAME;
        goto Cleanup;
    }
    
BuildResourceInfo:
    ProviderNameLen = DrProviderName.Length;
    TotalLength = ProviderNameLen + 0x26 + RemoteNameLen + (IsServerLevel ? 0 : 0);
    
    *lpBufferSize = TotalLength;
    
    if (*lpBufferSize < TotalLength)
    {
        ErrorCode = ERROR_MORE_DATA;
        goto Cleanup;
    }
    
    memset(lpBuffer, 0, TotalLength);
    
    ((DWORD*)lpBuffer)[0] = 0;
    ((DWORD*)lpBuffer)[1] = IsServerLevel ? 1 : 0;
    ((DWORD*)lpBuffer)[2] = IsServerLevel ? 1 : 2;
    ((DWORD*)lpBuffer)[3] = IsServerLevel ? 3 : 2;
    ((DWORD*)lpBuffer)[5] = (DWORD)((ULONG_PTR)lpBuffer + 0x20);
    
    memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x20), ConnectionName.Buffer, RemoteNameLen);
    
    ((DWORD*)lpBuffer)[7] = (DWORD)((ULONG_PTR)lpBuffer + 0x20 + RemoteNameLen + 2);
    
    memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x20 + RemoteNameLen + 2), 
           DrProviderName.Buffer, ProviderNameLen);
    
    if (lplpSystem)
    {
        if (IsServerLevel && ServerEnd)
        {
            SIZE_T SystemLen = (Current - ServerEnd) * sizeof(WCHAR);
            *lplpSystem = (LPWSTR)((ULONG_PTR)lpBuffer + 0x20 + RemoteNameLen + 2 + ProviderNameLen + 2);
            memcpy(*lplpSystem, ServerEnd + 1, SystemLen);
            (*lplpSystem)[SystemLen / sizeof(WCHAR)] = L'\0';
        }
        else
        {
            *lplpSystem = NULL;
        }
    }
    
    ErrorCode = ERROR_SUCCESS;
    
Cleanup:
    if (ConnectionName.Buffer)
    {
        HeapFree(ProcessHeap, 0, ConnectionName.Buffer);
    }
    
    if (FileInfo.NextEntryOffset != 0xFFFFFFFF)
    {
        NtClose((HANDLE)FileInfo.NextEntryOffset);
    }
    
    if (ErrorCode != ERROR_SUCCESS && IsTsClient && !IsValidWNetReturn(ErrorCode))
    {
        ErrorCode = ERROR_INVALID_ADDRESS;
    }
    
    return ErrorCode;
}

ULONG NPGetResourceParent(PNETRESOURCEW lpNetResource, LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    WCHAR RemoteNameCopy[520];
    WCHAR* TsClientPos;
    WCHAR* BackslashPos;
    WCHAR* LastBackslash;
    SIZE_T RemoteNameLen;
    SIZE_T ProviderNameLen = 0;
    SIZE_T TotalLength;
    ULONG ErrorCode = ERROR_SUCCESS;
    HANDLE ProcessHeap;
    const wchar_t* Current;
    
    ProcessHeap = GetProcessHeap();
    
    if (!lpNetResource)
    {
        return 0x32;
    }
    
    if (IsPortDriveName(lpNetResource->lpLocalName))
    {
        return ERROR_BAD_DEVICE;
    }
    
    if (!lpNetResource->lpRemoteName)
    {
        return 0x32;
    }
    
    if (!IsRemoteNameTsclient(lpNetResource->lpRemoteName))
    {
        return 0x32;
    }
    
    RemoteNameLen = 0;
    Current = lpNetResource->lpRemoteName;
    while (*Current != L'\0')
    {
        Current++;
    }
    RemoteNameLen = (Current - lpNetResource->lpRemoteName) * sizeof(WCHAR);
    
    RemoteNameCopy[0] = L'\0';
    memcpy(RemoteNameCopy, lpNetResource->lpRemoteName + 1, RemoteNameLen - sizeof(WCHAR));
    RemoteNameCopy[(RemoteNameLen / sizeof(WCHAR)) - 1] = L'\0';
    
    TsClientPos = wcsstr(RemoteNameCopy, L"tsclient");
    if (!TsClientPos)
    {
        ErrorCode = ERROR_BAD_DEVICE;
        goto Cleanup;
    }
    
    if (TsClientPos[8] != L'\\')
    {
        ErrorCode = ERROR_BAD_DEVICE;
        goto Cleanup;
    }
    
    BackslashPos = wcschr(TsClientPos + 9, L'\\');
    LastBackslash = wcsrchr(RemoteNameCopy, L'\\');
    
    if (!BackslashPos)
    {
        if (*lpBufferSize < 0x36)
        {
            *lpBufferSize = 0x36;
            ErrorCode = ERROR_MORE_DATA;
            goto Cleanup;
        }
        
        memset(lpBuffer, 0, 0x36);
        
        ((DWORD*)lpBuffer)[1] = 1;
        ((DWORD*)lpBuffer)[2] = 2;
        ((DWORD*)lpBuffer)[3] = 3;
        ((DWORD*)lpBuffer)[5] = (DWORD)((ULONG_PTR)lpBuffer + 0x20);
        
        RemoteNameCopy[8] = L'\0';
        memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x20), L"tsclient", 16);
        
        ProviderNameLen = 0;
        Current = DrProviderName.Buffer;
        while (*Current != L'\0')
        {
            Current++;
        }
        ProviderNameLen = (Current - DrProviderName.Buffer) * sizeof(WCHAR);
        
        TotalLength = 0x36 + ProviderNameLen + 2;
        
        if (*lpBufferSize < TotalLength)
        {
            *lpBufferSize = TotalLength;
            ErrorCode = ERROR_MORE_DATA;
            goto Cleanup;
        }
        
        ((DWORD*)lpBuffer)[7] = (DWORD)((ULONG_PTR)lpBuffer + 0x36);
        memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x36), DrProviderName.Buffer, ProviderNameLen);
        *((WCHAR*)((ULONG_PTR)lpBuffer + 0x36 + ProviderNameLen)) = L'\0';
    }
    else
    {
        SIZE_T ParentPathLen = (LastBackslash - RemoteNameCopy) * sizeof(WCHAR);
        TotalLength = 0x22 + ParentPathLen + ProviderNameLen + 2;
        
        if (*lpBufferSize < TotalLength)
        {
            *lpBufferSize = TotalLength;
            ErrorCode = ERROR_MORE_DATA;
            goto Cleanup;
        }
        
        memset(lpBuffer, 0, TotalLength);
        
        ((DWORD*)lpBuffer)[1] = 1;
        ((DWORD*)lpBuffer)[2] = 9;
        ((DWORD*)lpBuffer)[3] = 3;
        ((DWORD*)lpBuffer)[5] = (DWORD)((ULONG_PTR)lpBuffer + 0x20);
        
        memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x20), RemoteNameCopy, ParentPathLen);
        *((WCHAR*)((ULONG_PTR)lpBuffer + 0x20 + ParentPathLen)) = L'\0';
        
        ProviderNameLen = 0;
        Current = DrProviderName.Buffer;
        while (*Current != L'\0')
        {
            Current++;
        }
        ProviderNameLen = (Current - DrProviderName.Buffer) * sizeof(WCHAR);
        
        ((DWORD*)lpBuffer)[7] = (DWORD)((ULONG_PTR)lpBuffer + 0x20 + ParentPathLen + 2);
        memcpy((PVOID)((ULONG_PTR)lpBuffer + 0x20 + ParentPathLen + 2), 
               DrProviderName.Buffer, ProviderNameLen);
        *((WCHAR*)((ULONG_PTR)lpBuffer + 0x20 + ParentPathLen + 2 + ProviderNameLen)) = L'\0';
    }
    
    ErrorCode = ERROR_SUCCESS;
    
Cleanup:
    return ErrorCode;
}

ULONG NPGetUniversalName(LPWSTR lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, 
                         LPDWORD lpBufferSize)
{
    WCHAR DrivePath[4];
    WCHAR RemoteName[520];
    DWORD RemoteNameLen;
    ULONG ErrorCode = ERROR_SUCCESS;
    SIZE_T LocalPathLen;
    SIZE_T RequiredSize;
    const wchar_t* Current;
    
    if (dwInfoLevel != 1 && dwInfoLevel != 2)
    {
        return 0x7C;
    }
    
    if (!lpLocalPath || !lpBuffer || !lpBufferSize)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    Current = lpLocalPath;
    while (*Current != L'\0')
    {
        Current++;
    }
    
    LocalPathLen = (Current - lpLocalPath) * sizeof(WCHAR);
    
    if (LocalPathLen < 4 || lpLocalPath[1] != L':' || (LocalPathLen > 4 && lpLocalPath[2] != L'\\'))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    DrivePath[0] = lpLocalPath[0];
    DrivePath[1] = L':';
    DrivePath[2] = L'\0';
    
    RemoteNameLen = sizeof(RemoteName);
    ErrorCode = NPGetConnection(DrivePath, RemoteName, &RemoteNameLen);
    
    if (ErrorCode == ERROR_SUCCESS)
    {
        Current = RemoteName;
        while (*Current != L'\0')
        {
            Current++;
        }
        
        SIZE_T RemoteNameStrLen = (Current - RemoteName) * sizeof(WCHAR);
        
        if (dwInfoLevel == 1)
        {
            RequiredSize = RemoteNameStrLen + LocalPathLen - 2 + 2;
            
            if (*lpBufferSize < RequiredSize)
            {
                *lpBufferSize = RequiredSize;
                return ERROR_MORE_DATA;
            }
            
            *(LPWSTR*)lpBuffer = (LPWSTR)((ULONG_PTR)lpBuffer + 4);
            memcpy(*(LPWSTR*)lpBuffer, RemoteName, RemoteNameStrLen);
            (*(LPWSTR*)lpBuffer)[RemoteNameStrLen / sizeof(WCHAR)] = L'\0';
            
            StringCbCatW(*(LPWSTR*)lpBuffer, *lpBufferSize - 4, lpLocalPath + 2);
        }
        else
        {
            SIZE_T UniversalNameLen = RemoteNameStrLen + 2;
            RequiredSize = 0xC + UniversalNameLen + LocalPathLen - 2 + 2;
            
            if (*lpBufferSize < RequiredSize)
            {
                *lpBufferSize = RequiredSize;
                return ERROR_MORE_DATA;
            }
            
            *(DWORD*)lpBuffer = (DWORD)((ULONG_PTR)lpBuffer + 0xC);
            *((DWORD*)lpBuffer + 1) = (DWORD)((ULONG_PTR)lpBuffer + 0xC + UniversalNameLen);
            *((DWORD*)lpBuffer + 2) = (DWORD)((ULONG_PTR)lpBuffer + 0xC + UniversalNameLen + 
                                             LocalPathLen - 2 + 2);
            
            memcpy((PVOID)((ULONG_PTR)lpBuffer + 0xC), RemoteName, RemoteNameStrLen);
            *((WCHAR*)((ULONG_PTR)lpBuffer + 0xC + RemoteNameStrLen)) = L'\0';
            
            memcpy((PVOID)((ULONG_PTR)lpBuffer + 0xC + UniversalNameLen), 
                   lpLocalPath + 2, LocalPathLen - 2);
            *((WCHAR*)((ULONG_PTR)lpBuffer + 0xC + UniversalNameLen + LocalPathLen - 2)) = L'\0';
        }
        
        ErrorCode = ERROR_SUCCESS;
    }
    else if (ErrorCode == ERROR_BAD_NET_NAME)
    {
        ErrorCode = ERROR_BAD_DEVICE;
    }
    
    return ErrorCode;
}

ULONG NPOpenEnum(DWORD dwScope, DWORD dwType, DWORD dwUsage, PNETRESOURCEW lpNetResource, 
                 LPHANDLE lphEnum)
{
    BOOL IsSessionRemoteable = FALSE;
    BOOL IsRemoteable = FALSE;
    DWORD* EnumHandle;
    HANDLE ProcessHeap;
    const wchar_t* RemoteNameCurrent;
    SIZE_T RemoteNameLen;
    WCHAR* RemoteNameCopy;
    
    if (!lphEnum)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    *lphEnum = NULL;
    
    if (!WinStationIsSessionRemoteable(0, 0xFFFFFFFF, &IsRemoteable) || !IsRemoteable)
    {
        if (dwScope != 2)
        {
            return 0x32;
        }
        
        if (!lpNetResource)
        {
            return 0x32;
        }
        
        return 0x32;
    }
    
    ProcessHeap = GetProcessHeap();
    EnumHandle = HeapAlloc(ProcessHeap, 0, 0x24);
    
    if (!EnumHandle)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    memset(EnumHandle, 0, 0x24);
    
    if (dwScope == 1)
    {
        if (!lpNetResource)
        {
            EnumHandle[0] = 1;
            EnumHandle[1] = dwType;
            EnumHandle[2] = dwUsage;
            EnumHandle[5] = 2;
            EnumHandle[6] = 0;
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
    }
    else if (dwScope == 6)
    {
        if (!lpNetResource || !lpNetResource->lpRemoteName || 
            lpNetResource->lpRemoteName[0] != L'\\' || lpNetResource->lpRemoteName[1] != L'\\')
        {
            EnumHandle[0] = 6;
            EnumHandle[1] = dwType;
            EnumHandle[2] = dwUsage;
            EnumHandle[5] = 1;
            EnumHandle[6] = 0;
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
        
        if (!ValidateRemoteName(lpNetResource->lpRemoteName))
        {
            HeapFree(ProcessHeap, 0, EnumHandle);
            return 0x32;
        }
        
        EnumHandle[0] = 6;
        EnumHandle[1] = dwType;
        EnumHandle[2] = dwUsage;
        EnumHandle[5] = 1;
        EnumHandle[6] = 0;
        
        RemoteNameCurrent = lpNetResource->lpRemoteName;
        while (*RemoteNameCurrent != L'\0')
        {
            RemoteNameCurrent++;
        }
        
        RemoteNameLen = (RemoteNameCurrent - (lpNetResource->lpRemoteName + 1)) * sizeof(WCHAR) + 2;
        *(USHORT*)((ULONG_PTR)EnumHandle + 0xE) = (USHORT)RemoteNameLen;
        
        RemoteNameCopy = HeapAlloc(ProcessHeap, 0, RemoteNameLen);
        EnumHandle[4] = (DWORD)RemoteNameCopy;
        
        if (!RemoteNameCopy)
        {
            HeapFree(ProcessHeap, 0, EnumHandle);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        *(USHORT*)((ULONG_PTR)EnumHandle + 0xC) = (USHORT)(RemoteNameLen - 2);
        StringCbCopyW(RemoteNameCopy, RemoteNameLen, lpNetResource->lpRemoteName + 1);
        
        *lphEnum = EnumHandle;
        return ERROR_SUCCESS;
    }
    else if (dwScope == 2)
    {
        DWORD ValidUsage = dwUsage & 3;
        
        if (!lpNetResource || !lpNetResource->lpRemoteName)
        {
            EnumHandle[0] = 2;
            EnumHandle[6] = 0;
            EnumHandle[1] = dwType;
            
            if (ValidUsage == 1)
            {
                EnumHandle[2] = 1;
                EnumHandle[5] = 3;
            }
            else
            {
                EnumHandle[2] = ValidUsage;
                EnumHandle[5] = 0;
            }
            
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
        
        if ((ValidUsage == 1 || ValidUsage == 0) && 
            lpNetResource->lpRemoteName[0] == L'\\' && lpNetResource->lpRemoteName[1] == L'\\')
        {
            if (!ValidateRemoteName(lpNetResource->lpRemoteName))
            {
                HeapFree(ProcessHeap, 0, EnumHandle);
                return 0x32;
            }
            
            EnumHandle[2] = ValidUsage;
            EnumHandle[0] = 2;
            EnumHandle[1] = dwType;
            EnumHandle[5] = 1;
            EnumHandle[6] = 0;
            
            RemoteNameCurrent = lpNetResource->lpRemoteName;
            while (*RemoteNameCurrent != L'\0')
            {
                RemoteNameCurrent++;
            }
            
            RemoteNameLen = (RemoteNameCurrent - (lpNetResource->lpRemoteName + 1)) * sizeof(WCHAR) + 2;
            *(USHORT*)((ULONG_PTR)EnumHandle + 0xE) = (USHORT)RemoteNameLen;
            
            RemoteNameCopy = HeapAlloc(ProcessHeap, 0, RemoteNameLen);
            EnumHandle[4] = (DWORD)RemoteNameCopy;
            
            if (!RemoteNameCopy)
            {
                HeapFree(ProcessHeap, 0, EnumHandle);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            
            *(USHORT*)((ULONG_PTR)EnumHandle + 0xC) = (USHORT)(RemoteNameLen - 2);
            StringCbCopyW(RemoteNameCopy, RemoteNameLen, lpNetResource->lpRemoteName + 1);
            
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
        
        if ((ValidUsage == 2 || ValidUsage == 0) && lpNetResource->lpRemoteName[0] != L'\\')
        {
            EnumHandle[0] = 2;
            EnumHandle[1] = dwType;
            EnumHandle[2] = ValidUsage;
            EnumHandle[5] = 3;
            EnumHandle[6] = 0;
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
        
        if (ValidUsage == 1 && lpNetResource->lpRemoteName[0] != L'\\')
        {
            EnumHandle[0] = 2;
            EnumHandle[1] = dwType;
            EnumHandle[2] = ValidUsage;
            EnumHandle[5] = 3;
            EnumHandle[6] = 0;
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
        
        if (ValidUsage == 2 && lpNetResource->lpRemoteName[0] == L'\\' && 
            lpNetResource->lpRemoteName[1] == L'\\')
        {
            EnumHandle[0] = 2;
            EnumHandle[1] = dwType;
            EnumHandle[2] = ValidUsage;
            EnumHandle[5] = 3;
            EnumHandle[6] = 0;
            *lphEnum = EnumHandle;
            return ERROR_SUCCESS;
        }
    }
    
    HeapFree(ProcessHeap, 0, EnumHandle);
    *lphEnum = NULL;
    return ERROR_INVALID_PARAMETER;
}

ULONG OpenConnection(
    PUNICODE_STRING ConnectionName,
    DWORD DesiredAccess,
    DWORD ShareMode,
    PFILE_FULL_EA_INFORMATION FileInfo,
    DWORD CreateOptions,
    PVOID EaBuffer
)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    
    ULONG ErrorCode = ValidateConnectionName(ConnectionName);
    if (ErrorCode != ERROR_SUCCESS)
    {
        return ErrorCode;
    }
    
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = ConnectionName;
    ObjectAttributes.Attributes = 0x40;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;
    
    Status = NtCreateFile((PHANDLE)&FileInfo->NextEntryOffset, 0x100001, &ObjectAttributes, 
                         &IoStatusBlock, 0, 0x80, 7, 1, 0xA0, 0, 0);
    
    if (Status != 0)
    {
        return ERROR_BAD_NET_NAME;
    }
    
    return ERROR_SUCCESS;
}

ULONG ValidateConnectionName(PUNICODE_STRING ConnectionName)
{
    if (!ConnectionName || !ConnectionName->Buffer || ConnectionName->Length < 0x1A)
    {
        return ERROR_BAD_NET_NAME;
    }
    
    if (_wcsnicmp(ConnectionName->Buffer, L"\\Device\\RdpDr", 13) != 0)
    {
        return ERROR_BAD_NET_NAME;
    }
    
    return ERROR_SUCCESS;
}

ULONG ValidateRemoteName(LPCWSTR RemoteName)
{
    HANDLE FileHandle = NULL;
    ULONG ErrorCode = ERROR_SUCCESS;
    PVOID Buffer = NULL;
    ULONG BytesReturned = 0;
    USHORT* EnumData;
    SIZE_T RemoteNameLen;
    const wchar_t* Current;
    
    if (!RemoteName)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    ErrorCode = DrOpenMiniRdr(&FileHandle);
    if (ErrorCode != ERROR_SUCCESS)
    {
        return ErrorCode;
    }
    
    ErrorCode = DrDeviceControlGetInfo(&FileHandle, 0, &Buffer, 0, NULL, 0, 0, &BytesReturned);
    
    if (ErrorCode == ERROR_SUCCESS && Buffer)
    {
        EnumData = (USHORT*)Buffer;
        
        Current = RemoteName + 1;
        while (*Current != L'\0')
        {
            Current++;
        }
        RemoteNameLen = (Current - (RemoteName + 1)) * sizeof(WCHAR);
        
        if (RemoteNameLen == *EnumData)
        {
            if (_wcsnicmp(RemoteName + 1, (PWSTR)((ULONG_PTR)Buffer + 4), 
                         RemoteNameLen / sizeof(WCHAR)) == 0)
            {
                ErrorCode = ERROR_SUCCESS;
            }
            else
            {
                ErrorCode = ERROR_BAD_NET_NAME;
            }
        }
        else
        {
            ErrorCode = ERROR_BAD_NET_NAME;
        }
        
        HeapFree(GetProcessHeap(), 0, Buffer);
    }
    else
    {
        ErrorCode = ERROR_BAD_NET_NAME;
    }
    
    if (FileHandle)
    {
        CloseHandle(FileHandle);
    }
    
    return ErrorCode;
}

