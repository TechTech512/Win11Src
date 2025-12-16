#pragma warning (disable:4005)

#include <windows.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <strsafe.h>
#include <winternl.h>
#include <ntstatus.h>

// Global variables
CRITICAL_SECTION IrdaCs;
int CsInitialized = 0;
GUID IrdaProviderGuid = {0x3972523D, 0x2AF1, 0x11D1, {0xB6, 0x55, 0x00, 0x80, 0x5F, 0x36, 0x42, 0xCC}};

// Structures
typedef struct _SOCKADDR_INFO {
    DWORD AddressInfo;
    DWORD EndpointInfo;
} SOCKADDR_INFO, *PSOCKADDR_INFO;

typedef struct _WINSOCK_MAPPING {
    DWORD Rows;
    DWORD Columns;
    DWORD Mapping[3];
} WINSOCK_MAPPING, *PWINSOCK_MAPPING;

// Constants
DWORD SockaddrAddressInfoWildcard = 0x400000;
DWORD SockaddrEndpointInfoWildcard = 0x400000;
DWORD SockaddrAddressInfoNormal = 0x200000;
DWORD SockaddrEndpointInfoNormal = 0x200000;

WINSOCK_MAPPING IrDAMapping = {
    3,      // Rows
    3,      // Columns
    {0x1A, 0, 0xFFFF}  // Mapping
};

WSAPROTOCOL_INFOW Winsock2Protocols[1] = {0};

// Function prototypes
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

typedef void (NTAPI *PIO_APC_ROUTINE)(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
);

int IoStatusToWs(LONG status);

// Helper functions
int ControlIoctl(DWORD param1, char* param2, int* param3, void** param4)
{
    UNICODE_STRING usFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile = NULL;
    
    RtlInitUnicodeString(&usFileName, L"\\Device\\IrDA");
    
    if (param2 != NULL) {
        param2[0] = (char)0xFF;
        param2[1] = (char)0xFF;
        param2[2] = (char)0xFF;
        param2[3] = (char)0xFF;
    }
    
    InitializeObjectAttributes(&ObjectAttributes,
                              &usFileName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);
    
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return 0x2726;
    }
    
    PNT_CREATE_FILE NtCreateFile = (PNT_CREATE_FILE)GetProcAddress(hNtdll, "NtCreateFile");
    PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
    PNT_CLOSE NtClose = (PNT_CLOSE)GetProcAddress(hNtdll, "NtClose");
    
    if (!NtCreateFile || !NtDeviceIoControlFile || !NtClose) {
        return 0x2726;
    }
    
    NTSTATUS status = NtCreateFile(&hFile,
                                   GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                   &ObjectAttributes,
                                   &IoStatusBlock,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_OPEN_IF,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0);
    
    if (!NT_SUCCESS(status)) {
        return IoStatusToWs(status);
    }
    
    DWORD dwOutput = 0;
    status = NtDeviceIoControlFile(hFile,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   0x120010,
                                   NULL,
                                   0,
                                   &dwOutput,
                                   sizeof(dwOutput));
    
    if (NT_SUCCESS(status)) {
        *(DWORD*)param1 = dwOutput;
    }
    
    if (param2 == NULL || !NT_SUCCESS(status)) {
        NtClose(hFile);
    } else {
        *(HANDLE*)param2 = hFile;
    }
    
    return IoStatusToWs(status);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (CsInitialized != 0) {
            DeleteCriticalSection(&IrdaCs);
            CsInitialized = 0;
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        InitializeCriticalSection(&IrdaCs);
        CsInitialized = 1;
        
        // Initialize Winsock2Protocols
        ZeroMemory(&Winsock2Protocols[0], sizeof(WSAPROTOCOL_INFOW));
        Winsock2Protocols[0].dwServiceFlags1 = 6;
        Winsock2Protocols[0].dwServiceFlags2 = 0x1A;
        Winsock2Protocols[0].dwServiceFlags3 = 0x20;
        Winsock2Protocols[0].dwServiceFlags4 = 8;
        Winsock2Protocols[0].dwProviderFlags = 1;
        Winsock2Protocols[0].ProviderId = IrdaProviderGuid;
        Winsock2Protocols[0].dwCatalogEntryId = 1;
        Winsock2Protocols[0].ProtocolChain.ChainLen = 1;
        Winsock2Protocols[0].iVersion = 0;
        Winsock2Protocols[0].iAddressFamily = 0x1A;  // AF_IRDA
        Winsock2Protocols[0].iMaxSockAddr = 0x20;
        Winsock2Protocols[0].iMinSockAddr = 8;
        Winsock2Protocols[0].iSocketType = SOCK_STREAM;
        Winsock2Protocols[0].iProtocol = 1;
        Winsock2Protocols[0].iProtocolMaxOffset = 0;
        Winsock2Protocols[0].iNetworkByteOrder = BIGENDIAN;
        Winsock2Protocols[0].iSecurityScheme = SECURITY_PROTOCOL_NONE;
        Winsock2Protocols[0].dwMessageSize = 0;
        Winsock2Protocols[0].dwProviderReserved = 0;
        wcscpy_s(Winsock2Protocols[0].szProtocol, WSAPROTOCOL_LEN, L"IrDA");
    }
    return TRUE;
}

void NTAPI IoctlCompletionRoutine(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved)
{
    if (ApcContext) {
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)ApcContext;
        DWORD dwError = IoStatusToWs(IoStatusBlock->Status);
        DWORD dwBytes = (DWORD)IoStatusBlock->Information;
        
        // Create a dummy overlapped structure
        WSAOVERLAPPED Overlapped;
        ZeroMemory(&Overlapped, sizeof(Overlapped));
        
        lpCompletionRoutine(dwError, dwBytes, &Overlapped, 0);
    }
}

int IoStatusToWs(LONG status)
{
    switch (status) {
        case STATUS_SUCCESS:
            return 0;
        case STATUS_PENDING:
            return ERROR_IO_PENDING;
        case STATUS_INVALID_HANDLE:
            return WSAENOTSOCK;
        case STATUS_INVALID_PARAMETER:
            return WSAEINVAL;
        case STATUS_BUFFER_TOO_SMALL:
        case STATUS_BUFFER_OVERFLOW:
            return WSAEFAULT;
        case STATUS_ACCESS_DENIED:
            return WSAEACCES;
        case STATUS_INSUFFICIENT_RESOURCES:
            return WSAENOBUFS;
        case STATUS_NOT_SUPPORTED:
            return WSAEOPNOTSUPP;
        case STATUS_DEVICE_NOT_CONNECTED:
        case STATUS_NO_SUCH_DEVICE:
            return WSAENETDOWN;
        case STATUS_CONNECTION_REFUSED:
            return WSAECONNREFUSED;
        case STATUS_CONNECTION_RESET:
            return WSAECONNRESET;
        case STATUS_INVALID_DEVICE_REQUEST:
            return WSAEINVAL;
        case STATUS_DEVICE_BUSY:
            return WSAEWOULDBLOCK;
        case STATUS_DEVICE_DOES_NOT_EXIST:
            return WSAENETUNREACH;
        case STATUS_TIMEOUT:
            return WSAETIMEDOUT;
        case STATUS_CANCELLED:
            return WSAEINTR;
        case STATUS_FILE_CLOSED:
            return WSAENOTSOCK;
        case STATUS_OBJECT_NAME_NOT_FOUND:
            return WSAHOST_NOT_FOUND;
        case STATUS_OBJECT_PATH_NOT_FOUND:
            return WSAHOST_NOT_FOUND;
        case STATUS_SHARING_VIOLATION:
            return WSAEACCES;
        case STATUS_PORT_DISCONNECTED:
            return WSAENETRESET;
        case STATUS_LINK_FAILED:
            return WSAENETUNREACH;
        case STATUS_TOO_MANY_LINKS:
            return WSAEMFILE;
        case STATUS_REMOTE_NOT_LISTENING:
            return WSAECONNREFUSED;
        case STATUS_DUPLICATE_NAME:
            return WSAEADDRINUSE;
        case STATUS_BAD_NETWORK_NAME:
            return WSAENETUNREACH;
        case STATUS_LOCAL_DISCONNECT:
            return WSAENETRESET;
        case STATUS_REMOTE_DISCONNECT:
            return WSAECONNRESET;
        default:
            if (status < 0) {
                return WSAEFAULT;
            }
            return 0;
    }
}

INT WSAAPI WSHEnumProtocols(LPINT lpiProtocols, LPWSTR lpProtocolBuffer, LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    BOOL bIrDARequested = FALSE;
    
    if (lpiProtocols == NULL) {
        bIrDARequested = TRUE;
    } else {
        for (int i = 0; lpiProtocols[i] != 0; i++) {
            if (lpiProtocols[i] == 0x1A) {  // AF_IRDA
                bIrDARequested = TRUE;
                break;
            }
        }
    }
    
    if (bIrDARequested) {
        if (*lpdwBufferLength < 0x2A) {
            *lpdwBufferLength = 0x2A;
            return SOCKET_ERROR;
        }
        
        WSAPROTOCOL_INFOW* pInfo = (WSAPROTOCOL_INFOW*)lpBuffer;
        *pInfo = Winsock2Protocols[0];
        
        *lpdwBufferLength = 0x2A;
        return 1;
    }
    
    *lpdwBufferLength = 0;
    return 0;
}

INT WSAAPI WSHGetProviderGuid(LPWSTR lpProviderName, LPGUID lpProviderGuid)
{
    if (lpProviderName == NULL || lpProviderGuid == NULL) {
        return WSAEINVAL;
    }
    
    if (_wcsicmp(lpProviderName, L"irda") == 0) {
        *lpProviderGuid = IrdaProviderGuid;
        return ERROR_SUCCESS;
    }
    
    return WSAEINVAL;
}

INT WSAAPI WSHGetSockaddrType(PSOCKADDR pSockaddr, DWORD dwSockaddrLength, PSOCKADDR_INFO pSockaddrInfo)
{
    if (dwSockaddrLength < 0x20) {
        return WSAEINVAL;
    }
    
    if (pSockaddr->sa_family == 0x1A) {  // AF_IRDA
        if (pSockaddr->sa_data[4] == 0) {
            pSockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
            pSockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
        } else {
            pSockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
            pSockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
        }
        return ERROR_SUCCESS;
    }
    
    return WSAEAFNOSUPPORT;
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
            if ((DWORD)*lpiSocketInformationDataLength < 0x18) {
                return WSAEINVAL;
            }
            for (int i = 0; i < 6; i++) {
                ((DWORD*)lpSocketInformationData)[i] = ((DWORD*)lpSocketInformation)[i];
            }
        }
        *lpiSocketInformationDataLength = 0x18;
        return ERROR_SUCCESS;
    } else if (iSocketInformationType == 0xFF) {
        if (iSocketInformationSize == 0x10) {
            if ((DWORD)*lpiSocketInformationDataLength >= 0x20) {
                return ControlIoctl((DWORD)*lpiSocketInformationDataLength, NULL, NULL, NULL);
            }
        } else if (iSocketInformationSize == 0x12) {
            if ((DWORD)*lpiSocketInformationDataLength >= 0x54C) {
                return ControlIoctl((DWORD)*lpiSocketInformationDataLength, NULL, NULL, NULL);
            }
        } else if (iSocketInformationSize == 0x13) {
            if ((DWORD)*lpiSocketInformationDataLength >= 4) {
                HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
                PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = NULL;
                
                if (hNtdll) {
                    NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
                }
                
                if (NtDeviceIoControlFile) {
                    IO_STATUS_BLOCK ioStatus;
                    NTSTATUS status = NtDeviceIoControlFile((HANDLE)lpSocketContext,
                                                           NULL,
                                                           NULL,
                                                           NULL,
                                                           &ioStatus,
                                                           0x120008,
                                                           NULL,
                                                           0,
                                                           lpSocketInformationData,
                                                           *lpiSocketInformationDataLength);
                    
                    if (NT_SUCCESS(status)) {
                        *lpiSocketInformationDataLength = (int)ioStatus.Information;
                    }
                    
                    return IoStatusToWs(status);
                }
            } else {
                *lpiSocketInformationDataLength = 4;
                return WSAEINVAL;
            }
        }
        return WSAEINVAL;
    }
    
    return WSAEINVAL;
}

INT WSAAPI WSHGetWildcardSockaddr(PVOID pvSocketContext, PSOCKADDR pSockaddr, LPINT piSockaddrLength)
{
    if ((DWORD)*piSockaddrLength < 0x20) {
        return WSAEINVAL;
    }
    
    *piSockaddrLength = 0x20;
    ZeroMemory(pSockaddr, 0x20);
    pSockaddr->sa_family = 0x1A;  // AF_IRDA
    
    return ERROR_SUCCESS;
}

DWORD WSAAPI WSHGetWinsockMapping(PWINSOCK_MAPPING pMapping, DWORD dwMappingLength)
{
    if (dwMappingLength >= 0x2C) {
        CopyMemory(pMapping, &IrDAMapping, sizeof(IrDAMapping));
    }
    return 0x2C;
}

INT WSAAPI WSHGetWSAProtocolInfo(LPWSTR lpProtocolName, LPWSAPROTOCOL_INFOW* lppProtocolInfo, LPDWORD lpdwProtocolCount)
{
    if (lpProtocolName == NULL || lppProtocolInfo == NULL || lpdwProtocolCount == NULL) {
        return WSAEINVAL;
    }
    
    if (_wcsicmp(lpProtocolName, L"IrDA") == 0) {
        *lppProtocolInfo = Winsock2Protocols;
        *lpdwProtocolCount = 1;
        return ERROR_SUCCESS;
    }
    
    return WSAEINVAL;
}

INT WSAAPI WSHIoctl(
    PVOID pvSocketContext,
    DWORD dwCatalogEntryId,
    PVOID lpInputBuffer,
    PVOID lpSocketContext,
    DWORD dwIoControlCode,
    PVOID lpOutputBuffer,
    DWORD dwOutputBufferLength,
    PVOID lpOverlapped,
    DWORD dwOverlappedLength,
    LPDWORD lpdwBytesReturned,
    LPWSAOVERLAPPED lpWSAOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPINT lpiErrorCode)
{
    if (pvSocketContext == NULL || dwCatalogEntryId == 0xFFFFFFFF || lpdwBytesReturned == NULL || 
        lpiErrorCode == NULL || dwIoControlCode != 0x4004747F || 
        (lpCompletionRoutine != NULL && lpWSAOverlapped == NULL) || dwOverlappedLength < 0x21) {
        return WSAEINVAL;
    }
    
    int* pFileHandle = (int*)((PBYTE)pvSocketContext + 0x14);
    *lpiErrorCode = 0;
    *lpdwBytesReturned = dwOverlappedLength;
    
    if (*pFileHandle == 0) {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        PNT_CREATE_FILE NtCreateFile = NULL;
        
        if (hNtdll) {
            NtCreateFile = (PNT_CREATE_FILE)GetProcAddress(hNtdll, "NtCreateFile");
        }
        
        if (NtCreateFile) {
            UNICODE_STRING usFileName;
            OBJECT_ATTRIBUTES ObjectAttributes;
            IO_STATUS_BLOCK IoStatusBlock;
            
            RtlInitUnicodeString(&usFileName, L"\\Device\\IrDA");
            InitializeObjectAttributes(&ObjectAttributes,
                                      &usFileName,
                                      OBJ_CASE_INSENSITIVE,
                                      NULL,
                                      NULL);
            
            NTSTATUS status = NtCreateFile((PHANDLE)pFileHandle,
                                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                          &ObjectAttributes,
                                          &IoStatusBlock,
                                          NULL,
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          FILE_OPEN_IF,
                                          0,
                                          NULL,
                                          0);
            
            if (!NT_SUCCESS(status)) {
                return IoStatusToWs(status);
            }
        } else {
            return WSAEINVAL;
        }
    }
    
    if (lpCompletionRoutine == NULL) {
        BOOL bResult = DeviceIoControl((HANDLE)*pFileHandle,
                                      0x120018,
                                      NULL,
                                      0,
                                      lpOutputBuffer,
                                      dwOutputBufferLength,
                                      lpdwBytesReturned,
                                      (LPOVERLAPPED)lpWSAOverlapped);
        
        if (!bResult) {
            *lpiErrorCode = GetLastError();
            return WSAEINVAL;
        }
    } else {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = NULL;
        
        if (hNtdll) {
            NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
        }
        
        if (NtDeviceIoControlFile) {
            IO_STATUS_BLOCK ioStatus;
            NTSTATUS status = NtDeviceIoControlFile((HANDLE)*pFileHandle,
                                                   NULL,
                                                   IoctlCompletionRoutine,
                                                   lpCompletionRoutine,
                                                   &ioStatus,
                                                   0x120018,
                                                   NULL,
                                                   0,
                                                   lpOutputBuffer,
                                                   dwOutputBufferLength);
            
            if (NT_SUCCESS(status)) {
                *lpdwBytesReturned = (DWORD)ioStatus.Information;
            }
            
            return IoStatusToWs(status);
        }
    }
    
    return ERROR_SUCCESS;
}

INT WSAAPI WSHNotify(PVOID pvSocketContext, DWORD dwCatalogEntryId, PVOID lpNotificationData, PVOID lpNotificationContext, DWORD dwNotificationFlags)
{
    if (dwNotificationFlags == 1) {
        if (*(int*)((PBYTE)pvSocketContext + 0x10) != 0) {
            HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
            PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = NULL;
            
            if (hNtdll) {
                NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
            }
            
            if (NtDeviceIoControlFile) {
                IO_STATUS_BLOCK ioStatus;
                NtDeviceIoControlFile((HANDLE)lpNotificationContext,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     0x120004,
                                     (int*)((PBYTE)pvSocketContext + 0x10),
                                     4,
                                     NULL,
                                     0);
            }
        }
    } else if (dwNotificationFlags == 0x80) {
        EnterCriticalSection(&IrdaCs);
        
        int* pListHead = (int*)pvSocketContext;
        while (*pListHead != 0) {
            int* pCurrentNode = (int*)*pListHead;
            *pListHead = pCurrentNode[1];  // Next pointer
            
            HANDLE hHandle = (HANDLE)*pCurrentNode;
            if (hHandle != NULL && hHandle != INVALID_HANDLE_VALUE) {
                CloseHandle(hHandle);
            }
            
            HeapFree(GetProcessHeap(), 0, pCurrentNode);
        }
        
        LeaveCriticalSection(&IrdaCs);
        
        int hDeviceHandle = *(int*)((PBYTE)pvSocketContext + 0x14);
        if (hDeviceHandle != 0) {
            HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
            PNT_CLOSE NtClose = NULL;
            
            if (hNtdll) {
                NtClose = (PNT_CLOSE)GetProcAddress(hNtdll, "NtClose");
            }
            
            if (NtClose) {
                NtClose((HANDLE)hDeviceHandle);
            } else {
                CloseHandle((HANDLE)hDeviceHandle);
            }
        }
        
        HeapFree(GetProcessHeap(), 0, pvSocketContext);
    }
    
    return ERROR_SUCCESS;
}

INT WSAAPI WSHOpenSocket(LPINT AddressFamily, LPINT SocketType, LPINT Protocol, PUNICODE_STRING pFileName, PVOID* ppSocketContext, PDWORD pdwNotificationFlags)
{
    if (*AddressFamily == 0x1A) {  // AF_IRDA
        if ((*SocketType == 1 && (*Protocol == 1 || *Protocol == 0)) ||
            (*SocketType == 0 && *Protocol == 1)) {
            
            RtlInitUnicodeString(pFileName, L"\\Device\\IrDA");
            
            PVOID pContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x18);
            if (pContext == NULL) {
                return WSAENOBUFS;
            }
            
            *ppSocketContext = pContext;
            ((DWORD*)pContext)[1] = 0x1A;      // AF_IRDA
            ((DWORD*)pContext)[2] = 1;         // Socket type
            ((DWORD*)pContext)[3] = 1;         // Protocol
            ((DWORD*)pContext)[4] = 0;         // Flags
            *((DWORD*)pContext) = 0;           // List head
            ((DWORD*)pContext)[5] = 0;         // Device handle
            
            *pdwNotificationFlags = 0x81;
            
            return ERROR_SUCCESS;
        } else {
            return WSAEINVAL;
        }
    }
    
    return WSAEINVAL;
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
    if (iSocketInformationType == 0xFFFE && iSocketInformationSize == 1 && iSocketInformationDataLength >= 0x18) {
        if (lpSocketInformation != NULL) {
            // Update existing socket information
            for (int i = 0; i < 5; i++) {
                ((DWORD*)((PBYTE)lpSocketInformation + 4))[i] = ((DWORD*)lpSocketInformationData)[i];
            }
            return ERROR_SUCCESS;
        } else {
            // Create new socket information
            PVOID pBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x18);
            if (pBuffer == NULL) {
                return WSAENOBUFS;
            }
            
            // Copy the data
            for (int i = 0; i < 6; i++) {
                ((DWORD*)pBuffer)[i] = ((DWORD*)lpSocketInformationData)[i];
            }
            
            // Set list head to NULL
            *((DWORD*)pBuffer) = 0;
            
            // Return pointer to caller
            *(PVOID*)lpSocketInformationData = pBuffer;
            return ERROR_SUCCESS;
        }
    } else if (iSocketInformationType == 0xFF) {
        if (iSocketInformationSize == 0x11) {
            HANDLE hFile = INVALID_HANDLE_VALUE;
            int iResult = ControlIoctl((DWORD)&iSocketInformationDataLength, (char*)&hFile, NULL, NULL);
            
            if (iResult == ERROR_SUCCESS) {
                int* pNode = (int*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 8);
                if (pNode != NULL) {
                    *pNode = (int)hFile;
                    
                    EnterCriticalSection(&IrdaCs);
                    pNode[1] = *(int*)lpSocketInformation;  // Next pointer
                    *(int**)lpSocketInformation = pNode;    // Update head
                    LeaveCriticalSection(&IrdaCs);
                    
                    return ERROR_SUCCESS;
                }
                iResult = WSAENOBUFS;
            }
            
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
            }
            return iResult;
        } else if (iSocketInformationSize == 0x15) {
            DWORD* pFlags = (DWORD*)((PBYTE)lpSocketInformation + 0x10);
            *pFlags |= 1;
            return ERROR_SUCCESS;
        } else if (iSocketInformationSize == 0x16) {
            DWORD* pFlags = (DWORD*)((PBYTE)lpSocketInformation + 0x10);
            *pFlags |= 2;
            return ERROR_SUCCESS;
        }
        return WSAEINVAL;
    }
    
    return WSAEINVAL;
}

