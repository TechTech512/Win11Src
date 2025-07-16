// path.cpp

#include <windows.h>
#include <tchar.h>
#include <malloc.h>  // for malloc/free
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <stddef.h> // for size_t
#include <stdexcept>

namespace UnBCL {

// === Stub: Fallback allocation failure handler ===
class Allocator {
public:
    static void MemAllocFailed() {
        OutputDebugStringW(L"[UnBCL] Memory allocation failed\n");
        ExitProcess(1);
    }
};

// === Allocate wrapper ===
static wchar_t* AllocBuffer(size_t wcharCount)
{
    void* mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wcharCount * sizeof(wchar_t));
    if (!mem) Allocator::MemAllocFailed();
    return (wchar_t*)mem;
}

// === Sanitize Path ===
wchar_t* pSanitizePath(const wchar_t* inputPath) {
    if (!inputPath) return nullptr;

    size_t len = wcslen(inputPath);
    size_t maxLen = len + 4;
    wchar_t* buffer = AllocBuffer(maxLen);

    const wchar_t* src = inputPath;
    wchar_t* dst = buffer;
    bool skipSlash = false;

    if (src[0] == L'\\' && src[1] == L'\\') {
        *dst++ = *src++;
        *dst++ = *src++;
        if (src[0] == L'?' && src[1] == L'\\') {
            *dst++ = *src++;
            *dst++ = *src++;
        }
        skipSlash = true;
    }

    if (src[0] && src[1] == L':') {
        *dst++ = *src++;
        *dst++ = *src++;
    }

    wchar_t* segStart = dst;

    while (*src) {
        while (*src == L'\\') ++src;
        if (!*src) break;

        if (src[0] == L'.' && (src[1] == L'\\' || src[1] == 0)) {
            src += (src[1] == 0) ? 1 : 2;
            continue;
        }
        else if (src[0] == L'.' && src[1] == L'.' && (src[2] == L'\\' || src[2] == 0)) {
            if (dst > segStart) {
                --dst;
                while (dst > segStart && *dst != L'\\') --dst;
                if (*dst == L'\\') ++dst;
            }
            src += (src[2] == 0) ? 2 : 3;
            continue;
        }

        if (dst > segStart) *dst++ = L'\\';

        while (*src && *src != L'\\') {
            *dst++ = *src++;
        }
    }

    *dst = 0;
    return buffer;
}

// === Simple internal NT path generator ===
wchar_t* pGetPath(const wchar_t* input, int /*flags*/, int* outIsLocal = nullptr) {
    if (!input || !*input) return nullptr;

    const wchar_t* prefix = L"\\\\?\\";
    size_t len = wcslen(input);
    size_t totalLen = wcslen(prefix) + len + 1;

    wchar_t* buffer = AllocBuffer(totalLen);
    wcscpy(buffer, prefix);
    wcscat(buffer, input);

    if (outIsLocal) *outIsLocal = 1;
    return buffer;
}


extern "C" int __stdcall pReallocateBuffer(unsigned int newSize,
                                           wchar_t** current,
                                           wchar_t** end,
                                           wchar_t** limit)
{
    if (!current || !*current) return 0;

    SIZE_T currentLen = (SIZE_T)(*end - *current);
    SIZE_T byteCount = newSize * sizeof(wchar_t);

    // Allocate new buffer with VirtualAlloc
    wchar_t* newBuf = (wchar_t*)VirtualAlloc(NULL, byteCount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!newBuf)
        return 0;

    // Manual memcpy
    for (SIZE_T i = 0; i < currentLen; ++i)
        newBuf[i] = (*current)[i];

    // Free old buffer with VirtualFree
    VirtualFree(*current, 0, MEM_RELEASE);

    *current = newBuf;
    *end     = newBuf + currentLen;
    *limit   = newBuf + newSize;

    return 1;
}

} // namespace UnBCL
