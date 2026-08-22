/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    uart16550.h

Abstract:

    Standard 16550 UART serial support.

--*/

typedef struct _UART_16550_ADAPTER {
    PKDNET_SHARED_DATA KdNet;
    PUCHAR LegacyPort;
    BOOLEAN AutoFlowControlSupported;
    BOOLEAN AutoFlowControl;
    BOOLEAN FlowAllowed;
    BOOLEAN PortPresent;
    ULONG BaudRate;

    //
    // Debug counters:
    //

    ULONG ErrorCount;
    ULONG NotClearToSend;
    ULONG RxPacketFlowSpins;
    ULONG RxPacketFlowSpinCounts;
    ULONG FifoOverflows;

} UART_16550_ADAPTER, *PUART_16550_ADAPTER;

ULONG 
Uart16550GetContextSize(
    __inout PDEBUG_DEVICE_DESCRIPTOR Device
    );

NTSTATUS
Uart16550InitializeController(
    __in PKDNET_SHARED_DATA KdNet
    );

VOID
Uart16550ShutdownController(
    __in PUART_16550_ADAPTER Adapter
    );


NTSTATUS
Uart16550WriteSerialByte(
    __in PUART_16550_ADAPTER Adapter,
    __in const UCHAR Byte
    );

NTSTATUS
Uart16550ReadSerialByte(
    __in PUART_16550_ADAPTER Adapter,
    __out PUCHAR Byte
    );

NTSTATUS
Uart16550DeviceControl(
    __in PUART_16550_ADAPTER Adapter,
    __in ULONG RequestCode,
    __in_bcount(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount(OutputBufferSize) PVOID OutputBuffer,
    __in ULONG OutputBufferSize
    );

