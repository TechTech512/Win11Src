#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>

// GUIDs from the disassembly
GUID HostnameGuid = {0x0002A803, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
GUID AddressGuid = {0x0002A801, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// Forward declarations for functions that exist elsewhere
BOOL AcsHlpSendCommand(void* pNotification);

int WSAttemptAutodialAddr(SOCKADDR* param_1, int param_2)
{
    BYTE notification[1032]; // Size based on ACD_NOTIFICATION structure
    DWORD dwResult;
    
    // Clear the notification buffer
    memset(notification, 0, sizeof(notification));
    
    // Set command to 1
    *(DWORD*)notification = 1;
    
    // Check address family
    if (param_1->sa_family == AF_INET) {
        // IPv4 - type 0
        *(DWORD*)(notification + 4) = 0;
        SOCKADDR_IN* addr_in = (SOCKADDR_IN*)param_1;
        *(DWORD*)(notification + 8) = addr_in->sin_addr.S_un.S_addr;
        
        dwResult = AcsHlpSendCommand(notification);
        return (int)dwResult;
    } 
    else if (param_1->sa_family == AF_INET6) {
        // IPv6 - return 0 (not supported in original)
        return 0;
    }
    else if (param_1->sa_family == AF_NETBIOS) {
        // NetBIOS - type 2
        *(DWORD*)(notification + 4) = 2;
        memcpy(notification + 8, param_1->sa_data, 16);
        
        dwResult = AcsHlpSendCommand(notification);
        return (int)dwResult;
    }
    
    return 0;
}

int WSAttemptAutodialName(WSAQUERYSETW* param_1)
{
    BYTE notification[1032];
    CHAR buffer[16];
    DWORD dwIpAddr;
    int i;
    
    if (!param_1 || !param_1->lpszServiceInstanceName || !param_1->lpServiceClassId) {
        return 0;
    }
    
    memset(notification, 0, sizeof(notification));
    *(DWORD*)notification = 1;
    
    // Check if it's HostnameGuid
    if (memcmp(param_1->lpServiceClassId, &HostnameGuid, sizeof(GUID)) == 0) {
        // Type 3 for DNS name
        *(DWORD*)(notification + 4) = 3;
        
        // Copy DNS name (truncated to 1024 chars as per structure)
        wcsncpy((WCHAR*)(notification + 8), param_1->lpszServiceInstanceName, 511);
        ((WCHAR*)(notification + 8))[511] = L'\0';
        
        return (int)AcsHlpSendCommand(notification);
    }
    // Check if it's AddressGuid  
    else if (memcmp(param_1->lpServiceClassId, &AddressGuid, sizeof(GUID)) == 0) {
        // Convert to ANSI
        WideCharToMultiByte(CP_ACP, 0, param_1->lpszServiceInstanceName, -1, buffer, 16, NULL, NULL);
        
        // Convert to lowercase
        for (i = 0; buffer[i]; i++) {
            buffer[i] = (CHAR)tolower(buffer[i]);
        }
        
        // Check if it's an IP address
        dwIpAddr = inet_addr(buffer);
        if (dwIpAddr != INADDR_NONE) {
            // Type 0 for IP address
            *(DWORD*)(notification + 4) = 0;
            *(DWORD*)(notification + 8) = dwIpAddr;
            
            return (int)AcsHlpSendCommand(notification);
        }
    }
    
    return 0;
}

void WSNoteSuccessfulHostentLookup(CHAR* param_1, DWORD param_2)
{
    BYTE notification[1032];
    WCHAR wzHostname[256];
    
    if (!param_1 || !*param_1) {
        return;
    }
    
    memset(notification, 0, sizeof(notification));
    *(DWORD*)notification = 1;
    
    // Convert to wide char
    MultiByteToWideChar(CP_ACP, 0, param_1, -1, wzHostname, 256);
    
    // Simple DNS check: look for dots
    BOOL bIsDnsName = FALSE;
    for (int i = 0; wzHostname[i]; i++) {
        if (wzHostname[i] == L'.') {
            bIsDnsName = TRUE;
            break;
        }
    }
    
    if (bIsDnsName) {
        // Type 3 for DNS name
        *(DWORD*)(notification + 4) = 3;
        wcsncpy((WCHAR*)(notification + 8), wzHostname, 511);
        ((WCHAR*)(notification + 8))[511] = L'\0';
    } else {
        // Type 0 for IP address
        *(DWORD*)(notification + 4) = 0;
        *(DWORD*)(notification + 8) = param_2;
    }
    
    AcsHlpSendCommand(notification);
}

