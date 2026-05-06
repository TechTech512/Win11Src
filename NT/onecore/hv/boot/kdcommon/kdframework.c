#pragma warning (disable:4789)
#pragma warning (disable:4333)

#include "kdcommon.h"

UCHAR *KdDebuggerNotPresent = (UCHAR *)0x80019000;
unsigned int KdHvMaxPacketSize = 0;
unsigned int KdHvRetryCount = 0;
unsigned long KdHvpSendPacketId = 0;
UCHAR SendBuffer[4088] = {0};
UCHAR ReceiveBuffer[4088] = {0};

// External functions
extern ULONG KdHvComputeChecksum(PUCHAR Data, ULONG Length);
extern DBG_PACKET_RESULT KdHvPostDebugData(ULONG DataSize, ULONG Param2, PUCHAR DataBuffer, PULONG Param4);
extern DBG_PACKET_RESULT KdHvRetrieveDebugData(ULONG BufferSize, PUCHAR OutputBuffer, ULONG Param3, PULONG Param4, PULONG Param5);
extern void KdHvResetDebugSession(ULONG Param1);

// Forward declarations
void KdHvSendControlPacket(PCHAR PacketType, USHORT Param2, ULONG Param3);
ULONG KdHvReceivePacket(PCHAR PacketType, PCHAR DataBuffer, ULONG DataSize, PSTRING String1, PSTRING String2, PULONG Param6, PKD_CONTEXT Context);
void KdHvSendPacket(PCHAR PacketType, ULONG Param2, PSTRING String1, PSTRING String2, PKD_CONTEXT Context);

// Send control packet
void KdHvSendControlPacket(
    PCHAR PacketType,
    USHORT Param2,
    ULONG Param3
)
{
    RtlZeroMemory(SendBuffer, 0x80016060);
    RtlCopyMemory(SendBuffer, "iiii", 4);
    *(PULONG)(SendBuffer + 8) = (ULONG)PacketType;
    SendBuffer[0xc] = 0;
    SendBuffer[0xd] = 0;
    SendBuffer[0xe] = 0;
    SendBuffer[0xf] = 0;
    SendBuffer[6] = 0;
    SendBuffer[7] = 0;
    *(PUSHORT)(SendBuffer + 4) = Param2;
    
    KdHvPostDebugData(0x80016060, 0, NULL, NULL);
}

// Receive packet from hypervisor
ULONG
KdHvReceivePacket(
    PCHAR PacketType,
    PCHAR DataBuffer,
    ULONG DataSize,
    PSTRING String1,
    PSTRING String2,
    PULONG Param6,
    PKD_CONTEXT Context
)
{
    DBG_PACKET_RESULT result;
    USHORT length;
    UCHAR *buffer;
    ULONG returnValue;
    PCHAR controlPacketType;
    BOOLEAN sendControl;
    USHORT uVar8;
    ULONG uVar10;
    ULONG checksum;
    ULONG dataChecksum;
    
    controlPacketType = NULL;
    sendControl = FALSE;
    
    if (String1 != NULL) {
        length = *(PUSHORT)DataSize;
        String1->Length = (SHORT)length;
        String1->MaximumLength = (SHORT)(length >> 16);
    }
    
    if (PacketType == (PCHAR)8) {
        result = KdHvRetrieveDebugData(KdHvMaxPacketSize, (PUCHAR)&buffer, 0, NULL, NULL);
        if (result == DbgPrReceived) {
            KdDebuggerNotPresent = FALSE;
            if (*(PULONG)(ReceiveBuffer) == 'b' && (ULONG)PacketType == 1) {
                KdDebuggerNotPresent = FALSE;
                return 0;
            }
        }
        return 1;
    }
    
    while (TRUE) {
        while (TRUE) {
            if (sendControl) {
                KdHvSendControlPacket(controlPacketType, (USHORT)DataSize, (ULONG)&buffer);
                sendControl = FALSE;
            }
            
            result = KdHvRetrieveDebugData(KdHvMaxPacketSize, (PUCHAR)&buffer, 0, NULL, NULL);
            length = *(PUSHORT)&buffer;
            
            if (result != DbgPrReceived) {
                return 1;
            }
            
            if (*(PUCHAR)ReceiveBuffer == 'b' && (ULONG)PacketType == 1) {
                *(PUCHAR)&String2->Buffer = 1;
                return 2;
            }
            
            if ((ULONG)PacketType < 0x10) {
                return 2;
            }
            
            if (*(PULONG)ReceiveBuffer != 0x69696969) {
                break;
            }
            
            length = *(PUSHORT)(ReceiveBuffer + 4);
            if (length == 4) {
                if (PacketType == (PCHAR)4 && *(PULONG)(ReceiveBuffer + 8) == KdHvpSendPacketId) {
                    KdHvpSendPacketId++;
                    goto received;
                }
            } else if (length == 6) {
                KdHvResetDebugSession((ULONG)DataSize);
                KdHvSendControlPacket((PCHAR)KdHvpSendPacketId, length, (ULONG)&buffer);
                return 2;
            } else if (length == 5) {
                return 2;
            }
        }
        
        if (*(PULONG)ReceiveBuffer == 0x30303030 && PacketType == (PCHAR)4) {
            break;
        }
        
        if (PacketType == (PCHAR)(*(PUSHORT)(ReceiveBuffer + 4))) {
            uVar10 = (ULONG)PacketType;
            if ((*(PULONG)ReceiveBuffer & 0xffffff) == 0x303030) {
                if ((*(PULONG)ReceiveBuffer & 0xff000000) == 0x30000000) {
                    if (DataBuffer == NULL) {
                        goto received;
                    }
                    RtlCopyMemory(DataBuffer, ReceiveBuffer + 0x10, *(PUSHORT)(ReceiveBuffer + 6));
                    length = *(PUSHORT)(DataBuffer + 2);
                    if (*(PUSHORT)(ReceiveBuffer + 6) < 0xfa1 &&
                        *(PUSHORT)(ReceiveBuffer + 6) + 0x10 == (ULONG)PacketType &&
                        length <= *(PUSHORT)(ReceiveBuffer + 6)) {
                        
                        *(PUSHORT)DataBuffer = length;
                        checksum = KdHvComputeChecksum((PUCHAR)DataSize, (ULONG)&buffer);
                        
                        if (DataSize != 0) {
                            if (*(PUSHORT)(DataSize + 2) < ((ULONG)PacketType - length) - 0x10) {
                                controlPacketType = (PCHAR)9;
                                goto send_control;
                            }
                            *(PUSHORT)DataSize = *(PUSHORT)(ReceiveBuffer + 6) - length;
                            RtlCopyMemory((PUCHAR)DataSize + 2, ReceiveBuffer + 0x10 + length, *(PUSHORT)DataSize);
                            length = *(PUSHORT)DataSize;
                            dataChecksum = KdHvComputeChecksum((PUCHAR)DataSize, (ULONG)&buffer);
                            checksum += dataChecksum;
                            
                            if (String1 != NULL) {
                                String1->Length = (SHORT)length;
                                String1->MaximumLength = (SHORT)(length >> 16);
                            }
                        }
                        
                        if (checksum == *(PULONG)(ReceiveBuffer + 12)) {
                            goto received;
                        }
                        controlPacketType = (PCHAR)3;
                    } else {
                        controlPacketType = (PCHAR)((*(PUCHAR)(ReceiveBuffer + (ULONG)PacketType) << 16) | 2);
                    }
                } else {
                    controlPacketType = (PCHAR)((*(PULONG)ReceiveBuffer >> 24) << 8 | 7);
                }
            } else {
                controlPacketType = (PCHAR)(*(PULONG)ReceiveBuffer << 8 | 6);
            }
            
send_control:
            sendControl = TRUE;
        }
    }
    
    KdHvSendControlPacket((PCHAR)((*(PUCHAR)(ReceiveBuffer + 4) << 8) | 5), (USHORT)DataSize, (ULONG)&buffer);
    KdHvpSendPacketId++;
    
received:
    returnValue = 0;
    
    return returnValue;
}

// Send packet to hypervisor
void KdHvSendPacket(
    PCHAR PacketType,
    ULONG Param2,
    PSTRING String1,
    PSTRING String2,
    PKD_CONTEXT Context
)
{
    DBG_PACKET_RESULT result;
    USHORT totalLength;
    USHORT dataLength;
    ULONG packetId;
    ULONG retryCount;
    ULONG checksum;
    ULONG dataChecksum;
    ULONG retries;
    ULONG i;
    BOOLEAN shouldBreak;
    
    packetId = KdHvpSendPacketId;
    totalLength = *(PUSHORT)PacketType + 0x10;
    
    if (Param2 != 0) {
        totalLength += *(PUSHORT)Param2;
    }
    
    if (totalLength > KdHvMaxPacketSize) {
        return;
    }
    
    retries = KdHvRetryCount;
    
    do {
        RtlZeroMemory(SendBuffer, 0x80016060);
        RtlCopyMemory(SendBuffer, "0000", 4);
        dataLength = *(PUSHORT)PacketType;
        *(PUSHORT)(SendBuffer + 4) = (USHORT)Param2;
        *(PULONG)(SendBuffer + 8) = packetId;
        *(PUSHORT)(SendBuffer + 6) = dataLength;
        *(PULONG)(SendBuffer + 12) = KdHvComputeChecksum((PUCHAR)PacketType, dataLength);
        
        if (Param2 != 0) {
            dataChecksum = KdHvComputeChecksum((PUCHAR)Param2, *(PUSHORT)Param2);
            *(PULONG)(SendBuffer + 12) += dataChecksum;
            *(PUSHORT)(SendBuffer + 6) = dataLength + *(PUSHORT)Param2;
        }
        
        RtlCopyMemory(SendBuffer + 0x10, PacketType, dataLength);
        
        if (Param2 != 0) {
            RtlCopyMemory(SendBuffer + 0x10 + dataLength, (PUCHAR)Param2 + 2, *(PUSHORT)Param2);
        }
        
        result = KdHvPostDebugData(0x80016060, 0, SendBuffer, NULL);
        
        if (result != DbgPrSent) {
            KdDebuggerNotPresent = (PBOOLEAN)TRUE;
            KdHvResetDebugSession((ULONG)SendBuffer);
            return;
        }
        
        retryCount = 0;
        
        do {
            KdHvPostDebugData(0, (ULONG)&retryCount, NULL, NULL);
            
            if (retries == 0) {
                shouldBreak = FALSE;
                if (*(PULONG)((PUCHAR)PacketType + 4) == 0x3230) {
                    shouldBreak = TRUE;
                }
                if (shouldBreak) {
                    KdDebuggerNotPresent = (PBOOLEAN)TRUE;
                    KdHvResetDebugSession((ULONG)SendBuffer);
                    return;
                }
            }
            retryCount++;
        } while (retryCount < 0x80000);
        
        retries--;
    } while (TRUE);
}

