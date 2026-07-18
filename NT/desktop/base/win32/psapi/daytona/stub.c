/**
 * psapi_stub.c
 * Stub implementations for PSAPI functions that forward to K32* implementations
 */
#define PSAPI_VERSION 1
#pragma warning(disable: 4717)

#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "kernel32.lib")

/* 
 * PSAPI functions - these forward to the K32* implementations
 * The K32* functions are properly declared in psapi.h with dllimport attributes
 */

BOOL
WINAPI
K32EmptyWorkingSet(
    _In_ HANDLE hProcess
    );

BOOL
WINAPI
EmptyWorkingSet(
    _In_ HANDLE hProcess
    )
{
    return K32EmptyWorkingSet(hProcess);
}

BOOL
WINAPI
K32EnumDeviceDrivers (
    _Out_writes_bytes_(cb) LPVOID *lpImageBase,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    );

BOOL
WINAPI
EnumDeviceDrivers (
    _Out_writes_bytes_(cb) LPVOID *lpImageBase,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    )
{
    return K32EnumDeviceDrivers(lpImageBase, cb, lpcbNeeded);
}

BOOL
WINAPI
K32EnumPageFilesA (
    PENUM_PAGE_FILE_CALLBACKA pCallBackRoutine,
    LPVOID pContext
    );

BOOL
WINAPI
EnumPageFilesA (
    PENUM_PAGE_FILE_CALLBACKA pCallBackRoutine,
    LPVOID pContext
    )
{
    return K32EnumPageFilesA(pCallBackRoutine, pContext);
}

BOOL
WINAPI
K32EnumPageFilesW (
    PENUM_PAGE_FILE_CALLBACKW pCallBackRoutine,
    LPVOID pContext
    );

BOOL
WINAPI
EnumPageFilesW (
    PENUM_PAGE_FILE_CALLBACKW pCallBackRoutine,
    LPVOID pContext
    )
{
    return K32EnumPageFilesW(pCallBackRoutine, pContext);
}

BOOL
WINAPI
K32EnumProcesses(
    _Out_writes_bytes_(cb) DWORD* lpidProcess,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    );

BOOL
WINAPI
EnumProcesses(
    _Out_writes_bytes_(cb) DWORD* lpidProcess,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    )
{
    return K32EnumProcesses(lpidProcess, cb, lpcbNeeded);
}

BOOL
WINAPI
K32EnumProcessModules(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) HMODULE* lphModule,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    );

BOOL
WINAPI
EnumProcessModules(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) HMODULE* lphModule,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded
    )
{
    return K32EnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
}

BOOL
WINAPI
K32EnumProcessModulesEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) HMODULE* lphModule,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded,
    _In_ DWORD dwFilterFlag
    );

BOOL
WINAPI
EnumProcessModulesEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) HMODULE* lphModule,
    _In_ DWORD cb,
    _Out_ LPDWORD lpcbNeeded,
    _In_ DWORD dwFilterFlag
    )
{
    return K32EnumProcessModulesEx(hProcess, lphModule, cb, lpcbNeeded, dwFilterFlag);
}

DWORD
WINAPI
K32GetDeviceDriverBaseNameA (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetDeviceDriverBaseNameA (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetDeviceDriverBaseNameA(ImageBase, lpFilename, nSize);
}

DWORD
WINAPI
K32GetDeviceDriverBaseNameW (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPWSTR lpBaseName,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetDeviceDriverBaseNameW (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPWSTR lpBaseName,
    _In_ DWORD nSize
    )
{
    return K32GetDeviceDriverBaseNameW(ImageBase, lpBaseName, nSize);
}

DWORD
WINAPI
K32GetDeviceDriverFileNameA (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetDeviceDriverFileNameA (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetDeviceDriverFileNameA(ImageBase, lpFilename, nSize);
}

DWORD
WINAPI
K32GetDeviceDriverFileNameW (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPWSTR lpFilename,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetDeviceDriverFileNameW (
    _In_ LPVOID ImageBase,
    _Out_writes_(nSize) LPWSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetDeviceDriverFileNameW(ImageBase, lpFilename, nSize);
}

DWORD
WINAPI
K32GetMappedFileNameA (
    _In_ HANDLE hProcess,
    _In_ LPVOID lpv,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetMappedFileNameA (
    _In_ HANDLE hProcess,
    _In_ LPVOID lpv,
    _Out_writes_(nSize) LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetMappedFileNameA(hProcess, lpv, lpFilename, nSize);
}

DWORD
WINAPI
K32GetMappedFileNameW (
    _In_ HANDLE hProcess,
    _In_ LPVOID lpv,
    _Out_writes_(nSize) LPWSTR lpFilename,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetMappedFileNameW (
    _In_ HANDLE hProcess,
    _In_ LPVOID lpv,
    _Out_writes_(nSize) LPWSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetMappedFileNameW(hProcess, lpv, lpFilename, nSize);
}

DWORD
WINAPI
K32GetModuleBaseNameA(
    _In_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _Out_writes_(nSize) LPSTR lpBaseName,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetModuleBaseNameA(
    _In_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _Out_writes_(nSize) LPSTR lpBaseName,
    _In_ DWORD nSize
    )
{
    return K32GetModuleBaseNameA(hProcess, hModule, lpBaseName, nSize);
}

DWORD
WINAPI
K32GetModuleBaseNameW(
    _In_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _Out_writes_(nSize) LPWSTR lpBaseName,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetModuleBaseNameW(
    _In_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _Out_writes_(nSize) LPWSTR lpBaseName,
    _In_ DWORD nSize
    )
{
    return K32GetModuleBaseNameW(hProcess, hModule, lpBaseName, nSize);
}

_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
K32GetModuleFileNameExA(
    _In_opt_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _When_(return < nSize, _Out_writes_to_(nSize, return + 1))
    _When_(return == nSize, _Out_writes_all_(nSize))
         LPSTR lpFilename,
    _In_ DWORD nSize
    );

_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameExA(
    _In_opt_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _When_(return < nSize, _Out_writes_to_(nSize, return + 1))
    _When_(return == nSize, _Out_writes_all_(nSize))
         LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetModuleFileNameExA(hProcess, hModule, lpFilename, nSize);
}

_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
K32GetModuleFileNameExW(
    _In_opt_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _When_(return < nSize, _Out_writes_to_(nSize, return + 1))
    _When_(return == nSize, _Out_writes_all_(nSize))
         LPWSTR lpFilename,
    _In_ DWORD nSize
    );

_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameExW(
    _In_opt_ HANDLE hProcess,
    _In_opt_ HMODULE hModule,
    _When_(return < nSize, _Out_writes_to_(nSize, return + 1))
    _When_(return == nSize, _Out_writes_all_(nSize))
         LPWSTR lpFilename,
    _In_ DWORD nSize
    )
{
    return K32GetModuleFileNameExW(hProcess, hModule, lpFilename, nSize);
}

BOOL
WINAPI
K32GetModuleInformation(
    _In_ HANDLE hProcess,
    _In_ HMODULE hModule,
    _Out_ LPMODULEINFO lpmodinfo,
    _In_ DWORD cb
    );

BOOL
WINAPI
GetModuleInformation(
    _In_ HANDLE hProcess,
    _In_ HMODULE hModule,
    _Out_ LPMODULEINFO lpmodinfo,
    _In_ DWORD cb
    )
{
    return K32GetModuleInformation(hProcess, hModule, lpmodinfo, cb);
}

BOOL
WINAPI
K32GetPerformanceInfo (
    PPERFORMANCE_INFORMATION pPerformanceInformation,
    DWORD cb
    );

BOOL
WINAPI
GetPerformanceInfo (
    PPERFORMANCE_INFORMATION pPerformanceInformation,
    DWORD cb
    )
{
    return K32GetPerformanceInfo(pPerformanceInformation, cb);
}

DWORD
WINAPI
K32GetProcessImageFileNameA (
    _In_ HANDLE hProcess,
    _Out_writes_(nSize) LPSTR lpImageFileName,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetProcessImageFileNameA (
    _In_ HANDLE hProcess,
    _Out_writes_(nSize) LPSTR lpImageFileName,
    _In_ DWORD nSize
    )
{
    return K32GetProcessImageFileNameA(hProcess, lpImageFileName, nSize);
}

DWORD
WINAPI
K32GetProcessImageFileNameW (
    _In_ HANDLE hProcess,
    _Out_writes_(nSize) LPWSTR lpImageFileName,
    _In_ DWORD nSize
    );

DWORD
WINAPI
GetProcessImageFileNameW (
    _In_ HANDLE hProcess,
    _Out_writes_(nSize) LPWSTR lpImageFileName,
    _In_ DWORD nSize
    )
{
    return K32GetProcessImageFileNameW(hProcess, lpImageFileName, nSize);
}

BOOL
WINAPI
K32GetProcessMemoryInfo (
    _In_ HANDLE Process,
    _Out_writes_bytes_(cb) PPROCESS_MEMORY_COUNTERS ppsmemCounters,
    _In_ DWORD cb
    );

BOOL
WINAPI
GetProcessMemoryInfo (
    _In_ HANDLE Process,
    _Out_writes_bytes_(cb) PPROCESS_MEMORY_COUNTERS ppsmemCounters,
    _In_ DWORD cb
    )
{
    return K32GetProcessMemoryInfo(Process, ppsmemCounters, cb);
}

BOOL
WINAPI
K32GetWsChanges(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
    _In_ DWORD cb
    );

BOOL
WINAPI
GetWsChanges(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
    _In_ DWORD cb
    )
{
    return K32GetWsChanges(hProcess, lpWatchInfo, cb);
}

BOOL
WINAPI
K32GetWsChangesEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_to_(*cb, *cb) PPSAPI_WS_WATCH_INFORMATION_EX lpWatchInfoEx,
    _Inout_ PDWORD cb
    );

BOOL
WINAPI
GetWsChangesEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_to_(*cb, *cb) PPSAPI_WS_WATCH_INFORMATION_EX lpWatchInfoEx,
    _Inout_ PDWORD cb
    )
{
    return K32GetWsChangesEx(hProcess, lpWatchInfoEx, cb);
}

BOOL
WINAPI
K32InitializeProcessForWsWatch(
    _In_ HANDLE hProcess
    );

BOOL
WINAPI
InitializeProcessForWsWatch(
    _In_ HANDLE hProcess
    )
{
    return K32InitializeProcessForWsWatch(hProcess);
}

BOOL
WINAPI
K32QueryWorkingSet(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PVOID pv,
    _In_ DWORD cb
    );

BOOL
WINAPI
QueryWorkingSet(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PVOID pv,
    _In_ DWORD cb
    )
{
    return K32QueryWorkingSet(hProcess, pv, cb);
}

BOOL
WINAPI
K32QueryWorkingSetEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PVOID pv,
    _In_ DWORD cb
    );

BOOL
WINAPI
QueryWorkingSetEx(
    _In_ HANDLE hProcess,
    _Out_writes_bytes_(cb) PVOID pv,
    _In_ DWORD cb
    )
{
    return K32QueryWorkingSetEx(hProcess, pv, cb);
}

