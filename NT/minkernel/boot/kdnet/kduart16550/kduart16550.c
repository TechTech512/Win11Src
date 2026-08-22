/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    uart16550.c

Abstract:

    Standard 16550 UART serial support.

--*/

#include "pch.h"

ULONG Uart16550SerialBaudDividend = CLOCK_RATE;
const static ULONG DEFAULT_ADDRESS_SIZE_UART16550 = 8;

#define MC_CTSRTSFLOW 0x20
#define MC_RTS 0x02
#define LSR_TXEMPTY 0x40

#define LC_8DATABITS 0x03
#define LC_1STOPBIT 0x00
#define LC_NOPARITY 0x00

ULONG
Uart16550GetContextSize(
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    )

/*++

Routine Description:

    Return the amount of memory required for the 16550 adapter.

Arguments:

    Device - The debug device descriptor

Return Value:

    The size of memory required for the adapter.

--*/

{
    UNREFERENCED_PARAMETER(Device);

    return sizeof(UART_16550_ADAPTER);
}

VOID
Uart16550SetBaud (
    __in PUART_16550_ADAPTER Adapter,
    __in const ULONG Rate
    )

/*++

Routine Description:

    Set the baud rate for the port and record it in the port object.

Arguments:

    Adapter - the 16550 adapter object

    Rate - baud rate 

--*/

{
    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_LCR, LC_8DATABITS | LC_1STOPBIT | LC_NOPARITY);

    if (Uart16550SerialBaudDividend != 0) {

        //
        // Compute the divsor
        //

        const ULONG DivisorLatch = Uart16550SerialBaudDividend / Rate;
        UCHAR   Lcr;

        //
        // set the divisor latch access bit (DLAB) in the line control reg
        //

        Lcr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_LCR);
        WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_LCR, Lcr | LC_DLAB);
        WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_DLM,
                           (UCHAR)((DivisorLatch >> 8) & 0xff));

        WRITE_PORT_UCHAR(Adapter->LegacyPort, (UCHAR)(DivisorLatch & 0xff));
        WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_LCR, Lcr);
    }
}

BOOLEAN
Uart16750SetAutoFlow(
    __in PUART_16550_ADAPTER Adapter,
    __in BOOLEAN AutoFlow
    )

/*++

Routine Description:

    Sets the auto-flow control enable bit as specified.  This will only 
    succeed on a 16750+ UART.

Arguments:

    Adapter - the 16550 adapter object

    AutoFlow - the value to set the automatic flow control bit to

Return Value:

    Whether the automatic flow control could be set.

--*/

{
    BOOLEAN Succeeded;

    UCHAR Mcr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_MCR);
    Mcr |= MC_CTSRTSFLOW;
    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_MCR, Mcr);
    Mcr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_MCR);

    Succeeded = ((Mcr & MC_CTSRTSFLOW) == MC_CTSRTSFLOW);
    if (!Succeeded) {
        Adapter->AutoFlowControl = FALSE;
    } else {
        Adapter->AutoFlowControl = AutoFlow;
    }

    return Succeeded;
}

VOID
Uart16550RequestToSend(
    __in PUART_16550_ADAPTER Adapter,
    __in BOOLEAN Rts
    )

/*++

Routine Description:

    Sets the request to send bit as specified.  This will affect flow control
    from the other side.

Arguments:

    Adapter - the 16550 adapter object

    Rts - the state of the RTS bit

--*/

{
    UCHAR Mcr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_MCR);
    if (Rts) {
        Mcr |= MC_RTS;
    } else {
        Mcr &= ~MC_RTS;
    }
    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_MCR, Mcr);
    Adapter->FlowAllowed = Rts;
}

NTSTATUS
Uart16550InitializeController(
    __in PKDNET_SHARED_DATA KdNet
    )

/*++

Routine Description:

    Initializes the serial port for communication at the specified baud rate / N81.

Arguments:

    KdNet - The data shared with KdNet 

Return Value:

    STATUS_SUCCESS / STATUS_UNSUCCESSFUL

--*/

{
    PUART_16550_ADAPTER Adapter;
    NTSTATUS Status;

    //
    // Ensure the vendor ID is not selecting the MSHV_NET transport.
    // This check MUST NOT be removed, so that shared boot and hyper-v
    // debugging work correctly.
    //

    if (KdNet->Device->VendorID == PCI_VID_MSHV_NET) {
        Status = STATUS_NO_SUCH_DEVICE;
        goto Uart16550InitializeControllerEnd;
    }

    Adapter = (PUART_16550_ADAPTER)KdNet->Hardware;
    Adapter->KdNet = KdNet;

    if (!KdNet->Device->BaseAddress[0].Valid ||
        KdNet->Device->BaseAddress[0].Type != CmResourceTypePort ||
        KdNet->Device->BaseAddress[0].TranslatedAddress == NULL) {

        Status = STATUS_UNSUCCESSFUL;
        goto Uart16550InitializeControllerEnd;
    }

    Adapter->BaudRate = KdNet->SerialBaudRate;
    Adapter->PortPresent = TRUE;
    Adapter->LegacyPort = KdNet->Device->BaseAddress[0].TranslatedAddress;
    Adapter->FlowAllowed = TRUE;

    //
    // Disable all uart interrupts.
    //

    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_IEN, 0);
    Uart16550SetBaud(Adapter, Adapter->BaudRate);

    //
    // Enable the FIFO.
    //

    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_FCR, FC_ENABLE);

    //
    // We cannot support KDNET without some form of flow control.  The packets
    // which come across are simply too big to wait between the KD polling 
    // interval.
    //
    // If the underlying UART supports hardware flow control, turn it on. 
    // Otherwise, we need to perform flow control in the driver 
    // In a true 16550, the MC_CTSRTSFLOW bit is hard-wired 
    // to zero.  In compatible higher level UARTs (16750+), this bit 
    // is the flow control bit.  We can detect the presence of auto-flow 
    // by writing the bit and reading it back.  If it is still zero, we 
    // have no capability for auto-flow on the UART.
    //
    // Note that RTS is not a line assertion when MC_CTSRTSFLOW is enabled; rather an
    // enablement of the RTS side of flow control.
    //

    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_MCR, MC_DTRRTS);
    Adapter->AutoFlowControlSupported = Uart16750SetAutoFlow(Adapter, TRUE);
    Status = STATUS_SUCCESS;

Uart16550InitializeControllerEnd:

    return Status;
}

VOID
Uart16550ShutdownController(
    __in PUART_16550_ADAPTER Adapter
    )

/*++

Routine Description:

    Shuts down communication with the serial port.

Arguments:

    Adapter - The 16550 adapter object

--*/

{
    UCHAR lsr;
    ULONG Counter;

    Counter = 0;

    //
    // Wait for the transmit FIFO to completely empty.  Give up if we cannot
    // completely empty the FIFO in a "reasonable period" (currently defined
    // at 2 seconds).
    //

    for(;;)
    {
        lsr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_LSR);
        if (lsr & LSR_TXEMPTY) {
            break;
        }
        KeStallExecutionProcessor(1000); // 1 mS stall

        Counter += 1;
        if (Counter > 2000) {
            break;
        }
    }
}

BOOLEAN
Uart16550IsClearToSend(
    __in PUART_16550_ADAPTER Adapter
    )
/*++

Routine Description:

    Returns the CTS indicator which determines whether we are clear to send.

Arguments:

    Adapter - The 16550 adapter object.

Return Value:

    An indication of whether the receiver on the other side has enough FIFO room for the byte as indicated by
    the CTS line.

--*/
{
    UCHAR Msr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_MSR);
    return (Msr & SERIAL_MSR_CTS) != 0;
}

UCHAR
Uart16550ReadLsr (
    __in PUART_16550_ADAPTER Adapter,
    UCHAR   waiting
    )

/*++

Routine Description:

    Read LSR byte from specified port.  If HAL owns port & display
    it will also cause a debug status to be kept up to date.

    Handles entering & exiting modem control mode for debugger.

Arguments:

    Adapter - the 16550 adapter object
    
Returns:

    Byte read from port

--*/
{
    const static ULONG DBG_ACCEPTABLE_ERRORS = 25;

    UCHAR   lsr, msr;

    lsr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_LSR);

    //
    // Check to see if the port still exists.
    //

    if (lsr == SERIAL_LSR_NOT_PRESENT) {
        Adapter->ErrorCount += 1;
        if (Adapter->ErrorCount >= DBG_ACCEPTABLE_ERRORS) {
            Adapter->PortPresent = FALSE;
            Adapter->ErrorCount = 0;
        }

        return SERIAL_LSR_NOT_PRESENT;
    }

    if (lsr & waiting) {
        return lsr;
    }

    msr = READ_PORT_UCHAR(Adapter->LegacyPort + COM_MSR);
    return lsr;
}

NTSTATUS
Uart16550WriteSerialByte (
    __in PUART_16550_ADAPTER Adapter,
    __in const UCHAR Byte
    )

/*++

Routine Description:

    Write a byte out to the specified com port.

Arguments:

    Adapter - the 16550 adapter object

    Byte - data to emit

--*/

{
    UCHAR lsr;
	
    if (!Adapter->PortPresent) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Wait for port to not be busy and CTS to be asserted from the other side.
    //
    if (!Uart16550IsClearToSend(Adapter)) {
        Adapter->NotClearToSend++;
        return STATUS_IO_TIMEOUT;
    }

    lsr = Uart16550ReadLsr(Adapter, COM_OUTRDY);
    if ((lsr & COM_OUTRDY) == 0) {
        return STATUS_IO_TIMEOUT;
    }

    //
    // Send the byte
    //

    WRITE_PORT_UCHAR(Adapter->LegacyPort + COM_DAT, Byte);
    return STATUS_SUCCESS;
}

NTSTATUS
Uart16550ReadSerialByte (
    __in PUART_16550_ADAPTER Adapter,
    __out PUCHAR Byte
    )

/*++

Routine Description:

    Fetch a byte and return it.

Arguments:

    Adapter - the 16550 adapter object

    Byte - address of variable to hold the result

Return Value:

    STATUS_SUCCESS if data returned.

    STATUS_IO_TIMEOUT if no data available, but no error.

    STATUS_UNSUCCESSFUL if error (overrun, parity, etc.)

--*/

{
    UCHAR   lsr;
    UCHAR   value;

    //
    //  Make sure DTR and CTS are set
    //
    //  (What does CTS have to do with reading from a full duplex line???)
    //

    if (!Adapter->PortPresent) {
        if (Uart16550ReadLsr(Adapter, COM_DATRDY) == SERIAL_LSR_NOT_PRESENT) {

            return STATUS_IO_TIMEOUT;
        } else {
            Uart16550SetBaud(Adapter, Adapter->BaudRate);
            Adapter->PortPresent = TRUE;
        }
    }

    lsr = Uart16550ReadLsr(Adapter, COM_DATRDY);
    if (lsr == SERIAL_LSR_NOT_PRESENT) {
        return(STATUS_IO_TIMEOUT);
    }

    if ((lsr & COM_DATRDY) == COM_DATRDY) {

        //
        // Check for errors
        //

        if (lsr & (COM_FE | COM_PE | COM_OE)) {
            if (lsr & COM_OE) {
                Adapter->FifoOverflows++;
            } else {
                Adapter->ErrorCount++;
            }
            
            //
            // Even if there is a FIFO error or an indication of an error with
            // the byte, read it and return it.  The protocol layer will deal
            // with this.
            //
        }

        //
        // fetch the byte
        //

        value = READ_PORT_UCHAR(Adapter->LegacyPort + COM_DAT);
        *Byte = value & (UCHAR)0xff;
        return STATUS_SUCCESS;
    }

    Uart16550ReadLsr (Adapter, 0);
    return STATUS_IO_TIMEOUT;
}

NTSTATUS
Uart16550DeviceControl(
    __in PUART_16550_ADAPTER Adapter,
    __in ULONG RequestCode,
    __in_bcount(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount(OutputBufferSize) PVOID OutputBuffer,
    __in ULONG OutputBufferSize
    )

/*++

Routine Description:

    Synchronously handle a device control request.

Arguments:

    Adapter - Supplies a pointer to the debug adapter object.

    RequestCode - The code for the request being sent

    InputBuffer - The input data for the request

    InputBufferSize - The size of the input buffer

    OutputBuffer - The output data for the request

    OutputBufferSize - The size of the output buffer

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;

    Status = STATUS_INVALID_DEVICE_REQUEST;

    switch(RequestCode) {
        //
        // We support automatic flow control if we are a detected 16750 or 
        // (eventually) if we have an interrupt which can manage RTS/CTS and
        // FIFO draining.
        //

        case KD_DEVICE_CONTROL_SERIAL_QUERY_FLOW_CONTROL_SUPPORTED:
            if (OutputBufferSize < sizeof(BOOLEAN)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((BOOLEAN *)OutputBuffer) = Adapter->AutoFlowControlSupported;
            Status = STATUS_SUCCESS;
            break;

        case KD_DEVICE_CONTROL_SERIAL_SET_FLOW_CONTROL:
            if (InputBufferSize < sizeof(BOOLEAN)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (Uart16750SetAutoFlow(Adapter, *(BOOLEAN *)InputBuffer)) {
                Status = STATUS_SUCCESS;
            }
            break;

        case KD_DEVICE_CONTROL_SERIAL_SET_REMOTE_FLOW:
            if (InputBufferSize < sizeof(BOOLEAN)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Uart16550RequestToSend(Adapter, *(BOOLEAN *)InputBuffer);
            break;

        default:
            break;
    }

    return Status;
}
