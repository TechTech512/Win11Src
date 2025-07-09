#include <windows.h>
#include <stdint.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <stdio.h>

// External helpers assumed to exist
extern wchar_t* BfspEnvExpandString(void* envList, const wchar_t* input);
extern int BfspSetFileDirectorySecurityDescriptor(const wchar_t* formatStr, int useSystemSid, const wchar_t* targetPath);
extern uint64_t BfspGetExecutableVersion(const wchar_t* filePath, int isBootManager);
extern void BfspLogMessage(int level, const wchar_t* format, ...);
extern int BfspFileExists(const wchar_t* filePath);
extern ULONGLONG BfspGetExecutableVersion(const wchar_t* filePath, int isBootManager);

#define BfLogError 1
#define BfLogInformation 2

//
// Check if a file exists.
//
int BfspFileExists(const wchar_t* path) {
    DWORD attributes = GetFileAttributesW(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

//
// Copy the contents of one boot file directory to another.
//
int BfspCopyBootFileDirectory(const wchar_t* srcDir, const wchar_t* dstDir) {
    wchar_t searchPath[MAX_PATH];
    WIN32_FIND_DATAW findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t srcFile[MAX_PATH], dstFile[MAX_PATH];

    if (!srcDir || !dstDir) return 0;

    // Construct search pattern
    StringCchPrintfW(searchPath, MAX_PATH, L"%s\\*", srcDir);

    hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            StringCchPrintfW(srcFile, MAX_PATH, L"%s\\%s", srcDir, findData.cFileName);
            StringCchPrintfW(dstFile, MAX_PATH, L"%s\\%s", dstDir, findData.cFileName);

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                CreateDirectoryW(dstFile, NULL);
                BfspCopyBootFileDirectory(srcFile, dstFile);
            } else {
                CopyFileW(srcFile, dstFile, FALSE);
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return 1;
}

//
// Copy service files based on environment variables.
//
int BfspCopyServiceFiles(void* envList) {
    static const wchar_t* srcKey = L"BF_SOURCE_DIRECTORY";
    static const wchar_t* dstKey = L"BF_SERVICE_DIRECTORY";
    wchar_t* srcDir = BfspEnvExpandString(envList, srcKey);
    wchar_t* dstDir = BfspEnvExpandString(envList, dstKey);
    int result = 0;

    if (!srcDir || !dstDir) {
        BfspLogMessage(BfLogError, L"Failed to expand source or destination directory.");
        goto cleanup;
    }

    if (!CreateDirectoryW(dstDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        BfspLogMessage(BfLogError, L"Failed to create directory: %s", dstDir);
        goto cleanup;
    }

    if (!BfspCopyBootFileDirectory(srcDir, dstDir)) {
        BfspLogMessage(BfLogError, L"Failed to copy files from %s to %s", srcDir, dstDir);
        goto cleanup;
    }

    // Set security
    const wchar_t* sddlFmt = L"O:%sG:%sD:P(A;;FA;;;%s)(A;;FA;;;SY)";
    if (!BfspSetFileDirectorySecurityDescriptor(sddlFmt, 0, dstDir)) {
        BfspLogMessage(BfLogError, L"Failed to apply security to %s", dstDir);
        goto cleanup;
    }

    result = 1;

cleanup:
    if (srcDir) HeapFree(GetProcessHeap(), 0, srcDir);
    if (dstDir) HeapFree(GetProcessHeap(), 0, dstDir);
    return result;
}

//
// Copy and version-check debugger files.
//
int BfspServiceDebuggerFiles(void* envList) {
    static const wchar_t* srcKey = L"BF_DEBUGGER_SOURCE_PATH";
    static const wchar_t* dstKey = L"BF_DEBUGGER_TARGET_PATH";
    static const wchar_t* binName = L"kd.exe";

    wchar_t* srcPath = BfspEnvExpandString(envList, srcKey);
    wchar_t* dstPath = BfspEnvExpandString(envList, dstKey);
    int result = 0;

    if (!srcPath || !dstPath) {
        BfspLogMessage(BfLogError, L"Failed to expand debugger paths.");
        goto cleanup;
    }

    wchar_t srcFile[MAX_PATH], dstFile[MAX_PATH];
    StringCchPrintfW(srcFile, MAX_PATH, L"%s\\%s", srcPath, binName);
    StringCchPrintfW(dstFile, MAX_PATH, L"%s\\%s", dstPath, binName);

    if (!BfspFileExists(srcFile)) {
        BfspLogMessage(BfLogError, L"Source debugger file missing: %s", srcFile);
        goto cleanup;
    }

    ULONGLONG srcVer = BfspGetExecutableVersion(srcFile, 0);
    ULONGLONG dstVer = BfspFileExists(dstFile) ? BfspGetExecutableVersion(dstFile, 0) : 0;

    if (!BfspFileExists(dstFile) || srcVer > dstVer) {
        CreateDirectoryW(dstPath, NULL);
        if (!CopyFileW(srcFile, dstFile, FALSE)) {
            BfspLogMessage(BfLogError, L"Failed to copy debugger: %s to %s", srcFile, dstFile);
            goto cleanup;
        }
    }

    result = 1;

cleanup:
    if (srcPath) HeapFree(GetProcessHeap(), 0, srcPath);
    if (dstPath) HeapFree(GetProcessHeap(), 0, dstPath);
    return result;
}

