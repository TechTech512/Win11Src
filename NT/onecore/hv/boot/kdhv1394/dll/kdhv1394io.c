#include "kdcommon.h"

// External functions
extern ULONG
KdHvReceivePacket(
    PCHAR PacketType,
    PCHAR DataBuffer,
    ULONG DataSize,
    PSTRING String1,
    PSTRING String2,
    PULONG Param6,
    PKD_CONTEXT Context
);
extern void KdHvSendPacket(
    PCHAR PacketType,
    ULONG Param2,
    PSTRING String1,
    PSTRING String2,
    PKD_CONTEXT Context
);

// Receive packet wrapper
ULONG
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT KdContext
)
{
    return KdHvReceivePacket((PCHAR)PacketType, (PCHAR)MessageHeader, (ULONG)MessageData, (PSTRING)DataLength, NULL, NULL, KdContext);
}

VOID
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL,
    IN OUT PKD_CONTEXT KdContext
)
{
    KdHvSendPacket(NULL, 0, (PSTRING)MessageHeader, MessageData, KdContext);
	return;
}

