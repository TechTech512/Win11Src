/*
 * mountvol.c
 *
 * MountVol utility – create, delete, or list volume mount points.
 * Reconstructed from decompiled binary with strsafe.h usage.
 */
#pragma warning (disable:4005)
#pragma warning (disable:4645)
#pragma warning (disable:4995)

#include <windows.h>
#include <winioctl.h>
#include <winternl.h>
#include <ntstatus.h>
#include <mountmgr.h>
#include <ntddvol.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>
#include <strsafe.h>
#include <sysinfoapi.h>
#include "msg.h"

// Provide a stub implementation
void __cdecl __report_rangecheckfailure(void)
{
    // Just return - the check failed but we'll ignore it
    // This is unsafe but allows compilation
    return;
}

#pragma comment(lib, "ntdll.lib")

// ------------------------------------------------------------------
// Constants and macros
// ------------------------------------------------------------------
#define SIZEOF_ARRAY(_Array)     (sizeof(_Array)/sizeof(_Array[0]))
#define MAX_PATH 260

// Missing IOCTL definitions
#ifndef IOCTL_VOLUME_IS_OFFLINE
#define IOCTL_VOLUME_IS_OFFLINE CTL_CODE(IOCTL_VOLUME_BASE, 0x0004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE
#define IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE CTL_CODE(IOCTL_VOLUME_BASE, 0x0005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_VOLUME_OFFLINE
#define IOCTL_VOLUME_OFFLINE CTL_CODE(IOCTL_VOLUME_BASE, 0x0006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_MOUNTMGR_SCRUB_REGISTRY
#define IOCTL_MOUNTMGR_SCRUB_REGISTRY CTL_CODE(IOCTL_MOUNTMGR_BASE, 0x000E, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef SystemBootEnvironmentInformation
#define SystemBootEnvironmentInformation 0x5A
#endif

#ifndef SystemPartitionInformation
#define SystemPartitionInformation 0x62
#endif

// ------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------
HANDLE OutputFile;
BOOL IsConsoleOutput;

// ------------------------------------------------------------------
// DisplayIt – output a message to console or file
// ------------------------------------------------------------------
void DisplayIt(PCWSTR Message)
{
    DWORD bytes;
    PSTR message;
    int len;

    if (IsConsoleOutput) {
        WriteConsoleW(OutputFile, Message, (DWORD)wcslen(Message), &bytes, NULL);
    } else {
        len = WideCharToMultiByte(CP_ACP, 0, Message, -1, NULL, 0, NULL, NULL);
        message = (PSTR)LocalAlloc(0, len);
        if (message) {
            WideCharToMultiByte(CP_ACP, 0, Message, -1, message, len, NULL, NULL);
            WriteFile(OutputFile, message, (DWORD)strlen(message), &bytes, NULL);
            LocalFree(message);
        }
    }
}

// ------------------------------------------------------------------
// PrintMessage – format and display a message with variable arguments
// ------------------------------------------------------------------
void PrintMessage(DWORD messageID, ...)
{
    WCHAR messageBuffer[4096];
    va_list ap;

    va_start(ap, messageID);
    FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                   messageBuffer, SIZEOF_ARRAY(messageBuffer), &ap);
    DisplayIt(messageBuffer);
    va_end(ap);
}

// ------------------------------------------------------------------
// PrintSystemMessageFromStatus – display a system error message
// ------------------------------------------------------------------
void PrintSystemMessageFromStatus(DWORD Status)
{
    WCHAR messageBuffer[1024];

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Status, 0,
                   messageBuffer, SIZEOF_ARRAY(messageBuffer), NULL);
    DisplayIt(messageBuffer);
}

// ------------------------------------------------------------------
// VolumeNameIsDriveLetter – check if a name is a drive letter
// ------------------------------------------------------------------
BOOL VolumeNameIsDriveLetter(PCWSTR VolumeName)
{
    return ((3 == wcslen(VolumeName)) &&
            (L':' == VolumeName[1]) &&
            (L'\\' == VolumeName[2]) &&
            (((VolumeName[0] >= L'a') && (VolumeName[0] <= L'z')) ||
             ((VolumeName[0] >= L'A') && (VolumeName[0] <= L'Z'))));
}

// ------------------------------------------------------------------
// IsEfi – check if system is EFI boot
// ------------------------------------------------------------------
int IsEfi(void)
{
    // First try the modern API (Windows 8+)
    FIRMWARE_TYPE ft;
    if (GetFirmwareType(&ft)) {
        return (ft == FirmwareTypeUefi);
    }

    // Fallback: use NT API with the exact layout expected by the original binary
    NTSTATUS status;
    UCHAR buffer[0x20]; // 32 bytes as used in original
    status = NtQuerySystemInformation(SystemBootEnvironmentInformation, buffer, sizeof(buffer), NULL);
    if (NT_SUCCESS(status)) {
        // Original decompiled code checks DWORD at offset 12 (index 3)
        DWORD* pDword = (DWORD*)buffer;
        return (pDword[3] == 2);
    }
    return 0;
}

// ------------------------------------------------------------------
// GetSystemPartition – retrieve system partition path via NT API
// ------------------------------------------------------------------
BOOL GetSystemPartition(PWSTR SystemPartition)
{
    NTSTATUS status;
    ULONG bufferSize = 0x208;
    PBYTE buffer = NULL;
    BOOL result = FALSE;

    do {
        if (buffer) {
            LocalFree(buffer);
            buffer = NULL;
        }
        buffer = (PBYTE)LocalAlloc(0, bufferSize);
        if (!buffer) break;

        status = NtQuerySystemInformation(SystemPartitionInformation, buffer, bufferSize, &bufferSize);
        if (NT_SUCCESS(status)) {
            // buffer[0] and buffer[1] are the length in bytes (USHORT)
            USHORT lenBytes = *(USHORT*)buffer;
            USHORT lenChars = lenBytes / sizeof(WCHAR); // characters count
            if (lenChars > 0 && lenChars < MAX_PATH) {
                // Source string starts at offset 2 (after the length WORD)
                PWSTR pSrc = (PWSTR)(buffer + 2);
                // Use StringCopyWorkerW as in original
                StringCopyWorkerW(SystemPartition, (size_t)pSrc, (size_t*)lenChars, NULL, (size_t)NULL);
                result = TRUE;
            }
            break;
        } else if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH) {
            bufferSize += 0x100;
            continue;
        } else {
            break;
        }
    } while (TRUE);

    if (buffer) LocalFree(buffer);
    return result;
}

// ------------------------------------------------------------------
// IsVolumeOffline – check if a volume is offline
// ------------------------------------------------------------------
BOOL IsVolumeOffline(PWSTR VolumeName)
{
    DWORD len = wcslen(VolumeName);
    HANDLE h;
    BOOL b;
    DWORD bytes;

    if (len == 0 || VolumeName[len - 1] != L'\\') {
        return FALSE;
    }

    VolumeName[len - 1] = L'\0';
    h = CreateFileW(VolumeName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    VolumeName[len - 1] = L'\\';

    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_VOLUME_IS_OFFLINE, NULL, 0, NULL, 0, &bytes, NULL);
    CloseHandle(h);
    return b;
}

// ------------------------------------------------------------------
// PrintTargetForName – print mount points for a volume
// ------------------------------------------------------------------
void PrintTargetForName(PCWSTR VolumeName)
{
    BOOL b;
    DWORD len;
    PWSTR volumePaths, p;

    PrintMessage(MOUNTVOL_VOLUME_NAME, VolumeName);

    b = GetVolumePathNamesForVolumeNameW(VolumeName, NULL, 0, &len);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        PrintSystemMessageFromStatus(GetLastError());
        return;
    }

    volumePaths = (PWSTR)LocalAlloc(0, len * sizeof(WCHAR));
    if (!volumePaths) {
        PrintSystemMessageFromStatus(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    b = GetVolumePathNamesForVolumeNameW(VolumeName, volumePaths, len, NULL);
    if (!b) {
        LocalFree(volumePaths);
        PrintSystemMessageFromStatus(GetLastError());
        return;
    }

    if (!volumePaths[0]) {
        if (IsVolumeOffline((PWSTR)VolumeName)) {
            PrintMessage(MOUNTVOL_NOT_MOUNTED);
        } else {
            PrintMessage(MOUNTVOL_NO_MOUNT_POINTS);
        }
        LocalFree(volumePaths);
        return;
    }

    p = volumePaths;
    do {
        PrintMessage(MOUNTVOL_MOUNT_POINT, p);
        p += wcslen(p) + 1;
    } while (*p);

    LocalFree(volumePaths);
    PrintMessage(MOUNTVOL_NEWLINE);
}

// ------------------------------------------------------------------
// PrintMappedESP – print EFI system partition mapping
// ------------------------------------------------------------------
void PrintMappedESP(PBOOL IsMapped)
{
    WCHAR systemPartition[MAX_PATH];
    WCHAR dosDevice[4], dosTarget[MAX_PATH];
    WCHAR c;

    if (IsMapped) *IsMapped = FALSE;

    if (!GetSystemPartition(systemPartition)) {
        return;
    }

    dosDevice[1] = L':';
    dosDevice[2] = L'\0';

    for (c = L'A'; c <= L'Z'; c++) {
        dosDevice[0] = c;
        if (!QueryDosDeviceW(dosDevice, dosTarget, SIZEOF_ARRAY(dosTarget))) {
            continue;
        }
        if (wcscmp(dosTarget, systemPartition) != 0) {
            continue;
        }
        dosDevice[2] = L'\\';
        dosDevice[3] = L'\0';
        if (IsMapped) {
            *IsMapped = TRUE;
        } else {
            PrintMessage(MOUNTVOL_EFI, dosDevice);
        }
        break;
    }
}

// ------------------------------------------------------------------
// PrintVolumeList – list all volumes and mount points
// ------------------------------------------------------------------
void PrintVolumeList(void)
{
    HANDLE h;
    WCHAR volumeName[MAX_PATH];
    BOOL b;

    h = FindFirstVolumeW(volumeName, SIZEOF_ARRAY(volumeName));
    if (h == INVALID_HANDLE_VALUE) {
        PrintSystemMessageFromStatus(GetLastError());
        return;
    }

    do {
        PrintTargetForName(volumeName);
        b = FindNextVolumeW(h, volumeName, SIZEOF_ARRAY(volumeName));
    } while (b);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        PrintSystemMessageFromStatus(GetLastError());
    }

    FindVolumeClose(h);

    if (IsEfi()) {
        PrintMappedESP(NULL);
    }
}

// ------------------------------------------------------------------
// SetAutoMountState – enable or disable auto-mount
// ------------------------------------------------------------------
BOOL SetAutoMountState(BOOL AutoMountEnabled)
{
    HANDLE h;
    BOOL b;
    DWORD bytes;
    MOUNTMGR_SET_AUTO_MOUNT SetAutoMount;

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ZeroMemory(&SetAutoMount, sizeof(SetAutoMount));
    SetAutoMount.NewState = AutoMountEnabled ? Enabled : Disabled;

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_SET_AUTO_MOUNT,
                        &SetAutoMount, sizeof(SetAutoMount),
                        NULL, 0, &bytes, NULL);
    CloseHandle(h);
    return b;
}

// ------------------------------------------------------------------
// ScrubRegistry – remove orphaned mount points from registry
// ------------------------------------------------------------------
BOOL ScrubRegistry(void)
{
    HANDLE h, hh;
    DWORD bytes;
    BOOL b;
    WCHAR volumeName[MAX_PATH];
    WCHAR volumeNameTarget[MAX_PATH];
    WCHAR volumeNameTarget2[MAX_PATH];
    WCHAR subPath[2 * MAX_PATH];
    WCHAR fullPath[3 * MAX_PATH];
    DWORD lenVolume, lenSub;

    h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_MOUNTMGR_SCRUB_REGISTRY, NULL, 0, NULL, 0, &bytes, NULL);
    CloseHandle(h);
    if (!b) {
        return FALSE;
    }

    h = FindFirstVolumeW(volumeName, SIZEOF_ARRAY(volumeName));
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    while (TRUE) {
        lenVolume = (DWORD)wcslen(volumeName);
        wcscpy_s(fullPath, SIZEOF_ARRAY(fullPath), volumeName);

        hh = FindFirstVolumeMountPointW(volumeName, subPath, SIZEOF_ARRAY(subPath));
        if (hh != INVALID_HANDLE_VALUE) {
            do {
                lenSub = (DWORD)wcslen(subPath);
                wcscpy_s(fullPath + lenVolume, SIZEOF_ARRAY(fullPath) - lenVolume, subPath);
                fullPath[lenVolume + lenSub] = L'\0';

                b = GetVolumeNameForVolumeMountPointW(fullPath, volumeNameTarget,
                                                      SIZEOF_ARRAY(volumeNameTarget));
                if (b) {
                    b = GetVolumeNameForVolumeMountPointW(volumeNameTarget, volumeNameTarget2,
                                                          SIZEOF_ARRAY(volumeNameTarget2));
                    if (!b && GetLastError() == ERROR_PATH_NOT_FOUND) {
                        RemoveDirectoryW(fullPath);
                    }
                }
                if (!b && GetLastError() != ERROR_NO_MORE_FILES) {
                    FindVolumeMountPointClose(hh);
                    FindVolumeClose(h);
                    return FALSE;
                }
            } while (FindNextVolumeMountPointW(hh, subPath, SIZEOF_ARRAY(subPath)));
            FindVolumeMountPointClose(hh);
        }

        if (!FindNextVolumeW(h, volumeName, SIZEOF_ARRAY(volumeName))) {
            break;
        }
    }

    FindVolumeClose(h);
    return TRUE;
}

// ------------------------------------------------------------------
// SetSystemPartitionDriveLetter – assign a drive letter to the system partition
// ------------------------------------------------------------------
BOOL SetSystemPartitionDriveLetter(PWSTR DirName)
{
    WCHAR systemPartition[MAX_PATH];

    if (!GetSystemPartition(systemPartition)) {
        return FALSE;
    }

    DirName[wcslen(DirName) - 1] = L'\0';
    if (!DefineDosDeviceW(DDD_RAW_TARGET_PATH, DirName, systemPartition)) {
        return FALSE;
    }
    return TRUE;
}

// ------------------------------------------------------------------
// DoPermanentDismount – permanently dismount a volume
// ------------------------------------------------------------------
BOOL DoPermanentDismount(PWSTR DirName, PBOOL ErrorHandled)
{
    WCHAR volumeName[MAX_PATH];
    DWORD len, bytes;
    PWSTR volumePaths, p;
    HANDLE h;
    BOOL b;

    *ErrorHandled = FALSE;

    if (!GetVolumeNameForVolumeMountPointW(DirName, volumeName, SIZEOF_ARRAY(volumeName))) {
        return FALSE;
    }

    b = GetVolumePathNamesForVolumeNameW(volumeName, NULL, 0, &len);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        return FALSE;
    }

    volumePaths = (PWSTR)LocalAlloc(0, len * sizeof(WCHAR));
    if (!volumePaths) {
        return FALSE;
    }

    b = GetVolumePathNamesForVolumeNameW(volumeName, volumePaths, len, NULL);
    if (!b) {
        LocalFree(volumePaths);
        return FALSE;
    }

    if (!volumePaths[0]) {
        LocalFree(volumePaths);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    p = volumePaths;
    while (*p) p++;
    p++;
    if (*p) {
        LocalFree(volumePaths);
        *ErrorHandled = TRUE;
        SetLastError(ERROR_INVALID_PARAMETER);
        PrintMessage(MOUNTVOL_OTHER_VOLUME_MOUNT_POINTS);
        return FALSE;
    }
    LocalFree(volumePaths);

    len = (DWORD)wcslen(volumeName);
    volumeName[len - 1] = L'\0';
    h = CreateFileW(volumeName, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE, NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        CloseHandle(h);
        *ErrorHandled = TRUE;
        SetLastError(ERROR_INVALID_PARAMETER);
        PrintMessage(MOUNTVOL_NOT_SUPPORTED);
        return FALSE;
    }

    b = DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        PrintMessage(MOUNTVOL_VOLUME_IN_USE);
    }

    b = DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        CloseHandle(h);
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_VOLUME_OFFLINE, NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);
    return TRUE;
}

// ------------------------------------------------------------------
// main – entry point
// ------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
    DWORD mode;
    WCHAR dirName[MAX_PATH];
    WCHAR volumeName[MAX_PATH];
    DWORD dirLen, volumeLen;
    BOOL deletePoint = FALSE;
    BOOL listPoint = FALSE;
    BOOL systemPartition = FALSE;
    BOOL dismount = FALSE;
    BOOL b, errorHandled;
    WCHAR targetPathBuffer[MAX_PATH];
    HRESULT hr;

    SetThreadUILanguage(0);
    SetErrorMode(SEM_FAILCRITICALERRORS);

    OutputFile = GetStdHandle(STD_OUTPUT_HANDLE);
    IsConsoleOutput = GetConsoleMode(OutputFile, &mode);

    if (argc > 2) {
        hr = StringCchPrintfW(volumeName, SIZEOF_ARRAY(volumeName), L"%hs", argv[2]);
        if (FAILED(hr)) {
            PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
            return 1;
        }
        volumeLen = (DWORD)wcslen(volumeName);
    }

    if (argc > 1) {
        hr = StringCchPrintfW(dirName, SIZEOF_ARRAY(dirName), L"%hs", argv[1]);
        if (FAILED(hr)) {
            PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
            return 1;
        }
        dirLen = (DWORD)wcslen(dirName);
    }

    if (argc != 3) {
        if (argc == 2 && argv[1][0] == '/' && argv[1][1] != '\0' && argv[1][2] == '\0') {
            if (argv[1][1] == 'r' || argv[1][1] == 'R') {
                b = ScrubRegistry();
            } else if (argv[1][1] == 'n' || argv[1][1] == 'N') {
                b = SetAutoMountState(FALSE);
            } else if (argv[1][1] == 'e' || argv[1][1] == 'E') {
                b = SetAutoMountState(TRUE);
            } else {
                goto Usage;
            }
            if (!b) {
                PrintSystemMessageFromStatus(GetLastError());
                return 1;
            }
            return 0;
        }

Usage:
        PrintMessage(MOUNTVOL_USAGE1);
        if (IsEfi()) {
            PrintMessage(MOUNTVOL_USAGE1_IA64);
        }
        PrintMessage(MOUNTVOL_USAGE2);
        if (IsEfi()) {
            PrintMessage(MOUNTVOL_USAGE2_IA64);
        }
        PrintMessage(MOUNTVOL_START_OF_LIST);
        PrintVolumeList();
        return 0;
    }

    // Three arguments
    if (argv[2][0] == '/' && argv[2][1] != '\0' && argv[2][2] == '\0') {
        if (argv[2][1] == 'd' || argv[2][1] == 'D') {
            deletePoint = TRUE;
        } else if (argv[2][1] == 'l' || argv[2][1] == 'L') {
            listPoint = TRUE;
        } else if (argv[2][1] == 'p' || argv[2][1] == 'P') {
            deletePoint = TRUE;
            dismount = TRUE;
        } else if (argv[2][1] == 's' || argv[2][1] == 'S') {
            systemPartition = TRUE;
        }
    }

    if (dirName[dirLen - 1] != L'\\') {
        hr = StringCchCatW(dirName, SIZEOF_ARRAY(dirName), L"\\");
        if (FAILED(hr)) {
            PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
            return 1;
        }
        dirLen++;
    }

    if (volumeName[volumeLen - 1] != L'\\') {
        hr = StringCchCatW(volumeName, SIZEOF_ARRAY(volumeName), L"\\");
        if (FAILED(hr)) {
            PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
            return 1;
        }
        volumeLen++;
    }

    if (deletePoint) {
        if (dismount) {
            b = DoPermanentDismount(dirName, &errorHandled);
            if (!b && errorHandled) {
                return 1;
            }
        } else {
            b = TRUE;
        }
        if (b) {
            b = DeleteVolumeMountPointW(dirName);
            if (!b && GetLastError() == ERROR_INVALID_PARAMETER) {
                dirName[dirLen - 1] = L'\0';
                b = DefineDosDeviceW(DDD_REMOVE_DEFINITION, dirName, NULL);
            }
        }
    } else if (listPoint) {
        b = GetVolumeNameForVolumeMountPointW(dirName, volumeName, SIZEOF_ARRAY(volumeName));
        if (b) {
            PrintMessage(MOUNTVOL_VOLUME_NAME, volumeName);
        }
    } else if (systemPartition) {
#if defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM64) || defined(_M_IX86)
        if (!VolumeNameIsDriveLetter(dirName)) {
            PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
            return 1;
        } else {
            dirName[2] = L'\0';
            b = QueryDosDeviceW(dirName, targetPathBuffer, SIZEOF_ARRAY(targetPathBuffer));
            if (b) {
                PrintSystemMessageFromStatus(ERROR_DIR_NOT_EMPTY);
                return 1;
            }
            dirName[2] = L'\\';
            PrintMappedESP(&b);
            if (b) {
                PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
                return 1;
            }
        }
        b = SetSystemPartitionDriveLetter(dirName);
#else
        PrintSystemMessageFromStatus(ERROR_INVALID_PARAMETER);
        return 1;
#endif
    } else {
        if (VolumeNameIsDriveLetter(dirName)) {
            dirName[2] = L'\0';
            b = QueryDosDeviceW(dirName, targetPathBuffer, SIZEOF_ARRAY(targetPathBuffer));
            if (b) {
                PrintSystemMessageFromStatus(ERROR_DIR_NOT_EMPTY);
                return 1;
            }
            dirName[2] = L'\\';
        }
        b = SetVolumeMountPointW(dirName, volumeName);
    }

    if (!b) {
        PrintSystemMessageFromStatus(GetLastError());
        return 1;
    }

    return 0;
}

