/*
 * iumbase.c
 * 
 * Cleaned version of decompiled IUM base functions.
 * All external functions are declared as imports.
 */
#pragma warning (disable:4083)
#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* ------------------- External Imports ------------------- */

extern int32_t IumAssignMemoryToSocDomain(uint64_t *outHandle,
                                          uint64_t arg1, uint64_t arg2,
                                          uint32_t arg3, uint32_t arg4,
                                          uint32_t arg5, uint32_t arg6,
                                          uint32_t arg7);

extern int __cdecl IumAwaitSmc(void);

extern int32_t IumCreateSecureDevice(uint64_t arg1, uint64_t *outHandle);

extern int32_t IumCreateSecureSection(uint64_t *outHandle,
                                      uint64_t arg1, uint64_t arg2,
                                      uint32_t arg3, uint32_t arg4,
                                      uint32_t arg5);

extern int32_t IumCreateSecureSectionSpecifyPages(uint64_t *outHandle,
                                                  uint64_t arg1, uint64_t arg2,
                                                  uint32_t arg3, uint32_t arg4,
                                                  uint32_t arg5, uint32_t arg6);

extern uint32_t IumCrypto(void *context);

extern int32_t IumDmaMapMemory(uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4,
                               uint32_t arg5, uint64_t *outHandle,
                               uint64_t arg6, uint64_t arg7);

extern int __cdecl IumEmitSmc(void);

extern int __cdecl IumFlushSecureSectionBuffers(void);

extern int32_t IumGetDmaEnabler(uint64_t arg1, uint64_t arg2,
                                uint64_t *outHandle);

extern int32_t IumGetExposedSecureSection(uint64_t *outHandle, uint64_t arg1);

extern int32_t IumGetIdk(uint32_t type, uint64_t arg2, uint64_t *outHandle);

extern int32_t IumMapSecureIo(uint64_t arg1, uint64_t arg2,
                              uint64_t arg3, uint32_t arg4,
                              uint64_t arg5, uint64_t *outHandle);

extern int32_t IumOpenCurrentExtension(uint64_t *outHandle);

extern int32_t IumOpenSecureSection(uint64_t *outHandle, uint64_t arg1);

extern int __cdecl IumPostMailbox(void);

extern int32_t IumProtectSecureIo(uint64_t *arg1, uint64_t *arg2);

extern int __cdecl IumQuerySecureDeviceInformation(void);

extern int __cdecl IumSecureStorageGet(void);

extern int __cdecl IumSecureStoragePut(void);

extern int __cdecl IumSetDmaTargetProperties(void);

extern int __cdecl IumSetPolicyExtension(void);

extern int __cdecl IumUnmapSecureIo(void);

extern int __cdecl IumUpdateSecureDeviceState(void);

extern void RtlSetLastWin32Error(uint32_t error);
extern void RtlSetLastWin32ErrorAndNtStatusFromNtStatus(int32_t status);

/* ------------------- Helper for GS access (Windows) ------------------- */

#if defined(_MSC_VER)
#include <intrin.h>
#define READ_GS_QWORD(offset) __readgsqword(offset)
#else
static inline uint64_t read_gs_qword(uint32_t offset) {
    uint64_t value;
    __asm__("movq %%gs:(%1), %0" : "=r"(value) : "r"((uint64_t)offset));
    return value;
}
#define READ_GS_QWORD(offset) read_gs_qword(offset)
#endif

/* ------------------- Function Definitions ------------------- */

uint64_t AssignMemoryToSocDomain(uint64_t param1, uint64_t param2,
                                 uint32_t param3, uint32_t param4,
                                 uint32_t param5, uint32_t param6,
                                 uint32_t param7)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumAssignMemoryToSocDomain(&outHandle,
                                        param1, param2,
                                        param3, param4,
                                        param5, param6,
                                        param7);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

BOOL AwaitSmc(void)
{
    int32_t status = IumAwaitSmc();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

uint64_t CreateSecureDevice(uint64_t param1)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumCreateSecureDevice(param1, &outHandle);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint64_t CreateSecureSection(uint64_t param1, uint32_t param2,
                             uint32_t param3, uint64_t param4,
                             uint32_t param5)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumCreateSecureSection(&outHandle,
                                    param1, param4,
                                    param2, param3,
                                    param5);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint64_t CreateSecureSectionSpecifyPages(uint64_t param1, uint64_t param2,
                                         uint32_t param3, uint32_t param4,
                                         uint32_t param5, uint32_t param6)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumCreateSecureSectionSpecifyPages(&outHandle,
                                                param1, param2,
                                                param3, param4,
                                                param5, param6);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint32_t DecryptData(uint64_t param1, uint64_t param2,
                     uint64_t param3, uint64_t param4,
                     uint64_t *param5, uint32_t *param6)
{
    uint32_t status;

    if (param5 == NULL) {
        return 0xD000000D;
    }

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;
        uint64_t field1;  // param2
        uint64_t field2;  // param4
        uint64_t field3;  // param1
        uint64_t field4;  // param3
        uint64_t output1; // local_30
        uint32_t output2; // local_20
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 1;
    ctx.field1 = param2;
    ctx.field2 = param4;
    ctx.field3 = param1;
    ctx.field4 = param3;

    status = IumCrypto(&ctx);
    *param5 = ctx.output1;
    if ((int32_t)status >= 0 && param6 != NULL) {
        *param6 = ctx.output2;
    }
    return status | 0x10000000;
}

uint32_t DecryptISKBoundData(uint64_t param1, uint64_t param2,
                             uint64_t param3, uint64_t param4,
                             uint64_t *param5)
{
    uint32_t status;

    if (param5 == NULL) {
        return 0xD000000D;
    }

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;
        uint64_t field1;  // param2
        uint64_t field2;  // param4
        uint64_t field3;  // param1
        uint64_t field4;  // param3
        uint64_t output;  // local_30
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 2;
    ctx.field1 = param2;
    ctx.field2 = param4;
    ctx.field3 = param1;
    ctx.field4 = param3;

    status = IumCrypto(&ctx);
    *param5 = ctx.output;
    return status | 0x10000000;
}

uint64_t DmaMapMemory(uint64_t param1, uint64_t param2,
                      uint64_t param3, uint64_t param4,
                      int param5, uint64_t param6, uint64_t param7)
{
    int32_t status;
    uint64_t outHandle = 0;

    if (param1 == 0 || param2 == 0 || param6 == 0 || param7 == 0 || param5 != 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    status = IumDmaMapMemory(param1, param2, param3, param4, 0,
                             &outHandle, param6, param7);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    return outHandle;
}

BOOL EmitSmc(void)
{
    int32_t status = IumEmitSmc();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

uint32_t EncryptData(uint64_t param1, uint64_t param2,
                     uint64_t param3, uint64_t param4,
                     uint64_t param5, uint64_t param6,
                     uint64_t *param7, uint16_t param8)
{
    uint32_t status;

    if (param7 == NULL) {
        return 0xD000000D;
    }

    #pragma pack(push, 1)
    struct {
        uint8_t padding[4];  // local_88 (first 4 bytes)
        uint32_t field1;     // local_84 (param8)
        uint64_t field2;     // local_70 (param2)
        uint64_t field3;     // local_68 (param6)
        uint64_t field4;     // local_58 (param4)
        uint64_t field5;     // local_50 (param1)
        uint64_t field6;     // local_48 (param5)
        uint64_t field7;     // local_38 (param3)
        uint64_t output;     // local_30
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.field1 = param8;
    ctx.field6 = param5;
    ctx.field3 = param6;
    ctx.field2 = param2;
    ctx.field4 = param4;
    ctx.field5 = param1;
    ctx.field7 = param3;

    status = IumCrypto(&ctx);
    *param7 = ctx.output;
    return status | 0x10000000;
}

BOOL FlushSecureSectionBuffers(void)
{
    int32_t status = IumFlushSecureSectionBuffers();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

uint64_t GetDmaEnabler(uint64_t param1, uint64_t param2)
{
    int32_t status;
    uint64_t outHandle = 0;

    if (param1 == 0 || param2 == 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    status = IumGetDmaEnabler(param1, param2, &outHandle);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    return outHandle;
}

uint64_t GetExposedSecureSection(uint64_t param1)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumGetExposedSecureSection(&outHandle, param1);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint64_t GetFipsModeFromIumKernelState(uint64_t param1)
{
    int32_t status;

    if (param1 == 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;
        uint16_t unused;    // local_86
        uint64_t field1;    // local_68
        uint64_t field2;    // local_48
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 5;
    ctx.field1 = 1;
    ctx.field2 = param1;

    status = IumCrypto(&ctx);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    return 1;
}

uint64_t GetSecureIdentityKey(uint64_t param1, uint64_t *param2)
{
    int32_t status;
    uint64_t outHandle = *param2;

    status = IumGetIdk(0, param1, &outHandle);
    if (status < 0) {
        if (status == -0x3FFFFFDD) {
            *param2 = outHandle;
        }
        RtlSetLastWin32Error(RtlNtStatusToDosError(status));
        return 0;
    }
    *param2 = outHandle;
    return 1;
}

uint64_t GetSecureIdentitySigningKey(uint64_t param1, uint64_t *param2)
{
    int32_t status;
    uint64_t outHandle = *param2;

    status = IumGetIdk(1, param1, &outHandle);
    if (status < 0) {
        if (status == -0x3FFFFFDD) {
            *param2 = outHandle;
        }
        RtlSetLastWin32Error(RtlNtStatusToDosError(status));
        return 0;
    }
    *param2 = outHandle;
    return 1;
}

uint64_t GetSeedFromIumKernelState(uint64_t param1, uint64_t param2,
                                   uint64_t *param3)
{
    int32_t status;

    if (param3 == NULL) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;
        uint16_t unused;    // local_86
        uint64_t field1;    // local_68
        uint64_t field2;    // local_48
        uint64_t output;    // local_18
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 4;
    ctx.field1 = param2;
    ctx.field2 = param1;

    status = IumCrypto(&ctx);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    *param3 = ctx.output;
    return 1;
}

uint32_t GetSignedReport(uint32_t param1, uint32_t param2,
                         uint64_t param3, uint64_t param4,
                         uint64_t param5, uint64_t *param6,
                         uint64_t param7, uint64_t *param8,
                         uint32_t param9)
{
    uint32_t status;

    if (param6 == NULL || param8 == NULL) {
        return 0xD000000D;
    }

    // param6 is used as both input and output; we keep its value.
    // The decompiler set param6._0_4_ = param1 and param6._4_4_ = param2.
    // We reinterpret as two uint32_t fields.
    union {
        uint64_t as_u64;
        uint32_t as_u32[2];
    } u;
    u.as_u64 = *param6;
    u.as_u32[0] = param1;
    u.as_u32[1] = param2;
    *param6 = u.as_u64;

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;        // local_88[0]
        uint16_t padding;       // local_88[1]? (unused)
        uint32_t field1;        // local_84
        uint64_t field2;        // local_70
        uint64_t field3;        // local_58 (set to 8)
        uint64_t field4;        // local_50
        uint64_t field5;        // local_48
        uint64_t field6;        // local_40
        uint64_t *field7;       // local_38 (points to param6)
        uint64_t field8;        // local_30
        uint64_t field9;        // local_28
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 3;
    ctx.field1 = param9;
    ctx.field4 = param3;
    ctx.field5 = param5;
    ctx.field6 = param7;
    ctx.field3 = 8;            // local_58
    ctx.field2 = param4;
    ctx.field7 = param6;       // pointer to param6
    ctx.field8 = *param6;      // local_30 = *param6
    ctx.field9 = *param8;      // local_28 = *param8

    status = IumCrypto(&ctx);

    // If status indicates failure or buffer too small, update outputs
    if ((int32_t)status < 0 || status == STATUS_BUFFER_TOO_SMALL) {
        *param6 = ctx.field8;
        *param8 = ctx.field9;
    }
    return status | 0x10000000;
}

uint64_t GetTaggedData(uint64_t param1)
{
    // param1 points to a structure; at offset 0xC is a uint32_t size, add to base
    return param1 + (uint64_t)(*(uint32_t *)(param1 + 0xC));
}

uint32_t GetTaggedDataSize(uint64_t param1)
{
    return *(uint32_t *)(param1 + 8);
}

uint32_t GetTpmBindingInfo(uint64_t param1, uint64_t *param2)
{
    uint32_t status;

    if (param2 == NULL) {
        return 0xD000000D;
    }

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;        // local_88[0]
        uint8_t padding[0x76];  // fill rest (0x78 - 2)
        uint64_t field1;        // local_48
        uint64_t field2;        // local_30 (output)
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 6;
    ctx.field2 = *param2;      // local_30 = *param2
    ctx.field1 = param1;       // local_48 = param1

    status = IumCrypto(&ctx);
    if ((int32_t)status < 0 || status == STATUS_BUFFER_TOO_SMALL) {
        *param2 = ctx.field2;
        status = (status != 0) ? (status | 0x10000000) : 0;
    } else {
        status |= 0x10000000;
    }
    return status;
}

uint32_t IsSecureProcess(void)
{
    // Read from GS segment: GS+0x60 -> TEB, then +0x20, then +8, then bit 31.
    uint64_t teb = (uint64_t)NtCurrentTeb();
    uint64_t unknown1 = *(uint64_t *)(teb + 0x20);
    uint32_t value = *(uint32_t *)(unknown1 + 8);
    return (value >> 31) & 1;
}

uint64_t MapSecureIo(uint64_t param1, uint64_t param2,
                     uint32_t param3, uint64_t param4,
                     uint64_t param5)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumMapSecureIo(param1, param2, param4, param3, param5, &outHandle);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint64_t OpenCurrentExtension(void)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumOpenCurrentExtension(&outHandle);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

uint64_t OpenSecureSection(uint64_t param1)
{
    int32_t status;
    uint64_t outHandle = 0;

    status = IumOpenSecureSection(&outHandle, param1);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        outHandle = 0;
    }
    return outHandle;
}

BOOL PostMailbox(void)
{
    int32_t status = IumPostMailbox();
    if (status < 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(status));
    }
    return (status >= 0);
}

BOOL ProtectSecureIo(uint64_t param1, uint64_t param2)
{
    int32_t status;
    uint64_t local1 = param1;
    uint64_t local2 = param2;

    status = IumProtectSecureIo(&local1, &local2);
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

uint64_t QuerySecureDeviceInformation(uint64_t param1, uint64_t param2,
                                      uint64_t param3)
{
    int32_t status;

    if (param3 == 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    status = IumQuerySecureDeviceInformation();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    return 1;
}

BOOL SecureStorageGet(void)
{
    int32_t status = IumSecureStorageGet();
    if (status < 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(status));
    }
    return (status >= 0);
}

BOOL SecureStoragePut(void)
{
    int32_t status = IumSecureStoragePut();
    if (status < 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(status));
    }
    return (status >= 0);
}

uint64_t SetDmaTargetProperties(uint64_t param1, uint64_t param2)
{
    int32_t status;

    if (param2 == 0) {
        RtlSetLastWin32Error(RtlNtStatusToDosError(STATUS_INVALID_PARAMETER));
        return 0;
    }

    status = IumSetDmaTargetProperties();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
        return 0;
    }
    return 1;
}

BOOL SetPolicyExtension(void)
{
    int32_t status = IumSetPolicyExtension();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

BOOL UnmapSecureIo(void)
{
    int32_t status = IumUnmapSecureIo();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

BOOL UpdateSecureDeviceState(void)
{
    int32_t status = IumUpdateSecureDeviceState();
    if (status < 0) {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    }
    return (status >= 0);
}

uint32_t VbsVmSysCall(uint64_t param1, uint64_t param2,
                      uint64_t param3, uint64_t param4,
                      uint64_t *param5)
{
    uint32_t status;

    #pragma pack(push, 1)
    struct {
        uint16_t opcode;        // local_88[0]
        uint8_t padding[0x76];
        uint64_t field1;        // local_70
        uint64_t field2;        // local_68
        uint64_t field3;        // local_50
        uint64_t field4;        // local_48
        uint64_t output;        // local_30
    } ctx;
    #pragma pack(pop)

    memset(&ctx, 0, sizeof(ctx));
    ctx.opcode = 8;
    ctx.field1 = param2;
    ctx.field2 = param4;
    ctx.field3 = param1;
    ctx.field4 = param3;

    status = IumCrypto(&ctx);
    if (param5 != NULL) {
        *param5 = ctx.output;
    }
    return status | 0x10000000;
}

uint32_t VerifyEnclaveAttestationReport(int param1, uint64_t param2,
                                        uint32_t param3)
{
    uint32_t status;

    if (param1 == 0x10 && param2 != 0 && param3 != 0) {
        #pragma pack(push, 1)
        struct {
            uint16_t opcode;        // local_88[0]
            uint8_t padding[0x76];
            uint64_t field1;        // local_70 (param3)
            uint64_t field2;        // local_50 (param2)
        } ctx;
        #pragma pack(pop)

        memset(&ctx, 0, sizeof(ctx));
        ctx.opcode = 7;
        ctx.field1 = param3;
        ctx.field2 = param2;

        status = IumCrypto(&ctx);
        return status | 0x10000000;
    }
    return 0x80070057;
}

