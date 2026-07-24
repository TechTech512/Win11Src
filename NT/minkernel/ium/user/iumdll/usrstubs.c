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
#pragma warning (disable:4083)
#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
int32_t IumAssignMemoryToSocDomain(uint64_t *outHandle,
                                          uint64_t arg1, uint64_t arg2,
                                          uint32_t arg3, uint32_t arg4,
                                          uint32_t arg5, uint32_t arg6,
                                          uint32_t arg7){
											  DO_SYSCALL(0x01) \
                                              return 0x8000000; \
										  }

// Ordinal 2: IumAwaitSmc
IUM_STUB(0x02, IumAwaitSmc, 0x8000001)

// Ordinal 3: IumCreateSecureDevice
int32_t IumCreateSecureDevice(uint64_t arg1, uint64_t *outHandle){
	DO_SYSCALL(0x03) \
	return 0x8000002; \
}

// Ordinal 4: IumCreateSecureSection
int32_t IumCreateSecureSection(uint64_t *outHandle,
                                      uint64_t arg1, uint64_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5){
										  DO_SYSCALL(0x04) \
										  return 0x8000003; \
									  }

// Ordinal 5: IumCreateSecureSectionSpecifyPages
int32_t IumCreateSecureSectionSpecifyPages(uint64_t *outHandle,
                                                  uint64_t arg1, uint64_t arg2,
                                                  uint32_t arg3, uint32_t arg4,
                                                  uint32_t arg5, uint32_t arg6){
													  DO_SYSCALL(0x05) \
													  return 0x8000004; \
												  }

// Ordinal 6: IumCrypto
uint32_t IumCrypto(void *context){
	DO_SYSCALL(0x06) \
	return 0x8000005; \
}

// Ordinal 7: IumDmaMapMemory
int32_t IumDmaMapMemory(uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4,
                               uint32_t arg5, uint64_t *outHandle,
                               uint64_t arg6, uint64_t arg7){
								   DO_SYSCALL(0x07) \
								   return 0x8000006; \
							   }

// Ordinal 8: IumEmitSmc
IUM_STUB(0x08, IumEmitSmc, 0x8000007)

// Ordinal 9: IumFlushSecureSectionBuffers
IUM_STUB(0x09, IumFlushSecureSectionBuffers, 0x8000008)

// Ordinal 10: IumGetDmaEnabler
int32_t IumGetDmaEnabler(uint64_t arg1, uint64_t arg2,
                                uint64_t *outHandle){
									DO_SYSCALL(0x0A) \
									return 0x8000009; \
								}

// Ordinal 11: IumGetExposedSecureSection
int32_t IumGetExposedSecureSection(uint64_t *outHandle, uint64_t arg1){
	DO_SYSCALL(0x0B) \
	return 0x800000A; \
}

// Ordinal 12: IumGetIdk
int32_t IumGetIdk(uint32_t type, uint64_t arg2, uint64_t *outHandle){
	DO_SYSCALL(0x0C) \
	return 0x800000B; \
}

// Ordinal 13: IumMapSecureIo
int32_t IumMapSecureIo(uint64_t arg1, uint64_t arg2,
                              uint64_t arg3, uint32_t arg4,
                              uint64_t arg5, uint64_t *outHandle){
								  DO_SYSCALL(0x0D) \
								  return 0x800000C; \
							  }

// Ordinal 14: IumOpenCurrentExtension
int32_t IumOpenCurrentExtension(uint64_t *outHandle){
	DO_SYSCALL(0x0E) \
	return 0x800000D; \
}

// Ordinal 15: IumOpenSecureSection
int32_t IumOpenSecureSection(uint64_t *outHandle, uint64_t arg1){
	DO_SYSCALL(0x0F) \
	return 0x800000E; \
}

// Ordinal 16: IumPostMailbox
IUM_STUB(0x10, IumPostMailbox, 0x800000F)

// Ordinal 17: IumProtectSecureIo
int32_t IumProtectSecureIo(uint64_t *arg1, uint64_t *arg2){
	DO_SYSCALL(0x11) \
	return 0x8000010; \
}

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

