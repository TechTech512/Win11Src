#include <windows.h>

DWORD __cdecl Dispatch(
    int (__cdecl *Function)(DWORD, DWORD, DWORD, DWORD),
    DWORD Argument1,
    DWORD Argument2,
    DWORD Argument3,
    DWORD Argument4
)
{
    UINT PreviousErrorMode;
    HMODULE Wow32Library;
    FARPROC WowHandle32;
    DWORD Result;
    DWORD ProcessedArgument1;
    DWORD ProcessedArgument2;
    
    PreviousErrorMode = SetErrorMode(0x8000);
    Wow32Library = LoadLibraryA("wow32.dll");
    SetErrorMode(PreviousErrorMode);
    
    if (Wow32Library == NULL) {
        if (Argument1 == 0) {
            ProcessedArgument1 = 0;
        } else {
            ProcessedArgument1 = Argument1 | 0xFFFF0000;
        }
    } else {
        WowHandle32 = GetProcAddress(Wow32Library, "WOWHandle32");
        if (WowHandle32 == NULL) {
            if (Argument1 == 0) {
                ProcessedArgument1 = 0;
            } else {
                ProcessedArgument1 = Argument1 | 0xFFFF0000;
            }
        } else {
            ProcessedArgument1 = Argument1 & 0xFFFF;
            ProcessedArgument1 = ((DWORD (__cdecl *)(DWORD, DWORD))WowHandle32)(ProcessedArgument1, 0xE);
        }
        FreeLibrary(Wow32Library);
    }
    
    ProcessedArgument2 = Argument2 & 0xFFFF;
    Result = Function(ProcessedArgument1, ProcessedArgument2, Argument3, Argument4);
    
    return Result;
}

