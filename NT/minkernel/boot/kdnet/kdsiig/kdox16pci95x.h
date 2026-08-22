/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ox16pci95X.h

Abstract:

    Oxford Semi 16PCI95X serial port support.

    This is specifically targeted to the 18.432 MHz clock input to the 16PCI95X on 
    the SIIG CyberPro series of PCI serial cards.  Note that support for any 16PCI95X device
    can be easily added (up to standard 16550 baud rates) by changing recognition of
    the device and clock prescaler parameters.

--*/

typedef struct _OX16PCI95X_ADAPTER {
    PKDNET_SHARED_DATA KdNet;
    PUCHAR IoPort;
    BOOLEAN PortPresent;
    BOOLEAN Are650RegistersMapped;
    BOOLEAN Are950RegistersMapped;
    UCHAR ACR;

    ULONG BaudRate;
    ULONG ErrorCount;
    ULONG NotClearToSend;
    ULONG FifoOverflows;
    ULONG FifoDepth;
    ULONG LowFlowTrigger;
    ULONG HighFlowTrigger;

} OX16PCI95X_ADAPTER, *POX16PCI95X_ADAPTER;

ULONG
OX16PCI95XGetContextSize(
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    );

NTSTATUS
OX16PCI95XInitializeController(
    __in PKDNET_SHARED_DATA KdNet
    );

VOID
OX16PCI95XShutdownController(
    __in POX16PCI95X_ADAPTER Adapter
    );

NTSTATUS
OX16PCI95XWriteSerialByte (
    __in POX16PCI95X_ADAPTER Adapter,
    __in const UCHAR Byte
    );

NTSTATUS
OX16PCI95XReadSerialByte (
    __in POX16PCI95X_ADAPTER Adapter,
    __out PUCHAR Byte
    );

NTSTATUS
OX16PCI95XDeviceControl(
    __in POX16PCI95X_ADAPTER Adapter,
    __in ULONG RequestCode,
    __in_bcount(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount(OutputBufferSize) PVOID OutputBuffer,
    __in ULONG OutputBufferSize
    );

