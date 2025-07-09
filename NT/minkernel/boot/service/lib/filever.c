// filever.c

#include <windows.h>
#include <winver.h>
#include <wchar.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

//
// External helpers assumed to be defined elsewhere.
//
typedef struct _MAPPED_FILE_CONTEXT _MAPPED_FILE_CONTEXT;

extern void* BfspMapFileForRead(const wchar_t* filePath, unsigned long* fileSize, _MAPPED_FILE_CONTEXT* contextOut);
extern void BfspUnmapFile(_MAPPED_FILE_CONTEXT* context);
extern int LdrFindResource_U(uintptr_t module, void* resourceTypePath, ULONG level, void** resourceData);
extern int LdrAccessResource(uintptr_t module, void* resource, void** data, unsigned long* size);

//
// Forward declarations of functions defined later in this file
//
uint64_t __cdecl BfspGetMappedFileVersion(void* baseAddress);
uint64_t __cdecl BfspGetMappedBootManagerVersion(void* mappedAddress, unsigned long fileSize, uintptr_t fallbackArg);

//
// Extracts the version info from a file.
//
uint64_t __cdecl BfspGetExecutableVersion(wchar_t* filePath, int isBootManager) {
    void* mappedAddress = NULL;
    unsigned long fileSize = 0;
    uint64_t version = 0;
    _MAPPED_FILE_CONTEXT* mapContext = NULL;

    mappedAddress = BfspMapFileForRead(filePath, &fileSize, mapContext);
    if (mappedAddress == NULL)
        return 0;

    if (isBootManager == 0) {
        version = BfspGetMappedFileVersion(mappedAddress);
    } else {
        version = BfspGetMappedBootManagerVersion(mappedAddress, fileSize, (uintptr_t)filePath);
        if (version == 0) {
            version = BfspGetMappedBootManagerVersion((void*)1, 8, (uintptr_t)filePath);
        }
    }

    BfspUnmapFile(mapContext);
    return version;
}

//
// Tries to extract the version by searching through possible boot manager headers.
//
uint64_t __cdecl BfspGetMappedBootManagerVersion(void* mappedAddress, unsigned long fileSize, uintptr_t fallbackArg) {
    const uint16_t* base = (const uint16_t*)mappedAddress;
    const uint16_t mzMagic = 0x5A4D;
    const uint32_t peMagic = 0x00004550;
    uint64_t version = 0;

    if (base == NULL || fileSize < 0x1000)
        return 0;

    uintptr_t offset = 0;
    while (offset < fileSize - 0x1000) {
        const uint16_t* candidate = (const uint16_t*)((uintptr_t)base + offset);
        if (candidate[0] == mzMagic) {
            const uint32_t e_lfanew = *(const uint32_t*)((uintptr_t)candidate + 0x3C);
            const uint32_t* peHeader = (const uint32_t*)((uintptr_t)candidate + e_lfanew);
            if (peHeader[0] == peMagic) {
                const uint16_t machine = *(const uint16_t*)((uintptr_t)peHeader + 4);
                if (machine == 0x014c) { // IMAGE_FILE_MACHINE_I386
                    version = BfspGetMappedFileVersion(mappedAddress);
                    if (version != 0)
                        break;
                }
            }
        }
        offset += 0x200; // increment conservatively
    }

    return version;
}

//
// Extracts the file version from a mapped resource.
//
uint64_t __cdecl BfspGetMappedFileVersion(void* baseAddress) {
    uintptr_t module = (uintptr_t)baseAddress;
    void* resourceHandle = NULL;
    void* resourceData = NULL;
    unsigned long resourceSize = 0;
    uint64_t version = 0;

    struct {
        ULONG type;
        ULONG name;
        ULONG lang;
    } resourcePath = { 0x10, 1, 0 }; // RT_VERSION, ID=1, Lang=0

    if (LdrFindResource_U(module, &resourcePath, 3, &resourceHandle) >= 0) {
        if (LdrAccessResource(module, resourceHandle, &resourceData, &resourceSize) >= 0 &&
            resourceSize > sizeof(VS_FIXEDFILEINFO)) {

            VS_FIXEDFILEINFO* info = (VS_FIXEDFILEINFO*)((BYTE*)resourceData + 6);
            if (_wcsicmp((wchar_t*)info, L"VS_VERSION_INFO") == 0) {
                version = ((uint64_t)info->dwFileVersionMS << 32) | info->dwFileVersionLS;
            }
        }
    }

    return version;
}

//
// Determines if the given file is "bootmgr.exe".
//
int __cdecl BfspIsWindowsBootManager(wchar_t* filePath, int firmwareType) {
    static HINSTANCE versionDll = NULL;
    static BOOL (WINAPI *pfnGetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID) = NULL;
    static BOOL (WINAPI *pfnVerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT) = NULL;
    static DWORD (WINAPI *pfnGetFileVersionInfoSizeW)(LPCWSTR, LPDWORD) = NULL;

    if (versionDll == NULL) {
        versionDll = LoadLibraryExW(L"version.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!versionDll)
            return 0;
    }

    if (!pfnVerQueryValueW)
        pfnVerQueryValueW = (void*)GetProcAddress(versionDll, "VerQueryValueW");
    if (!pfnGetFileVersionInfoW)
        pfnGetFileVersionInfoW = (void*)GetProcAddress(versionDll, "GetFileVersionInfoW");
    if (!pfnGetFileVersionInfoSizeW)
        pfnGetFileVersionInfoSizeW = (void*)GetProcAddress(versionDll, "GetFileVersionInfoSizeW");

    if (!pfnVerQueryValueW || !pfnGetFileVersionInfoW || !pfnGetFileVersionInfoSizeW)
        return 0;

    DWORD dummy = 0;
    DWORD size = pfnGetFileVersionInfoSizeW(filePath, &dummy);
    if (size == 0)
        return 0;

    void* versionData = HeapAlloc(GetProcessHeap(), 0, size);
    if (!versionData)
        return 0;

    BOOL result = 0;
    if (pfnGetFileVersionInfoW(filePath, 0, size, versionData)) {
        void* translation = NULL;
        UINT transLen = 0;
        if (pfnVerQueryValueW(versionData, L"\\VarFileInfo\\Translation", &translation, &transLen) &&
            translation != NULL && transLen >= sizeof(WORD) * 2) {
            WORD* trans = (WORD*)translation;

            wchar_t queryPath[128];
            swprintf_s(queryPath, 128, L"\\StringFileInfo\\%04x%04x\\InternalName", trans[0], trans[1]);

            void* internalName = NULL;
            UINT internalNameLen = 0;
            if (pfnVerQueryValueW(versionData, queryPath, &internalName, &internalNameLen) &&
                internalName != NULL && internalNameLen > 0) {
                if (_wcsicmp((wchar_t*)internalName, L"bootmgr.exe") == 0) {
                    result = 1;
                }
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, versionData);
    return result;
}


