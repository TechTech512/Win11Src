#pragma warning (disable:4005)

#include <windows.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <strsafe.h>
#include <winternl.h>
#include <ntstatus.h>

// Global variables
CRITICAL_SECTION ConfigInfoLock;
HKEY NetbiosKey = NULL;
LARGE_INTEGER NetbiosUpdateTime;
DWORD ProviderCount = 0;
PVOID ConfigInfo = NULL;
PVOID ProviderInfo = NULL;

GUID NetBIOSProviderGuid = {0xAD5DC15D, 0xE7F1, 0x44C3, {0xB5, 0x95, 0x71, 0x50, 0x9C, 0x74, 0x3D, 0x6E}};

// Structures
typedef struct _WSHNETBS_CONFIG_INFO {
    DWORD ReferenceCount;
    BYTE Blob[1];
} WSHNETBS_CONFIG_INFO, *PWSHNETBS_CONFIG_INFO;

typedef struct _WSHNETBS_PROVIDER_INFO {
    BYTE Enum;
    BYTE LanaNumber;
    DWORD ProtocolNumber;
    LPWSTR ProviderName;
} WSHNETBS_PROVIDER_INFO, *PWSHNETBS_PROVIDER_INFO;

typedef struct _SOCKADDR_INFO {
    DWORD AddressInfo;
} SOCKADDR_INFO, *PSOCKADDR_INFO;

typedef struct _WINSOCK_MAPPING {
    DWORD Rows;
    DWORD Columns;
    DWORD Mapping[3];
} WINSOCK_MAPPING, *PWINSOCK_MAPPING;

typedef NTSTATUS (NTAPI *PNT_CREATE_FILE)(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
);

typedef NTSTATUS (NTAPI *PNT_DEVICE_IO_CONTROL_FILE)(
    HANDLE FileHandle,
    HANDLE Event,
    PVOID ApcRoutine,
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG IoControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

typedef NTSTATUS (NTAPI *PNT_CLOSE)(
    HANDLE Handle
);

DWORD SockaddrAddressInfoNormal = 0x200000;
DWORD VcMappingTriples[] = {0, 0, 0xFFFF};
DWORD DgMappingTriples[] = {0, 0, 0xFFFF};

DWORD LoadProviderInfo(void);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (lpvReserved == NULL) {
            if (ConfigInfo != NULL) {
                EnterCriticalSection(&ConfigInfoLock);
                PWSHNETBS_CONFIG_INFO pConfig = (PWSHNETBS_CONFIG_INFO)ConfigInfo;
                pConfig->ReferenceCount--;
                LeaveCriticalSection(&ConfigInfoLock);
                if (pConfig->ReferenceCount == 0) {
                    HeapFree(GetProcessHeap(), 0, ConfigInfo);
                }
                ConfigInfo = NULL;
            }
            if (NetbiosKey != NULL) {
                RegCloseKey(NetbiosKey);
                NetbiosKey = NULL;
            }
            DeleteCriticalSection(&ConfigInfoLock);
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH) {
        if (InitializeCriticalSectionAndSpinCount(&ConfigInfoLock, 0) == 0) {
            return FALSE;
        }
        LoadProviderInfo();
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

DWORD LoadProviderInfo(void)
{
    HKEY hKey = NULL;
    PWSHNETBS_CONFIG_INFO pConfig = NULL;
    DWORD dwError = 0;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwValueSize = 0;
    DWORD dwLanaSize = 0;
    
    if (NetbiosKey == NULL) {
        dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               L"SYSTEM\\CurrentControlSet\\Services\\Netbios\\Linkage",
                               0,
                               KEY_READ,
                               &hKey);
        if (dwError != ERROR_SUCCESS) {
            goto cleanup;
        }
    }
    else {
        hKey = NetbiosKey;
    }
    
    FILETIME ftLastWrite;
    dwError = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &ftLastWrite);
    if (dwError == ERROR_SUCCESS) {
        LARGE_INTEGER liTime;
        liTime.LowPart = ftLastWrite.dwLowDateTime;
        liTime.HighPart = ftLastWrite.dwHighDateTime;
        
        if (NetbiosKey != NULL && 
            liTime.LowPart == NetbiosUpdateTime.LowPart && 
            liTime.HighPart == NetbiosUpdateTime.HighPart) {
            dwError = ERROR_SUCCESS;
            goto cleanup;
        }
        
        dwSize = 0;
        dwError = RegQueryValueExW(hKey, L"Bind", NULL, &dwType, NULL, &dwSize);
        if ((dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA) && dwSize > 2) {
            dwLanaSize = 0;
            dwError = RegQueryValueExW(hKey, L"LanaMap", NULL, &dwType, NULL, &dwLanaSize);
            if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA) {
                DWORD dwTotalSize = sizeof(WSHNETBS_CONFIG_INFO) + 
                                   ((dwSize + 3) & ~3) + 
                                   ((dwLanaSize + 3) & ~3) +
                                   (dwLanaSize / 2) * sizeof(WSHNETBS_PROVIDER_INFO);
                
                pConfig = (PWSHNETBS_CONFIG_INFO)HeapAlloc(GetProcessHeap(), 0, dwTotalSize);
                if (pConfig != NULL) {
                    pConfig->ReferenceCount = 1;
                    PBYTE pBindData = pConfig->Blob;
                    PBYTE pLanaData = pBindData + ((dwSize + 3) & ~3);
                    PWSHNETBS_PROVIDER_INFO pProvider = (PWSHNETBS_PROVIDER_INFO)(pLanaData + ((dwLanaSize + 3) & ~3));
                    
                    dwValueSize = dwSize;
                    dwError = RegQueryValueExW(hKey, L"Bind", NULL, &dwType, pBindData, &dwValueSize);
                    if (dwError == ERROR_SUCCESS) {
                        dwValueSize = dwLanaSize;
                        dwError = RegQueryValueExW(hKey, L"LanaMap", NULL, &dwType, pLanaData, &dwValueSize);
                        if (dwError == ERROR_SUCCESS) {
                            LPWSTR pBindStr = (LPWSTR)pBindData;
                            PBYTE pLanaPtr = pLanaData;
                            DWORD dwProviderCount = dwLanaSize / 2;
                            DWORD i = 0;
                            
                            while (*pBindStr != L'\0' && i < dwProviderCount) {
                                pProvider[i].Enum = pLanaPtr[0];
                                pProvider[i].LanaNumber = pLanaPtr[1];
                                pProvider[i].ProtocolNumber = (DWORD)pLanaPtr[1];
                                pProvider[i].ProviderName = pBindStr;
                                
                                while (*pBindStr != L'\0') {
                                    pBindStr++;
                                }
                                pBindStr++;
                                
                                pLanaPtr += 2;
                                i++;
                            }
                            
                            EnterCriticalSection(&ConfigInfoLock);
                            if (ConfigInfo != NULL) {
                                PWSHNETBS_CONFIG_INFO pOldConfig = (PWSHNETBS_CONFIG_INFO)ConfigInfo;
                                pOldConfig->ReferenceCount--;
                                if (pOldConfig->ReferenceCount == 0) {
                                    HeapFree(GetProcessHeap(), 0, pOldConfig);
                                }
                            }
                            
                            if (NetbiosKey == NULL) {
                                NetbiosKey = hKey;
                                hKey = NULL;
                            }
                            
                            NetbiosUpdateTime.LowPart = ftLastWrite.dwLowDateTime;
                            NetbiosUpdateTime.HighPart = ftLastWrite.dwHighDateTime;
                            ProviderInfo = pProvider;
                            ProviderCount = i;
                            ConfigInfo = pConfig;
                            LeaveCriticalSection(&ConfigInfoLock);
                            
                            if (hKey != NULL && hKey != NetbiosKey) {
                                RegCloseKey(hKey);
                            }
                            return ERROR_SUCCESS;
                        }
                    }
                }
                else {
                    dwError = ERROR_OUTOFMEMORY;
                }
            }
        }
    }
    
cleanup:
    if (hKey != NULL && hKey != NetbiosKey) {
        RegCloseKey(hKey);
    }
    if (pConfig != NULL) {
        HeapFree(GetProcessHeap(), 0, pConfig);
    }
    return dwError;
}

INT WSAAPI WSHEnumProtocols(LPINT lpiProtocols, LPWSTR lpProtocolBuffer, LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    if (lpiProtocols == NULL) {
        DWORD dwError = LoadProviderInfo();
        PWSHNETBS_PROVIDER_INFO pProvider = (PWSHNETBS_PROVIDER_INFO)ProviderInfo;
        
        if (dwError == ERROR_SUCCESS) {
            DWORD dwRequiredSize = 0;
            
            for (DWORD i = 0; i < ProviderCount; i++) {
                LPWSTR pName = pProvider[i].ProviderName;
                while (*pName != L'\0') pName++;
                dwRequiredSize += ((pName - pProvider[i].ProviderName + 1) * sizeof(WCHAR) * 2) + 0x44;
            }
            
            if (*lpdwBufferLength < dwRequiredSize) {
                *lpdwBufferLength = dwRequiredSize;
                return WSAENOBUFS;
            }
            
            DWORD dwOffset = *lpdwBufferLength;
            PBYTE pBuffer = (PBYTE)lpBuffer;
            
            for (DWORD i = 0; i < ProviderCount; i += 2) {
                WSAPROTOCOL_INFOW* pInfo = (WSAPROTOCOL_INFOW*)pBuffer;
                
                // First protocol
                pInfo->dwServiceFlags1 = 0x100E;
                pInfo->dwServiceFlags2 = 0x11;
                pInfo->dwServiceFlags3 = 0x14;
                pInfo->dwServiceFlags4 = 0x14;
                pInfo->dwProviderFlags = 5;
                pInfo->ProviderId = NetBIOSProviderGuid;
                pInfo->dwCatalogEntryId = (DWORD)pProvider[i].LanaNumber;
                if (pInfo->dwCatalogEntryId == 0) {
                    pInfo->dwCatalogEntryId = 0x80000000;
                }
                pInfo->ProtocolChain.ChainLen = 1;
                pInfo->iVersion = 0;
                pInfo->iAddressFamily = AF_NETBIOS;
                pInfo->iMaxSockAddr = 0x14;
                pInfo->iMinSockAddr = 0x14;
                pInfo->iSocketType = SOCK_SEQPACKET;
                pInfo->iProtocol = pProvider[i].ProtocolNumber;
                pInfo->iProtocolMaxOffset = 0;
                pInfo->iNetworkByteOrder = BIGENDIAN;
                pInfo->iSecurityScheme = SECURITY_PROTOCOL_NONE;
                pInfo->dwMessageSize = 64000;
                pInfo->dwProviderReserved = 0;
                
                LPWSTR pName = pProvider[i].ProviderName;
                dwOffset -= (wcslen(pName) + 1) * sizeof(WCHAR);
                *((LPWSTR*)((PBYTE)pInfo + FIELD_OFFSET(WSAPROTOCOL_INFOW, szProtocol))) = (WCHAR*)dwOffset;
                StringCbCopyW((WCHAR*)dwOffset, *lpdwBufferLength - dwOffset, pName);
                
                // Second protocol (if exists)
                if (i + 1 < ProviderCount) {
                    pInfo++;
                    
                    pInfo->dwServiceFlags1 = 0x1209;
                    pInfo->dwServiceFlags2 = 0x11;
                    pInfo->dwServiceFlags3 = 0x14;
                    pInfo->dwServiceFlags4 = 0x14;
                    pInfo->dwProviderFlags = 2;
                    pInfo->ProviderId = NetBIOSProviderGuid;
                    pInfo->dwCatalogEntryId = (DWORD)pProvider[i + 1].LanaNumber;
                    if (pInfo->dwCatalogEntryId == 0) {
                        pInfo->dwCatalogEntryId = 0x80000000;
                    }
                    pInfo->ProtocolChain.ChainLen = 1;
                    pInfo->iVersion = 0;
                    pInfo->iAddressFamily = AF_NETBIOS;
                    pInfo->iMaxSockAddr = 0x14;
                    pInfo->iMinSockAddr = 0x14;
                    pInfo->iSocketType = SOCK_DGRAM;
                    pInfo->iProtocol = pProvider[i + 1].ProtocolNumber;
                    pInfo->iProtocolMaxOffset = 0;
                    pInfo->iNetworkByteOrder = BIGENDIAN;
                    pInfo->iSecurityScheme = SECURITY_PROTOCOL_NONE;
                    pInfo->dwMessageSize = 64000;
                    pInfo->dwProviderReserved = 0;
                    
                    pName = pProvider[i + 1].ProviderName;
                    dwOffset -= (wcslen(pName) + 1) * sizeof(WCHAR);
                    *((LPWSTR*)((PBYTE)pInfo + FIELD_OFFSET(WSAPROTOCOL_INFOW, szProtocol))) = (WCHAR*)dwOffset;
                    StringCbCopyW((WCHAR*)dwOffset, *lpdwBufferLength - dwOffset, pName);
                }
                
                pBuffer += sizeof(WSAPROTOCOL_INFOW);
            }
            
            *lpdwBufferLength = dwRequiredSize;
            return ProviderCount;
        }
    }
    
    *lpdwBufferLength = 0;
    return WSAENOBUFS;
}

INT WSAAPI WSHGetProviderGuid(LPWSTR lpProviderName, LPGUID lpProviderGuid)
{
    if (lpProviderName == NULL || lpProviderGuid == NULL) {
        return WSAEINVAL;
    }
    
    if (_wcsicmp(lpProviderName, L"NetBIOS") == 0) {
        *lpProviderGuid = NetBIOSProviderGuid;
        return ERROR_SUCCESS;
    }
    
    return WSAEINVAL;
}

INT WSAAPI WSHGetSockaddrType(PSOCKADDR pSockaddr, DWORD dwSockaddrLength, PSOCKADDR_INFO pSockaddrInfo)
{
    if (pSockaddr->sa_family == AF_NETBIOS) {
        if (dwSockaddrLength < 0x14) {
            return WSAEINVAL;
        }
        pSockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
        return ERROR_SUCCESS;
    }
    return WSAEINVAL;
}

INT WSAAPI WSHGetSocketInformation(
    HANDLE hAsyncTask,
    DWORD dwCatalogEntryId,
    PVOID lpSocketInformation,
    PVOID lpSocketContext,
    INT iSocketInformationType,
    INT iSocketInformationSize,
    PCHAR lpSocketInformationData,
    LPINT lpiSocketInformationDataLength)
{
    if (iSocketInformationType == 0xFFFE && iSocketInformationSize == 1) {
        if (lpSocketInformationData != NULL) {
            if ((DWORD)*lpiSocketInformationDataLength < 0x14) {
                return WSAEINVAL;
            }
            for (int i = 0; i < 5; i++) {
                ((DWORD*)lpSocketInformationData)[i] = ((DWORD*)lpSocketInformation)[i];
            }
        }
        *lpiSocketInformationDataLength = 0x14;
        return ERROR_SUCCESS;
    }
    return WSAEINVAL;
}

INT WSAAPI WSHGetWildcardSockaddr(PVOID pvSocketContext, PSOCKADDR pSockaddr, LPINT piSockaddrLength)
{
    ZeroMemory(pSockaddr, *piSockaddrLength);
    pSockaddr->sa_family = AF_NETBIOS;
    *piSockaddrLength = 0x14;
    
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        PNT_CREATE_FILE NtCreateFile = (PNT_CREATE_FILE)GetProcAddress(hNtdll, "NtCreateFile");
        PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
        PNT_CLOSE NtClose = (PNT_CLOSE)GetProcAddress(hNtdll, "NtClose");
        
        if (NtCreateFile && NtDeviceIoControlFile && NtClose) {
            UNICODE_STRING usFileName;
            OBJECT_ATTRIBUTES ObjectAttributes;
            IO_STATUS_BLOCK IoStatusBlock;
            HANDLE hFile = NULL;
            
            RtlInitUnicodeString(&usFileName, L"\\Device\\Netbios");
            
            InitializeObjectAttributes(&ObjectAttributes,
                                      &usFileName,
                                      OBJ_CASE_INSENSITIVE,
                                      NULL,
                                      NULL);
            
            NTSTATUS status = NtCreateFile(&hFile,
                                          GENERIC_READ | GENERIC_WRITE,
                                          &ObjectAttributes,
                                          &IoStatusBlock,
                                          NULL,
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          FILE_OPEN_IF,
                                          0,
                                          NULL,
                                          0);
            
            if (NT_SUCCESS(status)) {
                struct {
                    ULONG Unknown[6];
                    ULONG BufferSize;
                } InputBuffer = {0};
                
                struct {
                    CHAR Data[6];
                } OutputBuffer;
                
                InputBuffer.BufferSize = 0x100;
                
                status = NtDeviceIoControlFile(hFile,
                                              NULL,
                                              NULL,
                                              NULL,
                                              &IoStatusBlock,
                                              0x210012,
                                              &InputBuffer,
                                              sizeof(InputBuffer),
                                              &OutputBuffer,
                                              sizeof(OutputBuffer));
                
                if (status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW) {
                    NtClose(hFile);
                    CopyMemory(pSockaddr->sa_data + 12, &OutputBuffer.Data, 6);
                    return ERROR_SUCCESS;
                }
                NtClose(hFile);
            }
        }
    }
    
    return WSAENETDOWN;
}

DWORD WSAAPI WSHGetWinsockMapping(PWINSOCK_MAPPING pMapping, DWORD dwMappingLength)
{
    if (dwMappingLength >= 0x20) {
        pMapping->Rows = 2;
        pMapping->Columns = 3;
        CopyMemory(pMapping->Mapping, VcMappingTriples, sizeof(VcMappingTriples));
        CopyMemory((PBYTE)pMapping + sizeof(DWORD) * 2, DgMappingTriples, sizeof(DgMappingTriples));
    }
    return 0x20;
}

INT WSAAPI WSHNotify(PVOID pvSocketContext, DWORD dwCatalogEntryId, PVOID lpNotificationData, PVOID lpNotificationContext, DWORD dwNotificationFlags)
{
    if (dwNotificationFlags == 0x80) {
        EnterCriticalSection(&ConfigInfoLock);
        PWSHNETBS_CONFIG_INFO pConfig = *(PWSHNETBS_CONFIG_INFO*)((PBYTE)pvSocketContext + 0x10);
        pConfig->ReferenceCount--;
        LeaveCriticalSection(&ConfigInfoLock);
        if (pConfig->ReferenceCount == 0) {
            HeapFree(GetProcessHeap(), 0, pConfig);
        }
        HeapFree(GetProcessHeap(), 0, pvSocketContext);
        return ERROR_SUCCESS;
    }
    return WSAEINVAL;
}

INT WSAAPI WSHOpenSocket(LPINT AddressFamily, LPINT SocketType, LPINT Protocol, PUNICODE_STRING pFileName, PVOID *ppSocketContext, PDWORD pdwNotificationFlags)
{
    if (*SocketType == SOCK_SEQPACKET || *SocketType == SOCK_DGRAM) {
        DWORD dwError = LoadProviderInfo();
        if (dwError == ERROR_SUCCESS) {
            PVOID pContext = HeapAlloc(GetProcessHeap(), 0, 0x14);
            if (pContext == NULL) {
                return WSAENOBUFS;
            }
            
            if (*Protocol == 0x80000000) {
                *Protocol = 0;
            }
            
            EnterCriticalSection(&ConfigInfoLock);
            BOOL bFound = FALSE;
            DWORD dwIndex = 0;
            PWSHNETBS_PROVIDER_INFO pProvider = (PWSHNETBS_PROVIDER_INFO)ProviderInfo;
            
            for (DWORD i = 0; i < ProviderCount; i++) {
                if (*Protocol < 1) {
                    if ((DWORD)pProvider[i].LanaNumber == *Protocol && pProvider[i].Enum == 0) {
                        dwIndex = i;
                        bFound = TRUE;
                        break;
                    }
                } else {
                    if (pProvider[i].ProtocolNumber == *Protocol && pProvider[i].ProtocolNumber != 0) {
                        dwIndex = i;
                        bFound = TRUE;
                        break;
                    }
                }
            }
            
            if (bFound) {
                PWSHNETBS_CONFIG_INFO pConfig = (PWSHNETBS_CONFIG_INFO)ConfigInfo;
                pConfig->ReferenceCount++;
                *(DWORD*)((PBYTE)pContext + 0x10) = (DWORD)pConfig;
                *(DWORD*)((PBYTE)pContext + 0xC) = (DWORD)(pProvider + dwIndex);
                
                RtlInitUnicodeString(pFileName, pProvider[dwIndex].ProviderName);
                *(DWORD*)pContext = *AddressFamily;
                *(DWORD*)((PBYTE)pContext + 4) = *SocketType;
                *(DWORD*)((PBYTE)pContext + 8) = *Protocol;
                *pdwNotificationFlags = 0x80;
                *ppSocketContext = pContext;
                dwError = ERROR_SUCCESS;
            } else {
                HeapFree(GetProcessHeap(), 0, pContext);
                dwError = WSAENETDOWN;
            }
            
            LeaveCriticalSection(&ConfigInfoLock);
            return dwError;
        } else if (dwError != ERROR_OUTOFMEMORY) {
            return WSAENETDOWN;
        }
    }
    return WSAESOCKTNOSUPPORT;
}

INT WSAAPI WSHSetSocketInformation(
    HANDLE hAsyncTask,
    DWORD dwCatalogEntryId,
    PVOID lpSocketInformation,
    PVOID lpSocketContext,
    INT iSocketInformationType,
    INT iSocketInformationSize,
    PCHAR lpSocketInformationData,
    INT iSocketInformationDataLength)
{
    if (iSocketInformationType == 0xFFFE && iSocketInformationSize == 1 && iSocketInformationDataLength >= 0x14) {
        if (lpSocketInformation == NULL) {
            PVOID pBuffer = HeapAlloc(GetProcessHeap(), 0, 0x14);
            if (pBuffer == NULL) {
                return WSAENOBUFS;
            }
            CopyMemory(pBuffer, lpSocketInformationData, 0x14);
            *(PVOID*)lpSocketInformationData = pBuffer;
        }
        return ERROR_SUCCESS;
    }
    return WSAEINVAL;
}

