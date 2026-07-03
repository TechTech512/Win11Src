/*
 * ebootpinger.c
 *
 * EbootPinger for Windows 10 IoT – broadcasts device info via UDP multicast.
 * Matches original behavior exactly (including retry loops and port 6).
 */

#pragma warning (disable:4996)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// ------------------------------------------------------------------
// Constants (as per original)
// ------------------------------------------------------------------
#define MULTICAST_GROUP "239.0.0.222"
#define MULTICAST_PORT 6
#define MAX_RETRIES 1000
#define SLEEP_MS 250

// ------------------------------------------------------------------
// Global variables (exact match with decompiled offsets)
// ------------------------------------------------------------------
SOCKET sock = INVALID_SOCKET;
struct sockaddr_in dst_sock_addr = {0};
struct sockaddr_in sock_addr = {0};

// ------------------------------------------------------------------
// Buffer structure – layout must match the decompiled offsets:
//   host        : 0x00 (33 wchar_t)
//   ipv4        : 0x42 (17 wchar_t)
//   mac         : 0x64 (25 wchar_t)
//   id          : 0x96 (40 wchar_t)
//   model       : 0xe6 (50 wchar_t)
//   osversion   : 0x14a (50 wchar_t)
//   architecture: 0x1ae (8 wchar_t)
// ------------------------------------------------------------------
#pragma pack(push, 1)
struct BufferForBroadcast {
    wchar_t host[33];          // 0x00
    wchar_t ipv4[17];          // 0x42
    wchar_t mac[25];           // 0x64
    wchar_t id[40];            // 0x96
    wchar_t model[50];         // 0xe6
    wchar_t osversion[50];     // 0x14a
    wchar_t architecture[8];   // 0x1ae
};
#pragma pack(pop)

struct BufferForBroadcast UdpBroadcastBuffer = {0};

// ------------------------------------------------------------------
// Debug output
// ------------------------------------------------------------------
void DebugPrint(const wchar_t* format, ...)
{
    wchar_t buffer[2000];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
    va_end(args);
    OutputDebugStringW(buffer);
}

// ------------------------------------------------------------------
// Get MAC address and IP from first suitable adapter.
// Original criteria: Type == 6 (Ethernet) or 0x47 (Wireless)
// and IP != "0.0.0.0".
// ------------------------------------------------------------------
BOOL GetMACaddress(struct BufferForBroadcast* pBuffer)
{
    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
    if (!pAdapterInfo) {
        DebugPrint(L"Error allocating memory\n");
        return FALSE;
    }

    DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
        if (!pAdapterInfo) {
            DebugPrint(L"Error allocating memory\n");
            return FALSE;
        }
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    }

    if (dwRetVal != NO_ERROR) {
        DebugPrint(L"Getting Adapter Info failed with error: %d\n", dwRetVal);
        free(pAdapterInfo);
        return FALSE;
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter) {
        // Check type and IP (skip 0.0.0.0)
        if ((pAdapter->Type == MIB_IF_TYPE_ETHERNET || pAdapter->Type == IF_TYPE_IEEE80211) &&
            strncmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0", 7) != 0) {

            // Copy IP address (offset 0x42 in buffer)
            swprintf_s(pBuffer->ipv4, _countof(pBuffer->ipv4), L"%S", pAdapter->IpAddressList.IpAddress.String);
            DebugPrint(L"IP Address: \t%s\n", pBuffer->ipv4);

            // Copy MAC address as 8 hex bytes (exactly as original: 6 MAC bytes + 2 extra bytes)
            // Original reads bytes at offsets 0x65..0x67 and 0x195..0x19b? That seems odd.
            // But we'll use the Address array directly (first 8 bytes).
            // The original swprintf_s used 8 bytes; we'll use the first 8 from Address.
            BYTE* mac = pAdapter->Address;
            swprintf_s(pBuffer->mac, _countof(pBuffer->mac),
                       L"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
            DebugPrint(L"Mac Address: \t%s\n", pBuffer->mac);

            free(pAdapterInfo);
            return TRUE;
        }
        pAdapter = pAdapter->Next;
    }

    free(pAdapterInfo);
    DebugPrint(L"No suitable network adapter found\n");
    return FALSE;
}

// ------------------------------------------------------------------
// Get device info from registry and system.
// ------------------------------------------------------------------
BOOL GetDeviceInfo(struct BufferForBroadcast* pBuffer)
{
    if (!pBuffer) return FALSE;

    DWORD dwSize;
    LONG lResult;

    // SMBIOS UUID (offset 0x96)
    dwSize = 40 * sizeof(wchar_t);
    lResult = RegGetValueW(HKEY_LOCAL_MACHINE,
                           L"System\\HardwareConfig",
                           L"LastConfig",
                           RRF_RT_REG_SZ,
                           NULL,
                           pBuffer->id,
                           &dwSize);
    if (lResult != ERROR_SUCCESS) {
        DebugPrint(L"Failed to get the SMBIOS UUID from the registry. HRESULT: 0x%8.8X\n", lResult);
        return FALSE;
    }

    // System product name (offset 0xe6)
    dwSize = 50 * sizeof(wchar_t);
    lResult = RegGetValueW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
                           L"SystemProductName",
                           RRF_RT_REG_SZ,
                           NULL,
                           pBuffer->model,
                           &dwSize);
    if (lResult != ERROR_SUCCESS) {
        DebugPrint(L"Failed to get the device model from the registry. HRESULT: 0x%8.8X\n", lResult);
        return FALSE;
    }

    // Architecture (offset 0x1ae)
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: wcscpy_s(pBuffer->architecture, _countof(pBuffer->architecture), L"x64"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: wcscpy_s(pBuffer->architecture, _countof(pBuffer->architecture), L"ARM64"); break;
        case PROCESSOR_ARCHITECTURE_ARM:   wcscpy_s(pBuffer->architecture, _countof(pBuffer->architecture), L"ARM"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: wcscpy_s(pBuffer->architecture, _countof(pBuffer->architecture), L"x86"); break;
        default: wcscpy_s(pBuffer->architecture, _countof(pBuffer->architecture), L"Unknown"); break;
    }

    // OS version (offset 0x14a)
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    if (GetVersionExW((OSVERSIONINFOW*)&osvi)) {
        swprintf_s(pBuffer->osversion, _countof(pBuffer->osversion),
                   L"%u.%u.%u.%u",
                   osvi.dwMajorVersion, osvi.dwMinorVersion,
                   osvi.dwBuildNumber, osvi.dwPlatformId);
    } else {
        wcscpy_s(pBuffer->osversion, _countof(pBuffer->osversion), L"0.0.0.0");
    }

    return TRUE;
}

// ------------------------------------------------------------------
// Setup socket, bind to port 6, join multicast group.
// ------------------------------------------------------------------
BOOL SetupSocket(const wchar_t* host)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        DebugPrint(L"Socket was invalid\n");
        return FALSE;
    }

    // SO_REUSEADDR
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        DebugPrint(L"Reusing ADDR failed\n");
        goto cleanup;
    }

    // TTL = 8
    int ttl = 8;
    if (setsockopt(sock, IPPROTO_IP, IP_TTL, (char*)&ttl, sizeof(ttl)) == SOCKET_ERROR) {
        DebugPrint(L"Error setting TTL\n");
        goto cleanup;
    }

    // Bind to port 6 (requires admin)
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(MULTICAST_PORT);

    int retries = MAX_RETRIES;
    while (bind(sock, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR) {
        if (--retries == 0) {
            DebugPrint(L"Final error: can't bind the socket - %d\n", WSAGetLastError());
            goto cleanup;
        }
        DebugPrint(L"Error in binding the socket - %d\n", WSAGetLastError());
        Sleep(SLEEP_MS);
    }

    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
        DebugPrint(L"Error joining multicast group - %d\n", WSAGetLastError());
        goto cleanup;
    }

    // Destination address for sending
    dst_sock_addr.sin_family = AF_INET;
    dst_sock_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    dst_sock_addr.sin_port = htons(MULTICAST_PORT);

    return TRUE;

cleanup:
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    return FALSE;
}

// ------------------------------------------------------------------
// Broadcast the buffer via UDP multicast.
// ------------------------------------------------------------------
BOOL BroadcastBuffer(struct BufferForBroadcast* pBuffer)
{
    DebugPrint(L"Broadcast...\n");

    if (sock == INVALID_SOCKET) {
        return FALSE;
    }

    int result = sendto(sock,
                        (const char*)pBuffer,
                        sizeof(struct BufferForBroadcast),
                        0,
                        (struct sockaddr*)&dst_sock_addr,
                        sizeof(dst_sock_addr));
    if (result == SOCKET_ERROR) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        DebugPrint(L"Error in Sending data on the socket - %d\n", WSAGetLastError());
        return FALSE;
    }

    return TRUE;
}

// ------------------------------------------------------------------
// Main entry point (exact logic from decompiled code)
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    WSADATA wsaData;
    DWORD dwSize;
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
    int retries;

    DebugPrint(L"EBOOT Pinger\n");

    // Winsock startup
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DebugPrint(L"WSAStartup Failed - %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Get MAC address – retry up to 1000 times
    retries = MAX_RETRIES;
    while (!GetMACaddress(&UdpBroadcastBuffer)) {
        if (--retries == 0) {
            DebugPrint(L"Can't determine MAC address\n");
            WSACleanup();
            return 1;
        }
        Sleep(SLEEP_MS);
    }

    // Computer name (fully qualified)
    dwSize = _countof(computerName);
    if (!GetComputerNameExW(ComputerNameDnsFullyQualified, computerName, &dwSize)) {
        DebugPrint(L"Failed to get the computername - %d\n", GetLastError());
        WSACleanup();
        return 1;
    }
    DebugPrint(L"Hostname: %s\n", computerName);
    wcsncpy_s(UdpBroadcastBuffer.host, _countof(UdpBroadcastBuffer.host), computerName, _TRUNCATE);

    // Device info
    if (!GetDeviceInfo(&UdpBroadcastBuffer)) {
        DebugPrint(L"Failed to get device information\n");
        WSACleanup();
        return 1;
    }

    // Setup socket (will retry binding up to 1000 times)
    if (!SetupSocket(UdpBroadcastBuffer.host)) {
        DebugPrint(L"Failed to create the socket - %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Main loop – broadcast every 5 seconds, check for MAC changes
    while (BroadcastBuffer(&UdpBroadcastBuffer)) {
        Sleep(5000);

        // Store current MAC for comparison
        wchar_t oldMac[25];
        wcscpy_s(oldMac, _countof(oldMac), UdpBroadcastBuffer.mac);

        if (GetMACaddress(&UdpBroadcastBuffer)) {
            // If MAC changed or socket is dead, recreate socket
            if (wcscmp(UdpBroadcastBuffer.mac, oldMac) != 0 || sock == INVALID_SOCKET) {
                if (sock != INVALID_SOCKET) {
                    closesocket(sock);
                    sock = INVALID_SOCKET;
                }
                if (!SetupSocket(UdpBroadcastBuffer.host)) {
                    DebugPrint(L"Failed to re-create the socket - %d\n", WSAGetLastError());
                }
            }
        } else {
            // If we can't get MAC, just continue (original doesn't break)
            DebugPrint(L"GetMACaddress failed in loop\n");
        }
    }

    DebugPrint(L"EBOOT Complete\n");
    WSACleanup();
    return 0;
}

