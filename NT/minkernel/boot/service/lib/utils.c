// utils.c

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "bfsvc_types.h"

// Simulated internal system partition cache
static wchar_t *BfspSystemPartitionName = NULL;

// Enables SeDebugPrivilege
int __cdecl BfspAdjustDebugPrivilege() {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return 0;

    if (!LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(token);
        return 0;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(token);

    return GetLastError() == ERROR_SUCCESS;
}

// Maps a file read-only into memory
void *BfspMapFileForRead(const wchar_t *filePath, ULONG *outSize, _MAPPED_FILE_CONTEXT *ctx) {
    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        fwprintf(stderr, L"Failed to open file '%s' (Error: %lu)\n", filePath, err);
        return NULL;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size) || size.HighPart != 0) {
        fwprintf(stderr, L"File too large or failed to get size for '%s'\n", filePath);
        CloseHandle(hFile);
        return NULL;
    }

    HANDLE hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) {
        fwprintf(stderr, L"CreateFileMapping failed for '%s'\n", filePath);
        CloseHandle(hFile);
        return NULL;
    }

    void *base = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!base) {
        fwprintf(stderr, L"MapViewOfFile failed for '%s'\n", filePath);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    if (ctx) {
        ctx->hFile = hFile;
        ctx->hMapping = hMap;
        ctx->lpBaseAddress = base;
    }

    if (outSize) *outSize = size.LowPart;
    return base;
}

// Unmaps a previously mapped file
void __cdecl BfspUnmapFile(_MAPPED_FILE_CONTEXT *ctx) {
    if (!ctx) return;

    if (ctx->lpBaseAddress) UnmapViewOfFile(ctx->lpBaseAddress);
    if (ctx->hMapping) CloseHandle(ctx->hMapping);
    if (ctx->hFile) CloseHandle(ctx->hFile);

    ctx->lpBaseAddress = NULL;
    ctx->hMapping = NULL;
    ctx->hFile = NULL;
}

// Simulates getting the system partition
wchar_t * __cdecl BfspGetSystemPartition() {
    if (!BfspSystemPartitionName) {
        // Simulate system partition resolution
        BfspSystemPartitionName = _wcsdup(L"\\Device\\HarddiskVolume1\\Windows");
        if (!BfspSystemPartitionName) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }

    return _wcsdup(BfspSystemPartitionName);  // Caller must free
}

// Simulates retrieving thread memory priority
ULONG __cdecl BfspGetThreadPagePriority(void) {
    // Windows doesn’t provide direct page priority query in user mode
    // This is a placeholder returning a reasonable default
    return 5;
}

// Simulates setting thread page priority
long __cdecl BfspSetThreadPagePriority(ULONG priority) {
    // No public API to set thread page priority; return success
    return 0;
}

// Logs which process might be locking the file (best-effort simulation)
void __cdecl BfspPrintOwnerProcessOnSharingError(const wchar_t *filePath, DWORD errorCode) {
    if (errorCode == ERROR_SHARING_VIOLATION || errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_LOCK_VIOLATION) {
        fwprintf(stderr, L"File sharing error: '%s' may be in use (Error: %lu)\n", filePath, errorCode);
    }
}

