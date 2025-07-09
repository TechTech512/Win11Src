// security.c

#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <tchar.h>
#include <stdint.h>
#include <Aclapi.h>

// Assume these helpers are implemented elsewhere
extern void BfspLogMessage(int level, const wchar_t* format, ...);
extern void BfspPrintOwnerProcessOnSharingError(const wchar_t* filePath, ULONG param);

// Logging levels (placeholder)
#define BfLogError 1
#define BfLogInformation 2

//
// Forward declarations of local functions
//
int BfspGetUserToken(HANDLE* tokenOut);
int BfspGetUserSidString(wchar_t** sidStringOut);
int BfspCreateTokenPrivilegesInformation(wchar_t** privileges, ULONG count, int setEnabled, TOKEN_PRIVILEGES** output);
int BfspAdjustTokenPrivileges(TOKEN_PRIVILEGES* privileges, TOKEN_PRIVILEGES** previousState);
int BfspSetFileDirectorySecurityDescriptor(const wchar_t* formatStr, int useSystemSid, const wchar_t* path);
int BfspSetSecurityDescriptor(const wchar_t* filePath, const wchar_t* sddlString);

//
// Get user token handle.
//
int BfspGetUserToken(HANDLE* tokenOut) {
    HANDLE token = NULL;

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, TRUE, &token)) {
        *tokenOut = token;
        return 1;
    }

    if (GetLastError() == ERROR_NO_TOKEN) {
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token)) {
            *tokenOut = token;
            return 1;
        }
    }

    return 0;
}

//
// Adjust token privileges.
//
int BfspAdjustTokenPrivileges(TOKEN_PRIVILEGES* privileges, TOKEN_PRIVILEGES** previousState) {
    HANDLE token = NULL;
    DWORD bufferSize = 0;
    TOKEN_PRIVILEGES* buffer = NULL;
    DWORD lastError = 0;
    int result = 0;

    if (!BfspGetUserToken(&token)) {
        lastError = GetLastError();
        BfspLogMessage(BfLogError, L"Failed to get user token! Error code = %#x", lastError);
        SetLastError(lastError);
        return 0;
    }

    if (previousState != NULL) {
        bufferSize = sizeof(DWORD) + privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
        buffer = (TOKEN_PRIVILEGES*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
    }

    if (AdjustTokenPrivileges(token, FALSE, privileges, bufferSize, buffer, &bufferSize)) {
        if (previousState != NULL) {
            *previousState = buffer;
        }
        result = 1;
    } else {
        lastError = GetLastError();
        if (lastError == ERROR_INSUFFICIENT_BUFFER && buffer != NULL) {
            HeapFree(GetProcessHeap(), 0, buffer);
            buffer = (TOKEN_PRIVILEGES*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
            if (buffer) {
                if (AdjustTokenPrivileges(token, FALSE, privileges, bufferSize, buffer, &bufferSize)) {
                    if (previousState != NULL) {
                        *previousState = buffer;
                    }
                    result = 1;
                } else {
                    lastError = GetLastError();
                }
            }
        } else {
            BfspLogMessage(BfLogError, L"Failed to adjust token privileges! Error code = %#x", lastError);
        }
    }

    if (!result && buffer != NULL) {
        HeapFree(GetProcessHeap(), 0, buffer);
    }

    if (token) CloseHandle(token);
    if (!result) SetLastError(lastError);

    return result;
}

//
// Create a TOKEN_PRIVILEGES structure.
//
int BfspCreateTokenPrivilegesInformation(wchar_t** privilegeNames, ULONG count, int setEnabled, TOKEN_PRIVILEGES** outPrivileges) {
    TOKEN_PRIVILEGES* privileges = NULL;
    DWORD allocSize = sizeof(DWORD) + count * sizeof(LUID_AND_ATTRIBUTES);
    DWORD lastError = 0;

    privileges = (TOKEN_PRIVILEGES*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, allocSize);
    if (!privileges) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    privileges->PrivilegeCount = count;

    for (ULONG i = 0; i < count; ++i) {
        if (!LookupPrivilegeValueW(NULL, privilegeNames[i], &privileges->Privileges[i].Luid)) {
            lastError = GetLastError();
            BfspLogMessage(BfLogError, L"Failed to lookup privilege %s! Error code = %#x", privilegeNames[i], lastError);
            HeapFree(GetProcessHeap(), 0, privileges);
            SetLastError(lastError);
            return 0;
        }

        privileges->Privileges[i].Attributes = setEnabled ? SE_PRIVILEGE_ENABLED : 0;
    }

    *outPrivileges = privileges;
    return 1;
}

//
// Get current user SID string (e.g., "S-1-5-21-...").
//
int BfspGetUserSidString(wchar_t** sidStringOut) {
    HANDLE token = NULL;
    DWORD size = 0;
    PTOKEN_USER tokenUser = NULL;
    int result = 0;
    DWORD lastError = 0;

    if (!BfspGetUserToken(&token)) {
        lastError = GetLastError();
        BfspLogMessage(BfLogError, L"Failed to get user token! Error code = %#x", lastError);
        SetLastError(lastError);
        return 0;
    }

    GetTokenInformation(token, TokenUser, NULL, 0, &size);
    tokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!tokenUser) {
        CloseHandle(token);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    if (GetTokenInformation(token, TokenUser, tokenUser, size, &size)) {
        if (ConvertSidToStringSidW(tokenUser->User.Sid, sidStringOut)) {
            result = 1;
        } else {
            lastError = GetLastError();
            BfspLogMessage(BfLogError, L"Failed to convert user SID! Error code = %#x", lastError);
        }
    } else {
        lastError = GetLastError();
        BfspLogMessage(BfLogError, L"Failed to get token information! Error code = %#x", lastError);
    }

    HeapFree(GetProcessHeap(), 0, tokenUser);
    CloseHandle(token);

    if (!result) SetLastError(lastError);
    return result;
}

//
// Set security descriptor on a file/directory
//
int BfspSetFileDirectorySecurityDescriptor(const wchar_t* formatString, int useSystemSid, const wchar_t* targetPath) {
    wchar_t sddl[512] = {0};
    const wchar_t* sid = NULL;
    wchar_t* userSid = NULL;
    int result = 0;
    DWORD lastError = 0;

    if (useSystemSid) {
        // Use hardcoded SID
        sid = L"S-1-5-80-956008885-3418522649-1831038044-1853292631-2271478464";
    } else {
        if (!BfspGetUserSidString(&userSid)) {
            lastError = GetLastError();
            goto cleanup;
        }
        sid = userSid;
    }

    swprintf_s(sddl, 512, formatString, sid, sid, sid);

    if (!BfspSetSecurityDescriptor(targetPath, sddl)) {
        lastError = GetLastError();
        if (lastError != ERROR_FILE_NOT_FOUND) {
            BfspPrintOwnerProcessOnSharingError(targetPath, 8);
            BfspLogMessage(BfLogError, L"BfspSetSecurityDescriptor(%s) failed! Last Error = %#x", targetPath, lastError);
        }
        goto cleanup;
    }

    result = 1;

cleanup:
    if (userSid) LocalFree(userSid);
    if (!result) SetLastError(lastError);
    return result;
}

//
// Apply the given SDDL string to a file or directory.
//
int BfspSetSecurityDescriptor(const wchar_t* filePath, const wchar_t* sddlString) {
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    DWORD result = 0;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddlString, SDDL_REVISION_1, &securityDescriptor, NULL)) {
        result = GetLastError();
        BfspLogMessage(BfLogError, L"ConvertStringSecurityDescriptorToSecurityDescriptorW failed! Error = %#x", result);
        SetLastError(result);
        return 0;
    }

    result = SetNamedSecurityInfoW((LPWSTR)filePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL);

    LocalFree(securityDescriptor);

    if (result != ERROR_SUCCESS) {
        BfspLogMessage(BfLogError, L"SetNamedSecurityInfoW failed! Error = %#x", result);
        SetLastError(result);
        return 0;
    }

    return 1;
}

