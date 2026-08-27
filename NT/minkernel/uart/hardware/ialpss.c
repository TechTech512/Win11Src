/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ialpss.c

Abstract:

    This module contains support for the Intel Low Power Sub System UART.

--*/

// ------------------------------------------------------------------- Includes

#include "common.h"
#include "kdcom.h"

// ---------------------------------------------------------------- Definitions

//
// Software reset register offset.
//

#define IALPSS_RESET_REGISTER_OFFSET        0x81

//
// Defines to assert and deassert software reset.
//

#define IALPSS_REG_VAL_ASSERT_RESET         0x0
#define IALPSS_REG_VAL_DEASSERT_RESET       0x7
#define IALPSS_RESET_MASK 0x7

//
// Defines to access PRV_CLOCK_PARAMS register.
//

#define IALPSS_CLK_PARAMS_OFFSET            0x80
#define IALPSS_CLK_UPDATE_BIT_MASK          (1<<31)

//
// Defines to access registers within PCI config space.
//

#define IALPSS_CONFIG_SPACE_OFFSET          0x1000
#define IALPSS_PCI_COMMAND_STATUS_OFFSET    0x4
#define IALPSS_PMCSR_OFFSET                 0x84

// ------------------------------------------------------------ Data Structures

typedef enum {
    PowerStateD0,
    PowerStateD3
} CONTROLLER_POWER_STATE, *PCONTROLLER_POWER_STATE;

#pragma warning(push)
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

//
// The following structs are adapted from minkernel\ntos\inc\pci_x.h.
//
// Definition for PCI Power Management Control and Status Register.
//

typedef union _PCI_PMCSR {
    struct {
        ULONG      PowerState:2;
        ULONG      Rsvd1:1;
        ULONG      NoSoftReset:1;
        ULONG      Rsvd2:4;
        ULONG      PMEEnable:1;
        ULONG      DataSelect:4;
        ULONG      DataScale:2;
        ULONG      PMEStatus:1;
        ULONG      Rsvd3:32;
    };

    ULONG AsULONG;
} PCI_PMCSR, *PPCI_PMCSR;

//
// Definition for PCI Command and Status register.
//

typedef union _PCI_COMMAND_STATUS {
    struct {
        ULONG Reserved0:1;
        ULONG Mse:1;
        ULONG Bme:1;
        ULONG Reserved1:5;
        ULONG SerrEnable:1;
        ULONG Reserved2:1;
        ULONG IntrDisable:1;
        ULONG Reserved3:5;
        ULONG Reserved4:3;
        ULONG IntrStatus:1;
        ULONG CapList:1;
        ULONG Reserved5:6;
        ULONG Sta:1;
        ULONG Rta:1;
        ULONG Rma:1;
        ULONG Sse:1;
        ULONG Reserved6:1;
    };

    ULONG AsULONG;
} PCI_COMMAND_STATUS, *PPCI_COMMAND_STATUS;

#pragma warning(pop)

// -------------------------------------------------------------------- Globals

//
// Pointer to PCI Power Management Control and Status Register.
//

PULONG IaLpssPmcsr = NULL;

//
// Pointer to PCI Command and Status Register.
//

PULONG IaLpssCmdStatus = NULL;

//
// Global to store the value of PRV_CLOCK_PARAMS register during power down.
//

ULONG IaLpssClockParams = 0;

// ----------------------------------------------------------------- Prototypes

BOOLEAN
Uart16550InitializePortCommon (
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT Port,
    BOOLEAN MemoryMapped,
    UCHAR AccessSize,
    UCHAR BitWidth
    );

BOOLEAN
Uart16550SetBaud (
    _Inout_ PCPPORT Port,
    ULONG Rate
    );

UART_STATUS
Uart16550GetByte (
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR Byte
    );

UART_STATUS
Uart16550PutByte (
    _Inout_ PCPPORT Port,
    UCHAR Byte,
    BOOLEAN BusyWait
    );

BOOLEAN
Uart16550RxReady (
    _Inout_ PCPPORT Port
    );

// ------------------------------------------------------------------ Functions

BOOLEAN
IaLpssInitializePort (
    _In_opt_ _Null_terminated_ PCHAR LoadOptions,
    _Inout_ PCPPORT Port,
    BOOLEAN MemoryMapped,
    UCHAR AccessSize,
    UCHAR BitWidth
    )

/*++

Routine Description:

    This routine performs the initialization of an Intel LPSS UART.

Arguments:

    LoadOptions - Optional load option string.

    Port - Supplies a pointer to a CPPORT object which will be filled in as
        part of the port initialization.

    MemoryMapped - Unused.

    AccessSize - Unused.

    BitWidth - Unused.

Return Value:

    TRUE if the port has been successfully initialized, FALSE otherwise.

--*/

{

    BOOLEAN Success;

    UNREFERENCED_PARAMETER(AccessSize);
    UNREFERENCED_PARAMETER(BitWidth);
    UNREFERENCED_PARAMETER(MemoryMapped);

    Port->Flags = 0;
    Success = Uart16550InitializePortCommon(LoadOptions,
                                            Port,
                                            TRUE,
                                            AcpiGenericAccessSizeByte,
                                            8);

    if (Success != FALSE) {
        IaLpssPmcsr = (PULONG)(Port->Address +
                                IALPSS_CONFIG_SPACE_OFFSET +
                                IALPSS_PMCSR_OFFSET);

        IaLpssCmdStatus = (PULONG)(Port->Address +
                                    IALPSS_CONFIG_SPACE_OFFSET +
                                    IALPSS_PCI_COMMAND_STATUS_OFFSET);
    }

    return Success;
}

UART_STATUS
IaLpssReadPmcsr (
    _Out_ PPCI_PMCSR Pmcsr
    )

/*++

Routine Description:

    This routine reads the PMCSR register within the PCI config space.
    Hardware supports only 32-bit access to this register.

Arguments:

    Pmcsr - Supplies a pointer to the PMCSR structure to be filled in.

Return Value:

    UART_STATUS code.

--*/

{

    if ((IaLpssPmcsr == NULL) || (Pmcsr == NULL)) {
        return UartError;
    }

    Pmcsr->AsULONG = READ_REGISTER_ULONG(IaLpssPmcsr);
    return UartSuccess;
}

UART_STATUS
IaLpssWritePmcsr (
    _In_ PPCI_PMCSR Pmcsr
    )

/*++

Routine Description:

    This routine writes to the PMCSR register within the PCI config space.
    Hardware supports only 32-bit access to this register.

Arguments:

    Pmcsr - Supplies a pointer to the PMCSR structure to be written.

Return Value:

    UART_STATUS code.

--*/

{

    if ((IaLpssPmcsr == NULL) || (Pmcsr == NULL)) {
        return UartError;
    }

    WRITE_REGISTER_ULONG(IaLpssPmcsr, Pmcsr->AsULONG);
    return UartSuccess;
}

UART_STATUS
IaLpssReadCmdStatus (
    _Out_ PPCI_COMMAND_STATUS CmdStatus
    )

/*++

Routine Description:

    This routine reads the command and status register within the PCI config
    space. Hardware supports only 32-bit access to this register.

Arguments:

    CmdStatus - Supplies a pointer to the PCI_COMMAND_STATUS struct.

Return Value:

    UART_STATUS code.

--*/

{

    if ((IaLpssCmdStatus == NULL) || (CmdStatus == NULL)) {
        return UartError;
    }

    CmdStatus->AsULONG = READ_REGISTER_ULONG(IaLpssCmdStatus);
    return UartSuccess;
}

UART_STATUS
IaLpssWriteCmdStatus (
    _In_ PPCI_COMMAND_STATUS CmdStatus
    )

/*++

Routine Description:

    This routine writes to the command and status register within the PCI
    config space. Hardware supports only 32-bit access to this register.

Arguments:

    CmdStatus - Supplies a pointer to PCI_COMMAND_STATUS struct.

Return Value:

    UART_STATUS code.

--*/

{

    if ((IaLpssCmdStatus == NULL) || (CmdStatus == NULL)) {
        return UartError;
    }

    WRITE_REGISTER_ULONG(IaLpssCmdStatus, CmdStatus->AsULONG);
    return UartSuccess;
}

UART_STATUS
IaLpssPciSetPower (
    _In_ CONTROLLER_POWER_STATE PowerState
    )

/*++

Routine Description:

    This routine transitions the IALPSS UART to the D0 or D3 power state via
    its PMCSR.

Arguments:

    PowerState - Supplies the desired power state.

Return Value:

    UART_STATUS code.

--*/

{

    PCI_COMMAND_STATUS CmdStatus;
    UART_STATUS Status;
    PCI_PMCSR Pmcsr;

    Status = UartSuccess;
    if (PowerState == PowerStateD0) {

        //
        // Get current PMCSR.
        //

        Status = IaLpssReadPmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if (Pmcsr.PowerState == 0) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Update and write new PMCSR value.
        //

        Pmcsr.PowerState = 0;
        Pmcsr.PMEStatus = 1;

        Status = IaLpssWritePmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Read back PMCSR and check if the write was successful.
        //

        Status = IaLpssReadPmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if (Pmcsr.PowerState != 0) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Read current value of command and status register.
        //

        Status = IaLpssReadCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Set bus master and memory space enable bits.
        //

        CmdStatus.Bme = 1;
        CmdStatus.Mse = 1;

        Status = IaLpssWriteCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Read and verify if write was successful.
        //

        Status = IaLpssReadCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if ((CmdStatus.Bme != 1) || (CmdStatus.Mse != 1)) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

    } else if (PowerState == PowerStateD3) {

        //
        // Read current value of command and status register.
        //

        Status = IaLpssReadCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Clearing bus master and memory space enable bits during power down.
        //

        CmdStatus.Bme = 0;
        CmdStatus.Mse = 0;

        Status = IaLpssWriteCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Read and verify if write was successful.
        //

        Status = IaLpssReadCmdStatus(&CmdStatus);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if ((CmdStatus.Bme != 0) || (CmdStatus.Mse != 0)) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Get current PMCSR.
        //

        Status = IaLpssReadPmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if (Pmcsr.PowerState == 3) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Update and write new PMCSR value.
        //

        Pmcsr.PowerState = 3;
        Pmcsr.PMEStatus = 0;

        Status = IaLpssWritePmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        //
        // Read back PMCSR and check if the write was successful.
        //

        Status = IaLpssReadPmcsr(&Pmcsr);
        if (Status != UartSuccess) {
            goto IaLpssPciSetPowerEnd;
        }

        if (Pmcsr.PowerState != 3) {
            Status = UartError;
            goto IaLpssPciSetPowerEnd;
        }

    } else {

        //
        // Invalid power state.
        //

        Status = UartError;
        goto IaLpssPciSetPowerEnd;
    }

IaLpssPciSetPowerEnd:
    return Status;

}

ULONG
IaLpssReadResetRegister (
    _In_ PCPPORT Port
    )

/*++

Routine Description:

    This routine reads from the software reset register.
    Hardware supports only 8-bit access to this register.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

Return Value:

    The value of the software reset register.

--*/

{

    PUCHAR ResetReg;

    ResetReg = (PUCHAR)(Port->Address + IALPSS_RESET_REGISTER_OFFSET);
    return READ_REGISTER_UCHAR(ResetReg);
}

VOID
IaLpssWriteResetRegister (
    _In_ PCPPORT Port,
    _In_ UCHAR Value
    )

/*++

Routine Description:

    This routine writes to the software reset register.
    Hardware supports only 8-bit access to this register.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

    Value - Supplies the value to be written to software reset register.

Return Value:

    None.

--*/

{

    PUCHAR ResetReg;

    ResetReg = (PUCHAR)(Port->Address + IALPSS_RESET_REGISTER_OFFSET);
    WRITE_REGISTER_UCHAR(ResetReg, Value);
}

ULONG
IaLpssReadClockParams (
    _In_ PCPPORT Port
    )

/*++

Routine Description:

    This routine reads the PRV_CLOCK_PARAMS register.
    Hardware supports only 32-bit access to this register.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

Return Value:

    The value of the PRV_CLOCK_PARAMS register.

--*/

{

    PULONG ClkParams;

    ClkParams = (PULONG)(Port->Address + IALPSS_CLK_PARAMS_OFFSET);
    return READ_REGISTER_ULONG(ClkParams);
}

VOID
IaLpssWriteClockParams (
    _In_ PCPPORT Port,
    _In_ ULONG Value
    )

/*++

Routine Description:

    This routine writes to the PRV_CLOCK_PARAMS register.
    Hardware supports only 32-bit access to this register.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

    Value - Supplies the value to be written to PRV_CLOCK_PARAMS register.

Return Value:

    None.

--*/

{

    PULONG ClkParams;

    ClkParams = (PULONG)(Port->Address + IALPSS_CLK_PARAMS_OFFSET);
    WRITE_REGISTER_ULONG(ClkParams, Value);
}

UART_STATUS
IaLpssSetPowerD3 (
    _In_ PCPPORT Port
    )

/*++

Routine Description:

    This routine performs the steps needed to safely transition the UART to the
    low-power state D3.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

Return Value:

    UartSuccess on successful transition to D3.
    UartError on failure.
    UartNotReady if the port is not initalized.

--*/

{

    UCHAR Mcr;
    ULONG ResetValue;
    UART_STATUS Status;

    if ((Port == NULL) || (Port->Address == NULL)) {
        Status = UartNotReady;
        goto IaLpssSetPowerD3End;
    }

    //
    // Saving clock params register value.
    //

    IaLpssClockParams = IaLpssReadClockParams(Port);

    //
    // Check to ensure reset is de-asserted first.
    //

    ResetValue = IaLpssReadResetRegister(Port) & IALPSS_RESET_MASK;
    if (ResetValue != IALPSS_REG_VAL_DEASSERT_RESET) {
        Status = UartError;
        goto IaLpssSetPowerD3End;
    }

    //
    // De-assert aux output 2.
    //

    Mcr = Port->Read(Port, COM_MCR);
    Port->Write(Port, COM_MCR, Mcr & ~SERIAL_MCR_OUT2);

    //
    // Disable the FIFO.
    //

    Port->Write(Port, COM_FCR, 0);

    //
    // Assert reset.
    //

    IaLpssWriteResetRegister(Port, IALPSS_REG_VAL_ASSERT_RESET);

    //
    // Transition to D3.
    //

    Status = IaLpssPciSetPower(PowerStateD3);
    if (Status != UartSuccess) {
        goto IaLpssSetPowerD3End;
    }

IaLpssSetPowerD3End:
    return Status;
}

UART_STATUS
IaLpssSetPowerD0 (
    _In_ PCPPORT Port
    )

/*++

Routine Description:

    This routine performs the steps needed to safely transition the UART to the
    working state D0.

Arguments:

    Port - Supplies a pointer to a CPPORT object.

Return Value:

    UartSuccess on successful transition to D0.
    UartError on failure.
    UartNotReady if the port is not initalized.

--*/

{

    ULONG ResetValue;
    UART_STATUS Status;
    BOOLEAN Success;
    ULONG ClkParams;

    if ((Port == NULL) || (Port->Address == NULL)) {
        Status = UartNotReady;
        goto IaLpssSetPowerD0End;
    }

    //
    // Transition to D0.
    //

    Status = IaLpssPciSetPower(PowerStateD0);
    if (Status != UartSuccess) {
        goto IaLpssSetPowerD0End;
    }

    //
    // Check to ensure reset is asserted.
    //

    ResetValue = IaLpssReadResetRegister(Port) & IALPSS_RESET_MASK;
    if (ResetValue != IALPSS_REG_VAL_ASSERT_RESET) {
        Status = UartError;
        goto IaLpssSetPowerD0End;
    }

    //
    // Restore the saved clock params value.
    //

    if (IaLpssClockParams != 0) {

        ClkParams = IaLpssClockParams;

        //
        // Setting the clk_update bit within PRV_CLOCK_PARAMS register.
        // For new M & N values to be used by the divider, the
        // clk_update bit should be set to 1 and then cleared.
        //

        ClkParams |= IALPSS_CLK_UPDATE_BIT_MASK;
        IaLpssWriteClockParams(Port,ClkParams);

        ClkParams = IaLpssReadClockParams(Port);

        ClkParams &= ~IALPSS_CLK_UPDATE_BIT_MASK;
        IaLpssWriteClockParams(Port,ClkParams);
    }

    //
    // De-assert reset, PPPR register.
    //

    IaLpssWriteResetRegister(Port, IALPSS_REG_VAL_DEASSERT_RESET);

    //
    // Re-initialize UART.
    //

    Port->Flags = 0;
    Success = Uart16550InitializePortCommon(NULL,
                                            Port,
                                            TRUE,
                                            AcpiGenericAccessSizeByte,
                                            8);

    if (Success != TRUE) {
        Status = UartError;
        goto IaLpssSetPowerD0End;
    }

    Status = UartSuccess;

IaLpssSetPowerD0End:
    return Status;
}

// -------------------------------------------------------------------- Globals

UART_HARDWARE_DRIVER IaLpssHardwareDriver = {
    IaLpssInitializePort,
    Uart16550SetBaud,
    Uart16550GetByte,
    Uart16550PutByte,
    Uart16550RxReady,
    IaLpssSetPowerD0,
    IaLpssSetPowerD3
};
