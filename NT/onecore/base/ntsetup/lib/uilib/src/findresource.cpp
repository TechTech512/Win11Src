#include "pch.h"
#include <cstring>

// External function declarations
extern "C" wchar_t* FormFullPathName(wchar_t* outputVar, wchar_t** pathBuffer);

wchar_t* __cdecl DetermineImageDLLName(HINSTANCE__* moduleHandle)
{
    wchar_t* fullPath = nullptr;
    wchar_t* formattedPath = nullptr;
    wchar_t* finalResult = nullptr;
    wchar_t filePathBuffer[260];
    
    memset(filePathBuffer, 0, sizeof(filePathBuffer));
    
    int charsWritten = GetModuleFileNameW(moduleHandle, filePathBuffer, 260);
    if (charsWritten == 0 || (formattedPath = FormFullPathName(nullptr, reinterpret_cast<wchar_t**>(&filePathBuffer)), formattedPath == nullptr)) {
        GetLastError();
    }
    
    SetLastError(0);
    
    if (formattedPath != nullptr) {
        HANDLE heap = GetProcessHeap();
        HeapFree(heap, 0, formattedPath);
    }
    
    finalResult = nullptr;
    return finalResult;
}

