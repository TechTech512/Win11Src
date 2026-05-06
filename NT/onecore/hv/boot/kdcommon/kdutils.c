#define KDUTILS_C
#include "kdcommon.h"

#undef KdHvComputeChecksum

// Compute checksum for hypervisor communication
unsigned int KdHvComputeChecksum(PUCHAR Data, unsigned int Length)
{
    unsigned int checksum = 0;
    unsigned int i;
    
    for (i = 0; i < Length; i++) {
        checksum += Data[i];
    }
    
    return checksum;
}

