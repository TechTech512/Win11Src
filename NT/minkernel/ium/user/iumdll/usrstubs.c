/*
 * usrstubs.c
 *
 * IUM (Isolated User Mode) system service stubs.
 * Each function invokes a syscall (via int 0x2e on x86, or syscall on x64)
 * and returns a specific NTSTATUS error code.
 *
 * These are placeholders – the actual syscall numbers should be defined
 * according to the IUM service table.
 */

#ifdef _WIN64
// x64 inline assembly not supported in MSVC; use a helper function.
// For this stub, we'll just use a dummy call to force a syscall.
#define DO_SYSCALL(index) \
    __asm { mov eax, index } \
    __asm { syscall }
#else
#define DO_SYSCALL(index) \
    __asm { mov eax, index } \
    __asm { int 0x2e }
#endif

// Macro to generate each stub
#define IUM_STUB(index, name, status) \
    int __cdecl name() { \
        DO_SYSCALL(index) \
        return status; \
    }

// Ordinal 1: IumAssignMemoryToSocDomain
IUM_STUB(0x01, IumAssignMemoryToSocDomain, 0x8000000)

// Ordinal 2: IumAwaitSmc
IUM_STUB(0x02, IumAwaitSmc, 0x8000001)

// Ordinal 3: IumCreateSecureDevice
IUM_STUB(0x03, IumCreateSecureDevice, 0x8000002)

// Ordinal 4: IumCreateSecureSection
IUM_STUB(0x04, IumCreateSecureSection, 0x8000003)

// Ordinal 5: IumCreateSecureSectionSpecifyPages
IUM_STUB(0x05, IumCreateSecureSectionSpecifyPages, 0x8000004)

// Ordinal 6: IumCrypto
IUM_STUB(0x06, IumCrypto, 0x8000005)

// Ordinal 7: IumDmaMapMemory
IUM_STUB(0x07, IumDmaMapMemory, 0x8000006)

// Ordinal 8: IumEmitSmc
IUM_STUB(0x08, IumEmitSmc, 0x8000007)

// Ordinal 9: IumFlushSecureSectionBuffers
IUM_STUB(0x09, IumFlushSecureSectionBuffers, 0x8000008)

// Ordinal 10: IumGetDmaEnabler
IUM_STUB(0x0A, IumGetDmaEnabler, 0x8000009)

// Ordinal 11: IumGetExposedSecureSection
IUM_STUB(0x0B, IumGetExposedSecureSection, 0x800000A)

// Ordinal 12: IumGetIdk
IUM_STUB(0x0C, IumGetIdk, 0x800000B)

// Ordinal 13: IumMapSecureIo
IUM_STUB(0x0D, IumMapSecureIo, 0x800000C)

// Ordinal 14: IumOpenCurrentExtension
IUM_STUB(0x0E, IumOpenCurrentExtension, 0x800000D)

// Ordinal 15: IumOpenSecureSection
IUM_STUB(0x0F, IumOpenSecureSection, 0x800000E)

// Ordinal 16: IumPostMailbox
IUM_STUB(0x10, IumPostMailbox, 0x800000F)

// Ordinal 17: IumProtectSecureIo
IUM_STUB(0x11, IumProtectSecureIo, 0x8000010)

// Ordinal 18: IumQuerySecureDeviceInformation
IUM_STUB(0x12, IumQuerySecureDeviceInformation, 0x8000011)

// Ordinal 19: IumSecureStorageGet
IUM_STUB(0x13, IumSecureStorageGet, 0x8000012)

// Ordinal 20: IumSecureStoragePut
IUM_STUB(0x14, IumSecureStoragePut, 0x8000013)

// Ordinal 21: IumSetDmaTargetProperties
IUM_STUB(0x15, IumSetDmaTargetProperties, 0x8000014)

// Ordinal 22: IumSetPolicyExtension
IUM_STUB(0x16, IumSetPolicyExtension, 0x8000015)

// Ordinal 23: IumUnmapSecureIo
IUM_STUB(0x17, IumUnmapSecureIo, 0x8000016)

// Ordinal 24: IumUpdateSecureDeviceState
IUM_STUB(0x18, IumUpdateSecureDeviceState, 0x8000017)

