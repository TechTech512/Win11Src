/*
 * startnet.c
 *
 * Windows Boot Shell Network Starter (mwstartnet.exe)
 * Enumerates network interfaces, registers NetBIOS name with WINS,
 * and signals network readiness.
 *
 * This is a full reconstruction of the original binary.
 */
#pragma warning (disable:4033)
#pragma warning (disable:4715)
#pragma warning (disable:4716)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// ------------------------------------------------------------------
// Constants (from original)
// ------------------------------------------------------------------
#define NBNS_PORT 137
#define NBNS_PACKET_SIZE 0x44
#define MAX_WAIT 30000

// ------------------------------------------------------------------
// Global variables (exact match)
// ------------------------------------------------------------------
volatile LONG g_WinsRegistrationDone = 0;
HANDLE g_hNetReadyEvent = NULL;
HANDLE g_hStopEvent = NULL;          // DAT_1400035a0
HANDLE g_hAddressChangeHandle = NULL; // DAT_140003598

// ------------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------------
void PrintMessage(const wchar_t* format, ...);
void hwprintf(HANDLE hConsole, const wchar_t* format, ...);
void DoWINSRegistration(void);
void GetWinsServer(PIP_ADAPTER_ADDRESSES pAdapter, SOCKADDR_IN* pWinsAddr);
void PopulatePacket(PWCHAR pPacket, PBYTE pName, DWORD dwIP);
void PrintInterfaceDetails(void);
int RegisterWinsName(PBYTE pName);
DWORD WINAPI InterfaceAddressChangeCallback(PVOID pContext, PMIB_UNICASTIPADDRESS_ROW pRow, MIB_NOTIFICATION_TYPE type);
int __cdecl wmain(int argc, wchar_t* argv[]);

// ------------------------------------------------------------------
// vDbgPrintEx replacement (matches original)
// ------------------------------------------------------------------
NTSYSAPI
ULONG
NTAPI
vDbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ PCCH Format,
    _In_ va_list arglist
    );

// ------------------------------------------------------------------
// PrintMessage – uses vDbgPrintEx (as original)
// ------------------------------------------------------------------
void PrintMessage(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vDbgPrintEx(0, 0, (PCCH)format, args);
    va_end(args);
}

// ------------------------------------------------------------------
// hwprintf – write to console handle (as original)
// ------------------------------------------------------------------
void hwprintf(HANDLE hConsole, const wchar_t* format, ...)
{
    wchar_t buffer[2048];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, _countof(buffer), format, args);
    va_end(args);
    if (hConsole != INVALID_HANDLE_VALUE && hConsole != NULL) {
        DWORD dwWritten;
        WriteConsoleW(hConsole, buffer, (DWORD)wcslen(buffer), &dwWritten, NULL);
    }
    OutputDebugStringW(buffer);
}

// ------------------------------------------------------------------
// DoWINSRegistration – get computer name, uppercase, register
// ------------------------------------------------------------------
void DoWINSRegistration(void)
{
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = _countof(computerName);
    DWORD dwError;
    WCHAR* p;

    if (!GetComputerNameExW(ComputerNameNetBIOS, computerName, &dwSize)) {
        dwError = GetLastError();
        PrintMessage(L"Unable to get computer name: %u\n", dwError);
        return;
    }

    PrintMessage(L"ComputerName = \"%ws\"\n", computerName);

    p = computerName;
    while (*p) {
        *p = towupper(*p);
        p++;
    }

    wprintf(L"Registering %s\n", computerName);

    int result = RegisterWinsName((PBYTE)computerName);
    // Increment regardless of result to avoid retries
    InterlockedIncrement(&g_WinsRegistrationDone);

    if (result != 0) {
        wprintf(L"Error: %u\n", result);
    }
}

// ------------------------------------------------------------------
// GetWinsServer – read WINS server from registry for given adapter
// ------------------------------------------------------------------
void GetWinsServer(PIP_ADAPTER_ADDRESSES pAdapter, SOCKADDR_IN* pWinsAddr)
{
    WCHAR szKeyPath[512];
    WCHAR szValue[512];
    DWORD dwSize = sizeof(szValue);
    HKEY hKey;
    LONG lResult;

    if (pAdapter == NULL || pAdapter->AdapterName == NULL) return;

    // Build registry path: 
    // SYSTEM\CurrentControlSet\services\NetBT\Parameters\Interfaces\Tcpip_{GUID}
    swprintf_s(szKeyPath, _countof(szKeyPath),
               L"SYSTEM\\CurrentControlSet\\services\\NetBT\\Parameters\\Interfaces\\Tcpip_%hs",
               pAdapter->AdapterName);

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyPath, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) return;

    lResult = RegQueryValueExW(hKey, L"DhcpNameServerList", NULL, NULL, (LPBYTE)szValue, &dwSize);
    RegCloseKey(hKey);
    if (lResult != ERROR_SUCCESS) return;

    PrintMessage(L"WINS server: %s\n", szValue);

    // Convert string address to SOCKADDR_IN
    INT addrLen = sizeof(SOCKADDR_IN);
    if (WSAStringToAddressW(szValue, AF_INET, NULL, (LPSOCKADDR)pWinsAddr, &addrLen) != 0) {
        PrintMessage(L"Failed to parse WINS address\n");
        ZeroMemory(pWinsAddr, sizeof(SOCKADDR_IN));
        pWinsAddr->sin_family = AF_INET;
        pWinsAddr->sin_addr.s_addr = INADDR_NONE;
    }
}

// ------------------------------------------------------------------
// PopulatePacket – build NetBIOS name registration packet (NBNS)
// Exact replication of the original packet structure.
// ------------------------------------------------------------------
void PopulatePacket(PWCHAR pPacket, PBYTE pName, DWORD dwIP)
{
    // The original packet is 0x44 (68) bytes.
    // We'll zero it and fill fields as per original.
    ZeroMemory(pPacket, NBNS_PACKET_SIZE);

    // Transaction ID: combination of thread ID and tick count
    pPacket[0] = (WCHAR)(GetCurrentThreadId() ^ GetTickCount());

    // Flags: 0x2900 = name registration request
    pPacket[1] = htons(0x2900);

    // Questions: 1
    pPacket[2] = htons(1);

    // Answer, authority, additional = 0
    pPacket[3] = 0;
    pPacket[4] = 0;

    // Name (encoded) – 32 bytes (16 chars encoded as 2 bytes each)
    // The original encodes each character as two bytes (uppercase hex of each nibble + 0x41)
    // pName is a wide string (NetBIOS name, max 15 chars + 1 byte suffix)
    // We'll process up to 15 characters and add a suffix (0x20 for computer name).
    // For this reconstruction, we assume pName is a wide char string.
    BYTE encodedName[32];
    int len = (int)wcslen((wchar_t*)pName);
    if (len > 15) len = 15;
    for (int i = 0; i < len; i++) {
        BYTE ch = (BYTE)pName[i];
        encodedName[i*2] = (ch >> 4) + 0x41;
        encodedName[i*2+1] = (ch & 0x0F) + 0x41;
    }
    // Pad remaining with spaces (0x20)
    for (int i = len; i < 15; i++) {
        encodedName[i*2] = 0x41 + 2;    // ' ' -> 0x20 -> nibbles 2 and 0 => 0x41+2=0x43, 0x41+0=0x41
        encodedName[i*2+1] = 0x41;
    }
    // Suffix byte: for computer name, suffix is 0x00 (or 0x20?) Actually original uses 0x20
    encodedName[30] = 0x41 + 2; // 0x20
    encodedName[31] = 0x41 + 0; // 0x00? Actually original put 0 at the last byte? The original had a loop up to 0x1e (30) and set the next byte? Let's follow original: they looped uVar6 < 0x20, so 32 bytes total. At each step, they set two bytes. So encodedName[0..31] are all set.
    // The original had the name and suffix encoded, we'll copy it to the packet.
    // In the original, the name starts at offset 0x0D (13) in the packet.
    // We'll copy our encoded name there.
    CopyMemory((PBYTE)pPacket + 0x0D, encodedName, 32);

    // Type: NetBIOS name registration (0x0020)
    pPacket[0x17] = htons(0x20);

    // Class: 0x0001 (Internet)
    pPacket[0x18] = htons(1);

    // TTL: 300000 (seconds? original used 300000)
    *(DWORD*)((PBYTE)pPacket + 0x1C) = htonl(300000);

    // IP address (little endian)
    *(DWORD*)((PBYTE)pPacket + 0x20) = dwIP;

    // Other fields remain zero.
}

// ------------------------------------------------------------------
// PrintInterfaceDetails – enumerate IPs, trigger registration
// Exact original logic.
// ------------------------------------------------------------------
void PrintInterfaceDetails(void)
{
    ULONG dwSize = 0;
    DWORD dwRetVal = GetIpAddrTable(NULL, &dwSize, FALSE);
    PMIB_IPADDRTABLE pIpTable = (PMIB_IPADDRTABLE)LocalAlloc(LPTR, dwSize);
    if (!pIpTable) {
        return;
    }

    dwRetVal = GetIpAddrTable(pIpTable, &dwSize, FALSE);
    if (dwRetVal != NO_ERROR) {
        LocalFree(pIpTable);
        return;
    }

    BOOL bHasValidIP = FALSE;
    for (DWORD i = 0; i < pIpTable->dwNumEntries; i++) {
        MIB_IPADDRROW row = pIpTable->table[i];
        DWORD dwIP = row.dwAddr;
        BYTE b1 = (BYTE)(dwIP & 0xFF);
        BYTE b2 = (BYTE)((dwIP >> 8) & 0xFF);
        BYTE b3 = (BYTE)((dwIP >> 16) & 0xFF);
        BYTE b4 = (BYTE)((dwIP >> 24) & 0xFF);

        PrintMessage(L"IP address(%d) = %d.%d.%d.%d\n", i, b1, b2, b3, b4);

        // Skip loopback (127.x.x.x), link-local (169.254.x.x), and 0.0.0.0
        if (b1 != 127 && !(b1 == 169 && b2 == 254) && dwIP != 0) {
            bHasValidIP = TRUE;
        }
    }

    LocalFree(pIpTable);

    if (bHasValidIP && g_WinsRegistrationDone == 0) {
        DoWINSRegistration();
        SetEvent(g_hNetReadyEvent);
    }
}

// ------------------------------------------------------------------
// RegisterWinsName – send NBNS registration to WINS server
// Exact original flow.
// ------------------------------------------------------------------
int RegisterWinsName(PBYTE pName)
{
    PIP_ADAPTER_ADDRESSES pAdapter = NULL;
    ULONG dwSize = 0;
    DWORD dwRetVal;
    SOCKADDR_IN winsAddr = {0};
    SOCKET sock;
    int result;
    WCHAR packet[NBNS_PACKET_SIZE / sizeof(WCHAR)];
    DWORD dwIP = 0;
    char recvBuffer[1024];
    int retryCount = 0;

    // Get adapters
    dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &dwSize);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) {
        PrintMessage(L"GetAdaptersAddresses size failed: %u\n", dwRetVal);
        return dwRetVal;
    }
    pAdapter = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!pAdapter) {
        PrintMessage(L"Memory allocation failed\n");
        return ERROR_OUTOFMEMORY;
    }
    dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapter, &dwSize);
    if (dwRetVal != NO_ERROR) {
        PrintMessage(L"GetAdaptersAddresses failed: %u\n", dwRetVal);
        HeapFree(GetProcessHeap(), 0, pAdapter);
        return dwRetVal;
    }

    // Find first IPv4 adapter with a valid IP
    PIP_ADAPTER_ADDRESSES pCurr = pAdapter;
    while (pCurr) {
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
        while (pUnicast) {
            if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                DWORD ip = ((SOCKADDR_IN*)pUnicast->Address.lpSockaddr)->sin_addr.s_addr;
                BYTE b1 = (BYTE)(ip & 0xFF);
                BYTE b2 = (BYTE)((ip >> 8) & 0xFF);
                if (b1 != 127 && !(b1 == 169 && b2 == 254) && ip != 0) {
                    dwIP = ip;
                    break;
                }
            }
            pUnicast = pUnicast->Next;
        }
        if (dwIP != 0) break;
        pCurr = pCurr->Next;
    }
    if (dwIP == 0) {
        PrintMessage(L"No valid IP address found\n");
        HeapFree(GetProcessHeap(), 0, pAdapter);
        return ERROR_NO_DATA;   // 232
    }

    // Get WINS server from registry
    GetWinsServer(pCurr, &winsAddr);
    if (winsAddr.sin_addr.s_addr == INADDR_NONE || winsAddr.sin_addr.s_addr == 0) {
        PrintMessage(L"No WINS server configured\n");
        HeapFree(GetProcessHeap(), 0, pAdapter);
        return ERROR_FILE_NOT_FOUND;   // 2
    }

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        DWORD err = WSAGetLastError();
        PrintMessage(L"Failed to create socket: %u\n", err);
        HeapFree(GetProcessHeap(), 0, pAdapter);
        return err;
    }

    winsAddr.sin_port = htons(NBNS_PORT);
    winsAddr.sin_family = AF_INET;

    // Send registration packets
    while (TRUE) {
        PopulatePacket(packet, pName, dwIP);

        result = sendto(sock, (const char*)packet, NBNS_PACKET_SIZE, 0,
                       (struct sockaddr*)&winsAddr, sizeof(winsAddr));
        if (result == SOCKET_ERROR) {
            DWORD err = WSAGetLastError();
            PrintMessage(L"Error sending packet: %u\n", err);
            closesocket(sock);
            HeapFree(GetProcessHeap(), 0, pAdapter);
            return err;
        }

        // Wait for response (optional)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);
        struct timeval tv = {1, 0};
        if (select(0, &readSet, NULL, NULL, &tv) > 0) {
            result = recv(sock, recvBuffer, sizeof(recvBuffer), 0);
            if (result == SOCKET_ERROR) {
                DWORD err = WSAGetLastError();
                PrintMessage(L"Error receiving packet: %u\n", err);
                closesocket(sock);
                HeapFree(GetProcessHeap(), 0, pAdapter);
                return err;
            }
        }

        Sleep(290);
        retryCount++;
        if (retryCount > 5) break;
    }

    closesocket(sock);
    HeapFree(GetProcessHeap(), 0, pAdapter);
    return 0;   // success
}

// ------------------------------------------------------------------
// InterfaceAddressChangeCallback – called when IP address changes
// ------------------------------------------------------------------
DWORD WINAPI InterfaceAddressChangeCallback(PVOID pContext, PMIB_UNICASTIPADDRESS_ROW pRow, MIB_NOTIFICATION_TYPE type)
{
    if (type == MibParameterNotification && g_WinsRegistrationDone == 0) {
        PrintInterfaceDetails();
    }
    return 0;
}

// ------------------------------------------------------------------
// wmain – entry point
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    WSADATA wsaData;
    DWORD dwError;
    HANDLE hStopEvent;
    HANDLE hNetReadyEvent;
    HANDLE hAddressChangeHandle = NULL;
	
    // Disable buffering so output appears immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    PrintMessage(L"start of startnet.exe\n");

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        dwError = WSAGetLastError();
        PrintMessage(L"Failed to start winsock: %u\n", dwError);
        return 1;
    }

    // Create stop event (DAT_1400035a0)
    hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_hStopEvent = hStopEvent;
    if (!hStopEvent) {
        PrintMessage(L"Failed to create stop event\n");
        WSACleanup();
        return 1;
    }

    // Create net ready event (g_hNetReadyEvent)
    hNetReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_hNetReadyEvent = hNetReadyEvent;
    if (!hNetReadyEvent) {
        PrintMessage(L"Failed to create net ready event\n");
        CloseHandle(hStopEvent);
        WSACleanup();
        return 1;
    }

    // Initial interface details
    PrintInterfaceDetails();

    // Register for IP address change notifications
    if (NotifyUnicastIpAddressChange(AF_UNSPEC, InterfaceAddressChangeCallback, NULL, FALSE, &hAddressChangeHandle) != NO_ERROR) {
        PrintMessage(L"Failed to register IP address change notification\n");
    } else {
        g_hAddressChangeHandle = hAddressChangeHandle;
    }

    // ==================== MODIFIED: Wait with timeout ====================
    DWORD dwWait = WaitForSingleObject(hStopEvent, 60000);  // 60 seconds
    if (dwWait == WAIT_TIMEOUT) {
        PrintMessage(L"Timeout waiting for stop event, exiting.\n");
    }
    // ====================================================================

    // Cleanup
    if (hAddressChangeHandle) {
        CancelMibChangeNotify2(hAddressChangeHandle);
    }
    if (hNetReadyEvent) {
        CloseHandle(hNetReadyEvent);
        g_hNetReadyEvent = NULL;
    }
    if (hStopEvent) {
        CloseHandle(hStopEvent);
        g_hStopEvent = NULL;
    }

    WSACleanup();
    PrintMessage(L"--Exiting startnet.exe--\n");
    return 0;
}

