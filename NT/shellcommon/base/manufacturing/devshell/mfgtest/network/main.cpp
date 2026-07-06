/*
 * networktest.cpp
 *
 * Enumerates network adapters and Wi‑Fi networks.
 * Supports filtering by interface type, operational status, and address family.
 */

#include "precomp.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ws2_32.lib")

// ------------------------------------------------------------------
// Helper: Get numeric hostname from SOCKET_ADDRESS
// ------------------------------------------------------------------
static BOOL GetSocketAddressHostName(const SOCKET_ADDRESS* pSockAddr, wchar_t* host, DWORD hostLen)
{
    int ret = GetNameInfoW(
        pSockAddr->lpSockaddr,
        pSockAddr->iSockaddrLength,
        host,
        hostLen,
        NULL,
        0,
        NI_NUMERICHOST
    );
    if (ret != 0) {
        DWORD err = WSAGetLastError();
        if (err == 0) err = ERROR_INVALID_DATA;
        SetLastError(err);
        return FALSE;
    }
    return TRUE;
}

// ------------------------------------------------------------------
// Comparator for sorting WLAN networks by signal quality (descending)
// ------------------------------------------------------------------
static int __cdecl SortWlanNetworkCompareCallback(const void* a, const void* b)
{
    const WLAN_AVAILABLE_NETWORK* pA = (const WLAN_AVAILABLE_NETWORK*)a;
    const WLAN_AVAILABLE_NETWORK* pB = (const WLAN_AVAILABLE_NETWORK*)b;
    return (int)pB->wlanSignalQuality - (int)pA->wlanSignalQuality;
}

// ------------------------------------------------------------------
// Test network adapters
// ------------------------------------------------------------------
static DWORD TestNetworkAdapters(ULONG family, ULONG flags)
{
    // flags bit definitions:
    // bits 0-3: interface type inclusion (1=Ethernet, 2=Wireless, 4=Tunnel, 8=Loopback)
    // bit 8: include Up interfaces
    // bit 9: include Down interfaces
    // bit 10: include other operational states (if neither up nor down bits set)

    wprintf(L"Network Adapters...\n\n");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        wprintf(L"Failed to start Winsock. Error = 0x%08X\n", WSAGetLastError());
        return 0;
    }

    ULONG outBufLen = 0;
    DWORD dwRetVal = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &outBufLen);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) {
        wprintf(L"Failed get adapter addresses. Error = 0x%08X\n", dwRetVal);
        WSACleanup();
        return dwRetVal;
    }

    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
    if (!pAddresses) {
        wprintf(L"Out of memory\n");
        WSACleanup();
        return ERROR_OUTOFMEMORY;
    }

    dwRetVal = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);
    if (dwRetVal != NO_ERROR) {
        wprintf(L"Failed get adapter addresses. Error = 0x%08X\n", dwRetVal);
        free(pAddresses);
        WSACleanup();
        return dwRetVal;
    }

    PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
    while (pCurr) {
        BOOL include = FALSE;
        ULONG ifType = pCurr->IfType;

        // Check if this interface type is requested
        if (ifType == IF_TYPE_ETHERNET_CSMACD) {
            include = (flags & 1) != 0;
        } else if (ifType == IF_TYPE_IEEE80211) {
            include = (flags & 2) != 0;
        } else if (ifType == IF_TYPE_TUNNEL) {
            include = (flags & 4) != 0;
        } else if (ifType == IF_TYPE_SOFTWARE_LOOPBACK) {
            include = (flags & 8) != 0;
        } else if (ifType == 9) { // Token Ring
            include = (flags & 1) != 0; // treat as Ethernet? In original, they printed "Tokenring" but didn't have a specific flag; we'll include if Ethernet bit is set
        } else {
            include = (flags & 0x10) != 0; // bit 4: "other" types?
        }

        if (!include) {
            pCurr = pCurr->Next;
            continue;
        }

        // Check operational status
        if (pCurr->OperStatus == IfOperStatusUp) {
            if (!(flags & 0x100)) include = FALSE;
        } else if (pCurr->OperStatus == IfOperStatusDown) {
            if (!(flags & 0x200)) include = FALSE;
        } else {
            if (!(flags & 0x400)) include = FALSE;
        }

        if (!include) {
            pCurr = pCurr->Next;
            continue;
        }

        // Print adapter info
        wprintf(L"AdapterName: %ws\n", pCurr->FriendlyName);
        if (pCurr->DnsSuffix && *pCurr->DnsSuffix) {
            wprintf(L"DnsSuffix: %ws\n", pCurr->DnsSuffix);
        }
        if (pCurr->Description && *pCurr->Description) {
            wprintf(L"Description: %ws\n", pCurr->Description);
        }

        // Physical address
        if (pCurr->PhysicalAddressLength) {
            wprintf(L"PhysicalAddress: ");
            for (DWORD i = 0; i < pCurr->PhysicalAddressLength; i++) {
                if (i > 0) wprintf(L"-");
                wprintf(L"%02X", pCurr->PhysicalAddress[i]);
            }
            wprintf(L"\n");
        }

        // Flags
        wprintf(L"Flags:");
        if (pCurr->Flags & IP_ADAPTER_DDNS_ENABLED) wprintf(L" DDNS");
        if (pCurr->Flags & IP_ADAPTER_REGISTER_ADAPTER_SUFFIX) wprintf(L" RegisterAdapterSuffix");
        if (pCurr->Flags & IP_ADAPTER_DHCP_ENABLED) wprintf(L" DHCP");
        if (pCurr->Flags & IP_ADAPTER_RECEIVE_ONLY) wprintf(L" ReceiveOnly");
        if (pCurr->Flags & IP_ADAPTER_NO_MULTICAST) wprintf(L" NoMulticast");
        if (pCurr->Flags & IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG) wprintf(L" OtherStatefulConfig");
        if (pCurr->Flags & IP_ADAPTER_NETBIOS_OVER_TCPIP_ENABLED) wprintf(L" NetBiosOverTCPIP");
        if (pCurr->Flags & IP_ADAPTER_IPV4_ENABLED) wprintf(L" IPv4");
        if (pCurr->Flags & IP_ADAPTER_IPV6_ENABLED) wprintf(L" IPv6");
        if (pCurr->Flags & IP_ADAPTER_IPV6_MANAGE_ADDRESS_CONFIG) wprintf(L" ManageAddressConfig");
        wprintf(L"\n");

        // Interface type
        const wchar_t* typeStr = L"Unknown";
        switch (ifType) {
            case 1: typeStr = L"Other"; break;
            case 6: typeStr = L"Ethernet"; break;
            case 9: typeStr = L"Tokenring"; break;
            case 23: typeStr = L"PPP"; break;
            case 24: typeStr = L"Software Loopback"; break;
            case 37: typeStr = L"ATM"; break;
            case 71: typeStr = L"Wireless"; break;
            case 131: typeStr = L"Tunnel"; break;
            case 144: typeStr = L"1394"; break;
            default: break;
        }
        wprintf(L"IfType: %ws (%u)\n", typeStr, ifType);

        // Oper status
        const wchar_t* opStr = L"Unknown";
        switch (pCurr->OperStatus) {
            case IfOperStatusUp: opStr = L"Up"; break;
            case IfOperStatusDown: opStr = L"Down"; break;
            case IfOperStatusTesting: opStr = L"Testing"; break;
            case IfOperStatusUnknown: opStr = L"Unknown"; break;
            case IfOperStatusDormant: opStr = L"Dormant"; break;
            case IfOperStatusNotPresent: opStr = L"NotPresent"; break;
            case IfOperStatusLowerLayerDown: opStr = L"LowerLayerDown"; break;
            default: break;
        }
        wprintf(L"OperationStatus: %ws (%u)\n", opStr, pCurr->OperStatus);

        // Connection type - we skip as it's not in IP_ADAPTER_ADDRESSES

        // Tunnel type - skip

        // Unicast addresses
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
        while (pUnicast) {
            wchar_t host[1024];
            if (GetSocketAddressHostName(&pUnicast->Address, host, _countof(host))) {
                const wchar_t* familyStr = L"Unknown";
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) familyStr = L"IPv4";
                else if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) familyStr = L"IPv6";
                wprintf(L"%wsAddress: %ws\n", familyStr, host);
            }
            pUnicast = pUnicast->Next;
        }

        // Anycast addresses
        PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = pCurr->FirstAnycastAddress;
        while (pAnycast) {
            wchar_t host[1024];
            if (GetSocketAddressHostName(&pAnycast->Address, host, _countof(host))) {
                const wchar_t* familyStr = L"Unknown";
                if (pAnycast->Address.lpSockaddr->sa_family == AF_INET) familyStr = L"IPv4";
                else if (pAnycast->Address.lpSockaddr->sa_family == AF_INET6) familyStr = L"IPv6";
                wprintf(L"%wsAnycastAddress: %ws\n", familyStr, host);
            }
            pAnycast = pAnycast->Next;
        }

        // Multicast addresses
        PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = pCurr->FirstMulticastAddress;
        while (pMulticast) {
            wchar_t host[1024];
            if (GetSocketAddressHostName(&pMulticast->Address, host, _countof(host))) {
                const wchar_t* familyStr = L"Unknown";
                if (pMulticast->Address.lpSockaddr->sa_family == AF_INET) familyStr = L"IPv4";
                else if (pMulticast->Address.lpSockaddr->sa_family == AF_INET6) familyStr = L"IPv6";
                wprintf(L"%wsMulticastAddress: %ws\n", familyStr, host);
            }
            pMulticast = pMulticast->Next;
        }

        // DNS server addresses
        PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = pCurr->FirstDnsServerAddress;
        while (pDns) {
            wchar_t host[1024];
            if (GetSocketAddressHostName(&pDns->Address, host, _countof(host))) {
                const wchar_t* familyStr = L"Unknown";
                if (pDns->Address.lpSockaddr->sa_family == AF_INET) familyStr = L"IPv4";
                else if (pDns->Address.lpSockaddr->sa_family == AF_INET6) familyStr = L"IPv6";
                wprintf(L"%wsDnsServerAddress: %ws\n", familyStr, host);
            }
            pDns = pDns->Next;
        }

        // Prefixes - we skip because they are not easily accessible in IP_ADAPTER_ADDRESSES
        // (they require separate enumeration)

        // DHCP server - skip

        // MTU
        wprintf(L"MTU: %u bytes\n", pCurr->Mtu);

        // Link speeds
        wprintf(L"TransmitLinkSpeed: %I64u Kb/s\n", pCurr->TransmitLinkSpeed >> 10);
        wprintf(L"ReceiveLinkSpeed: %I64u Kb/s\n", pCurr->ReceiveLinkSpeed >> 10);

        wprintf(L"\n");
        pCurr = pCurr->Next;
    }

    free(pAddresses);
    WSACleanup();
    return NO_ERROR;
}

// ------------------------------------------------------------------
// Test Wi‑Fi networks
// ------------------------------------------------------------------
static DWORD TestWifiNetwork(void)
{
    wprintf(L"Wifi Networks...\n\n");

    DWORD dwNegotiatedVersion = 0;
    HANDLE hClient = NULL;
    DWORD dwRet = WlanOpenHandle(2, NULL, &dwNegotiatedVersion, &hClient);
    if (dwRet != ERROR_SUCCESS) {
        wprintf(L"Failed to open WLAN client session. Error = 0x%08X\n", dwRet);
        return dwRet;
    }

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    dwRet = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if (dwRet != ERROR_SUCCESS) {
        wprintf(L"Failed to enumerate WLAN interfaces. Error = 0x%08X\n", dwRet);
        WlanCloseHandle(hClient, NULL);
        return dwRet;
    }

    for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
        WLAN_INTERFACE_INFO* pIf = &pIfList->InterfaceInfo[i];
        wprintf(L"Interface%u.Description: %ws\n", i, pIf->strInterfaceDescription);

        const wchar_t* stateStr = L"Unknown";
        switch (pIf->isState) {
            case wlan_interface_state_not_ready: stateStr = L"Not Ready"; break;
            case wlan_interface_state_connected: stateStr = L"Connected"; break;
            case wlan_interface_state_ad_hoc_network_formed: stateStr = L"Ad Hoc Network Formed"; break;
            case wlan_interface_state_disconnecting: stateStr = L"Disconnecting"; break;
            case wlan_interface_state_disconnected: stateStr = L"Disconnected"; break;
            case wlan_interface_state_associating: stateStr = L"Associating"; break;
            case wlan_interface_state_discovering: stateStr = L"Discovering"; break;
            case wlan_interface_state_authenticating: stateStr = L"Authenticating"; break;
            default: break;
        }
        wprintf(L"Interface%u.State: %ws\n", i, stateStr);
        wprintf(L"\n");

        PWLAN_AVAILABLE_NETWORK_LIST pNetworkList = NULL;
        dwRet = WlanGetAvailableNetworkList(hClient, &pIf->InterfaceGuid, 0, NULL, &pNetworkList);
        if (dwRet != ERROR_SUCCESS) {
            wprintf(L"Unable to enumerate available networks. Error = 0x%08X\n", dwRet);
            continue;
        }

        // Sort by signal quality (descending)
        qsort(pNetworkList->Network, pNetworkList->dwNumberOfItems, sizeof(WLAN_AVAILABLE_NETWORK),
              SortWlanNetworkCompareCallback);

        for (DWORD j = 0; j < pNetworkList->dwNumberOfItems; j++) {
            WLAN_AVAILABLE_NETWORK* pNet = &pNetworkList->Network[j];

            wprintf(L"Interface%u.Network%u.ProfileName: %ws\n", i, j, pNet->strProfileName);

            // SSID
            wprintf(L"Interface%u.Network%u.SSID: ", i, j);
            if (pNet->dot11Ssid.uSSIDLength == 0) {
                wprintf(L"<unnamed>");
            } else {
                for (DWORD k = 0; k < pNet->dot11Ssid.uSSIDLength; k++) {
                    wprintf(L"%wc", pNet->dot11Ssid.ucSSID[k]);
                }
            }
            wprintf(L"\n");

            // BSS type
            const wchar_t* bssType = L"Unknown";
            switch (pNet->dot11BssType) {
                case dot11_BSS_type_infrastructure: bssType = L"Infrastructure"; break;
                case dot11_BSS_type_independent: bssType = L"Ad hoc"; break;
                case dot11_BSS_type_any: bssType = L"Any"; break;
                default: break;
            }
            wprintf(L"Interface%u.Network%u.BssType: %ws\n", i, j, bssType);

            wprintf(L"Interface%u.Network%u.BssIds: %u\n", i, j, pNet->uNumberOfBssids);
            wprintf(L"Interface%u.Network%u.Connectable: %ws\n", i, j, pNet->bNetworkConnectable ? L"Yes" : L"No");
            wprintf(L"Interface%u.Network%u.PhyTypes: %u\n", i, j, pNet->uNumberOfPhyTypes);
            wprintf(L"Interface%u.Network%u.SignalQuality: %u%%\n", i, j, pNet->wlanSignalQuality);

            // RSSI approximation
            int rssi;
            if (pNet->wlanSignalQuality == 0) rssi = -100;
            else if (pNet->wlanSignalQuality == 100) rssi = -50;
            else rssi = (pNet->wlanSignalQuality / 2) - 100;
            wprintf(L"Interface%u.Network%u.RssiSignalStrength: %d dBm\n", i, j, rssi);

            wprintf(L"Interface%u.Network%u.SecurityEnabled: %ws\n", i, j, pNet->bSecurityEnabled ? L"Yes" : L"No");

            // Auth algorithm - using numeric values from original decompiled code
            const wchar_t* authStr = L"Unknown";
            switch (pNet->dot11DefaultAuthAlgorithm) {
                case 0: authStr = L"Open"; break;
                case 1: authStr = L"Shared"; break;
                case 2: authStr = L"WPA"; break;
                case 3: authStr = L"WPA_PSK"; break;
                case 4: authStr = L"WPA_None"; break;
                case 5: authStr = L"RSNA"; break;
                case 6: authStr = L"RSNA_PSK"; break;
                case 7: authStr = L"WEP"; break;
                default: authStr = L"Unknown"; break;
            }
            wprintf(L"Interface%u.Network%u.AuthAlgorithm: %ws\n", i, j, authStr);

            // Cipher algorithm - using numeric values
            const wchar_t* cipherStr = L"Unknown";
            switch (pNet->dot11DefaultCipherAlgorithm) {
                case 0: cipherStr = L"None"; break;
                case 1: cipherStr = L"WEP40"; break;
                case 2: cipherStr = L"TKIP"; break;
                case 4: cipherStr = L"CCMP"; break;
                case 5: cipherStr = L"WEP104"; break;
                default: cipherStr = L"Unknown"; break;
            }
            wprintf(L"Interface%u.Network%u.CipherAlgorithm: %ws\n", i, j, cipherStr);

            // Flags
            wprintf(L"Interface%u.Network%u.Flags: ", i, j);
            if (pNet->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED) wprintf(L" Connected");
            if (pNet->dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) wprintf(L" HasProfile");
            wprintf(L"\n");

            wprintf(L"\n");
        }

        WlanFreeMemory(pNetworkList);
    }

    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, NULL);
    return ERROR_SUCCESS;
}

// ------------------------------------------------------------------
// Usage
// ------------------------------------------------------------------
static void Usage(const wchar_t* progName)
{
    wprintf(L"USAGE %ws [/adapters <ethernet | wireless | tunnel | loopback> <up | down> <all>] [/wifi]\n", progName);
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    // Disable buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2) {
        // Default: test adapters with default flags
        TestNetworkAdapters(0, 0x103); // 0x103 = bit0 (Ethernet) + bit1 (Wireless) + bit8 (Up)
        return 0;
    }

    // Check for help
    if (_wcsicmp(argv[1], L"-?") == 0 || _wcsicmp(argv[1], L"/?") == 0) {
        Usage(argv[0]);
        return 0;
    }

    // Parse command
    if (_wcsicmp(argv[1], L"/adapters") == 0 || _wcsicmp(argv[1], L"-adapters") == 0) {
        // Parse filters
        ULONG family = 0;       // 0 = both, 2 = IPv4, 0x17 = IPv6
        ULONG flags = 0;        // type and status bits
        BOOL hasType = FALSE;
        BOOL hasStatus = FALSE;

        for (int i = 2; i < argc; i++) {
            if (_wcsicmp(argv[i], L"ipv4") == 0) {
                family = (family != 0x17) ? 2 : 0;
            } else if (_wcsicmp(argv[i], L"ipv6") == 0) {
                family = (family != 2) ? 0x17 : 0;
            } else if (_wcsicmp(argv[i], L"ethernet") == 0) {
                flags |= 1;
                hasType = TRUE;
            } else if (_wcsicmp(argv[i], L"wireless") == 0) {
                flags |= 2;
                hasType = TRUE;
            } else if (_wcsicmp(argv[i], L"tunnel") == 0) {
                flags |= 4;
                hasType = TRUE;
            } else if (_wcsicmp(argv[i], L"loopback") == 0) {
                flags |= 8;
                hasType = TRUE;
            } else if (_wcsicmp(argv[i], L"up") == 0) {
                flags |= 0x100;
                hasStatus = TRUE;
            } else if (_wcsicmp(argv[i], L"down") == 0) {
                flags |= 0x200;
                hasStatus = TRUE;
            } else if (_wcsicmp(argv[i], L"all") == 0) {
                flags = 0x1F | 0x700;   // all types (0-4) and all statuses (8,9,10)
                hasType = TRUE;
                hasStatus = TRUE;
            } else {
                // unknown argument; ignore (as original did)
            }
        }

        // Default type: if none specified, include Ethernet and Wireless (bits 0 and 1)
        if (!hasType) {
            flags |= 1 | 2;
        }
        // Default status: if none specified, include Up (bit 8)
        if (!hasStatus) {
            flags |= 0x100;
        }

        TestNetworkAdapters(family, flags);
        return 0;
    }

    if (_wcsicmp(argv[1], L"/wifi") == 0 || _wcsicmp(argv[1], L"-wifi") == 0) {
        TestWifiNetwork();
        return 0;
    }

    // Unknown argument
    wprintf(L"Invalid argument \"%ws\".\n", argv[1]);
    Usage(argv[0]);
    return ERROR_INVALID_PARAMETER;
}

