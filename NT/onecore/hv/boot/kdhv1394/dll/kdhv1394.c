#include "kdcommon.h"

// Global variables
BOOLEAN Kd1394Initialized = FALSE;
ULONG KdHvMaxPacketSize;
ULONG KdHvRetryCount;

// External functions
extern UCHAR KdHvDetectHypervisor(void);
extern UCHAR KdHvConnectHypervisor(void);
extern void KdHvResetDebugSession(ULONG Param1);

// Forward declarations
NTSTATUS KdDebuggerInitialize0(PLOADER_PARAMETER_BLOCK LoaderBlock);
NTSTATUS KdDebuggerInitialize1(PLOADER_PARAMETER_BLOCK LoaderBlock);
NTSTATUS KdDebuggerInitialize2(PLOADER_PARAMETER_BLOCK LoaderBlock);

// KdDebuggerInitialize0 - Phase 0 initialization
NTSTATUS
KdDebuggerInitialize0(
    PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    UCHAR hypervisorPresent;
    
    if (Kd1394Initialized == FALSE) {
        hypervisorPresent = KdHvDetectHypervisor();
        if (hypervisorPresent != 0) {
            KdHvConnectHypervisor();
            KdHvResetDebugSession(0);
            KdHvMaxPacketSize = 0xff8;
            KdHvRetryCount = 5;
        }
        Kd1394Initialized = TRUE;
    }
    
    return STATUS_SUCCESS;
}

// KdDebuggerInitialize1 - Phase 1 initialization
NTSTATUS
KdDebuggerInitialize1(
    PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    return STATUS_SUCCESS;
}

// KdDebuggerInitialize2 - Phase 2 initialization
NTSTATUS
KdDebuggerInitialize2(
    PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    return STATUS_SUCCESS;
}

// KdInitialize - Main initialization dispatcher
NTSTATUS
KdInitialize(
    ULONG Phase,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    PKD_CONTEXT KdContext
)
{
    NTSTATUS status;
    
    status = STATUS_NOT_IMPLEMENTED;
    
    if (Phase == 0) {
        if (LoaderBlock != NULL) {
            __security_init_cookie();
        }
        status = KdDebuggerInitialize0(LoaderBlock);
    } else if (Phase == 1) {
        status = KdDebuggerInitialize1(LoaderBlock);
    } else if (Phase == 2) {
        status = KdDebuggerInitialize2(LoaderBlock);
    }
    
    return status;
}

// KdPower - Power transition handler
NTSTATUS
KdPower(
    ULONG PowerState,
    PKD_CONTEXT KdContext
)
{
    NTSTATUS status;
    
    status = STATUS_NOT_IMPLEMENTED;
    
    if (PowerState == 1 || PowerState == 4) {
        status = STATUS_SUCCESS;
    }
    
    return status;
}

// KdSetHiberRange - Set hibernation range
NTSTATUS
KdSetHiberRange(
    PKD_CONTEXT KdContext
)
{
    PoSetHiberRange(0, 0x10000, KdSetHiberRange, 0, 0x3168644b);
    return STATUS_SUCCESS;
}

