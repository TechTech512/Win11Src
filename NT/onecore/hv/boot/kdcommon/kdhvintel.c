#include "kdcommon.h"

// Global variables
extern DBG_CPU_TYPE KdHvpCpuType;
extern BOOLEAN IsHypercallInitialized;

// Forward declarations
extern UCHAR KdHvDetectHypervisor(void);
UCHAR KdHvpIsIntelCpu(void);
UCHAR KdHvpIsAmdCpu(void);

// AMD VMM call hypercall
HV_HYPERCALL_OUTPUT
HcAmdVmcall(
    HV_HYPERCALL_INPUT Input,
    unsigned long long Param2,
    unsigned long long Param3,
    unsigned long long Param4
)
{
    HV_HYPERCALL_OUTPUT Output;
    
#ifdef _AMD64_
    Output.AsUINT64 = __vmmcall();
#else
    __asm {
        _emit 0x0F
        _emit 0x01
        _emit 0xD9
    }
    Output.AsUINT64 = Input.AsUINT64;
#endif
    
    return Output;
}

// Intel VM call hypercall
HV_HYPERCALL_OUTPUT
HcIntelVmcall(
    HV_HYPERCALL_INPUT Input,
    unsigned long long Param2,
    unsigned long long Param3,
    unsigned long long Param4
)
{
    HV_HYPERCALL_OUTPUT Output;
    
#ifdef _AMD64_
    Output.AsUINT64 = __vmcall();
#else
    __asm {
        _emit 0x0F
        _emit 0x01
        _emit 0xC1
    }
    Output.AsUINT64 = Input.AsUINT64;
#endif
    
    return Output;
}

// Connect to hypervisor
UCHAR
KdHvConnectHypervisor(void)
{
    unsigned long long msrValue;
    UCHAR isHypervisorPresent;
    
    isHypervisorPresent = KdHvDetectHypervisor();
    if (isHypervisorPresent == 0) {
        return 0;
    }
    
    msrValue = __readmsr(0x40000001);
    if ((msrValue & 1) == 0) {
        __writemsr(0x40000000, 0x1000101010001);
    } else {
        msrValue = __readmsr(0x40000000);
        if ((ULONG)msrValue == 0 && (ULONG)(msrValue >> 32) == 0) {
            return 0;
        }
    }
    
    if (KdHvpIsIntelCpu() != 0) {
        KdHvpCpuType = DbgCpuIntel;
    } else if (KdHvpIsAmdCpu() != 0) {
        KdHvpCpuType = DbgCpuAmd;
    } else {
        return 0;
    }
    
    IsHypercallInitialized = TRUE;
    return 1;
}

// Check if CPU is AMD
UCHAR
KdHvpIsAmdCpu(void)
{
    int cpuInfo[4];
    
    __cpuid(cpuInfo, 0);
    
    if (cpuInfo[1] == 0x68747541 &&  // "Auth"
        cpuInfo[2] == 0x69746e65 &&  // "enti"
        cpuInfo[3] == 0x444d4163) {  // "cAMD"
        return 1;
    }
    
    return 0;
}

// Check if CPU is Intel
UCHAR
KdHvpIsIntelCpu(void)
{
    int cpuInfo[4];
    
    __cpuid(cpuInfo, 0);
    
    if (cpuInfo[1] == 0x756e6547 &&  // "Genu"
        cpuInfo[2] == 0x49656e69 &&  // "ineI"
        cpuInfo[3] == 0x6c65746e) {  // "ntel"
        return 1;
    }
    
    return 0;
}

