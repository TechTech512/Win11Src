#include <windows.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <guiddef.h>
#include <winternl.h>
#include <objbase.h>

// Forward declarations for external functions
extern int RtlGUIDFromString(UNICODE_STRING *GuidString, GUID *Guid);

// Basic type definitions
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

// Sockaddr info structures
typedef struct _SOCKADDR_ENDPOINT_INFO {
    ULONG InfoType;
    ULONG InfoValue;
} SOCKADDR_ENDPOINT_INFO, *PSOCKADDR_ENDPOINT_INFO;

typedef struct _SOCKADDR_ADDRESS_INFO {
    ULONG InfoType;
    ULONG InfoValue;
} SOCKADDR_ADDRESS_INFO, *PSOCKADDR_ADDRESS_INFO;

typedef struct _SOCKADDR_INFO {
    SOCKADDR_ADDRESS_INFO AddressInfo;
    SOCKADDR_ENDPOINT_INFO EndpointInfo;
} SOCKADDR_INFO, *PSOCKADDR_INFO;

// Winsock mapping structure
typedef struct _WINSOCK_MAPPING {
    ULONG Rows;
    ULONG Columns;
    ULONG Mapping[9];
} WINSOCK_MAPPING, *PWINSOCK_MAPPING;

// Global variable definitions
const GUID HV_GUID_LOOPBACK = {0x1234191b, 0x4bf7, 0x4ca7, {0x86, 0xe0, 0xdf, 0xd7, 0xc3, '+', 'T', 'E'}};
const GUID HV_GUID_BROADCAST = {0xffffffff, 0xffff, 0xffff, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const GUID HV_GUID_ZERO = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

const SOCKADDR_ENDPOINT_INFO SockaddrEndpointInfoNormal = {0, 0};
const SOCKADDR_ENDPOINT_INFO SockaddrEndpointInfoWildcard = {1, 0};
const SOCKADDR_ADDRESS_INFO SockaddrAddressInfoLoopback = {2, 0};
const SOCKADDR_ADDRESS_INFO SockaddrAddressInfoBroadcast = {3, 0};

const WINSOCK_MAPPING HyperVMappingTriples[] = {
    {3, 3, {0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

const WSAPROTOCOL_INFOW Winsock2Protocols[] = {
    {
        0, 0, 0, 0, 0,
        {0x1234191b, 0x4bf7, 0x4ca7, {0x86, 0xe0, 0xdf, 0xd7, 0xc3, '+', 'T', 'E'}},
        0, {0, 0, 0}, 0, 0x22, 0x24, 0x24, 1, 1, 0, 0, 0, 0, 0, 0,
        L"Vmbus"
    }
};

int __cdecl WSHAddressToString(SOCKADDR *sa, int addressLength, WSAPROTOCOL_INFOW *protocolInfo, wchar_t *addressString, ULONG *addressStringLength)
{
    if (((sa != NULL) && (0xf < (ULONG)addressLength)) && (addressStringLength != NULL) && ((addressString != NULL || (*addressStringLength == 0)))) {
        if (sa->sa_family != 0x22) {
            return 0x2726;
        }
        memset(addressString, 0, *addressStringLength * sizeof(wchar_t));
        int result = StringFromGUID2((GUID *)(sa->sa_data + 2), addressString, *addressStringLength);
        if (result >= 0) {
            return 0;
        }
    }
    return 0x271e;
}

int __cdecl WSHEnumProtocols(int *lpiProtocols, wchar_t *lpProtocolBuffer, void *lpProtocolBuffer2, ULONG *lpdwBufferLength)
{
    if (*lpdwBufferLength < 0x2c) {
        return -1;
    }
    
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 0x18) = 0;
    *(ULONG *)lpProtocolBuffer2 = 0x26;
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 4) = 0x22;
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 8) = 0x24;
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 0xc) = 0x24;
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 0x10) = 1;
    *(ULONG *)((INT_PTR)lpProtocolBuffer2 + 0x14) = 1;
    
    short *bufferPos = (short *)((INT_PTR)lpProtocolBuffer2 + (*lpdwBufferLength - 0xc));
    *(short **)((INT_PTR)lpProtocolBuffer2 + 0x1c) = bufferPos;
    
    const wchar_t *vmbusStr = L"Vmbus";
    int charCount = 6;
    
    do {
        if (charCount == 0) break;
        wchar_t currentChar = *vmbusStr;
        if (currentChar == 0) break;
        *bufferPos = (short)currentChar;
        bufferPos++;
        vmbusStr++;
        charCount--;
    } while (charCount != 0);
    
    *bufferPos = 0;
    *lpdwBufferLength = 0x2c;
    return 1;
}

int __cdecl WSHGetBroadcastSockaddr(void *socketContext, SOCKADDR *broadcastAddress, int *addressLength)
{
    if ((ULONG)*addressLength < 0x24) {
        return 0x271e;
    }
    
    *addressLength = 0x24;
    broadcastAddress->sa_family = 0x22;
    memset(broadcastAddress->sa_data, 0xFF, 14);
    
    // Clear the second part of the address structure
    SOCKADDR *secondAddr = (SOCKADDR *)((char *)broadcastAddress + sizeof(SOCKADDR));
    secondAddr->sa_family = 0;
    memset(secondAddr->sa_data, 0, 14);
    
    return 0;
}

int __cdecl WSHGetProviderGuid(wchar_t *providerName, GUID *providerGuid)
{
    if ((providerName == NULL) || (providerGuid == NULL)) {
        return 0x271e;
    }
    
    if (_wcsicmp(providerName, L"Vmbus") == 0) {
        providerGuid->Data1 = 0x1234191b;
        providerGuid->Data2 = 0x4bf7;
        providerGuid->Data3 = 0x4ca7;
        providerGuid->Data4[0] = 0x86;
        providerGuid->Data4[1] = 0xe0;
        providerGuid->Data4[2] = 0xdf;
        providerGuid->Data4[3] = 0xd7;
        providerGuid->Data4[4] = 0xc3;
        providerGuid->Data4[5] = '+';
        providerGuid->Data4[6] = 'T';
        providerGuid->Data4[7] = 'E';
        return 0;
    }
    
    return 0x2726;
}

int __cdecl WSHGetSockaddrType(SOCKADDR *sa, ULONG addressLength, SOCKADDR_INFO *addressInfo)
{
    if (addressLength < 0x24) {
        return 0x271e;
    }
    
    if (sa->sa_family != 0x22) {
        return 0x273f;
    }
    
    // Check for loopback
    int isLoopback = 1;
    for (int i = 0; i < 4; i++) {
        if (*(int *)(sa->sa_data + i * 4 + 2) != *(int *)(HV_GUID_LOOPBACK.Data4 + i * 4 - 8)) {
            isLoopback = 0;
            break;
        }
    }
    
    if (isLoopback) {
        addressInfo->AddressInfo = SockaddrAddressInfoLoopback;
        return 0;
    }
    
    // Check for broadcast
    int isBroadcast = 1;
    for (int i = 0; i < 4; i++) {
        if (*(int *)(sa->sa_data + i * 4 + 2) != *(int *)(HV_GUID_BROADCAST.Data4 + i * 4 - 8)) {
            isBroadcast = 0;
            break;
        }
    }
    
    if (isBroadcast) {
        addressInfo->AddressInfo = SockaddrAddressInfoBroadcast;
        return 0;
    }
    
    // Check for wildcard
    SOCKADDR *secondAddr = (SOCKADDR *)((char *)sa + sizeof(SOCKADDR));
    int isWildcard = 1;
    for (int i = 0; i < 4; i++) {
        if (*(int *)(secondAddr->sa_data + i * 4 + 2) != *(int *)(HV_GUID_ZERO.Data4 + i * 4 - 8)) {
            isWildcard = 0;
            break;
        }
    }
    
    if (isWildcard) {
        addressInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    } else {
        addressInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }
    
    return 0;
}

int __cdecl WSHGetSocketInformation(void *socketContext, ULONG level, void *optname, void *optval, int optionName, int optionLevel, char *optionValue, int *optionLength)
{
    if ((optionLevel == 0xfffe) && (optionName == 1)) {
        if (optionValue != NULL) {
            if ((ULONG)*optionLength < 4) {
                return 0x271e;
            }
            *(void **)optionValue = socketContext;
        }
        *optionLength = 4;
        return 0;
    }
    
    return 0x273a;
}

int __cdecl WSHGetWildcardSockaddr(void *socketContext, SOCKADDR *wildcardAddress, int *addressLength)
{
    if ((ULONG)*addressLength < 0x24) {
        return 0x271e;
    }
    
    *addressLength = 0x24;
    wildcardAddress->sa_family = 0x22;
    memset(wildcardAddress->sa_data, 0, 14);
    
    // Clear the second part of the address structure
    SOCKADDR *secondAddr = (SOCKADDR *)((char *)wildcardAddress + sizeof(SOCKADDR));
    secondAddr->sa_family = 0;
    memset(secondAddr->sa_data, 0, 14);
    
    return 0;
}

ULONG __cdecl WSHGetWinsockMapping(WINSOCK_MAPPING *mapping, ULONG mappingLength)
{
    if (0x2b < mappingLength) {
        mapping->Rows = 3;
        mapping->Columns = 3;
        memcpy(mapping->Mapping, HyperVMappingTriples, 0x24);
    }
    return 0x2c;
}

int __cdecl WSHGetWSAProtocolInfo(wchar_t *providerName, WSAPROTOCOL_INFOW **protocolInfo, ULONG *protocolCount)
{
    if (((providerName == NULL) || (protocolInfo == NULL)) || (protocolCount == NULL)) {
        return 0x271e;
    }
    
    if (_wcsicmp(providerName, L"Vmbus") == 0) {
        *protocolInfo = (WSAPROTOCOL_INFOW *)Winsock2Protocols;
        *protocolCount = 1;
        return 0;
    }
    
    return 0x2726;
}

int __cdecl WSHIoctl(void *socketContext, ULONG command, void *inBuffer, void *outBuffer, ULONG inBufferLength, void *outBuffer2, ULONG outBufferLength, void *overlapped, ULONG overlappedLength, ULONG *bytesReturned, OVERLAPPED *completionRoutine, void (*completionFunction)(ULONG, ULONG, OVERLAPPED *, ULONG), int *errorCode)
{
    return 0x2726;
}

int __cdecl WSHJoinLeaf(void *socketContext, ULONG addressFamily, void *socketType, void *protocol, void *flags, ULONG timeout, SOCKADDR *remoteAddress, ULONG remoteAddressLength, WSABUF *localAddress, WSABUF *remoteAddress2, struct _QualityOfService *qos, struct _QualityOfService *qos2, ULONG group)
{
    return (socketContext != NULL ? 0x2726 : 0x2726);
}

int __cdecl WSHNotify(void *socketContext, ULONG notificationType, void *notificationData, void *overlapped, ULONG completionValue)
{
    return 0;
}

int __cdecl WSHOpenSocket2(int *addressFamily, int *socketType, int *protocol, ULONG flags, ULONG reserved, UNICODE_STRING *deviceName, void **socketContext, ULONG *socketFlags)
{
    if ((((*addressFamily == 0x22) && (*socketType == 1)) && (*protocol == 1)) && ((reserved & 0xfffffffe) == 0)) {
        RtlInitUnicodeString(deviceName, NULL);
        *socketContext = (void *)(ULONG)(USHORT)*addressFamily;
        *socketFlags = 0;
        return 0;
    }
    
    return 0x2726;
}

int __cdecl WSHOpenSocket(int *addressFamily, int *socketType, int *protocol, UNICODE_STRING *deviceName, void **socketContext, ULONG *flags)
{
    return WSHOpenSocket2(addressFamily, socketType, protocol, 0, 0, deviceName, socketContext, flags);
}

int __cdecl WSHSetSocketInformation(void *socketContext, ULONG level, void *optname, void *optval, int optionName, int optionLevel, char *optionValue, int optionLength)
{
    if ((optionLevel == 0xfffe) && (optionName == 1)) {
        return 0;
    }
    
    return 0x2726;
}

int __cdecl WSHStringToAddress(wchar_t *addressString, ULONG addressFamily, WSAPROTOCOL_INFOW *protocolInfo, SOCKADDR *sa, int *addressLength)
{
    if (addressFamily == 0x22) {
        if ((((addressString == NULL) || (sa == NULL)) || (addressLength == NULL)) || ((ULONG)*addressLength < 0x24)) {
            return 0x271e;
        }
        
        // Clear the entire sockaddr structure
        memset(sa, 0, *addressLength);
        
        UNICODE_STRING guidString;
        RtlInitUnicodeString(&guidString, addressString);
        
        int result = RtlGUIDFromString(&guidString, (GUID *)(sa->sa_data + 2));
        if (result < 0) {
            return 0x2726;
        }
        
        sa->sa_family = 0x22;
        *addressLength = 0x24;
        return 0;
    }
    
    return 0x2726;
}

