#ifndef SYSBOOT_H
#define SYSBOOT_H

// ----------- Mode Detection ------------
#if defined(_KERNEL_MODE)
// Kernel-mode headers
#include <ntifs.h>
#include <ntddk.h>
#else
// User-mode headers
#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef _KERNEL_MODE
NTSYSAPI NTSTATUS NTAPI NtOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI NTSTATUS NTAPI NtQuerySymbolicLinkObject(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
);
#endif

// ----------- Type Aliases ------------
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

// ----------- Status Code Guards ------------
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x00000000L
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL 0xC0000001L
#endif

#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY 0xC0000017L
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL 0xC0000023L
#endif

#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED 0xC0000002L
#endif

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER 0xC000000DL
#endif

#ifndef STATUS_NOT_FOUND
#define STATUS_NOT_FOUND 0xC0000225L
#endif

// ----------- Symbolic Link Definitions ------------
#ifndef SYMBOLIC_LINK_QUERY
#define SYMBOLIC_LINK_QUERY 0x0001
#endif

#ifndef OBJ_CASE_INSENSITIVE
#define OBJ_CASE_INSENSITIVE 0x00000040L
#endif

// ----------- Kernel-User Compatibility Layer ------------
#ifdef _KERNEL_MODE
#define LogPrint DbgPrint
#define MemAlloc(size) ExAllocatePoolWithTag(NonPagedPoolNx, size, 'SYSP')
#define MemFree(ptr) ExFreePool(ptr)
#else
#define LogPrint printf
#define MemAlloc(size) malloc(size)
#define MemFree(ptr) free(ptr)
#endif

// ----------- Function Declarations Shared Across Modes ------------
#ifdef __cplusplus
extern "C" {
#endif

// priv.c
long BiAdjustPrivilege(ULONG privilegeId, UCHAR enable, UCHAR *wasEnabled);
long BiOpenEffectiveToken(void **outToken);

// efi.c
long SiGetEfiSystemDevice(int type, ulong arg, wchar_t **outPath);

// bios.c
long SiGetBiosSystemDisk(wchar_t **outPath);
long SiGetBiosSystemPartition(wchar_t **outPath);

// syspart.c
long SiGetSystemDeviceName(void *buf, wchar_t *output, ulong maxSize, ulong *written);
long SiGetSystemPartition(int fwType, wchar_t **outPath);
int  SiGetFirmwareType(void);
long SiTranslateSymbolicLink(wchar_t *path, wchar_t **translated);
long SiGetFirmwareBootDeviceName(int type, wchar_t **outPath);
uchar SiIsWinPEBoot(void);

// utility.c
long SiGetDeviceNumberInformation(wchar_t *devicePath, ulong *diskNum, ulong *partNum);
long SiGetDiskPartitionInformation(wchar_t *path, void *info);
long SiGetDriveLayoutInformation(wchar_t *path, void **layout);
long SiValidateSystemPartition(wchar_t *device, void *info);
long SiIssueSynchronousIoctl(wchar_t *device, ulong code, void *inBuf, ulong inLen, void *outBuf, ulong outLen);

#ifdef __cplusplus
}
#endif

#endif // SYSBOOT_H


