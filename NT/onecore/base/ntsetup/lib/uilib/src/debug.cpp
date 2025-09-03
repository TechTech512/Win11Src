#include "pch.h"
#include <wdslog.h>

// External function declarations
extern "C" void WdsSetupLogMessageW(tagLOG_PARTIAL_MSG* partialMsg, DWORD unknown1, const wchar_t* prefix, 
                                   const char* message, ULONG param1, char* param2, ULONG param3, 
                                   wchar_t* param4, DWORD lastError, DWORD unknown5, DWORD unknown6);

void __cdecl Ui_Assert(ULONG param_1, char* param_2, ULONG param_3, wchar_t* param_4, wchar_t* param_5, void* param_6)
{
    DWORD lastError = GetLastError();
    const wchar_t* prefix = L"D";
    
    tagLOG_PARTIAL_MSG* partialMsg = ConstructPartialMsgW(0x1000000, param_2);
    WdsSetupLogMessageW(partialMsg, 0, prefix, param_2, param_1, param_2, param_3, param_4, lastError, 0, 0);
    
    RaiseException(0xE0000100, 0, 0, 0);
}

