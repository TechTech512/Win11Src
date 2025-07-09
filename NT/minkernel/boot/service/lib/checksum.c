// checksum.c

#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include "bfsvc_types.h"

#pragma comment(lib, "imagehlp.lib")  // For CheckSumMappedFile

void *BfspMapFileForRead(const wchar_t *filePath, ULONG *outSize, _MAPPED_FILE_CONTEXT *context);
void BfspUnmapFile(_MAPPED_FILE_CONTEXT *context);

// Validate PE checksum using CheckSumMappedFile
int __cdecl BfspValidateMappedFileChecksum(const void *baseAddress, ULONG fileSize) {
    ULONG headerSum = 0, checkSum = 0;

    DWORD result = CheckSumMappedFile((LPVOID)baseAddress, fileSize, &headerSum, &checkSum);
    return (result != 0 && headerSum == checkSum);
}

// Validate mapped Boot Manager PE image by scanning memory in steps
int __cdecl BfspValidateMappedBootManagerChecksum(const void *base, ULONG size, ULONG step) {
    const BYTE *ptr = (const BYTE *)base;
    const BYTE *end = ptr + size;
    int result = 0;

    while (ptr + sizeof(IMAGE_DOS_HEADER) < end) {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)ptr;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            break;

        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(ptr + dos->e_lfanew);
        if ((BYTE *)nt >= end)
            break;

        if (nt->Signature == IMAGE_NT_SIGNATURE &&
            nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
            if (BfspValidateMappedFileChecksum(base, size)) {
                result = 1;
                break;
            }
        }

        ptr += step;
    }

    return result;
}

// Validate checksum entry point
int __cdecl BfspValidateChecksum(const wchar_t *filePath, int isBootManager) {
    int result = 0;
    ULONG fileSize = 0;
    void *mappedBase = NULL;
    _MAPPED_FILE_CONTEXT context;

    mappedBase = BfspMapFileForRead(filePath, &fileSize, &context);
    if (mappedBase) {
        if (!isBootManager) {
            result = BfspValidateMappedFileChecksum(mappedBase, fileSize);
        } else {
            result = BfspValidateMappedBootManagerChecksum(mappedBase, fileSize, 8);
            if (result == 0) {
                result = BfspValidateMappedBootManagerChecksum(mappedBase, fileSize, 1);
            }
        }
        BfspUnmapFile(&context);
    }

    return result;
}
