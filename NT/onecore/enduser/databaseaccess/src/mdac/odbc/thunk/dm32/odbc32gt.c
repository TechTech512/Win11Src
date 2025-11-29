#include <windows.h>

DWORD __cdecl Dispatch(
    DWORD FunctionCode,
    int (__cdecl *Function)(),
    DWORD Argument1,
    DWORD Argument2,
    DWORD Argument3,
    DWORD Argument4,
    DWORD Argument5,
    DWORD Argument6,
    DWORD Argument7,
    DWORD Argument8,
    DWORD Argument9,
    DWORD Argument10,
    DWORD Argument11,
    DWORD Argument12,
    DWORD Argument13
)
{
    DWORD Result = 0;
    DWORD SecurityCookie;
    
    switch (FunctionCode) {
    case 1:
    case 3:
    case 0x12:
    case 0x14:
    case 0x30:
    case 0x3F:
        Result = Function();
        SecurityCookie = Argument2;
        break;
        
    case 2:
    case 5:
    case 9:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x3D:
        Result = Function();
        SecurityCookie = Argument3;
        break;
        
    case 4:
    case 0x2B:
        Argument3 = (WORD)Argument3;
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5);
        SecurityCookie = Argument6;
        break;
        
    case 6:
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            DWORD ProcessedArg3 = (WORD)Argument3;
            Result = Function(Argument1, ProcessedArg2, ProcessedArg3, Argument4, (WORD)Argument5, Argument6);
            SecurityCookie = Argument7;
        }
        break;
        
    case 7:
    case 0x41:
    case 0x43:
    case 0x46:
        Argument7 = (WORD)Argument7;
        {
            DWORD ProcessedArg3 = (WORD)Argument3;
            Result = Function(Argument1, Argument2, ProcessedArg3, Argument4, (WORD)Argument5, Argument6);
            SecurityCookie = Argument7;
        }
        break;
        
    case 8:
        Argument4 = (WORD)Argument4;
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2, Argument3, Argument4, (WORD)Argument5, Argument6, (WORD)Argument7, Argument8);
        SecurityCookie = Argument9;
        break;
        
    case 10:
        Argument7 = (WORD)Argument7;
        Result = Function(Argument1, Argument7);
        SecurityCookie = Argument3;
        break;
        
    case 0xB:
    case 0x13:
    case 0x31:
    case 0x40:
        Result = Function(Argument1, Argument2);
        SecurityCookie = Argument3;
        break;
        
    case 0x10:
        Argument2 = (WORD)Argument2;
        Result = Function();
        SecurityCookie = Argument2;
        break;
        
    case 0x11:
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2, Argument3);
        SecurityCookie = Argument4;
        break;
        
    case 0x15:
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2);
        SecurityCookie = Argument3;
        break;
        
    case 0x16:
        Result = Function(Argument1, Argument7);
        SecurityCookie = Argument3;
        break;
        
    case 0x17:
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2);
        SecurityCookie = Argument3;
        break;
        
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x39:
    case 0x47:
        SecurityCookie = 0;
        break;
        
    case 0x28:
    case 0x36:
    case 0x38:
    case 0x42:
        Argument9 = (WORD)Argument9;
        Argument7 = (WORD)Argument7;
        Argument5 = (WORD)Argument5;
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5, Argument6, Argument7, Argument8);
        SecurityCookie = Argument9;
        break;
        
    case 0x29:
        {
            UINT PreviousErrorMode = SetErrorMode(0x8000);
            HMODULE Wow32Library = LoadLibraryA("wow32.dll");
            SetErrorMode(PreviousErrorMode);
            
            DWORD ProcessedArg2;
            if (Wow32Library == NULL) {
                if (Argument2 == 0) {
                    ProcessedArg2 = 0;
                } else {
                    ProcessedArg2 = Argument2 | 0xFFFF0000;
                }
            } else {
                FARPROC WowHandle32 = GetProcAddress(Wow32Library, "WOWHandle32");
                if (WowHandle32 == NULL) {
                    if (Argument2 == 0) {
                        ProcessedArg2 = 0;
                    } else {
                        ProcessedArg2 = Argument2 | 0xFFFF0000;
                    }
                } else {
                    ProcessedArg2 = ((DWORD (__cdecl *)(DWORD))WowHandle32)(Argument2 & 0xFFFF);
                }
                FreeLibrary(Wow32Library);
            }
            Result = Function(Argument1, ProcessedArg2);
            SecurityCookie = Argument3;
        }
        break;
        
    case 0x2A:
    case 0x2C:
    case 0x2E:
    case 0x33:
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2);
        SecurityCookie = Argument3;
        break;
        
    case 0x2D:
        Argument4 = (WORD)Argument4;
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            Result = Function(Argument1, ProcessedArg2, Argument3, Argument4);
            SecurityCookie = Argument5;
        }
        break;
        
    case 0x2F:
        Argument2 = (WORD)Argument2;
        Result = Function();
        SecurityCookie = Argument2;
        break;
        
    case 0x32:
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            if ((WORD)Argument2 == 0x6F) {
                UINT PreviousErrorMode = SetErrorMode(0x8000);
                HMODULE Wow32Library = LoadLibraryA("wow32.dll");
                SetErrorMode(PreviousErrorMode);
                
                if (Wow32Library == NULL) {
                    if (Argument3 == 0) {
                        Argument3 = 0;
                    } else {
                        Argument3 = Argument3 | 0xFFFF0000;
                    }
                } else {
                    FARPROC WowHandle32 = GetProcAddress(Wow32Library, "WOWHandle32");
                    if (WowHandle32 == NULL) {
                        if (Argument3 == 0) {
                            Argument3 = 0;
                        } else {
                            Argument3 = Argument3 | 0xFFFF0000;
                        }
                        FreeLibrary(Wow32Library);
                    } else {
                        DWORD TempArg = Argument3 & 0xFFFF;
                        Argument3 = ((DWORD (__cdecl *)(DWORD, DWORD))WowHandle32)(TempArg, 0xE);
                        FreeLibrary(Wow32Library);
                    }
                }
            }
            Result = Function(Argument1, ProcessedArg2, Argument3);
            SecurityCookie = 0;
        }
        break;
        
    case 0x34:
        Argument10 = (WORD)Argument10;
        Argument9 = (WORD)Argument9;
        Argument8 = (WORD)Argument8;
        Argument6 = (WORD)Argument6;
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            Result = Function(Argument1, ProcessedArg2, Argument3, (WORD)Argument4, Argument5, Argument6, Argument7, Argument8, Argument9);
            SecurityCookie = Argument10;
        }
        break;
        
    case 0x35:
        Argument9 = (WORD)Argument9;
        Argument8 = (WORD)Argument8;
        Argument7 = (WORD)Argument7;
        Argument5 = (WORD)Argument5;
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5, Argument6, Argument7, Argument8);
        SecurityCookie = Argument9;
        break;
        
    case 0x37:
        Argument5 = (WORD)Argument5;
        Argument3 = (WORD)Argument3;
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5);
        SecurityCookie = Argument6;
        break;
        
    case 0x3A:
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5);
        SecurityCookie = Argument6;
        break;
        
    case 0x3B:
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            Result = Function(Argument1, ProcessedArg2, Argument3, Argument4);
            SecurityCookie = Argument5;
        }
        break;
        
    case 0x3C:
        {
            int IntArg3 = (WORD)Argument3;
            int IntArg5 = (WORD)Argument5;
            int IntArg7 = (WORD)Argument7;
            int IntArg9 = (WORD)Argument9;
            int IntArg11 = (WORD)Argument11;
            int IntArg13 = (WORD)Argument13;
            Result = Function(Argument1, Argument2, IntArg3, Argument4, IntArg5, Argument6, IntArg7, Argument8, IntArg9, Argument10, IntArg11, Argument12);
        }
        break;
        
    case 0x3E:
        Result = Function(Argument1, Argument2, Argument3, Argument4, Argument5);
        SecurityCookie = Argument6;
        break;
        
    case 0x44:
        Argument4 = (WORD)Argument4;
        Argument3 = (WORD)Argument3;
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2, Argument3);
        SecurityCookie = Argument4;
        break;
        
    case 0x45:
        Argument4 = (WORD)Argument4;
        Argument2 = (WORD)Argument2;
        Result = Function(Argument1, Argument2, Argument3);
        SecurityCookie = Argument4;
        break;
        
    case 0x48:
        Argument7 = (WORD)Argument7;
        Argument5 = (WORD)Argument5;
        Argument3 = (WORD)Argument3;
        {
            DWORD ProcessedArg2 = (WORD)Argument2;
            Result = Function(Argument1, ProcessedArg2, Argument3, (WORD)Argument4, Argument5, Argument6, Argument7, Argument8, Argument9);
            SecurityCookie = Argument10;
        }
        break;
        
    default:
        SecurityCookie = 0;
        break;
    }
    
    return Result;
}

DWORD __cdecl Dispatch2(
    DWORD FunctionCode,
    int (__cdecl *Function)(),
    DWORD Argument1,
    DWORD Argument2,
    DWORD Argument3,
    DWORD Argument4,
    DWORD Argument5,
    DWORD Argument6,
    DWORD Argument7,
    DWORD Argument8,
    DWORD Argument9,
    DWORD Argument10,
    DWORD Argument11,
    DWORD Argument12,
    DWORD Argument13
)
{
    DWORD Result = 0;
    DWORD SecurityCookie;
    
    if (FunctionCode == 1) {
        int IntArg4 = (WORD)Argument4;
        DWORD ProcessedArg3 = (WORD)Argument3;
        DWORD ProcessedArg1 = (WORD)Argument1;
        Function(ProcessedArg1, Argument2, ProcessedArg3, IntArg4);
        SecurityCookie = Argument2;
    } else if (FunctionCode == 2) {
        HMODULE OdbcIntLibrary = LoadLibraryA("odbcint.dll");
        if (OdbcIntLibrary == NULL) {
            return 0;
        }
        Result = LoadStringA(OdbcIntLibrary, Argument1, (LPSTR)Argument2, Argument3);
        FreeLibrary(OdbcIntLibrary);
        return Result;
    } else {
        SecurityCookie = 0;
    }
    
    return Result;
}

