/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    kdextension.c

Abstract:

    Network Kernel Debug Extensibility Support.  This module implements the
    extensibility entry points for the 16550 KDNET serial extensibility module.

Author:

    Bill Messmer (wmessmer)

--*/

#include "pch.h"

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4127 4310)

PKDNET_EXTENSIBILITY_IMPORTS KdNetExtensibilityImports;

VOID
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is a dummy function that allows the extensibility module to
    link against the kernel mode GS buffer overflow library.

Arguments:

    DriverObject - Supplies a driver object.

    RegistryPath - Supplies the driver's registry path.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    return;
}

NTSTATUS
KdWriteSerialByte(
    __in PVOID Adapter,
    __in const UCHAR Byte
    )

/*++

Routine Description:

    Write a single byte to the serial port.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Byte - The byte to write to the serial port

Return Value:

    STATUS_SUCCESS / STATUS_UNSUCCESSFUL


--*/

{
    return OX16PCI95XWriteSerialByte(Adapter, Byte);
}

NTSTATUS
KdReadSerialByte(
    __in PVOID Adapter,
    __out PUCHAR Byte
    )

/*++

Routine Description:

    Synchronously read a single byte from the serial port or time out.  This will immediately timeout if there
    is not a byte on the serial FIFO.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    Byte - The byte read from the serial port will be placed here.

Return Value:

    STATUS_SUCCESS on a successful read from the serial port.  STATUS_IO_TIMEOUT if there is no data available.

--*/

{
    return OX16PCI95XReadSerialByte(Adapter, Byte);
}

NTSTATUS
KdDeviceControl(
    __in PVOID Adapter,
    __in ULONG RequestCode,
    __in_bcount(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount(OutputBufferSize) PVOID OutputBuffer,
    __in ULONG OutputBufferSize
    )

/*++

Routine Description:

    Synchronously send a device control request.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    RequestCode - The code for the request being send

    InputBuffer - The input data for the request

    InputBufferSize - The size of the input buffer

    OutputBuffer - The output data for the request

    OutputBufferSize - The size of the output buffer

Return Value:

    NT status code.

--*/

{
    return OX16PCI95XDeviceControl(
        Adapter, 
        RequestCode, 
        InputBuffer, 
        InputBufferSize, 
        OutputBuffer, 
        OutputBufferSize
        );
}

VOID
KdSetHibernateRange (
    VOID
    )
{
    //
    // Mark kdnet extension code.
    //

    PoSetHiberRange(NULL,
                    PO_MEM_BOOT_PHASE,
                    (PVOID) (ULONG_PTR) KdSetHibernateRange,
                    0,
                    'endK');
}

NTSTATUS
KdInitializeController(
    __in PKDNET_SHARED_DATA KdNet
    )

/*++

Routine Description:

    This function initializes the Network controller.  The controller is setup
    to send and recieve packets at the fastest rate supported by the hardware
    link.  Packet send and receive will be functional on successful exit of
    this routine.  The controller will be initialized with Interrupts masked
    since all debug devices must operate without interrupt support.

Arguments:

    KdNet - Supplies a pointer to the data shared with KdNet.

Return Value:

    STATUS_SUCCESS on successful initialization.  Appropriate failure code if
    initialization fails.

--*/

{
    return OX16PCI95XInitializeController(KdNet);
}

VOID
KdShutdownController (
    __in PVOID Adapter
    )

/*++

Routine Description:

    This function shuts down the Network controller.  No further packets can
    be sent or received until the controller is reinitialized.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

Return Value:

    None.

--*/

{
    OX16PCI95XShutdownController(Adapter);
    return;
}

ULONG
KdGetHardwareContextSize (
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    )

/*++

Routine Description:

    This function returns the required size of the hardware context in bytes.

Arguments:

    Device - Supplies a pointer to the debug device descriptor.

Return Value:

    None.

--*/

{
    return OX16PCI95XGetContextSize(Device);
}

NTSTATUS
KdInitializeLibrary (
    __in PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    __in_opt PCHAR LoaderOptions,
    __inout PDEBUG_DEVICE_DESCRIPTOR Device
    )
{
    PKDNET_EXTENSIBILITY_EXPORTS Exports;
    NTSTATUS Status;

    __security_init_cookie();
    UNREFERENCED_PARAMETER(LoaderOptions);
    UNREFERENCED_PARAMETER(Device);
    Status = STATUS_SUCCESS;
    KdNetExtensibilityImports = ImportTable;
    if ((KdNetExtensibilityImports == NULL) ||
        (KdNetExtensibilityImports->FunctionCount != KDNET_EXT_IMPORTS)) {
        Status = STATUS_INVALID_PARAMETER;
        goto KdInitializeLibraryEnd;
    }

    Exports = KdNetExtensibilityImports->Exports;
    if ((Exports == NULL) || (Exports->FunctionCount != KDNET_EXT_EXPORTS)) {
        Status = STATUS_INVALID_PARAMETER;
        goto KdInitializeLibraryEnd;
    }

    //
    // Return the function pointers this KDNET extensibility module exports.
    //

    Exports->KdInitializeController = KdInitializeController;
    Exports->KdShutdownController = KdShutdownController;
    Exports->KdSetHibernateRange = KdSetHibernateRange;
    Exports->KdGetHardwareContextSize = KdGetHardwareContextSize;
    Exports->KdReadSerialByte = KdReadSerialByte;
    Exports->KdWriteSerialByte = KdWriteSerialByte;
    Exports->KdDeviceControl = KdDeviceControl;

    //
    // Return the hardware context size required to support this device.
    //

    Device->Memory.Length = OX16PCI95XGetContextSize(Device);

KdInitializeLibraryEnd:
    return Status;
}


#pragma warning(default:4127 4310)
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

