// directory.cpp

#include <windows.h>
#include <winioctl.h>

namespace UnBCL {

#pragma pack(push, 1)
typedef struct _REPARSE_GUID_DATA_BUFFER {
    DWORD ReparseTag;
    WORD ReparseDataLength;
    WORD Reserved;
    GUID ReparseGuid;
    BYTE GenericReparseBuffer[1];
} REPARSE_GUID_DATA_BUFFER;

#define REPARSE_GUID_DATA_BUFFER_HEADER_SIZE \
    FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, GenericReparseBuffer)
#pragma pack(pop)

extern "C" int __stdcall pRemoveReparsePointData(wchar_t* path)
{
    HANDLE h = CreateFileW(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return 1;

    // Keep stack usage under 4KB to avoid __chkstk
    BYTE smallBuf[1024];
    memset(smallBuf, 0, sizeof(smallBuf));

    DWORD bytes = 0;
    BOOL ok = DeviceIoControl(
        h,
        FSCTL_GET_REPARSE_POINT,
        nullptr,
        0,
        smallBuf,
        sizeof(smallBuf),
        &bytes,
        nullptr);

    if (!ok) {
        CloseHandle(h);
        return 2;
    }

    // Get the reparse tag
    DWORD tag = *(DWORD*)smallBuf;

    // Prepare delete request buffer
    REPARSE_GUID_DATA_BUFFER hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.ReparseTag = tag;

    ok = DeviceIoControl(
        h,
        FSCTL_DELETE_REPARSE_POINT,
        &hdr,
        REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
        nullptr,
        0,
        &bytes,
        nullptr);

    CloseHandle(h);
    return ok ? 0 : 3;
}

int __cdecl pShouldEnumerateReparsePoint(const wchar_t* path)
{
    int shouldEnumerate = 0;

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr
    );

    // Retry once if failed
    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = CreateFileW(
            path,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr
        );
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD outSize = 0;
        DWORD bufferSize = 0x4000;
        BYTE* buffer = static_cast<BYTE*>(operator new[](bufferSize));

        if (buffer) {
            if (DeviceIoControl(
                    hFile,
                    FSCTL_GET_REPARSE_POINT,
                    nullptr,
                    0,
                    buffer,
                    bufferSize,
                    &outSize,
                    nullptr)) {
                
                DWORD reparseTag = *(DWORD*)buffer;

                if ((reparseTag & 0x80000000) != 0 &&
                    reparseTag != IO_REPARSE_TAG_MOUNT_POINT &&
                    reparseTag != IO_REPARSE_TAG_SYMLINK) {
                    shouldEnumerate = 1;
                }
            }

            operator delete[](buffer);
        }

        CloseHandle(hFile);
    }

    return shouldEnumerate;
}

int __cdecl WIN32_FROM_HRESULT(HRESULT hr, ULONG* outCode) {
    if (!outCode)
        return 0;

    if ((hr & 0xFFFF0000) == 0x80070000) {
        *outCode = hr & 0xFFFF;
        return 1;
    }

    if (hr == S_OK) {
        *outCode = 0;
        return 1;
    }

    return 0;
}

void __stdcall pGetDirectoriesAndFiles(
    const wchar_t* filterType,     // param_1 (e.g., files, dirs, both)
    const wchar_t* recurseType,    // param_2 (whether to recurse)
    const wchar_t* pathPrefix,     // param_3 (parent path for recursion)
    int callback,                  // param_4 (callback object pointer)
    const wchar_t* directory,      // param_5 (directory to enumerate)
    void* list                     // param_6 (void* list, acts as collection)
) {
    wchar_t searchPath[MAX_PATH];
    wsprintfW(searchPath, L"%s\\*", directory);

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            // Simulate throw new Win32Exception(...)
            ExitProcess(err);
        }
        return;
    }

    do {
        const wchar_t* name = findData.cFileName;

        // Skip "." and ".."
        if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
            continue;

        // Construct full path
        wchar_t fullPath[MAX_PATH];
        wsprintfW(fullPath, L"%s\\%s", directory, name);

        bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool isReparsePoint = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

        // Decide whether to add this to the list
        bool shouldAdd = false;
        if (wcscmp(filterType, L"files") == 0 && !isDirectory)
            shouldAdd = true;
        else if (wcscmp(filterType, L"dirs") == 0 && isDirectory)
            shouldAdd = true;
        else if (wcscmp(filterType, L"both") == 0)
            shouldAdd = true;

        if (shouldAdd && list != nullptr) {
            // Assuming list is a function pointer taking (const wchar_t*)
            typedef void(__stdcall* AddFunc)(const wchar_t*);
            ((AddFunc)list)(fullPath);
        }

        // Recursively process subdirectories if enabled
        if (isDirectory && !isReparsePoint && recurseType && wcscmp(recurseType, L"yes") == 0) {
            pGetDirectoriesAndFiles(
                filterType,
                recurseType,
                pathPrefix,
                callback,
                fullPath,
                list
            );
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

} // namespace UnBCL

// Disable intrinsic version of memset so we can safely define it
#pragma function(memset)

extern "C" void* __cdecl memset(void* dest, int val, size_t len)
{
    unsigned char* ptr = (unsigned char*)dest;
    while (len--) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

