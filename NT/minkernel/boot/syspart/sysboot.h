#ifndef SYSBOOT_H
#define SYSBOOT_H

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winternl.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

// typedef enum {
//     FirmwareTypeUnknown = 0,
//     FirmwareTypeBios,
//     FirmwareTypeUefi
// } FIRMWARE_TYPE;

typedef enum {
    SyspartDeviceTypeUnknown = 0
} SYSPART_DEVICE_TYPE;

typedef struct {
    int dummy;
} DRIVE_LAYOUT_INFORMATION_EX;

typedef struct {
    int dummy;
} PARTITION_INFORMATION_EX;

typedef struct {
    int dummy;
} BCD_PRIVILEGE_STATE;

#define STATUS_SUCCESS           0x00000000L
#define STATUS_UNSUCCESSFUL      0xC0000001L
#define STATUS_BUFFER_TOO_SMALL  0xC0000023L
#define STATUS_INVALID_PARAMETER 0xC000000DL
#define STATUS_NOT_FOUND         0xC0000225L
#define STATUS_NOT_IMPLEMENTED   0xC0000002L
#define STATUS_NO_MEMORY         0xC0000017L

#endif // SYSBOOT_H

