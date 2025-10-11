#include <windows.h>
#include <winternl.h>
#include <evntprov.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>

// ETW Provider GUID for MUI setup
const GUID MUISETUP_ETW_PROVIDER = 
{0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

// ETW Event flags
const DWORD MUISETUP_ETW_EVENT_TEST_CLEAR_FLAG = 0x1;
const DWORD MUISETUP_ETW_EVENT_TEST_INIT_FLAG = 0x2;
const DWORD MUISETUP_ETW_EVENT_TEST_SWITCH_FLAG = 0x4;
const DWORD MUISETUP_ETW_EVENT_TEST_ADD_FLAG = 0x8;
const DWORD MUISETUP_ETW_EVENT_TEST_REMOVE_FLAG = 0x10;
const DWORD MUISETUP_ETW_EVENT_TEST_CALLBACK_FAILURE = 0x20;

BOOL CALLBACK WritePendingDeleteKey(LPWSTR pszLanguage, LONG_PTR lParam);
void TryMergeSystemPriFiles(void);
DWORD TestLogger(DWORD dwFlags, DWORD dwEventFlag, DWORD dwCallbackId, 
                LPCWSTR pszEventName, LPCWSTR pszParam1, LPCWSTR pszParam2);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}

ULONG64 GetInitTimeAsUINT64(void)
{
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize = sizeof(ULONG64);
    ULONG64 initTime = 0;
    LONG result;

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                          L"SYSTEM\\CurrentControlSet\\Control\\CMF", 
                          0, 
                          KEY_QUERY_VALUE, 
                          &hKey);
    
    if (result == ERROR_SUCCESS) {
        result = RegQueryValueExW(hKey, L"MuiInit", NULL, &dwType, 
                                 (LPBYTE)&initTime, &dwSize);
        if (result != ERROR_SUCCESS || dwType != REG_QWORD) {
            initTime = 0;
        }
        RegCloseKey(hKey);
    }
    
    return initTime;
}

HRESULT MuiEtwGetStringFromMultiString(LPCWSTR pMultiString, LPWSTR* ppSingleString)
{
    if (ppSingleString == NULL || *ppSingleString != NULL) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    LPWSTR pSingleString = NULL;
    size_t cchTotalLength = 0;
    size_t cchCurrentLength = 0;

    // Calculate total length needed
    if (pMultiString == NULL || pMultiString[0] == L'\0') {
        cchTotalLength = 8; // "(null)" + null terminator
    } else {
        LPCWSTR pCurrent = pMultiString;
        while (*pCurrent != L'\0') {
            hr = StringCchLengthW(pCurrent, STRSAFE_MAX_CCH, &cchCurrentLength);
            if (FAILED(hr)) break;
            
            cchTotalLength += cchCurrentLength + 1; // +1 for comma
            pCurrent += cchCurrentLength + 1;
        }
        if (cchTotalLength > 0) {
            cchTotalLength--; // Remove last comma
        }
    }

    if (SUCCEEDED(hr)) {
        pSingleString = (LPWSTR)LocalAlloc(LPTR, (cchTotalLength + 1) * sizeof(WCHAR));
        if (pSingleString == NULL) {
            hr = E_OUTOFMEMORY;
        } else {
            pSingleString[0] = L'\0';
            
            if (pMultiString == NULL || pMultiString[0] == L'\0') {
                hr = StringCchCopyW(pSingleString, cchTotalLength + 1, L"(null)");
            } else {
                LPCWSTR pCurrent = pMultiString;
                BOOL bFirst = TRUE;
                
                while (*pCurrent != L'\0' && SUCCEEDED(hr)) {
                    if (!bFirst) {
                        hr = StringCchCatW(pSingleString, cchTotalLength + 1, L",");
                    }
                    if (SUCCEEDED(hr)) {
                        hr = StringCchCatW(pSingleString, cchTotalLength + 1, pCurrent);
                    }
                    pCurrent += wcslen(pCurrent) + 1;
                    bFirst = FALSE;
                }
            }
            
            if (FAILED(hr)) {
                LocalFree(pSingleString);
                pSingleString = NULL;
            } else {
                *ppSingleString = pSingleString;
            }
        }
    }

    return hr;
}

DWORD _MuipLaunchExecutable(LPCWSTR pszExecutable, LPCWSTR pszCommandLine)
{
    WCHAR szWindowsDir[MAX_PATH];
    WCHAR szExecutablePath[MAX_PATH];
    LPWSTR pszCommandLineCopy = NULL;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD dwResult = ERROR_SUCCESS;

	si.cb = sizeof(si);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = 0;
	si.cbReserved2 = 0;
	si.lpReserved2 = NULL;

	pi.hProcess = NULL;
	pi.hThread = NULL;
	pi.dwProcessId = 0;
	pi.dwThreadId = 0;

    // Get Windows directory
    if (GetWindowsDirectoryW(szWindowsDir, MAX_PATH) == 0) {
        return GetLastError();
    }

    // Build full path to mcbuilder.exe in system32
    HRESULT hr = StringCchCopyW(szExecutablePath, MAX_PATH, szWindowsDir);
	if (SUCCEEDED(hr)) {
		hr = StringCchCatW(szExecutablePath, MAX_PATH, L"\\system32\\mcbuilder.exe");
	}
    if (FAILED(hr)) {
        return HRESULT_CODE(hr);
    }

    // Duplicate command line if provided
    if (pszCommandLine != NULL) {
        size_t len = wcslen(pszCommandLine) + 1;
        pszCommandLineCopy = (LPWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
        if (pszCommandLineCopy == NULL) {
            return ERROR_OUTOFMEMORY;
        }
        StringCchCopyW(pszCommandLineCopy, len, pszCommandLine);
    }

    // Create the process
    if (!CreateProcessW(szExecutablePath,
                       pszCommandLineCopy,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_NO_WINDOW,  // 0x8004000
                       NULL,
                       NULL,
                       &si,
                       &pi)) {
        dwResult = GetLastError();
    } else {
        // Process launched successfully
        dwResult = ERROR_SUCCESS;
    }

    // Cleanup
    if (pi.hProcess != NULL) {
        CloseHandle(pi.hProcess);
    }
    if (pi.hThread != NULL) {
        CloseHandle(pi.hThread);
    }
    if (pszCommandLineCopy != NULL) {
        LocalFree(pszCommandLineCopy);
    }

    return dwResult;
}

DWORD OnMachineUILanguageClear(LPCWSTR pszLanguage, DWORD dwFlags)
{
    return TestLogger(dwFlags, MUISETUP_ETW_EVENT_TEST_CLEAR_FLAG, 0, 
                     L"OnUILanguageClear", NULL, NULL);
}

DWORD OnMachineUILanguageInit(LPCWSTR pszLanguage, DWORD dwFlags)
{
    HKEY hKey;
    FILETIME ftSystemTime;
    ULONG64 initTime;
    LONG result;

    // Record initialization time
    GetSystemTimeAsFileTime(&ftSystemTime);
    initTime = *(ULONG64*)&ftSystemTime;

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                          L"SYSTEM\\CurrentControlSet\\Control\\CMF", 
                          0, 
                          KEY_SET_VALUE, 
                          &hKey);
    if (result == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"MuiInit", 0, REG_QWORD, 
                      (const BYTE*)&initTime, sizeof(initTime));
        RegCloseKey(hKey);
    }

    // Create pending delete keys for preinstalled languages
    HKEY hMuiKey;
    result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Control\\MUI\\PreinstalledLanguages",
                            0, NULL, 0, KEY_WRITE, NULL, &hMuiKey, NULL);
    if (result == ERROR_SUCCESS) {
        EnumUILanguagesW(WritePendingDeleteKey, 0, (LONG_PTR)hMuiKey);
        RegCloseKey(hMuiKey);
    }

    TryMergeSystemPriFiles();

    return TestLogger(dwFlags, MUISETUP_ETW_EVENT_TEST_INIT_FLAG, 0, 
                     L"OnUILanguageInit", NULL, NULL);
}

DWORD OnMachineUILanguageSwitch(LPCWSTR pszOldLanguage, LPCWSTR pszNewLanguage, DWORD dwFlags)
{
    DWORD dwResult = ERROR_SUCCESS;
    LPWSTR pMultiString = NULL;
    LPWSTR pCommandLine = NULL;
    ULONG64 initTime;
    FILETIME ftCurrentTime;
    LARGE_INTEGER liInitTime, liCurrentTime;

    if (pszNewLanguage == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    initTime = GetInitTimeAsUINT64();
    GetSystemTimeAsFileTime(&ftCurrentTime);

    liInitTime.QuadPart = initTime;
    liCurrentTime.QuadPart = *(ULONG64*)&ftCurrentTime;

    // Check if enough time has passed since initialization (12 seconds)
    if (liCurrentTime.QuadPart - liInitTime.QuadPart >= 120000000) {
        HRESULT hr = MuiEtwGetStringFromMultiString(pszNewLanguage, &pMultiString);
        if (SUCCEEDED(hr)) {
            size_t cchCommandLine = wcslen(pMultiString) + 50;
            pCommandLine = (LPWSTR)LocalAlloc(LPTR, cchCommandLine * sizeof(WCHAR));
            if (pCommandLine != NULL) {
                hr = StringCchCopyW(pCommandLine, cchCommandLine, L"muiunattend.exe ");
                if (SUCCEEDED(hr)) {
                    hr = StringCchCatW(pCommandLine, cchCommandLine, pMultiString);
                    if (SUCCEEDED(hr)) {
                        dwResult = _MuipLaunchExecutable(L"muiunattend.exe", pCommandLine);
                    }
                }
                LocalFree(pCommandLine);
            } else {
                dwResult = ERROR_OUTOFMEMORY;
            }
            LocalFree(pMultiString);
        } else {
            dwResult = HRESULT_CODE(hr);
        }
    }

    TestLogger(dwFlags, MUISETUP_ETW_EVENT_TEST_SWITCH_FLAG, 0,
               L"OnMachineUILanguageSwitch", pszNewLanguage, pszOldLanguage);

    return dwResult;
}

DWORD OnUILanguageAdd(LPCWSTR pszLanguage, DWORD dwFlags)
{
    TryMergeSystemPriFiles();
    return TestLogger(dwFlags, MUISETUP_ETW_EVENT_TEST_ADD_FLAG, 0, 
                     L"OnUILanguageAdd", NULL, NULL);
}

DWORD OnUILanguageRemove(LPCWSTR pszLanguage, DWORD dwFlags)
{
    TryMergeSystemPriFiles();
    return TestLogger(dwFlags, MUISETUP_ETW_EVENT_TEST_REMOVE_FLAG, 0, 
                     L"OnUILanguageRemove", NULL, NULL);
}

DWORD TestLogger(DWORD dwFlags, DWORD dwEventFlag, DWORD dwCallbackId, 
                LPCWSTR pszEventName, LPCWSTR pszParam1, LPCWSTR pszParam2)
{
    REGHANDLE hEventReg = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    LPWSTR pMultiString1 = NULL;
    LPWSTR pMultiString2 = NULL;
    HKEY hCallbackKey = NULL;
    DWORD dwCallbackValue = 0;
    DWORD dwValueSize = sizeof(DWORD);

    // Register ETW provider
    dwResult = EventRegister(&MUISETUP_ETW_PROVIDER, NULL, NULL, &hEventReg);
    
    if (dwResult == ERROR_SUCCESS) {
        // Create event descriptor
        EVENT_DESCRIPTOR eventDesc = {0};
        eventDesc.Id = (USHORT)dwEventFlag;
        
        // Check if event is enabled
        if (EventEnabled(hEventReg, &eventDesc)) {
            // Convert multi-string parameters
            if (SUCCEEDED(MuiEtwGetStringFromMultiString(pszParam1, &pMultiString1)) &&
                SUCCEEDED(MuiEtwGetStringFromMultiString(pszParam2, &pMultiString2))) {
                
                size_t cchParam1, cchParam2;
                if (SUCCEEDED(StringCchLengthW(pszParam1, STRSAFE_MAX_CCH, &cchParam1)) &&
                    SUCCEEDED(StringCchLengthW(pszParam2, STRSAFE_MAX_CCH, &cchParam2))) {
                    
                    // Check for callback DLL
                    if (dwCallbackId != 0) {
                        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                         L"SYSTEM\\CurrentControlSet\\Control\\MUI\\CallbackDlls",
                                         0, KEY_QUERY_VALUE, &hCallbackKey) == ERROR_SUCCESS) {
                            RegQueryValueExW(hCallbackKey, (LPCWSTR)dwCallbackId, NULL, NULL,
                                           (LPBYTE)&dwCallbackValue, &dwValueSize);
                        }
                    }

                    // Prepare event data
                    EVENT_DATA_DESCRIPTOR eventData[4];
                    EventDataDescCreate(&eventData[0], pMultiString1, (ULONG)(cchParam1 + 1) * sizeof(WCHAR));
                    EventDataDescCreate(&eventData[1], pMultiString2, (ULONG)(cchParam2 + 1) * sizeof(WCHAR));
                    EventDataDescCreate(&eventData[2], &dwCallbackValue, sizeof(dwCallbackValue));
                    EventDataDescCreate(&eventData[3], &dwFlags, sizeof(dwFlags));

                    // Write the event
                    dwResult = EventWrite(hEventReg, &eventDesc, 4, eventData);
                }
            }

            // Log callback failure if needed
            if (dwResult != ERROR_SUCCESS && pszEventName != NULL) {
                size_t cchEventName;
                if (SUCCEEDED(StringCchLengthW(pszEventName, STRSAFE_MAX_CCH, &cchEventName))) {
                    EVENT_DATA_DESCRIPTOR failData[2];
                    EVENT_DESCRIPTOR callbackFailureEvent = {0};
                    callbackFailureEvent.Id = MUISETUP_ETW_EVENT_TEST_CALLBACK_FAILURE;
                    
                    EventDataDescCreate(&failData[0], pszEventName, (ULONG)(cchEventName + 1) * sizeof(WCHAR));
                    EventDataDescCreate(&failData[1], &dwResult, sizeof(dwResult));
                    EventWrite(hEventReg, &callbackFailureEvent, 2, failData);
                }
            }
        }
    }

    // Cleanup
    if (hCallbackKey != NULL) {
        RegCloseKey(hCallbackKey);
    }
    if (pMultiString1 != NULL) {
        LocalFree(pMultiString1);
    }
    if (pMultiString2 != NULL) {
        LocalFree(pMultiString2);
    }
    if (hEventReg != NULL) {
        EventUnregister(hEventReg);
    }

    return dwResult;
}

void TryMergeSystemPriFiles(void)
{
    HMODULE hMrmCore = LoadLibraryW(L"mrmcoreR.dll");
    if (hMrmCore != NULL) {
        FARPROC pfnMergeSystemPriFiles = GetProcAddress(hMrmCore, "MergeSystemPriFiles");
        if (pfnMergeSystemPriFiles != NULL) {
            pfnMergeSystemPriFiles();
        }
        FreeLibrary(hMrmCore);
    }
}

BOOL CALLBACK WritePendingDeleteKey(LPWSTR pszLanguage, LONG_PTR lParam)
{
    HKEY hKey = (HKEY)lParam;
    HKEY hSubKey;
    
    if (RegCreateKeyExW(hKey, pszLanguage, 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL) == ERROR_SUCCESS) {
        RegCloseKey(hSubKey);
    }
    
    return TRUE;
}

