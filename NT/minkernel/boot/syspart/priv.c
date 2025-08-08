#include "sysboot.h"

#ifdef _KERNEL_MODE

long BiOpenEffectiveToken(void **outToken) {
    if (!outToken) return STATUS_INVALID_PARAMETER;

    PACCESS_TOKEN token = PsReferencePrimaryToken(PsGetCurrentProcess());
    if (!token) return STATUS_UNSUCCESSFUL;

    *outToken = (void *)token;
    return STATUS_SUCCESS;
}

long BiAdjustPrivilege(ULONG privilegeId, UCHAR enable, UCHAR *wasEnabled) {
    void *token = NULL;
    long status = BiOpenEffectiveToken(&token);
    if (status != STATUS_SUCCESS) return status;

    LUID luid;
    RtlConvertLongToLuid(privilegeId, &luid);

    PRIVILEGE_SET privSet;
    privSet.PrivilegeCount = 1;
    privSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    privSet.Privilege[0].Luid = luid;
    privSet.Privilege[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

    BOOLEAN hasPrivilege = SePrivilegeCheck(&privSet, (PACCESS_TOKEN)token, KernelMode);
    if (wasEnabled) {
        *wasEnabled = hasPrivilege ? 1 : 0;
    }

    ObDereferenceObject(token);
    return STATUS_SUCCESS;
}

#else // USER MODE

long BiOpenEffectiveToken(void **outToken) {
    if (!outToken) return STATUS_INVALID_PARAMETER;

    HANDLE token;
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &token)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
                return STATUS_UNSUCCESSFUL;
            }
        } else {
            return STATUS_UNSUCCESSFUL;
        }
    }

    *outToken = (void *)token;
    return STATUS_SUCCESS;
}

long BiAdjustPrivilege(ULONG privilegeId, UCHAR enable, UCHAR *wasEnabled) {
    void *tokenVoid = NULL;
    long status = BiOpenEffectiveToken(&tokenVoid);
    if (status != STATUS_SUCCESS) return status;

    HANDLE token = (HANDLE)tokenVoid;

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(token);
        return STATUS_UNSUCCESSFUL;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

    TOKEN_PRIVILEGES previous;
    DWORD returnLength = sizeof(previous);

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(previous), &previous, &returnLength)) {
        CloseHandle(token);
        return STATUS_UNSUCCESSFUL;
    }

    if (wasEnabled) {
        *wasEnabled = (previous.PrivilegeCount > 0 &&
                       (previous.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)) ? 1 : 0;
    }

    CloseHandle(token);
    return STATUS_SUCCESS;
}

#endif

