#include <windows.h>
#include <stdio.h>

long BiOpenEffectiveToken(HANDLE *outToken) {
    HANDLE token = NULL;
    BOOL result = FALSE;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TRUE, &token)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
        }
    } else {
        result = TRUE;
    }

    if (!result || token == NULL) {
        return -1;  // NTSTATUS-style failure
    }

    *outToken = token;
    return 0;  // STATUS_SUCCESS
}

long BiAdjustPrivilege(ULONG privilegeId, UCHAR enable, UCHAR *wasEnabled) {
    HANDLE token = NULL;
    LUID luid;
    TOKEN_PRIVILEGES tp, oldTp;
    DWORD returnLength = 0;

    if (wasEnabled) *wasEnabled = FALSE;

    if (BiOpenEffectiveToken(&token) != 0) {
        return -1;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(token);
        return -1;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), &oldTp, &returnLength)) {
        CloseHandle(token);
        return -1;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        CloseHandle(token);
        return -1;
    }

    if (wasEnabled && returnLength >= sizeof(TOKEN_PRIVILEGES)) {
        *wasEnabled = (oldTp.PrivilegeCount > 0 &&
                       (oldTp.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)) ? TRUE : FALSE;
    }

    CloseHandle(token);
    return 0;
}

