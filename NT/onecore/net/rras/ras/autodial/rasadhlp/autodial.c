#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <ras.h>
#include <winternl.h>
#include <lm.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "rasapi32.lib")
#pragma comment(lib, "netapi32.lib")

#define DnsNameDomain 2

// Structures
typedef struct _ACD_ADDR {
    DWORD dwType;
    union {
        DWORD dwIpAddr;
        CHAR  szNetBiosName[16];
        WCHAR szDnsName[1024];
    };
} ACD_ADDR, *PACD_ADDR;

typedef struct _ACD_NOTIFICATION {
    DWORD dwCommand;
    ACD_ADDR addr;
    DWORD dwReserved;
} ACD_NOTIFICATION, *PACD_NOTIFICATION;

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

DWORD ulGetAutodialSleepInterval();
BOOL fIsDnsName(LPWSTR lpName);
BOOL AcsHlpSendCommand(PACD_NOTIFICATION pNotification);
DWORD DwGetAcdAddr(PACD_ADDR pAddr, LPWSTR lpRemoteName);

// Helper functions
BOOL AcsHlpNbConnection(LPWSTR lpRemoteName)
{
    ACD_NOTIFICATION notification;
    DWORD dwResult;
    BOOL bSuccess = FALSE;
    
    ZeroMemory(&notification, sizeof(notification));
    notification.dwCommand = 1;  // Assume command 1 for connection
    
    dwResult = DwGetAcdAddr(&notification.addr, lpRemoteName);
    if (dwResult == ERROR_SUCCESS) {
        bSuccess = AcsHlpSendCommand(&notification);
        
        if (bSuccess) {
            DWORD dwSleepInterval = ulGetAutodialSleepInterval();
            if (dwSleepInterval != 0) {
                Sleep(dwSleepInterval);
            }
        }
    }
    
    return bSuccess;
}

BOOL AcsHlpSendCommand(PACD_NOTIFICATION pNotification)
{
    UNICODE_STRING usFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile = NULL;
    HANDLE hEvent = NULL;
    NTSTATUS status;
    DWORD dwLastError = ERROR_SUCCESS;
    
    RtlInitUnicodeString(&usFileName, L"\\Device\\RasAcd");
    
    InitializeObjectAttributes(&ObjectAttributes,
                              &usFileName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);
    
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return FALSE;
    }
    
    PNT_CREATE_FILE NtCreateFile = (PNT_CREATE_FILE)GetProcAddress(hNtdll, "NtCreateFile");
    PNT_DEVICE_IO_CONTROL_FILE NtDeviceIoControlFile = (PNT_DEVICE_IO_CONTROL_FILE)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
    
    if (!NtCreateFile || !NtDeviceIoControlFile) {
        return FALSE;
    }
    
    status = NtCreateFile(&hFile,
                          FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
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
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    
    hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    status = NtDeviceIoControlFile(hFile,
                                   hEvent,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   0xF14014,  // IOCTL_RASACD_NOTIFY
                                   pNotification,
                                   sizeof(ACD_NOTIFICATION),
                                   NULL,
                                   0);
    
    if (status == STATUS_PENDING) {
        WaitForSingleObject(hEvent, INFINITE);
        status = IoStatusBlock.Status;
    }
    
    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }
    
    CloseHandle(hFile);
    
    return NT_SUCCESS(status);
}

DWORD DwGetAcdAddr(PACD_ADDR pAddr, LPWSTR lpRemoteName)
{
    if (pAddr == NULL || lpRemoteName == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    CHAR* pszAnsiName = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    
    // Convert wide string to ANSI
    int nLength = WideCharToMultiByte(CP_ACP, 0, lpRemoteName, -1, NULL, 0, NULL, NULL);
    if (nLength == 0) {
        return GetLastError();
    }
    
    pszAnsiName = (CHAR*)LocalAlloc(LPTR, nLength);
    if (pszAnsiName == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    
    WideCharToMultiByte(CP_ACP, 0, lpRemoteName, -1, pszAnsiName, nLength, NULL, NULL);
    
    // Check if it's an IP address
    IN_ADDR inAddr;
    inAddr.S_un.S_addr = inet_addr(pszAnsiName);
    if (inAddr.S_un.S_addr != INADDR_NONE) {
        pAddr->dwType = 0;  // IP address
        pAddr->dwIpAddr = inAddr.S_un.S_addr;
        dwResult = ERROR_SUCCESS;
    } else {
        // Check if it's a DNS name
        BOOL bIsDnsName = fIsDnsName(lpRemoteName);
        if (bIsDnsName) {
            // DNS name
            pAddr->dwType = 3;  // DNS name
            if (wcslen(lpRemoteName) >= 1024) {
                dwResult = ERROR_INVALID_PARAMETER;
            } else {
                wcscpy_s(pAddr->szDnsName, 1024, lpRemoteName);
                dwResult = ERROR_SUCCESS;
            }
        } else {
            // NetBIOS name (strip leading \\ if present)
            LPWSTR lpName = lpRemoteName;
            if (wcslen(lpRemoteName) >= 2 && 
                lpRemoteName[0] == L'\\' && 
                lpRemoteName[1] == L'\\') {
                lpName = lpRemoteName + 2;
            }
            
            // Convert to ANSI for NetBIOS
            if (strlen(pszAnsiName) >= 16) {
                dwResult = ERROR_INVALID_PARAMETER;
            } else {
                pAddr->dwType = 2;  // NetBIOS name
                strcpy_s(pAddr->szNetBiosName, 16, pszAnsiName);
                dwResult = ERROR_SUCCESS;
            }
        }
    }
    
    if (pszAnsiName != NULL) {
        LocalFree(pszAnsiName);
    }
    
    return dwResult;
}

BOOL fIsDnsName(LPWSTR lpName)
{
    HMODULE hDnsApi = NULL;
    FARPROC pDnsValidateName = NULL;
    BOOL bIsDnsName = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    
    hDnsApi = LoadLibraryExW(L"dnsapi.dll", NULL, 0);
    if (hDnsApi != NULL) {
        pDnsValidateName = GetProcAddress(hDnsApi, "DnsValidateName_W");
        if (pDnsValidateName != NULL) {
            // DnsValidateName returns ERROR_SUCCESS (0) for valid DNS names
            typedef DWORD (WINAPI *DNSVALIDATENAME)(LPCWSTR, DWORD);
            DNSVALIDATENAME DnsValidateNameW = (DNSVALIDATENAME)pDnsValidateName;
            
            dwError = DnsValidateNameW(lpName, DnsNameDomain);
            bIsDnsName = (dwError == ERROR_SUCCESS);
        } else {
            dwError = GetLastError();
        }
    } else {
        dwError = GetLastError();
    }
    
    if (hDnsApi != NULL) {
        FreeLibrary(hDnsApi);
    }
    
    // Fallback: check if it contains a dot (simple heuristic)
    if (!bIsDnsName && dwError != ERROR_SUCCESS) {
        for (int i = 0; lpName[i] != L'\0'; i++) {
            if (lpName[i] == L'.') {
                bIsDnsName = TRUE;
                break;
            }
        }
    }
    
    return bIsDnsName;
}

CHAR* pszDupWtoA(LPWSTR lpWideStr)
{
    if (lpWideStr == NULL) {
        return NULL;
    }
    
    int nLength = WideCharToMultiByte(CP_ACP, 0, lpWideStr, -1, NULL, 0, NULL, NULL);
    if (nLength == 0) {
        return NULL;
    }
    
    CHAR* pszAnsi = (CHAR*)LocalAlloc(LPTR, nLength);
    if (pszAnsi == NULL) {
        return NULL;
    }
    
    if (WideCharToMultiByte(CP_ACP, 0, lpWideStr, -1, pszAnsi, nLength, NULL, NULL) == 0) {
        LocalFree(pszAnsi);
        return NULL;
    }
    
    return pszAnsi;
}

DWORD ulGetAutodialSleepInterval()
{
    HKEY hKey = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwType = REG_DWORD;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(DWORD);
    
    dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\RasAuto\\Parameters",
                            0,
                            KEY_READ,
                            &hKey);
    
    if (dwResult == ERROR_SUCCESS) {
        RegQueryValueExW(hKey,
                        L"NewTransportWaitInterval",
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &dwSize);
    }
    
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    
    return dwValue;
}

