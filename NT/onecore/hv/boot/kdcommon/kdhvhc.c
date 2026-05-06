#include "kdcommon.h"

BOOLEAN HviIsHypervisorMicrosoftCompatible(void);

// Global variables
BOOLEAN HypervisorDetected = FALSE;
DBG_CPU_TYPE KdHvpCpuType = DbgCpuUnknown;
BOOLEAN IsHypercallInitialized = FALSE;

// Hypercall buffers - 0x3000 bytes (from 0x80013040 to 0x8001603c)
UCHAR HypercallBuffers[0x3000] = {0};

// External functions
extern HV_HYPERCALL_OUTPUT HcIntelVmcall(HV_HYPERCALL_INPUT Input, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);
extern HV_HYPERCALL_OUTPUT HcAmdVmcall(HV_HYPERCALL_INPUT Input, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);

// Detect if running under Microsoft hypervisor
UCHAR
KdHvDetectHypervisor(void)
{
    if (HypervisorDetected != FALSE) {
        return 1;
    }
    
    HypervisorDetected = HviIsHypervisorMicrosoftCompatible();
    return HypervisorDetected;
}

// Post debug data to hypervisor
DBG_PACKET_RESULT
KdHvPostDebugData(
    ULONG DataSize,
    ULONG Param2,
    PUCHAR DataBuffer,
    PULONG Param4
)
{
    UCHAR isHypervisorPresent;
    HV_HYPERCALL_OUTPUT hypercallOutput;
    HV_HYPERCALL_INPUT hypercallInput;
    DBG_PACKET_RESULT result;
    
    isHypervisorPresent = KdHvDetectHypervisor();
    if (isHypervisorPresent == 0 || IsHypercallInitialized == 0) {
        return DbgPrNoHost;
    }
    
    RtlZeroMemory(HypercallBuffers, 0x1000);
    RtlZeroMemory(HypercallBuffers + 0x1000, 0x1000);
    
    *(PULONG)(HypercallBuffers + 0xfc0) = DataSize;
    *(PULONG)(HypercallBuffers + 0xfc4) = Param2;
    
    if (DataSize != 0) {
        RtlCopyMemory(HypercallBuffers + 0x1000, DataBuffer, DataSize);
    }
    
    result = DbgPrNoHost;
    hypercallInput.AsUINT64 = 0x69;
    
    if (KdHvpCpuType == DbgCpuIntel) {
        hypercallOutput = HcIntelVmcall(hypercallInput, 0, 
                          MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                          MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    } else if (KdHvpCpuType == DbgCpuAmd) {
        hypercallOutput = HcAmdVmcall(hypercallInput, 0,
                         MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                         MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    } else {
        return DbgPrNoHost;
    }
    
    if (hypercallOutput._s_0.CallStatus == 0) {
        result = DbgPrSent;
    }
    
    if (Param2 != 0) {
        *(PULONG)Param2 = *(PULONG)(HypercallBuffers + 0x1fc0);
    }
    
    return result;
}

// Reset debug session
void
KdHvResetDebugSession(
    ULONG Param1
)
{
    UCHAR isHypervisorPresent;
    HV_HYPERCALL_OUTPUT hypercallOutput;
    HV_HYPERCALL_INPUT hypercallInput;
    
    isHypervisorPresent = KdHvDetectHypervisor();
    if (isHypervisorPresent == 0 || IsHypercallInitialized == 0) {
        return;
    }
    
    RtlZeroMemory(HypercallBuffers, 0x1000);
    
    *(PULONG)(HypercallBuffers + 0xfc0) = Param1;
    
    hypercallInput.AsUINT64 = 0x6b;
    
    if (KdHvpCpuType == DbgCpuIntel) {
        hypercallOutput = HcIntelVmcall(hypercallInput, 0,
                          MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                          MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    } else if (KdHvpCpuType == DbgCpuAmd) {
        hypercallOutput = HcAmdVmcall(hypercallInput, 0,
                         MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                         MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    }
}

// Retrieve debug data from hypervisor
DBG_PACKET_RESULT
KdHvRetrieveDebugData(
    ULONG BufferSize,
    PUCHAR OutputBuffer,
    ULONG Param3,
    PULONG Param4,
    PULONG Param5
)
{
    UCHAR isHypervisorPresent;
    HV_HYPERCALL_OUTPUT hypercallOutput;
    HV_HYPERCALL_INPUT hypercallInput;
    BOOLEAN noData;
    ULONG bytesAvailable;
    
    if (OutputBuffer != NULL) {
        OutputBuffer[0] = 0;
        OutputBuffer[1] = 0;
        OutputBuffer[2] = 0;
        OutputBuffer[3] = 0;
    }
    
    if (Param3 != 0) {
        *(PULONG)Param3 = 0;
    }
    
    isHypervisorPresent = KdHvDetectHypervisor();
    if (isHypervisorPresent == 0 || IsHypercallInitialized == 0) {
        return DbgPrTimeout;
    }
    
    RtlZeroMemory(HypercallBuffers, 0x1000);
    RtlZeroMemory(HypercallBuffers + 0x1000, 0x1000);
    
    HypercallBuffers[0xfc4] = 0;
    HypercallBuffers[0xfc5] = 0;
    HypercallBuffers[0xfc6] = 0;
    HypercallBuffers[0xfc7] = 0;
    
    *(PULONG)(HypercallBuffers + 0xfc0) = BufferSize;
    
    noData = TRUE;
    hypercallInput.AsUINT64 = 0x6a;
    
    if (KdHvpCpuType == DbgCpuIntel) {
        hypercallOutput = HcIntelVmcall(hypercallInput, 0,
                          MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                          MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    } else if (KdHvpCpuType == DbgCpuAmd) {
        hypercallOutput = HcAmdVmcall(hypercallInput, 0,
                         MmGetPhysicalAddress(HypercallBuffers + 0xfc0).QuadPart,
                         MmGetPhysicalAddress(HypercallBuffers + 0x1fc0).QuadPart);
    } else {
        return DbgPrTimeout;
    }
    
    if (hypercallOutput._s_0.CallStatus == 0 || hypercallOutput._s_0.CallStatus == 0x1f) {
        noData = FALSE;
    }
    
    if (!noData) {
        bytesAvailable = *(PULONG)(HypercallBuffers + 0x1fc0);
        if (BufferSize < bytesAvailable) {
            *(PULONG)(HypercallBuffers + 0x1fc0) = BufferSize;
        }
        RtlCopyMemory(OutputBuffer, HypercallBuffers + 0x1000, *(PULONG)(HypercallBuffers + 0x1fc0));
    }
    
    if (Param3 != 0) {
        *(PULONG)Param3 = *(PULONG)(HypercallBuffers + 0x1fc4);
    }
    
    if (OutputBuffer != NULL) {
        *(PULONG)OutputBuffer = *(PULONG)(HypercallBuffers + 0x1fc0);
    }
    
    return noData ? DbgPrTimeout : DbgPrReceived;
}

