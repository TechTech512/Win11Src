#include <windows.h>
#include <winternl.h>
#include <stdlib.h>
#include "fmifsutil.h"

// Global variables
BOOL g_triggered = FALSE;
DWORD g_serviceControlFlags = 0;
DWORD g_finishedResult = 0;

// FMIFS callback packet types
typedef enum _FMIFS_PACKET_TYPE {
    FmIfsFinished,
    FmIfsIncompatibleFileSystem,
    FmIfsAccessDenied,
    FmIfsVolumeInUse,
    FmIfsCantLock,
    FmIfsCantQuickFormat,
    FmIfsIoError,
    FmIfsBadLabel,
    FmIfsCantDetermineFileSystem,
    FmIfsCantContinueInReadOnly,
    FmIfsMediaWriteProtected
} FMIFS_PACKET_TYPE;

// Function prototypes
unsigned char __cdecl _FmifsCallbackHandler(FMIFS_PACKET_TYPE packetType, DWORD packetLength, void* packetData);
void __cdecl DoCheck(void* param_1);
wchar_t* __cdecl FindDriveName(wchar_t* drivePath);
unsigned char __cdecl IncrementRetryCount(wchar_t* drivePath);
void __cdecl ResetRetryCount(wchar_t* drivePath);
DWORD __cdecl ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, void* lpEventData, void* lpContext);
void __cdecl ServiceMain(DWORD dwArgc, wchar_t** lpszArgv);

// External functions
extern "C" void LOCK();
extern "C" void UNLOCK();
extern "C" void DbgPrint(const char* format, ...);

#define LOCK() // No-op for simple cases, or use:
#define UNLOCK() // No-op for simple cases

unsigned char __cdecl _FmifsCallbackHandler(FMIFS_PACKET_TYPE packetType, DWORD packetLength, void* packetData)
{
    if (((packetType != FmIfsAccessDenied) && (packetType != FmIfsMediaWriteProtected)) &&
        (packetType != FmIfsIoError)) {
        if (packetType == FmIfsFinished) {
            g_finishedResult = *(DWORD*)((char*)packetData + 4);
            return *(unsigned char*)packetData;
        }
        if (packetType != FmIfsCantContinueInReadOnly) {
            return ~(unsigned char)((DWORD)g_serviceControlFlags >> 1) & 1;
        }
    }
    return 0;
}

void __cdecl DoCheck(void* param_1)
{
    DWORD lastError;
    unsigned char loadResult;
    wchar_t* driveName;
    DWORD* buffer;
    BOOL deviceIoSuccess;
    DWORD bytesReturned;
    HANDLE hToken;
    BOOL tokenAdjusted;
    DWORD bufferSize;
    BOOL bufferAllocated;
    DWORD* currentEntry;
    DWORD entrySize;
    DWORD corruptionState;
    unsigned char incrementResult;
    
    BYTE ioctlBuffer[36] = {0};
    DWORD privilegeValue = 0;
    TOKEN_PRIVILEGES tokenPrivileges;
    DWORD oldBufferSize;
    BOOL oldDeviceIoSuccess;
    
    FMIFSLib* fmifsLib = (FMIFSLib*)param_1;
    
    loadResult = LoadFMIFS(fmifsLib);
    if (loadResult == 0) {
        return;
    }
    
    HANDLE hDevice = CreateFileW(L"\\\\.\\Ntfs", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return;
    }
    
    // Initialize IOCTL buffer
    memset(ioctlBuffer, 0, sizeof(ioctlBuffer));
    ioctlBuffer[0] = 0x81;
    ioctlBuffer[4] = 0x24;
    
    HANDLE hProcess = GetCurrentProcess();
    BOOL tokenOpened = OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!tokenOpened) {
        CloseHandle(hDevice);
        return;
    }
    
    BOOL privilegeFound = LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", (PLUID)&privilegeValue);
    if (!privilegeFound) {
        CloseHandle(hToken);
        CloseHandle(hDevice);
        return;
    }
    
    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Luid.LowPart = privilegeValue;
    tokenPrivileges.Privileges[0].Luid.HighPart = 0;
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    tokenAdjusted = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, 0, NULL, NULL);
    
    bufferSize = 0x400;
    buffer = (DWORD*)malloc(bufferSize);
    bufferAllocated = (buffer != NULL);
    
    DWORD retryFlag = 0;
    
    while (TRUE) {
        if ((!tokenAdjusted) || ((g_triggered == FALSE) && (retryFlag == 0))) {
            break;
        }
        
        g_triggered = FALSE;
        retryFlag = 0;
        
        if (bufferAllocated) {
            DWORD* tempBuffer = (DWORD*)realloc(buffer, bufferSize);
            if (tempBuffer == NULL) {
                bufferAllocated = FALSE;
            } else {
                buffer = tempBuffer;
                deviceIoSuccess = DeviceIoControl(hDevice, 0x90260, ioctlBuffer, 36, buffer, bufferSize, &bytesReturned, NULL);
                if (deviceIoSuccess == FALSE) {
                    lastError = GetLastError();
                    if ((lastError == ERROR_MORE_DATA) && (bufferSize == 0x400)) {
                        bufferSize = buffer[0];
                    }
                } else if (((buffer[6] & 8) != 0)) {
                    currentEntry = buffer + 8;
                    while ((DWORD*)currentEntry < (DWORD*)((char*)buffer + bytesReturned)) {
                        if (*(SHORT*)((char*)currentEntry + 10) == 0) {
                            break;
                        }
                        if ((*currentEntry & 8) != 0) {
                            incrementResult = IncrementRetryCount(L"");
                            if (fmifsLib->ChkdskEx != NULL) {
                                DWORD flags = ((incrementResult != 0) ? 0x40000000 : 0) | 0xB0004000;
                                ((void (__cdecl*)(BYTE*, wchar_t*, DWORD, void*, void*))fmifsLib->ChkdskEx)(
                                    (BYTE*)((char*)currentEntry + 10), 
                                    L"NTFS", 
                                    0, 
                                    &flags, 
                                    _FmifsCallbackHandler);
                                
                                if (g_finishedResult == 10) {
                                    retryFlag = 1;
                                } else {
                                    ResetRetryCount(L"");
                                }
                            }
                        }
                        entrySize = (DWORD)(*(SHORT*)((char*)currentEntry + 4));
                        currentEntry = (DWORD*)((char*)currentEntry + entrySize);
                    }
                    if (retryFlag != 0) {
                        WaitForSingleObject(hDevice, 600000);
                        oldBufferSize = bufferSize;
                        oldDeviceIoSuccess = deviceIoSuccess;
                        continue;
                    }
                }
            }
        }
        WaitForSingleObject(hDevice, 120000);
        oldBufferSize = bufferSize;
        oldDeviceIoSuccess = deviceIoSuccess;
    }
    
    CloseHandle(hToken);
    CloseHandle(hDevice);
    if (bufferAllocated) {
        free(buffer);
    }
    
    return;
}

wchar_t* __cdecl FindDriveName(wchar_t* drivePath)
{
    wchar_t* currentChar = drivePath;
    wchar_t* lastBackslash = drivePath;
    
    while (*currentChar != L'\0') {
        currentChar++;
    }
    
    int length = (int)(currentChar - drivePath);
    wchar_t* result = drivePath;
    
    if (length != 2) {
        for (DWORD i = 0; i < (DWORD)(length - 2); i++) {
            if (drivePath[i] == L'\\') {
                result = drivePath + i + 1;
            }
        }
    }
    
    if (*result == L'\\') {
        result = NULL;
    }
    
    return result;
}

unsigned char __cdecl IncrementRetryCount(wchar_t* drivePath)
{
    wchar_t* driveName;
    LONG regResult;
    HKEY hKey;
    DWORD disposition;
    DWORD retryCount = 0;
    DWORD valueSize = sizeof(DWORD);
    
    driveName = FindDriveName(drivePath);
    if (driveName == NULL) {
        return 0;
    }
    
    regResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Chkdsk\\Verify", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &disposition);
    if (regResult != ERROR_SUCCESS) {
        return 0;
    }
    
    if (disposition == REG_OPENED_EXISTING_KEY) {
        regResult = RegQueryValueExW(hKey, driveName, NULL, NULL, (LPBYTE)&retryCount, &valueSize);
    } else {
        retryCount = 0;
        regResult = ERROR_SUCCESS;
    }
    
    if ((regResult == ERROR_SUCCESS) || (regResult == ERROR_FILE_NOT_FOUND)) {
        retryCount++;
        regResult = RegSetValueExW(hKey, driveName, 0, REG_DWORD, (const BYTE*)&retryCount, sizeof(DWORD));
        RegCloseKey(hKey);
        if ((retryCount < 3) && (regResult == ERROR_SUCCESS)) {
            return 1;
        }
    } else {
        RegCloseKey(hKey);
    }
    
    return 0;
}

void __cdecl ResetRetryCount(wchar_t* drivePath)
{
    wchar_t* driveName;
    LONG regResult;
    HKEY hKey;
    DWORD zeroValue = 0;
    
    driveName = FindDriveName(drivePath);
    if (driveName == NULL) {
        return;
    }
    
    regResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Chkdsk\\Verify", 0, KEY_WRITE, &hKey);
    if (regResult == ERROR_SUCCESS) {
        RegSetValueExW(hKey, driveName, 0, REG_DWORD, (const BYTE*)&zeroValue, sizeof(DWORD));
        RegCloseKey(hKey);
    }
    
    return;
}

DWORD __cdecl ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, void* lpEventData, void* lpContext)
{
    DWORD currentFlags;
    DWORD newFlags;
    BOOL flagsChanged;
    DWORD status = 0x78; // ERROR_CALL_NOT_IMPLEMENTED
    
    DbgPrint(" [SVC] SetServiceStatus, opcode %d\n", dwControl);
    
    currentFlags = g_serviceControlFlags;
    do {
        LOCK();
        flagsChanged = (currentFlags != g_serviceControlFlags);
        newFlags = currentFlags | 1;
        if (flagsChanged) {
            currentFlags = g_serviceControlFlags;
            newFlags = g_serviceControlFlags;
        }
        g_serviceControlFlags = newFlags;
        UNLOCK();
    } while (flagsChanged);
    
    if ((currentFlags & 2) == 0) {
        if ((dwControl == SERVICE_CONTROL_STOP) || (dwControl == SERVICE_CONTROL_SHUTDOWN)) {
            LOCK();
            g_serviceControlFlags = g_serviceControlFlags | 2;
            UNLOCK();
            SetEvent(*(HANDLE*)lpContext);
            status = NO_ERROR;
        } else if (dwControl == 0x20) { // Custom control code
            status = NO_ERROR;
            g_triggered = TRUE;
            SetEvent(*(HANDLE*)lpContext);
        }
    } else {
        status = 0x45b; // ERROR_SERVICE_CANNOT_ACCEPT_CTRL
    }
    
    LOCK();
    g_serviceControlFlags = g_serviceControlFlags & 0xFFFFFFFE;
    UNLOCK();
    
    return status;
}

void __cdecl ServiceMain(DWORD dwArgc, wchar_t** lpszArgv)
{
    DWORD currentFlags;
    DWORD newFlags;
    BOOL flagsChanged;
    DWORD lastError;
    SERVICE_STATUS_HANDLE hServiceStatus;
    BOOL serviceStatusSet;
    
    HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    g_triggered = TRUE;
    
    LOCK();
    g_serviceControlFlags = 0;
    UNLOCK();
    
    if (hEvent == NULL) {
        lastError = GetLastError();
        DbgPrint(" [SVC] CreateEvent error = %d, svc %ws\n", lastError, lpszArgv[0]);
        return;
    }
    
    DbgPrint(" [SVC] Service %ws starting\n", lpszArgv[0]);
    
    hServiceStatus = RegisterServiceCtrlHandlerExW(lpszArgv[0], (LPHANDLER_FUNCTION_EX)ServiceCtrlHandler, &hEvent);
    if (hServiceStatus == NULL) {
        lastError = GetLastError();
        DbgPrint(" [SVC] RegisterServiceCtrlHandler failed %d\n", lastError);
        CloseHandle(hEvent);
        return;
    }
    
    SERVICE_STATUS serviceStatus;
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | 0x20;
    serviceStatus.dwWin32ExitCode = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode = 0;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;
    
    serviceStatusSet = SetServiceStatus(hServiceStatus, &serviceStatus);
    if (!serviceStatusSet) {
        lastError = GetLastError();
        DbgPrint(" [SVC] SetServiceStatus error %ld, svc %ws\n", lastError, lpszArgv[0]);
    }
    
    FMIFSLib fmifsLib = {0};
    DoCheck(&fmifsLib);
    
    currentFlags = g_serviceControlFlags;
    do {
        LOCK();
        flagsChanged = (currentFlags != g_serviceControlFlags);
        newFlags = currentFlags | 2;
        if (flagsChanged) {
            currentFlags = g_serviceControlFlags;
            newFlags = g_serviceControlFlags;
        }
        g_serviceControlFlags = newFlags;
        UNLOCK();
    } while ((flagsChanged) || ((newFlags = currentFlags & 1, currentFlags = g_serviceControlFlags, newFlags != 0)));
    
    serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus(hServiceStatus, &serviceStatus);
    
    CloseHandle(hEvent);
    
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;
    serviceStatusSet = SetServiceStatus(hServiceStatus, &serviceStatus);
    if (!serviceStatusSet) {
        lastError = GetLastError();
        DbgPrint(" [SVC] SetServiceStatus on stopped error %ld, svc %ws\n", lastError, lpszArgv[0]);
    }
    
    return;
}

