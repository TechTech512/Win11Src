// heapmemorymanager.cpp

#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.

namespace UnBCL {

static void* (__stdcall* s_OOMHandler)(unsigned long) = nullptr;

void* __stdcall Allocator_OutOfMemory(unsigned long size)
{
    if (s_OOMHandler)
        return s_OOMHandler(size);
    return nullptr;
}

void __stdcall Allocator_SetOOMHandler(void* (__stdcall* handler)(unsigned long))
{
    s_OOMHandler = handler;
}

void __stdcall Allocator_InvokeOOMHandler(unsigned long size)
{
    wchar_t buffer[32];
    wsprintfW(buffer, L"%lu", size); // replaces CRT _ultow safely
    OutputDebugStringW(L"[UnBCL] OOM: ");
    OutputDebugStringW(buffer);
    OutputDebugStringW(L"\n");
    Allocator_OutOfMemory(size);
}

} // namespace UnBCL

