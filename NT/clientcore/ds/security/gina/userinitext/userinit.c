#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <winternl.h>
#include <strsafe.h>
#include <wincrypt.h>
#include <ntstatus.h>
#include <winuser.h>
#include <winuserp.h>
#include <wtsapi32.h>
#include <winsta.h>
#include "resource.h"

// External function declarations - ALL REPLACED
// ExecApplication REMOVED - replaced with RunApplication
// ExecProcesses REMOVED - replaced with RunApplication (buffer already contains full command)
extern int CheckXForestLogon(HANDLE token);

// Helper function to replace ExecApplication and ExecProcesses using CreateProcessW
// Returns 1 on success, 0 on failure
// waitForExit: 1 = wait for process to complete, 0 = return immediately
int RunApplication(wchar_t* cmdLine, int waitForExit) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL success = FALSE;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Create a modifiable copy of the command line (CreateProcessW may modify it)
    wchar_t* cmdLineCopy = NULL;
    if (cmdLine != NULL) {
        size_t len = wcslen(cmdLine) + 1;
        cmdLineCopy = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(wchar_t));
        if (cmdLineCopy != NULL) {
            wcscpy_s(cmdLineCopy, len, cmdLine);
        } else {
            return 0; // Allocation failed
        }
    }
    
    success = CreateProcessW(
        NULL,           // No application name - use command line
        cmdLineCopy,    // Command line (modifiable copy)
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure
    );
    
    if (cmdLineCopy != NULL) {
        HeapFree(GetProcessHeap(), 0, cmdLineCopy);
    }
    
    if (success) {
        // Wait for the process to exit if requested
        if (waitForExit) {
            WaitForSingleObject(pi.hProcess, INFINITE);
        }
        // Close thread and process handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1; // Success
    } else {
        return 0; // Failure
    }
}

NTSTATUS WINAPI NtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

void SetShellDesktopSwitchEvent(void);
unsigned char RegCheckCtfmon(wchar_t* param_1, unsigned long param_2);

// Global variables
int bFirst = 1;
int bAppCompatOn = 0;

void CheckVideoSelection(HINSTANCE param_1, unsigned char param_2)
{
    UNICODE_STRING registryPath;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE keyHandle;
    NTSTATUS status;
    wchar_t buffer[260];

    RtlInitUnicodeString(&registryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay");
    
    objectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory = NULL;
    objectAttributes.ObjectName = &registryPath;
    objectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;

    status = NtOpenKey(&keyHandle, KEY_READ, &objectAttributes);
    
    if (status < 0) {
        RtlInitUnicodeString(&registryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay");
        status = NtOpenKey(&keyHandle, KEY_READ, &objectAttributes);
        
        if (status < 0) {
            RtlInitUnicodeString(&registryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay");
            status = NtOpenKey(&keyHandle, KEY_READ, &objectAttributes);
            if (status < 0) {
                return;
            }
        }
    }
    
    NtClose(keyHandle);
    
    LoadStringW(NULL, IDS_DISPLAYAPPLET, buffer, sizeof(buffer)/sizeof(wchar_t));
    // REPLACED: ExecApplication(buffer, 0, 1, 0, 1, 0);
    RunApplication(buffer, 1);
}

int CreateExplorerSessionKey(void)
{
    wchar_t sessionKeyPath[76];
    HKEY hKey;
    DWORD disposition;
    int result = 0;

    if (StringCchPrintfW(sessionKeyPath, sizeof(sessionKeyPath)/sizeof(wchar_t), 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo\\%d", GetCurrentProcessId()) >= 0) {
        
        RegDeleteTreeW(HKEY_CURRENT_USER, sessionKeyPath);
        
        if (RegCreateKeyExW(HKEY_CURRENT_USER, sessionKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, 
                           KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (disposition == REG_CREATED_NEW_KEY) {
                result = 1;
            }
        }
    }
    
    return result;
}

void DisplayMessageAndExitWindows(wchar_t* message, wchar_t* title)
{
    MessageBoxW(NULL, message, title, MB_OK);
    ExitWindowsEx(EWX_LOGOFF, 0);
}

void* ImmWorker(int (*appFunc)(wchar_t*, int, int, int, unsigned short, int), 
                unsigned long (*procFunc)(wchar_t*, wchar_t*, int, unsigned long, int, int))
{
    void* immHandle = NULL;
    
    // NOTE: Both function pointers are no longer needed - we have RunApplication
    // The parameters are kept for function signature compatibility but not used
    
    if (GetSystemMetrics(SM_REMOTESESSION) != 0) {
        immHandle = LoadLibraryExW(L"imm32.dll", NULL, 0);
        if (immHandle != NULL) {
            FARPROC immDisableIME = GetProcAddress(immHandle, "ImmDisableIME");
            if (immDisableIME != NULL) {
                immDisableIME();
            }
        }
    }
    
    return immHandle;
}

void InitializeMisc(HINSTANCE param_1, unsigned char param_2)
{
    HKEY hKey;
    DWORD valueType;
    DWORD noPageFile = 0;
    DWORD valueSize = sizeof(DWORD);
    wchar_t buffer[260];
    
    // Check for rover monitor
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID\\{16d12736-7a9e-4765-bec6-f301d679caaa}", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        // REPLACED: ExecApplication(L"%SystemRoot%\\System32\\RunDll32.exe %SystemRoot%\\System32\\rover.dll,RunMonitor", 0, 0, 0, 0, 0);
        RunApplication(L"%SystemRoot%\\System32\\RunDll32.exe %SystemRoot%\\System32\\rover.dll,RunMonitor", 0);
    }
    
    // Check page file settings
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"PagingFiles", NULL, &valueType, (LPBYTE)&noPageFile, &valueSize) != ERROR_SUCCESS || 
            valueType != REG_DWORD || valueSize != sizeof(DWORD)) {
            noPageFile = 0;
        }
        RegCloseKey(hKey);
    } else {
        noPageFile = 0;
    }
    
    // Launch virtual memory applet if no page file
    if (noPageFile == 1) {
        LoadStringW(NULL, IDS_VMAPPLET, buffer, sizeof(buffer)/sizeof(wchar_t));
        // REPLACED: ExecProcesses(L"vmapplet", buffer, 1, 0, 1, 0);
        // buffer contains the full command line from IDS_VMAPPLET: "systemPropertiesPerformance.exe /pagefile"
        RunApplication(buffer, 1);
    }
    
    // Check video selection if needed
    if (param_2 != 0) {
        CheckVideoSelection(param_1, param_2);
    }
    
    // Some cryptographic operation
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    dataIn.pbData = NULL;
    dataIn.cbData = 0;
    CryptProtectData(&dataIn, L"", NULL, NULL, NULL, 0, &dataOut);
    
    // Signal RAS autodial if needed
    if (param_2 != 0) {
        HANDLE eventHandle = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"RasAutodialNewLogonUser");
        if (eventHandle != NULL) {
            SetEvent(eventHandle);
            CloseHandle(eventHandle);
        }
    }
}

int IsSubDesktopSession(void)
{
    WINSTATIONINFORMATION winStationInfo;
    DWORD returnLength;
    
    memset(&winStationInfo, 0, sizeof(winStationInfo));
    
    if (WinStationQueryInformationW(NULL, 0xFFFFFFFF, WinStationInformation, 
                                   &winStationInfo, sizeof(winStationInfo), &returnLength)) {
        if (winStationInfo.ConnectState & 0x8000) {
            return 1;
        }
    }
    
    return 0;
}

int IsTSAppCompatOn(void)
{
    HKEY hKey;
    DWORD valueType;
    DWORD tsAppCompat = 0;
    DWORD valueSize = sizeof(DWORD);
    
    if (bFirst != 0) {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Terminal Server", 
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            valueSize = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TSAppCompat", NULL, &valueType, 
                                (LPBYTE)&tsAppCompat, &valueSize) == ERROR_SUCCESS) {
                bAppCompatOn = (tsAppCompat != 0);
            }
            RegCloseKey(hKey);
        }
        bFirst = 0;
    }
    
    return bAppCompatOn;
}

void LoadRemoteFontsAndInitMiscWorker(HINSTANCE param_1, unsigned char param_2)
{
    LoadRemoteFonts();
    InitializeMisc(param_1, param_2);
}

int PerformXForestLogonCheck(void)
{
    HANDLE processToken;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) {
        int result = CheckXForestLogon(processToken);
        CloseHandle(processToken);
        if (result == 1) {
            return 1;
        }
    }
    
    return 0;
}

unsigned char ProcesRemoteSessionInitialCommand(void)
{
    wchar_t* applicationName = NULL;
    wchar_t* workingDirectory = NULL;
    unsigned short flags = 0;
    DWORD sessionId;
    BOOL success;
    DWORD lastError;
    wchar_t expandedPath[258];
    wchar_t errorBuffer[256];
    wchar_t messageBuffer[512];
    wchar_t* messageText = NULL;
    
    success = WinStationGetInitialApplication(0xFFFFFFFF, &sessionId, (PWSTR)&applicationName, 
                                             (PWSTR)&workingDirectory, (PULONG)&flags);
    
    if (!success) {
        if (applicationName != NULL) {
            goto cleanup_and_logoff;
        }
        OutputDebugStringW(L"USERINIT: Logging off session since WinStationGetInitialApplication failed! \n");
        ExitWindowsEx(EWX_LOGOFF, 0);
        return 0;
    }
    
    if (applicationName == NULL) {
        goto success_cleanup;
    }
    
    if ((flags & 0xFF000000) == 0) {
        SetLastError(ERROR_ACCESS_DENIED);
        success = FALSE;
    } else {
        // Set working directory if provided
        if (workingDirectory != NULL) {
            DWORD expandedLength = ExpandEnvironmentStringsW(workingDirectory, expandedPath, 
                                                           sizeof(expandedPath)/sizeof(wchar_t));
            if (expandedLength != 0) {
                if (!SetCurrentDirectoryW(expandedPath)) {
                    OutputDebugStringW(L"USERINIT: Failed to set working directory %ws for SessionId %u\n");
                    success = FALSE;
                    goto error_handling;
                }
            } else {
                if (StringCchCopyW(expandedPath, sizeof(expandedPath)/sizeof(wchar_t), 
                                  workingDirectory) < 0) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    goto cleanup_and_logoff;
                }
                if (!SetCurrentDirectoryW(expandedPath)) {
                    OutputDebugStringW(L"USERINIT: Failed to set working directory %ws for SessionId %u\n");
                    success = FALSE;
                    goto error_handling;
                }
            }
        }
        
        // Check if ctfmon should run
        if (RegCheckCtfmon(NULL, 0)) {
            wchar_t ctfmonPath[260];
            DWORD ctfmonExpanded = ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\ctfmon.exe", 
                                                           ctfmonPath, sizeof(ctfmonPath)/sizeof(wchar_t));
            if (ctfmonExpanded == 0) {
                StringCchCopyW(ctfmonPath, sizeof(ctfmonPath)/sizeof(wchar_t), 
                              L"%SystemRoot%\\System32\\ctfmon.exe");
            }
            
            // REPLACED: ExecApplication(ctfmonPath, 0, 0, 0, 1, 0);
            if (!RunApplication(ctfmonPath, 0)) {
                OutputDebugStringW((LPCWSTR)"USERINIT: Failed to start ctfmon.exe in TS Single App mode ! \n");
            }
        }
        
        // Execute the main application
        unsigned short execFlags = ((flags & 0x00FF0000) != 0) ? 3 : 5;
        // REPLACED: success = ExecApplication(applicationName, 0, 0, 0, execFlags, 1);
        int waitForExit = (execFlags == 3) ? 1 : 0; // execFlags 3 = wait, 5 = don't wait
        success = RunApplication(applicationName, waitForExit);
    }

error_handling:
    lastError = GetLastError();
    
    // Set shell desktop switch event if not in sub-desktop session
    if (!IsSubDesktopSession()) {
        SetShellDesktopSwitchEvent();
    }
    
    if (!success) {
        // Format error message
        wchar_t* errorMessage = NULL;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                      NULL, lastError, 0, (wchar_t*)&errorMessage, 0, NULL);
        
        int stringId = (workingDirectory != NULL) ? IDS_ERROR_WORKINGDIR : 
                                                   IDS_ERROR_SHELLCOMMAND;
        
        if (LoadStringW(NULL, stringId, errorBuffer, sizeof(errorBuffer)/sizeof(wchar_t)) != 0) {
            messageText = (wchar_t*)LocalAlloc(LPTR, 1024);
            if (messageText != NULL) {
                if (errorMessage != NULL) {
                    StringCchPrintfW(messageText, 512, errorBuffer, errorMessage, applicationName);
                } else {
                    StringCchPrintfW(messageText, 512, errorBuffer, lastError, applicationName);
                }
            }
        }
        
        if (errorMessage != NULL) {
            LocalFree(errorMessage);
        }
        
        if (messageText != NULL) {
            MessageBoxW(NULL, messageText, NULL, MB_ICONERROR);
            LocalFree(messageText);
        }
    }

cleanup_and_logoff:
    if (applicationName != NULL) {
        WinStationFreeMemory(applicationName);
    }
    if (workingDirectory != NULL) {
        WinStationFreeMemory(workingDirectory);
    }
    
    if (!success) {
        ExitWindowsEx(EWX_LOGOFF, 0);
        return 0;
    }

success_cleanup:
    return 1;
}

int ProcessTermSrvIniFiles(wchar_t* param_1)
{
    int result = 0;
    
    if (IsTSAppCompatOn()) {
        result = (param_1 != NULL) ? 1 : 0;
        
        HMODULE tsappcmp = LoadLibraryExW(L"tsappcmp.dll", NULL, 0);
        if (tsappcmp != NULL) {
            FARPROC termsrvCheck = GetProcAddress(tsappcmp, "TermsrvCheckNewIniFiles");
            if (termsrvCheck != NULL) {
                termsrvCheck();
            }
            FreeLibrary(tsappcmp);
        }
    }
    
    return result;
}

unsigned char RegCheckCtfmon(wchar_t* param_1, unsigned long param_2)
{
    HKEY hKey;
    DWORD valueType;
    DWORD ctfmonValue = 0;
    DWORD valueSize = sizeof(DWORD);
    unsigned char result = 0;
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        valueSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ctfmon.exe", NULL, &valueType, 
                            (LPBYTE)&ctfmonValue, &valueSize) == ERROR_SUCCESS) {
            if (valueType == REG_DWORD && ctfmonValue == 1) {
                result = 1;
            }
        }
        RegCloseKey(hKey);
    }
    
    return result;
}

void SetShellDesktopSwitchEvent(void)
{
    HANDLE eventHandle = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"ShellDesktopSwitchEvent");
    if (eventHandle != NULL) {
        SetEvent(eventHandle);
        CloseHandle(eventHandle);
    }
}

int SetupHotKeyForKeyboardLayout(void)
{
    HKEY hKey;
    DWORD disposition;
    wchar_t ctfmonRunonce[] = L"ctfmon.exe";
    wchar_t ctfmonCommand[] = L"ctfmon.exe /n";
    DWORD hotkeyValue = 0x33;
    DWORD currentHotkey;
    DWORD valueSize = sizeof(DWORD);
    
    if (GetSystemMetrics(SM_REMOTESESSION) == 0) {
        return 1;
    }
    
    LANGID currentLayout = (LANGID)GetKeyboardLayout(0);
    LANGID defaultLangID = GetUserDefaultLangID();
    
    if (defaultLangID == currentLayout) {
        return 1;
    }
    
    // Add ctfmon to RunOnce
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Runonce", 
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, ctfmonRunonce, 0, REG_SZ, (BYTE*)ctfmonCommand, sizeof(ctfmonCommand));
        RegCloseKey(hKey);
    }
    
    // Setup keyboard layout hotkey
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Toggle", 0, NULL, 
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        valueSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Hotkey", NULL, NULL, (BYTE*)&currentHotkey, &valueSize) != ERROR_SUCCESS || 
            currentHotkey != hotkeyValue) {
            RegSetValueExW(hKey, L"Hotkey", 0, REG_DWORD, (BYTE*)&hotkeyValue, sizeof(DWORD));
            SystemParametersInfoW(SPI_SETKEYBOARDCUES, 0, NULL, 0);
        }
        RegCloseKey(hKey);
    }
    
    return 1;
}

int UserinitExt(void)
{
    return 1;
}

