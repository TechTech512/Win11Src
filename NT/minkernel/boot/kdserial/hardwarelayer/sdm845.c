/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sdm845.c

Abstract:

    This module contains support for the Qualcomm SDM 845 serial UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"
#include "sdm845.h"

//
// This is the control transfer data block.
//

static FIFO_TX_BLOCK Transfer;


// ------------------------------------------------------------------ Functions

BOOLEAN
SDM845ReinitializePort (
    _Inout_ PCPPORT Port
    )

/*++

Routine Description:

    This routine performs the re-initialization of an SDM845 serial UART.

Arguments:

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{

    ULONG ConfigMask;
    ULONG Retries;

    //
    // Wait for the state machine to clear up.
    //

    Retries = 0;
    while ((UART_DM_READ_REG(Port->Address + GENI4_CFG, HWIO_GENI_STATUS_OFFS) &
            HWIO_GENI_STATUS_M_GENI_CMD_ACTIVE_BMSK)) {

        Retries += 1;
        if (Retries >= MAX_RETRIES) {
            return FALSE;
        }
    }

    UART_DM_WRITE_REG(Port->Address + GENI4_CFG, HWIO_GENI_DFS_IF_CFG_OFFS, 0x0);

    ConfigMask = (HWIO_DMA_GENERAL_CFG_AHB_SEC_SLV_CLK_CGC_ON_BMSK << HWIO_DMA_GENERAL_CFG_AHB_SEC_SLV_CLK_CGC_ON_SHFT) |
                 (HWIO_DMA_GENERAL_CFG_DMA_AHB_SLV_CLK_CGC_ON_BMSK << HWIO_DMA_GENERAL_CFG_DMA_AHB_SLV_CLK_CGC_ON_SHFT) |
                 (HWIO_DMA_GENERAL_CFG_DMA_TX_CLK_CGC_ON_BMSK << HWIO_DMA_GENERAL_CFG_DMA_TX_CLK_CGC_ON_SHFT) |
                 (HWIO_DMA_GENERAL_CFG_DMA_RX_CLK_CGC_ON_BMSK << HWIO_DMA_GENERAL_CFG_DMA_RX_CLK_CGC_ON_SHFT);

    UART_DM_WRITE_REG(Port->Address + QUPV3_SE_DMA, HWIO_DMA_GENERAL_CFG_OFFS, ConfigMask);
    UART_DM_WRITE_REG(Port->Address + GENI4_CFG, HWIO_GENI_CGC_CTRL_OFFS, 0x7F);
    UART_DM_WRITE_REG(Port->Address + GENI4_CFG, HWIO_GENI_FORCE_DEFAULT_REG_OFFS, 0x1);
    UART_DM_WRITE_REG(Port->Address + GENI4_CFG, HWIO_GENI_OUTPUT_CTRL_OFFS, 0x7F);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_GENI_DMA_MODE_EN_OFFS, 0);
    UART_DM_WRITE_REG(Port->Address + QUPV3_SE_DMA, HWIO_SE_IRQ_EN_OFFS, 0xFFFFFFFF);
    UART_DM_WRITE_REG(Port->Address + QUPV3_SE_DMA, HWIO_SE_GSI_EVENT_EN_OFFS, 0);
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_M_IRQ_ENABLE_OFFS, 0xB300005F);
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_S_IRQ_ENABLE_OFFS, 0xB3007E5F);

    //
    // Get TX FIFO width.
    //

    ConfigMask = UART_DM_READ_REG(Port->Address + QUPV3_SE_DMA, HWIO_SE_HW_PARAM_0_OFFS);
    ConfigMask = (ConfigMask & TX_FIFO_DEPTH_MASK) >> TX_FIFO_DEPTH_SHIFT;
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_TX_WATERMARK_REG_OFFS, 4);

    //
    // Get RX FIFO width.
    //

    ConfigMask = UART_DM_READ_REG(Port->Address + QUPV3_SE_DMA, HWIO_SE_HW_PARAM_1_OFFS);
    ConfigMask = (ConfigMask & RX_FIFO_DEPTH_MASK) >> RX_FIFO_DEPTH_SHIFT;
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_RX_WATERMARK_REG_OFFS, (ConfigMask - 8));
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_RX_RFR_WATERMARK_REG_OFFS, (ConfigMask - 4));

    SDM845SetBaud(Port, Port->BaudRate);

    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_TX_WORD_LEN_OFFS, 0x8);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_RX_WORD_LEN_OFFS, 0x8);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_TX_PARITY_CFG_OFFS, 0x0);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_TX_TRANS_CFG_OFFS, 0x2);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_RX_PARITY_CFG_OFFS, 0x0);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_RX_TRANS_CFG_OFFS, 0x0);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_TX_STOP_BIT_LEN_OFFS, 0x0);
    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_RX_STALE_CNT_OFFS, (0x16 * 10));
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_S_CMD0_OFFS, 0x8000000);
    return TRUE;
}

BOOLEAN
SDM845InitializePort (
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT Port,
    BOOLEAN MemoryMapped,
    UCHAR AccessSize,
    UCHAR BitWidth
    )

/*++

Routine Description:

    This routine performs the initialization of an SDM845 serial UART.

Arguments:

    LoadOptions - Optional load option string. Currently unused.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Supplies a boolean which indicates if the UART is accessed
        through memory-mapped registers or legacy port I/O.

    AccessSize - Supplies an ACPI Generic Access Size value which indicates the
        type of bus access that should be performed when accessing the UART.

    BitWidth - Supplies a number indicating how wide the UART's registers are.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{

    UNREFERENCED_PARAMETER(LoadOptions);
    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);

    if (MemoryMapped == FALSE) {
        return FALSE;
    }

    Port->Flags = 0;
    return SDM845ReinitializePort(Port);
}

BOOLEAN
SDM845SetBaud (
    _Inout_ PCPPORT Port,
    ULONG Rate
    )

/*++

Routine Description:

    Set the baud rate for the UART hardware and record it in the port object.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Rate - Supplies the desired baud rate in bits per second.

Return Value:

    TRUE if the baud rate was programmed, FALSE if it was not.

--*/

{

    UINT32 DivisorLatch = 0;
    UINT32 ValueTempMask = 0;

    if ((Port == NULL) || (Port->Address == NULL)) {
        return FALSE;
    }

    DivisorLatch = UART_DM_READ_REG(Port->Address + GENI4_CFG,
                                    HWIO_GENI_SER_M_CLK_CFG_OFFS);

    if (DivisorLatch == 0x11) {
        return FALSE;
    }

    //
    // Clock rate is 1843200.
    //

    switch (Rate) {
    case 7200:
        DivisorLatch = 0x20;
        break;
    case 9600:
        DivisorLatch = 0x18;
        break;
    case 14400:
        DivisorLatch = 0x10;
        break;
    case 19200:
        DivisorLatch = 0xC;
        break;
    case 28800:
        DivisorLatch = 0x8;
        break;
    case 38400:
        DivisorLatch = 0x6;
        break;
    case 57600:
        DivisorLatch = 0x4;
        break;
    case 115200:
        DivisorLatch = 0x2;
        break;
    default:
        DivisorLatch = 0x1;
        break;
    }

    ValueTempMask = (DivisorLatch << (HWIO_GENI_SER_M_CLK_CFG_CLK_DIV_VALUE_SHFT)) |
                     HWIO_GENI_SER_M_CLK_CFG_SER_CLK_EN_BMSK;

    UART_DM_WRITE_REG(Port->Address + GENI4_CFG,
                      HWIO_GENI_SER_M_CLK_CFG_OFFS,
                      ValueTempMask);

    UART_DM_WRITE_REG(Port->Address + GENI4_CFG,
                      HWIO_GENI_SER_S_CLK_CFG_OFFS,
                      ValueTempMask);

    Port->BaudRate = Rate;
    return TRUE;
}

UART_STATUS
SDM845GetByte (
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR Byte
    )

/*++

Routine Description:

    Fetch a data byte from the UART device and return it.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the address of variable to hold the result.

Return Value:

    UART_STATUS code.

--*/

{

    ULONG AvailableBytes;
    PUCHAR BaseAddress;
    ULONG IrqStatus;
    ULONG RxFifoStatus;
    ULONG PartialBytesToRead;
    ULONG WordsToRead;
    ULONG ArrayIndex;
    ULONG Index;
    ULONG RxFifo;

    if ((Port == NULL) || (Port->Address == NULL)) {
        return UartNotReady;
    }

    BaseAddress = Port->Address;
    ArrayIndex = 0;

    //
    // Check if there is any available data. If not, then read from the FIFO
    // and fill the local transfer buffer.
    //

    if (Transfer.AvailableBytes == 0) {
        Transfer.PtrToFifoBuffer = (UCHAR *)Transfer.FifoBuffer;
        IrqStatus = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_S_IRQ_STATUS_OFFS);
        UART_DM_WRITE_REG(BaseAddress + GENI4_DATA, HWIO_GENI_S_IRQ_CLEAR_OFFS, IrqStatus);
        RxFifoStatus = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_FIFO_STATUS_OFFS);

        //
        // Calculate the number of bytes to read.
        //

        PartialBytesToRead = (RxFifoStatus & RX_LAST_VALID_BYTES_MASK) >> RX_LAST_VALID_BYTES_SHIFT;
        WordsToRead = RxFifoStatus & RX_FIFO_WC;
        if ((PartialBytesToRead > 0) || (WordsToRead > 0)) {
            if ((PartialBytesToRead != 0) && (PartialBytesToRead != 4)) {
                WordsToRead -= 1;
            }

        } else if ((IrqStatus & RX_FIFO_WATERMARK_IRQ) != 0) {
            WordsToRead = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_WATERMARK_REG_OFFS);
        }

        //
        // Don't overrun the local transfer buffer.
        //

        AvailableBytes = (WordsToRead * 4) + PartialBytesToRead;
        if (AvailableBytes > MAX_RX_FIFO_SIZE) {
            SDM845ReinitializePort(Port);
            return UartNotReady;
        }

        Transfer.AvailableBytes = AvailableBytes;
        for (Index = 0; Index < WordsToRead; Index += 1) {
            RxFifo = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_FIFOn_OFFS(BaseAddress, Index));
            Transfer.FifoBuffer[0 + ArrayIndex] = (UCHAR)(RxFifo >>  0);
            Transfer.FifoBuffer[1 + ArrayIndex] = (UCHAR)(RxFifo >>  8);
            Transfer.FifoBuffer[2 + ArrayIndex] = (UCHAR)(RxFifo >> 16);
            Transfer.FifoBuffer[3 + ArrayIndex] = (UCHAR)(RxFifo >> 24);
            ArrayIndex += 4;
        }

        if (PartialBytesToRead > 0) {
            RxFifo = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_FIFOn_OFFS(BaseAddress, Index));
            for (Index = 0; Index < PartialBytesToRead; Index += 1) {
                Transfer.FifoBuffer[ArrayIndex] = (UCHAR)(RxFifo >> Index * 8);
                ArrayIndex += 1;
            }
        }
    }

    if (Transfer.AvailableBytes != 0) {
        *Byte = *Transfer.PtrToFifoBuffer;
        Transfer.PtrToFifoBuffer += 1;
        Transfer.AvailableBytes -= 1;
        return UartSuccess;
    }

    return UartNoData;
}

UART_STATUS
SDM845PutByte (
    _Inout_ PCPPORT Port,
    UCHAR Byte,
    BOOLEAN BusyWait
    )

/*++

Routine Description:

    Write a data byte out to the UART device.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

    Byte - Supplies the data to emit.

    BusyWait - Supplies a flag to control whether this routine will busy
        wait (spin) for the UART hardware to be ready to transmit.

Return Value:

    UART_STATUS code.

--*/

{
    PUCHAR Address;

    if ((Port == NULL) || (Port->Address == NULL)) {
        return UartNotReady;
    }

    Address = Port->Address;

    //
    // Wait for the state machine to clear up.
    //

    if (BusyWait != FALSE) {
        while((UART_DM_READ_REG(Port->Address + GENI4_CFG, HWIO_GENI_STATUS_OFFS) & HWIO_GENI_STATUS_M_GENI_CMD_ACTIVE_BMSK));

    } else if ((UART_DM_READ_REG(Port->Address + GENI4_CFG, HWIO_GENI_STATUS_OFFS) & HWIO_GENI_STATUS_M_GENI_CMD_ACTIVE_BMSK)) {

        return UartNotReady;
    }

    UART_DM_WRITE_REG(Port->Address + GENI4_IMAGE_REGS, HWIO_UART_TX_TRANS_LEN_OFFS, 1);
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_M_CMD0_OFFS, 0x8000000);

    UINT32 word_value = 0;
    word_value = word_value | Byte;
    UART_DM_WRITE_REG(Port->Address + GENI4_DATA, HWIO_GENI_TX_FIFOn_OFFS(Port->Address, 0), word_value);
    return UartSuccess;
}

BOOLEAN
SDM845RxReady (
    _Inout_ PCPPORT Port
    )

/*++

Routine Description:

    This routine determines if there is data pending in the UART.

Arguments:

    Port - Supplies the address of the port object that describes the UART.

Return Value:

    TRUE if data is available, FALSE otherwise.

--*/

{

    PUCHAR BaseAddress;
    ULONG FifoStatReg;
    ULONG PartialBytesToRead;
    ULONG WordsToRead;
    BOOLEAN IsAvailableBytes;
    ULONG IrqStatus;

    if ((Port == NULL) || (Port->Address == NULL)) {
        return FALSE;
    }

    //
    // Check if there is any waiting pending data.
    //

    if (Transfer.AvailableBytes != 0) {
        IsAvailableBytes = TRUE;
        goto SDM845ReceiveDataAvailableEnd;
    }

    //
    // Read the FIFO status register
    //

    BaseAddress = Port->Address;
    FifoStatReg = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_FIFO_STATUS_OFFS);

    //
    // Calculate the number of bytes to read
    //

    PartialBytesToRead = (FifoStatReg & RX_LAST_VALID_BYTES_MASK) >> RX_LAST_VALID_BYTES_SHIFT;
    WordsToRead = FifoStatReg & RX_FIFO_WC;
    IsAvailableBytes = FALSE;
    if ((PartialBytesToRead > 0) || (WordsToRead > 0)) {
        IsAvailableBytes = TRUE;

    } else {
        IrqStatus = UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_S_IRQ_STATUS_OFFS);
        if ((IrqStatus & RX_FIFO_WATERMARK_IRQ) != 0) {
            if (UART_DM_READ_REG(BaseAddress + GENI4_DATA, HWIO_GENI_RX_WATERMARK_REG_OFFS) > 0) {
                IsAvailableBytes = TRUE;
            }
        }
    }

SDM845ReceiveDataAvailableEnd:
    return IsAvailableBytes;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER SDM845HardwareDriver = {
    SDM845InitializePort,
    SDM845SetBaud,
    SDM845GetByte,
    SDM845PutByte,
    SDM845RxReady,
    NULL,
    NULL
};
