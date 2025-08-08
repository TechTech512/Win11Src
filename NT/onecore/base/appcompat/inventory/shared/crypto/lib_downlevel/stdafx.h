#pragma once

// Standard Windows headers
#include <windows.h>
#include <wincrypt.h>
#include <strsafe.h>
#include <wchar.h>
#include <ctype.h>
#include <stdint.h>

// Common type definitions
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;

// WPP (Windows Software Trace Preprocessor) related definitions
typedef struct _WPP_PROJECT_CONTROL_BLOCK {
    // Add proper WPP structure members if needed
    // Placeholder implementation:
    struct {
        void* Logger;
        struct {
            void* Win2kCb;
            void* field2_0x10;
        } Control;
        UCHAR ReserveSpace[32];
    };
} WPP_PROJECT_CONTROL_BLOCK;

extern WPP_PROJECT_CONTROL_BLOCK WPP_GLOBAL_Control;

// Structure definitions
#ifndef _IMAGE_NT_HEADERS_DEFINED
#define _IMAGE_NT_HEADERS_DEFINED
#endif

typedef struct _AEID001_INFO {
    // Structure for hash information
    BYTE data[20]; // Assuming SHA-1 hash size
} AEID001_INFO;

typedef struct _PROGRAM_IDENTITY_INFO {
    // Structure for program identity information
    wchar_t* name;
    wchar_t* publisher;
    wchar_t* version;
    wchar_t* language;
} PROGRAM_IDENTITY_INFO;

// Function prototypes
uint AeComputePeHeaderHash(IMAGE_NT_HEADERS* ntHeaders, ULONGLONG param2, AEID001_INFO* hashInfo);
uint AeComputeProgramIdentityHash(PROGRAM_IDENTITY_INFO* identityInfo, wchar_t* output);
uint AeComputeStringHash(wchar_t* input, wchar_t* output);
wchar_t* AeTrimString(wchar_t* str);
uint FormatNPVString(wchar_t* p1, wchar_t* p2, wchar_t* p3, wchar_t* p4, ULONG param5, ULONG* param6);
uint FormatAeHashString(AEID001_INFO* hashInfo, wchar_t* output);
uint ComputeMd5Hash(uchar* input, ulong inputSize, uchar* output);
uint ComputeSha1Hash(uchar* input, ulong inputSize, uchar* output);
ULONG PageErrorExceptionHandler(PEXCEPTION_POINTERS exceptionInfo, char* param2, ULONG param3);
DWORD StringToDword(wchar_t* str, int param2, ULONG* output);

// Helper macros for error handling
#define HRESULT_FROM_WIN32(x) ((x) <= 0 ? ((DWORD)(x)) : (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000))

