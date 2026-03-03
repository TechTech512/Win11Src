#pragma warning (disable:4715)

#include <windows.h>
#include <winternl.h>
#include <stdlib.h>
#include <stdio.h>

// External functions that would be in other source files
extern "C" {
    DWORD RtlNtStatusToDosErrorNoTeb(NTSTATUS Status);
	void __cdecl _guard_check_icall_nop(unsigned int);
}

extern ULONG __cdecl UpdateAttemptCount(void);
extern uintptr_t __security_cookie;
extern void __fastcall __security_check_cookie(uintptr_t cookie);

// Function implementations
int __cdecl RjvInvokePlatformForPhase(int phasePath, wchar_t* systemDrive, wchar_t* param3, wchar_t* param4)
{
    NTSTATUS status;
    FARPROC rejuvStartup;
    FARPROC rejuvShutdown;
    HMODULE hModule;
    DWORD errorCode;
    wchar_t dllPath[260];
    uintptr_t securityCookie;
    int returnValue = 0;
    
    securityCookie = __security_cookie;
    
    // This would normally come from a register parameter
    int edxValue = 0; // Placeholder - actual value comes from EDX register
    
    if (((edxValue == 0) || (phasePath == 0)) || (systemDrive == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        __security_check_cookie(securityCookie);
        return 0;
    }
    
    memset(dllPath, 0, sizeof(dllPath));
    hModule = NULL;
    rejuvShutdown = NULL;
    
    swprintf_s(dllPath, 0x104, L"%s\\Stack\\RjvPlatform.dll", (wchar_t*)phasePath);
    
    if ((dllPath[0] == 0) || 
        ((hModule = LoadLibraryExW(dllPath, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32), hModule == NULL) ||
         (rejuvStartup = GetProcAddress(hModule, "RejuvStartup"), rejuvStartup == NULL))) {
        if (hModule != NULL) {
            FreeLibrary(hModule);
        }
        __security_check_cookie(securityCookie);
        return 0;
    }
    
    rejuvShutdown = GetProcAddress(hModule, "RejuvShutdown");
    if (rejuvShutdown == NULL) {
        FreeLibrary(hModule);
        __security_check_cookie(securityCookie);
        return 0;
    }

    _guard_check_icall_nop(1);
    
    // The actual call to RejuvStartup with proper parameters
    typedef int (__cdecl *REJUV_STARTUP_FUNC)(int, int, int, wchar_t*, int, int, DWORD*, int**);
    REJUV_STARTUP_FUNC startupFunc = (REJUV_STARTUP_FUNC)rejuvStartup;
    
    DWORD contextData = 0;
    int* interfacePtr = NULL;
    
    status = startupFunc(1, 0, 0, systemDrive, phasePath, edxValue, &contextData, &interfacePtr);
    
    if (status < 0) {
        errorCode = RtlNtStatusToDosErrorNoTeb(status);
        SetLastError(errorCode);
        returnValue = 0;
    } else {
        _guard_check_icall_nop(8);
        returnValue = 1;
        
        if (interfacePtr != NULL) {
            typedef void (__cdecl *INTERFACE_FUNC)(void);
            INTERFACE_FUNC interfaceFunc = (INTERFACE_FUNC)(*(int**)interfacePtr)[2];
            _guard_check_icall_nop((uintptr_t)interfaceFunc);
            interfaceFunc();
        }
    }
    
    if (interfacePtr != NULL) {
        typedef void (__cdecl *DESTRUCTOR_FUNC)(void);
        DESTRUCTOR_FUNC destructor = (DESTRUCTOR_FUNC)(*(int**)interfacePtr)[0];
        _guard_check_icall_nop((uintptr_t)destructor);
        destructor();
        interfacePtr = NULL;
    }
    
    if (rejuvShutdown != NULL) {
        _guard_check_icall_nop((uintptr_t)&contextData);
        ((void (__cdecl*)(void))rejuvShutdown)();
    }
    
    if (hModule != NULL) {
        FreeLibrary(hModule);
    }
    
    __security_check_cookie(securityCookie);
    return returnValue;
}

int __cdecl wmain(void)
{
    ULONG attemptCount;
    int result;
    wchar_t systemDrive[260];
    wchar_t sysResetPath[260];
    wchar_t tempPath[260];
	NTSTATUS status = NULL;
    
    memset(systemDrive, 0, sizeof(systemDrive));
    memset(sysResetPath, 0, sizeof(sysResetPath));
    memset(tempPath, 0, sizeof(tempPath));
    
    attemptCount = UpdateAttemptCount();
    if (attemptCount > 3) {
        RegDeleteKeyW(HKEY_CURRENT_USER,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects\\{872f8dc8-dde4-43bd-ac7a-e3d9fe86ceac}");
    }
    
    if (ExpandEnvironmentStringsW(L"%systemdrive%", systemDrive, 0x104) != 0) {
        if (ExpandEnvironmentStringsW(L"%systemdrive%\\$SysReset\\Framework", sysResetPath, 0x104) != 0) {
            if (ExpandEnvironmentStringsW(L"%localappdata%\\Temp", tempPath, 0x104) != 0) {
                // The original assembly would have passed tempPath as the third parameter
                // and some other value (possibly NULL or a register value) as the fourth
				RtlNtStatusToDosErrorNoTeb(status);
				RjvInvokePlatformForPhase((int)sysResetPath, systemDrive, tempPath, NULL);
                result = RjvInvokePlatformForPhase((int)sysResetPath, systemDrive, tempPath, NULL);
                
                if (result == 1) {
                    RegDeleteKeyW(HKEY_CURRENT_USER,
                                  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects\\{872f8dc8-dde4-43bd-ac7a-e3d9fe86ceac}");
                }
            }
        }
    }
    
    return 0;
}

