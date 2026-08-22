/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ox16pci95X.c

Abstract:

    Oxford Semi 16PCI95X (16950) serial port support.

    This is specifically targeted to the 18.432 MHz clock input to the 16PCI95X
    on the SIIG CyberPro series of PCI serial cards.  Note that support for 
    any 16PCI95X device can be easily added (up to standard 16550 baud rates) by
    changing recognition of the device and clock prescaler parameters.

--*/

#include "pch.h"

ULONG OX16PCI95XSerialBaudDividend = CLOCK_RATE;
const static ULONG DEFAULT_ADDRESS_SIZE_UART16550 = 8;

//
// The clock on the SIIG CyberPro 2S is 18.432 MHz.  The OX16PCI95X needs 
// to be programmed with a prescaler divisor of (M=10 + N=0/8) in order to 
// allow the DLL and DLM divisor latch to behave identically to a 16550 
// and be programmable as such.
//

#define PCI_VID_OXFORDSEMI 0x1415
#define PCI_DID_SIIG_CYBERPRO_OX16PCI95X 0x950A

#define ENABLED 1
#define DISABLED 0

//
// 550 registers:
//

#define OX16PCI95X_COM_ICR 0x05
#define OX16PCI95X_COM_SPR 0x07

#define OX16PCI95X_LSR_TXEMPTY_SHIFT 6
#define OX16PCI95X_LSR_TXEMPTY_MASK 0x40

#define OX16PCI95X_LCR_DATALENGTH_SHIFT 0
#define OX16PCI95X_LCR_DATALENGTH_MASK 0x03
#define OX16PCI95X_LCR_DATALENGTH_5_BIT 0x00
#define OX16PCI95X_LCR_DATALENGTH_6_BIT 0x01
#define OX16PCI95X_LCR_DATALENGTH_7_BIT 0x02
#define OX16PCI95X_LCR_DATALENGTH_8_BIT 0x03

#define OX16PCI95X_LCR_STOP_SHIFT 2
#define OX16PCI95X_LCR_STOP_MASK 0x04
#define OX16PCI95X_LCR_STOP_1_BIT 0x00
#define OX16PCI95X_LCR_STOP_2_BIT 0x01

#define OX16PCI95X_LCR_PARITY_SHIFT 3
#define OX16PCI95X_LCR_PARITY_MASK 0x1C
#define OX16PCI95X_LCR_PARITY_NONE 0x00
#define OX16PCI95X_LCR_PARITY_ODD 0x01
#define OX16PCI95X_LCR_PARITY_EVEN 0x03
#define OX16PCI95X_LCR_PARITY_FORCED_1 0x05
#define OX16PCI95X_LCR_PARITY_FORCED_0 0x07

#define OX16PCI95X_COM_LCR_650_REGISTER_ACCESS 0xBF

#define OX16PCI95X_MCR_BAUD_PRESCALE_SHIFT 7
#define OX16PCI95X_MCR_BAUD_PRESCALE_MASK 0x80
#define OX16PCI95X_MCR_BAUD_PRESCALE_BYPASS 0x00
#define OX16PCI95X_MCR_BAUD_PRESCALE_CPR_MN8 0x01

#define OX16PCI95X_MCR_RTSCTS_ENABLE_SHIFT 5

#define OX16PCI95X_MSR_CTS_SHIFT 4
#define OX16PCI95X_MSR_CTS_MASK 0x10
#define OX16PCI95X_MSR_CTS_ASSERTED 0x01
#define OX16PCI95X_MSR_CTS_NOT_ASSERTED 0x0

//
// 650 registers: (must write 0xBF to LCR to access)
//

#define OX16PCI95X_COM_650_EFR 0x02

#define OX16PCI95X_EFR_MODE_SHIFT 4
#define OX16PCI95X_EFR_MODE_MASK 0x10
#define OX16PCI95X_EFR_MODE_NORMAL 0
#define OX16PCI95X_EFR_MODE_ENHANCED 1

#define OX16PCI95X_EFR_CTS_SHIFT 7
#define OX16PCI95X_EFR_CTS_MASK 0x80

#define OX16PCI95X_EFR_RTS_SHIFT 6
#define OX16PCI95X_EFR_RTS_MASK 0x40

//
// Any IDX* register is written through the following procedure:
//
//     - Ensure LCR was not last written for extended register access (0xBF)
//     - Write the index of the register to COM_SPR
//     - Write the value to COM_ICR
//
// Any IDX* register is read through the following procedure:
//
//     - Ensure LCR was not last written for extended register access (0xBF)
//     - Write the ACR index (0x00) to COM_SPR
//     - Set ACR_READENABLE in the ACR (you cannot read the ACR ...  do not 
//       change other bits from desired values)
//     - Write the index of the register to COM_SPR
//     - Read the value from COM_ICR
//     - Write the ACR index (0x00) to COM_SPR
//     - Clear ACR_READENABLE in the ACR (you cannot read the ACR ...  do not
//       change other bits from desired values)
//

//
// ACR Indexed Register:
//

#define OX16PCI95X_IDX_ACR 0x00

#define OX16PCI95X_ACR_ADDITIONALSTATUS_SHIFT 7 
#define OX16PCI95X_ACR_ADDITIONALSTATUS_MASK 0x80
#define OX16PCI95X_ACR_READENABLE_SHIFT 6
#define OX16PCI95X_ACR_READENABLE_MASK 0x40
#define OX16PCI95X_ACR_950TRIGGERLEVEL_SHIFT 5
#define OX16PCI95X_ACR_950TRIGGERLEVEL_MASK 0x20
#define OX16PCI95X_ACR_DTRCTL_SHIFT 3
#define OX16PCI95X_ACR_DTRCTL_MASK 0x18
#define OX16PCI95X_ACR_AUTODSRFLOW_SHIFT 2
#define OX16PCI95X_ACR_AUTODSRFLOW_MASK 0x04
#define OX16PCI95X_ACR_TXDISABLE_SHIFT 1
#define OX16PCI95X_ACR_TXDISABLE_MASK 0x02
#define OX16PCI95X_ACR_RXDISABLE_SHIFT 0
#define OX16PCI95X_ACR_RXDISABLE_MASK 0x01

//
// CPR Indexed Register:
//

#define OX16PCI95X_IDX_CPR 0x01
#define OX16PCI95X_CPR_PRESCALER_INT_SHIFT 3
#define OX16PCI95X_CPR_PRESCALER_INT_MASK 0xF8
#define OX16PCI95X_CPR_PRESCALER_FRAC_SHIFT 0
#define OX16PCI95X_CPR_PRESCALER_FRAC_MASK 0x07

//
// TCR Indexed Register:
//

#define OX16PCI95X_IDX_TCR 0x02
#define OX16PCI95X_TCR_SAMPCLOCK_SHIFT 0
#define OX16PCI95X_TCR_SAMPCLOCK_MASK 0x0F
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_16 0x00
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_15 0x0F
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_14 0x0E
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_13 0x0D
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_12 0x0C
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_11 0x0B
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_10 0x0A
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_9 0x09
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_8 0x08
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_7 0x07
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_6 0x06
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_5 0x05
#define OX16PCI95X_TCR_SAMPCLOCK_DIV_4 0x04

//
// FCL Indexed Register:
//

#define OX16PCI95X_IDX_FCL 0x06
#define OX16PCI95X_AUTOFLOW_LOW_MASK 0x7F

//
// FCH Indexed Register:
//

#define OX16PCI95X_IDX_FCH 0x07
#define OX16PCI95X_AUTOFLOW_HIGH_MASK 0x7F

//
// 950 specific registers:
// (These are "mostly" read only status registers mapped on top of IER, LCR, and MCR.
//

#define OX16PCI95X_COM_950_ASR 0x01

#define OX16PCI95X_ASR_TXIDLE_SHIFT 7
#define OX16PCI95X_ASR_TXIDLE_MASK 0x80
#define OX16PCI95X_ASR_TXIDLE_IDLE 1
#define OX16PCI95X_ASR_TXIDLE_TRANSMITTING 0

#define OX16PCI95X_ASR_FIFOSIZE_SHIFT 6
#define OX16PCI95X_ASR_FIFOSIZE_MASK 0x40
#define OX16PCI95X_ASR_FIFOSIZE_16DEEP 0
#define OX16PCI95X_ASR_FIFOSIZE_128DEEP 1

#define OX16PCI95X_ASR_FIFOSEL_SHIFT 5
#define OX16PCI95X_ASR_FIFOSEL_MASK 0x20

#define OX16PCI95X_COM_950_RFL 0x03
#define OX16PCI95X_COM_950_TFL 0x04

ULONG
OX16PCI95XGetContextSize(
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    )

/*++

Routine Description:

    Return the amount of memory required for the OX16PCI95X adapter.

Arguments:

    Device - The debug device descriptor

Return Value:

    The size of memory required for the adapter.

--*/

{
    UNREFERENCED_PARAMETER(Device);

    return sizeof(OX16PCI95X_ADAPTER);
}

VOID
OX16PCI95XGetClockPrescalerDivisor(
    __in PDEBUG_DEVICE_DESCRIPTOR Device,
    __out PUCHAR Integral,
    __out PUCHAR Fractional
    )
/*++

Routine Description:

    Returns the clock prescaler settings required to downsample the clock
    input of the OX16PCI95X to the 1.8432 MHz standard clock of a 16550 UART. 

Arguments:

    Device - The debug device descriptor

    Integral - The integral portion of the clock prescaler divisor is 
               returned here

    Fractional - The fractional (as 8-ths) portion of the clock prescaler
                 divisor is returned here.

--*/

{
    //
    // Currently, this only supports the 18.432 MHz clock input on the SIIG 
    // CyberPro 2S board.  Any other OX16PCI95X board can be supported by
    // returning the appropriate prescaler divisor for the clock input to the
    // chip in this method.
    //

    switch(Device->VendorID)
    {
        case PCI_VID_OXFORDSEMI:
            switch(Device->DeviceID)
            {
                case PCI_DID_SIIG_CYBERPRO_OX16PCI95X:
                    // 
                    // Clock is 18.432 MHz.  Downscale to 1.8432 MHz as
                    // standard 16550 clock rate by setting a prescaler
                    // divisor of 10 + 0/8
                    //

                    *Integral = 10;
                    *Fractional = 0;
                    break;

                default:
                    //
                    // Should this assert ever fire, this is out of sync with
                    // the supported device list in OX16PCI95XIsSupportedDevice.
                    //

                    NT_ASSERT(FALSE);
                    break;
            }

            break;

        default:
            //
            // Should this assert ever fire, this is out of sync with
            // the supported device list in OX16PCI95XIsSupportedDevice.
            //

            NT_ASSERT(FALSE);
            break;

    }
}

BOOLEAN
OX16PCI95XIsSupportedDevice(
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    )
/*++

Routine Description:

    Checks whether or not this is a supported device running the 
    OX16PCI95x chip.

Arguments:

    Device - The debug device descriptor

Return Value:

    TRUE - The device is supported by this KD extension module.

    FALSE - The device is not supported by this KD extension module.

--*/

{
    BOOLEAN IsSupported = TRUE;

    if (Device->NameSpace != KdNameSpacePCI) {
        IsSupported = FALSE;
        goto OX16PCI95XIsSupportedDeviceEnd;
    }

    switch (Device->VendorID) {
        case PCI_VID_OXFORDSEMI: { /* 1415 */
            switch (Device->DeviceID) {
                case PCI_DID_SIIG_CYBERPRO_OX16PCI95X: /* 950A */
                    break;

                default:
                    IsSupported = FALSE;
            }
            break;
        }

        default:
            IsSupported = FALSE;
    }

OX16PCI95XIsSupportedDeviceEnd:

    return IsSupported;
}

VOID 
OX16PCI95XWriteIndexedRegister (
    __in POX16PCI95X_ADAPTER Adapter, 
    __in UCHAR RegisterIndex,
    __in UCHAR Value
    )

/*++

Routine Description:

    Writes a value to an index controlled register on the 16PCI95X.

Arguments:

    Adapter - The OX16PCI95X adapter object.

    RegisterIndex - The index of the register to write.

    Value - The value to write to the register

--*/

{
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_SPR, RegisterIndex);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_ICR, Value);
}

UCHAR
OX16PCI95XReadIndexedRegister(
    __in POX16PCI95X_ADAPTER Adapter,
    __in UCHAR RegisterIndex
    )

/*++

Routine Description:

    Reads a value from an index controlled register on the 16PCI95X.

Arguments:

    Adapter - The OX16PCI95X adapter object.

    RegisterIndex - The index of the register to read.

Return Value:

    The value read from the specified index controlled register on the 16PCI95X.

--*/

{
    UCHAR Value;

    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_SPR, OX16PCI95X_IDX_ACR);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_ICR, 
                     Adapter->ACR | (ENABLED << OX16PCI95X_ACR_READENABLE_SHIFT)); 
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_SPR, RegisterIndex);
    Value = READ_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_ICR);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_SPR, OX16PCI95X_IDX_ACR);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_ICR, Adapter->ACR); 
    return Value;
}


VOID
OX16PCI95XMap950Registers(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Maps the 950 register set into I/O space.

Arguments:

    Adapter - the OX16PCI95X adapter object

--*/

{
    if (!Adapter->Are950RegistersMapped) {
        Adapter->ACR |= (ENABLED << OX16PCI95X_ACR_ADDITIONALSTATUS_SHIFT);
        OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_ACR, Adapter->ACR);
        Adapter->Are950RegistersMapped = TRUE;
    }
}

VOID
OX16PCI95XUnmap950Registers(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Unmaps the 950 register set from I/O space.

Arguments:

    Adapter - the OX16PCI95X adapter objecto

--*/

{
    Adapter->ACR &= ~(ENABLED << OX16PCI95X_ACR_ADDITIONALSTATUS_SHIFT);
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_ACR, Adapter->ACR);
    Adapter->Are950RegistersMapped = FALSE;
}

VOID
OX16PCI95XMap650Registers(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Maps the 650 register set into I/O space.  

Arguments:

    Adapter - the OX16PCI95X adapter object

--*/
        
{
    if (!Adapter->Are650RegistersMapped) {
        WRITE_PORT_UCHAR(Adapter->IoPort + COM_LCR, 
                         OX16PCI95X_COM_LCR_650_REGISTER_ACCESS);
        Adapter->Are650RegistersMapped = TRUE;
    }
}

VOID
OX16PCI95XUnmap650Registers(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Unmaps the 650 register set from I/O space undoing a prior call 
    to OX16PCI95XMap650Registers.

Arguments:

    Adapter - the OX16PCI95X adapter object

--*/
{
    //
    // The value read from the Lcr will *NOT* be 0xBF.  The LCR will 
    // read as if the divisor latch registers are mapped instead.  Simply
    // clear the DLAB bit.
    //

    UCHAR Lcr = READ_PORT_UCHAR(Adapter->IoPort + COM_LCR);
    Lcr &= ~LC_DLAB;
    WRITE_PORT_UCHAR(Adapter->IoPort + COM_LCR, Lcr);
    Adapter->Are650RegistersMapped = FALSE;
}

BOOLEAN
OX16PCI95XIsClearToSend(
    __in POX16PCI95X_ADAPTER Adapter
    )
/*++

Routine Description:

    Returns the CTS indicator which determines whether we are clear to send.

Arguments:

    Adapter - The OX16PCI95X adapter object.

Return Value:

    An indication of whether the receiver on the other side has enough FIFO
    room for the byte as indicated by the CTS line.

--*/
{
    UCHAR Msr = READ_PORT_UCHAR(Adapter->IoPort + COM_MSR);
    return (Msr & (OX16PCI95X_MSR_CTS_ASSERTED << OX16PCI95X_MSR_CTS_SHIFT)) != 0;
}

UCHAR
OX16PCI95XReadLsr (
    __in POX16PCI95X_ADAPTER Adapter,
    UCHAR   waiting
    )

/*++

Routine Description:

    Read LSR byte from specified port.  If HAL owns port & display
    it will also cause a debug status to be kept up to date.

    Handles entering & exiting modem control mode for debugger.

Arguments:

    Adapter - The OX16PCI95X adapter object.

Returns:

    Byte read from port

--*/
{
    const static ULONG DBG_ACCEPTABLE_ERRORS = 25;

    UCHAR   lsr, msr;

    lsr = READ_PORT_UCHAR(Adapter->IoPort + COM_LSR);

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

    msr = READ_PORT_UCHAR(Adapter->IoPort + COM_MSR);
    return lsr;
}

VOID
OX16PCI95XSetBaud (
    __in POX16PCI95X_ADAPTER Adapter,
    const ULONG Rate
    )

/*++

Routine Description:

    Set the baud rate for the port and record it in the port object.

Arguments:

    Adapter - The OX16PCI95X adapter object.

    Rate - baud rate 

--*/

{
    //
    // While the 16PCI95X has a register set which is compatible with a 
    // standard 16550 UART, the clock frequency of the SIIG board is 18.432 MHz
    // which is 10x the clock rate of the standard 16550.  Make it behave as a
    // standard // 16550 by setting the clock pre-scaler to scale down the 
    // hardware clock 10x.  Any set of the baud rate divisor latch will then 
    // be standard 16550 compatible (though the device will not be able to go
    // above 115200 baud).
    //
    // Further, setup N81.
    //

    UCHAR ClockPrescale;
    UCHAR Lcr;
    UCHAR PrescalerInt;
    UCHAR PrescalerFrac;

    Lcr = (OX16PCI95X_LCR_PARITY_NONE << OX16PCI95X_LCR_PARITY_SHIFT) |
          (OX16PCI95X_LCR_STOP_1_BIT << OX16PCI95X_LCR_STOP_SHIFT) |
          (OX16PCI95X_LCR_DATALENGTH_8_BIT << OX16PCI95X_LCR_DATALENGTH_SHIFT);
    WRITE_PORT_UCHAR(Adapter->IoPort + COM_LCR, Lcr);

    OX16PCI95XGetClockPrescalerDivisor(
        Adapter->KdNet->Device, 
        &PrescalerInt, 
        &PrescalerFrac
        );

    ClockPrescale = (PrescalerInt << OX16PCI95X_CPR_PRESCALER_INT_SHIFT) |
                    (PrescalerFrac << OX16PCI95X_CPR_PRESCALER_FRAC_SHIFT);
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_CPR, ClockPrescale);

    UCHAR ClockSampling = (OX16PCI95X_TCR_SAMPCLOCK_DIV_16 << 
                           OX16PCI95X_TCR_SAMPCLOCK_SHIFT);
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_TCR, ClockSampling);

    if (OX16PCI95XSerialBaudDividend != 0) {

        //
        // Compute the divsor
        //

        const ULONG DivisorLatch = OX16PCI95XSerialBaudDividend / Rate;

        //
        // set the divisor latch access bit (DLAB) in the line control reg
        //

        Lcr = READ_PORT_UCHAR(Adapter->IoPort + COM_LCR);
        WRITE_PORT_UCHAR(Adapter->IoPort + COM_LCR, Lcr | LC_DLAB);
        WRITE_PORT_UCHAR(Adapter->IoPort + COM_DLM,
                           (UCHAR)((DivisorLatch >> 8) & 0xff));

        WRITE_PORT_UCHAR(Adapter->IoPort, (UCHAR)(DivisorLatch & 0xff));
        WRITE_PORT_UCHAR(Adapter->IoPort + COM_LCR, Lcr);
    }
}

UCHAR
OX16PCI95XReadRxFifoLength(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Reads the current FIFO length of the receive FIFO.  The adapter must be in 
    950 mode (albeit 950 registers do not need to be mapped) before this is
    called.

Arguments:

    Adapter - the OX16PCI95X adapter object

Return Value:

    The number of bytes in the receive FIFO

--*/

{
    BOOLEAN Map950 = !Adapter->Are950RegistersMapped;
    UCHAR RxFifoLength;

    if (Map950) {
        OX16PCI95XMap950Registers(Adapter);
    }

    RxFifoLength = READ_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_950_RFL);

    if (Map950) {
        OX16PCI95XUnmap950Registers(Adapter);
    }

    return RxFifoLength;
}

NTSTATUS
OX16PCI95XInitializeController(
    __in PKDNET_SHARED_DATA KdNet
    )

/*++

Routine Description:

    Fill in the com port port object, set the initial baud rate,
    turn on the hardware.

Arguments:

    KdNet - The data shared with KdNet

Return Value:

    STATUS_SUCCESS / STATUS_UNSUCCESSFUL

--*/

{
    POX16PCI95X_ADAPTER Adapter = (POX16PCI95X_ADAPTER)KdNet->Hardware;
    NTSTATUS Status;
    UCHAR Asr;

    Adapter->KdNet = KdNet;
    Adapter->Are650RegistersMapped = FALSE;
    Adapter->Are950RegistersMapped = FALSE;

    //
    // Verify that this is a supported device.
    //
    if (!OX16PCI95XIsSupportedDevice(KdNet->Device)) {
        Status = STATUS_NO_SUCH_DEVICE;
        goto OX16PCI95XInitializeControllerEnd;
    }

    if (!KdNet->Device->BaseAddress[0].Valid ||
        KdNet->Device->BaseAddress[0].Type != CmResourceTypePort ||
        KdNet->Device->BaseAddress[0].TranslatedAddress == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto OX16PCI95XInitializeControllerEnd;
    }

    Adapter->BaudRate = KdNet->SerialBaudRate;
    Adapter->PortPresent = TRUE;
    Adapter->IoPort = KdNet->Device->BaseAddress[0].TranslatedAddress;

    //
    // The ACR register cannot be read but requires manipulation for
    // indexed registers.  The driver must keep responsibility for
    // tracking the value of ACR rather than trying to perform a readback.
    //
    Adapter->ACR = 0;

    //
    // Disable all uart interrupts.
    //

    WRITE_PORT_UCHAR(Adapter->IoPort + COM_IEN, 0);

    //
    // Enable the FIFO.
    //

    WRITE_PORT_UCHAR(Adapter->IoPort + COM_FCR, FC_ENABLE);

    //
    // Assert DTR, RTS.
    //

    WRITE_PORT_UCHAR(Adapter->IoPort + COM_MCR, MC_DTRRTS);

    //
    // In order to utilize the onboard clock prescaler to reduce the incoming
    // clock rate from 18.432 MHz to 1.8432 MHz as per a standard 550, the 
    // hardware must be put into the hardware must be put into "enhanced" 
    // (650) register mode.  The control bit in MCR which bypasses the 
    // prescaler (set on hardware reset) is not accessible in pure 550 mode.
    //

    OX16PCI95XMap650Registers(Adapter);

    //
    // The 650 registers are now overlayed in our 8 bytes of I/O space.  Turn 
    // on the clock prescaler by setting the appropriate bits in MCR after
    // enabling this functionality through enabling enhanced mode in EFR.
    //

    UCHAR Efr = READ_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_650_EFR);
    Efr |= (OX16PCI95X_EFR_MODE_ENHANCED << OX16PCI95X_EFR_MODE_SHIFT);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_650_EFR, Efr);

    //
    // Further, turn on hardware flow control by setting the RTS/CTS bits.
    //

    Efr |= (ENABLED << OX16PCI95X_EFR_RTS_SHIFT) |
           (ENABLED << OX16PCI95X_EFR_CTS_SHIFT);
    WRITE_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_650_EFR, Efr);

    //
    // Unmap the 650 registers by restoring the LCR prior to turning on the
    // prescaler.  The MCR register in which the prescaler bit is located 
    // is not accessible in this mode -- the address is XON1.
    //

    OX16PCI95XUnmap650Registers(Adapter);

    //
    // Take the FIFO 128 deep since the chip is capable of this once enhanced 
    // mode is set.  This bit was already initialized in the cached ACR value.
    //

    Adapter->ACR = (ENABLED << OX16PCI95X_ACR_950TRIGGERLEVEL_SHIFT);
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_ACR, Adapter->ACR);

    //
    // Verify the FIFO depth and other parameters.
    //

    OX16PCI95XMap950Registers(Adapter);
    Asr = READ_PORT_UCHAR(Adapter->IoPort + OX16PCI95X_COM_950_ASR);
    if (((Asr & OX16PCI95X_ASR_FIFOSIZE_MASK) >> OX16PCI95X_ASR_FIFOSIZE_SHIFT) == 
          OX16PCI95X_ASR_FIFOSIZE_128DEEP) {
        Adapter->FifoDepth = 128;
    } else {
        Adapter->FifoDepth = 16;
    }
    OX16PCI95XUnmap950Registers(Adapter);

    //
    // Set the flow control trigger levels based on the 128 byte FIFO.
    //
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_FCL, 32);
    OX16PCI95XWriteIndexedRegister(Adapter, OX16PCI95X_IDX_FCH, 96);

    Adapter->LowFlowTrigger = OX16PCI95XReadIndexedRegister(
        Adapter, 
        OX16PCI95X_IDX_FCL
        ) & OX16PCI95X_AUTOFLOW_LOW_MASK;
    Adapter->HighFlowTrigger = OX16PCI95XReadIndexedRegister(
        Adapter, 
        OX16PCI95X_IDX_FCH
        ) & OX16PCI95X_AUTOFLOW_HIGH_MASK;

    UCHAR Mcr = READ_PORT_UCHAR(Adapter->IoPort + COM_MCR);
    Mcr |= (OX16PCI95X_MCR_BAUD_PRESCALE_CPR_MN8 << 
            OX16PCI95X_MCR_BAUD_PRESCALE_SHIFT);
    WRITE_PORT_UCHAR(Adapter->IoPort + COM_MCR, Mcr);

    //
    // Make sure to initialize the baud rate on our side so that the appropriate clock prescaler values
    // are set and the standard 16550 I/O can function on the device.
    //

    OX16PCI95XSetBaud(Adapter, Adapter->BaudRate);

    Status = STATUS_SUCCESS;

OX16PCI95XInitializeControllerEnd:

    return Status;
}

VOID
OX16PCI95XShutdownController(
    __in POX16PCI95X_ADAPTER Adapter
    )

/*++

Routine Description:

    Shuts down communication with the serial port.

Arguments:

    Adapter - The OX16PCI95x adapter object

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
        lsr = READ_PORT_UCHAR(Adapter->IoPort + COM_LSR);
        if (lsr & OX16PCI95X_LSR_TXEMPTY_MASK) {
            break;
        }
        KeStallExecutionProcessor(1000); // 1 mS stall

        Counter += 1;
        if (Counter > 2000) {
            break;
        }
    }
}

NTSTATUS
OX16PCI95XWriteSerialByte(
    __in POX16PCI95X_ADAPTER Adapter,
    __in const UCHAR Byte
    )

/*++

Routine Description:

    Write a byte out to the specified com port.

Arguments:

    Adapter - The OX16PCI95X adapter object.

    Byte - data to emit

Return Value:

    STATUS_SUCCESS - the byte was written
    
    STATUS_IO_TIMEOUT - the transmitter isn't ready

    other - error code

--*/

{
    if (!Adapter->PortPresent) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Wait for port to not be busy and CTS to be asserted from the other side.
    //

    if (!OX16PCI95XIsClearToSend(Adapter)) {
        Adapter->NotClearToSend++;
        return STATUS_IO_TIMEOUT;
    }

    if ((OX16PCI95XReadLsr(Adapter, COM_OUTRDY) & COM_OUTRDY) == 0) {
        return STATUS_IO_TIMEOUT;
    }

    //
    // Send the byte
    //

    WRITE_PORT_UCHAR(Adapter->IoPort + COM_DAT, Byte);
    return STATUS_SUCCESS;
}

NTSTATUS
OX16PCI95XReadSerialByte (
    __in POX16PCI95X_ADAPTER Adapter,
    __out PUCHAR Byte
    )

/*++

Routine Description:

    Fetch a byte and return it.

Arguments:

    Adapter - The OX16PCI95X adapter object.

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


    //
    // Check to make sure the CPPORT we were passed has been initialized.
    // (The only time it won't be initialized is when the kernel debugger
    // is disabled, in which case we just return.)
    //

    if (!Adapter->PortPresent) {
        if (OX16PCI95XReadLsr(Adapter, COM_DATRDY) == SERIAL_LSR_NOT_PRESENT) {

            return(STATUS_IO_TIMEOUT);
        } else {
            OX16PCI95XSetBaud(Adapter, Adapter->BaudRate);
            Adapter->PortPresent = TRUE;
        }
    }

    lsr = OX16PCI95XReadLsr(Adapter, COM_DATRDY);
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

        value = READ_PORT_UCHAR(Adapter->IoPort + COM_DAT);
        *Byte = value & (UCHAR)0xff;
        return STATUS_SUCCESS;
    }

    OX16PCI95XReadLsr (Adapter, 0);
    return STATUS_IO_TIMEOUT;
}

NTSTATUS
OX16PCI95XDeviceControl(
    __in POX16PCI95X_ADAPTER Adapter,
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

    UNREFERENCED_PARAMETER(Adapter);

    Status = STATUS_INVALID_DEVICE_REQUEST;

    switch(RequestCode) {

        //
        // This device is a 16950 and supports automatic flow control.  We
        // simply refuse to turn it off and similarly refuse any request
        // to manually manipulate the flow.  That's how this device works.
        //

        case KD_DEVICE_CONTROL_SERIAL_QUERY_FLOW_CONTROL_SUPPORTED:
            if (OutputBufferSize < sizeof(BOOLEAN)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((BOOLEAN *)OutputBuffer) = TRUE;
            Status = STATUS_SUCCESS;
            break;

        case KD_DEVICE_CONTROL_SERIAL_SET_FLOW_CONTROL:
            if (InputBufferSize < sizeof(BOOLEAN)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (*(BOOLEAN *)InputBuffer) {
                Status = STATUS_SUCCESS;
            }
            break;

        default:
            break;
    }

    return Status;
}

