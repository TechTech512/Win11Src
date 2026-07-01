/*
 * getadapterinfo.c
 *
 * Enumerates network adapters and displays adapter information,
 * including name, description, IP address, interface type, and MAC address.
 */

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <iphlpapi.h>
#include <strsafe.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

// Debug output function
static void DebugPrint(const wchar_t* format, ...)
{
    wchar_t buffer[2000];
    va_list args;
    va_start(args, format);
    StringCchVPrintfW(buffer, _countof(buffer), format, args);
    va_end(args);
    OutputDebugStringW(buffer);
}

// Display the computer name with fallback methods
static void DisplayComputerName(void)
{
    wchar_t computerName[256];
    DWORD size = sizeof(computerName) / sizeof(wchar_t);
    DWORD dwErr;

    // Try fully qualified DNS name first
    if (GetComputerNameExW(ComputerNameDnsFullyQualified, computerName, &size)) {
        wprintf(L"Hostname: %s\n", computerName);
        DebugPrint(L"Hostname: %s\n", computerName);
        return;
    }

    // If buffer too small, allocate dynamic buffer and try again
    if (GetLastError() == ERROR_MORE_DATA) {
        wchar_t* pName = (wchar_t*)malloc(size * sizeof(wchar_t));
        if (pName) {
            if (GetComputerNameExW(ComputerNameDnsFullyQualified, pName, &size)) {
                wprintf(L"Hostname: %s\n", pName);
                DebugPrint(L"Hostname: %s\n", pName);
                free(pName);
                return;
            }
            free(pName);
        }
    }

    // Fallback to NetBIOS name
    size = sizeof(computerName) / sizeof(wchar_t);
    if (GetComputerNameExW(ComputerNameNetBIOS, computerName, &size)) {
        wprintf(L"Hostname: %s\n", computerName);
        DebugPrint(L"Hostname: %s\n", computerName);
        return;
    }

    // Fallback to GetComputerNameW
    size = sizeof(computerName) / sizeof(wchar_t);
    if (GetComputerNameW(computerName, &size)) {
        wprintf(L"Hostname: %s\n", computerName);
        DebugPrint(L"Hostname: %s\n", computerName);
        return;
    }

    dwErr = GetLastError();
    wprintf(L"Failed to get Computer Name (error %lu)\n", dwErr);
    DebugPrint(L"Failed to get Computer Name (error %lu)\n", dwErr);
}

// Convert interface type to string
static const char* GetInterfaceTypeName(DWORD type)
{
    switch (type) {
        case MIB_IF_TYPE_OTHER:       return "MIB_IF_TYPE_OTHER";
        case MIB_IF_TYPE_ETHERNET:    return "MIB_IF_TYPE_ETHERNET";
        case MIB_IF_TYPE_TOKENRING:   return "MIB_IF_TYPE_TOKENRING";
        case MIB_IF_TYPE_FDDI:        return "MIB_IF_TYPE_FDDI";
        case MIB_IF_TYPE_PPP:         return "MIB_IF_TYPE_PPP";
        case MIB_IF_TYPE_LOOPBACK:    return "MIB_IF_TYPE_LOOPBACK";
        case MIB_IF_TYPE_SLIP:        return "MIB_IF_TYPE_SLIP";
        case IF_TYPE_IEEE80211:       return "IF_TYPE_IEEE80211 [wireless adapter]";
        default:                      return "Unknown";
    }
}

// Enumerate and display network adapters
static void EnumAdapters(void)
{
    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
    DWORD dwRetVal;

    if (pAdapterInfo == NULL) {
        DebugPrint(L"Error allocating memory\n");
        return;
    }

    // First call to get required buffer size
    dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
        if (pAdapterInfo == NULL) {
            DebugPrint(L"Error allocating memory\n");
            return;
        }
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &outBufLen);
    }

    if (dwRetVal != NO_ERROR) {
        DebugPrint(L"Getting Adapter Info failed with error: %d\n", dwRetVal);
        free(pAdapterInfo);
        return;
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter) {
        printf("Adapter Name: %s\n", pAdapter->AdapterName);
        printf("Description : %s\n", pAdapter->Description);

        // Get first IP address (IPv4)
        char ipAddr[16] = "0.0.0.0";
        if (pAdapter->IpAddressList.IpAddress.String) {
            strcpy_s(ipAddr, sizeof(ipAddr), pAdapter->IpAddressList.IpAddress.String);
        }
        printf("IP Address  : %s\n", ipAddr);

        // Interface type
        printf("Ethernet: %s\n", GetInterfaceTypeName(pAdapter->Type));

        // MAC address (6 bytes) - print as hex with colons, then append ":00:00" to match sample
        if (pAdapter->AddressLength == 6) {
            printf("Mac Address: %02x:%02x:%02x:%02x:%02x:%02x:00:00\n",
                   pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
                   pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
        } else {
            printf("Mac Address: (length %d)\n", pAdapter->AddressLength);
        }

        DebugPrint(L"Mac Address: \t%s\n", L"<debug>");
        printf("\n");

        pAdapter = pAdapter->Next;
    }

    free(pAdapterInfo);
}

// Main entry point
int __cdecl wmain(void)
{
    wprintf(L"Enumerate Network Adapters\n\n");
    DisplayComputerName();
    wprintf(L"\n");  // extra blank line to match original output
    EnumAdapters();
    return 0;
}

