#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <stdlib.h>
#include <guiddef.h>
#include <unknwn.h>
#include <stdio.h>
#include <wchar.h>
#include <rpc.h>
#include <rpcdce.h>

WCHAR* __cdecl FindThisCharacter(WCHAR* String, WCHAR Character, BOOL CheckSpaces)
{
    if (String == NULL) {
        return NULL;
    }

    while (*String != L'\0') {
        if (*String == Character) {
            break;
        }
        
        if (CheckSpaces && *String != L' ') {
            ExitProcess(1);
        }
        
        String++;
    }

    if (*String == L'\0') {
        return String;
    }
    
    return String + 1;
}

WCHAR* __cdecl GetClsCtxFromString(DWORD* ClsCtx, WCHAR* String)
{
    WCHAR* Start = FindThisCharacter(String, L',', TRUE);
    if (Start == NULL || *Start == L'\0') {
        ExitProcess(1);
    }

    WCHAR* End = FindThisCharacter(Start, L',', FALSE);
    if (swscanf(Start, L"%X", ClsCtx) != 1) {
        ExitProcess(1);
    }

    return End;
}

WCHAR* __cdecl GetGUIDFromString(GUID* Guid, WCHAR* String)
{
    WCHAR* Start = FindThisCharacter(String, L',', TRUE);
    if (Start == NULL || *Start == L'\0') {
        ExitProcess(1);
    }

    WCHAR* End = FindThisCharacter(Start, L',', FALSE);
    if (End != NULL && *End != L'\0') {
        *End = L'\0';
        End++;
    }

    if (IIDFromString(Start, Guid) < 0) {
        ExitProcess(1);
    }

    return End;
}

extern "C" BOOL WINAPI ApphelpCheckShellObject(REFGUID rguid, DWORD dwFlags, LPVOID pReserved);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    GUID ClassId = {0};
    GUID InterfaceId = {0};
    DWORD ClsContext = 0x401;
    DWORD CoInitFlags = 8;
    
    WCHAR* CommandLine = GetCommandLineW();
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    WCHAR* CurrentPos = CommandLine;
    if (CurrentPos != NULL && *CurrentPos != L'\0') {
        CurrentPos = FindThisCharacter(CurrentPos, L'\0', FALSE);
    }
    
    while (CurrentPos != NULL && *CurrentPos != L'\0') {
        WCHAR Option = *CurrentPos;
        
        if (Option == L'C' || Option == L'I') {
            CurrentPos = GetGUIDFromString(&ClassId, CurrentPos);
        } else if (Option == L'M') {
            CurrentPos++;
        } else if (Option == L'S') {
            CoInitFlags |= 2;
            CurrentPos++;
        } else if (Option == L'X') {
            CurrentPos = GetClsCtxFromString(&ClsContext, CurrentPos);
        } else {
            break;
        }
    }
    
    HRESULT hr = CoInitializeEx(NULL, CoInitFlags);
    if (FAILED(hr)) {
        ExitProcess(2);
    }
    
    BOOL ShellObjectAllowed = ApphelpCheckShellObject(ClassId, TRUE, NULL);
    if (!ShellObjectAllowed) {
        HANDLE Process = GetCurrentProcess();
        TerminateProcess(Process, 3);
    }
    
    IUnknown* Object = NULL;
    hr = CoCreateInstance(ClassId, NULL, ClsContext | 0x400, IID_IUnknown, (void**)Object);
    if (FAILED(hr) || Object == NULL) {
        HANDLE Process = GetCurrentProcess();
        TerminateProcess(Process, 3);
    }
    
    IUnknown* TempObject = NULL;
    hr = Object->QueryInterface(IID_IUnknown, (void**)&TempObject);
    if (SUCCEEDED(hr)) {
        TempObject->Release();
        HANDLE Process = GetCurrentProcess();
        TerminateProcess(Process, 3);
    }
    
    Object->Release();
    CoUninitialize();
    
    return 0;
}

