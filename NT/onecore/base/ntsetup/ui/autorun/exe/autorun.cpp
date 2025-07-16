/*

Copyright (c) Microsoft Corporation
Reverse-engineered by github/masonleeback

*/

#define _HAS_SIZED_DEALLOCATION 1

#include <Windows.h>
#include <strsafe.h>

#include "resource.h"

#include <..\\..\\..\\lib\\uilib\\inc\\utlstring.h>
#include <wdslib.h>

void __cdecl operator delete(void* ptr, unsigned int) {
    ::operator delete(ptr);
}

void atr_MsgBox(HINSTANCE hInstance, const wchar_t* pszMessage)
{
    UTLString utlTitle;

    utlTitle.LoadStringW(hInstance, 0x65);
    if (utlTitle.m_lpsz && *utlTitle.m_lpsz && pszMessage) {
        MessageBoxW(NULL, pszMessage, utlTitle.m_lpsz, MB_ICONSTOP | MB_TOPMOST);
    }
    utlTitle.v_Free();
}

UTLString* atr_GetModPath()
{
    UTLString* dstPath = NULL;
    wchar_t* pszFile = NULL;
    wchar_t szFullName[MAX_PATH];
    wchar_t szModName[MAX_PATH];

    if (GetModuleFileName(0, szModName, MAX_PATH)) {
        if (GetFullPathName(szModName, MAX_PATH, szFullName, &pszFile)) {
            if (pszFile != NULL) {
                *pszFile = NULL;
                dstPath = new UTLString(szFullName);
            }
        }
    }

    return dstPath;
}

UTLString* atr_GetModFile()
{
    UTLString* dstPath = NULL;
    wchar_t* pszFile = NULL;
    wchar_t szFullName[MAX_PATH];
    wchar_t szModName[MAX_PATH];

    if (GetModuleFileName(0, szModName, MAX_PATH)) {
        if (GetFullPathName(szModName, MAX_PATH, szFullName, &pszFile)) {
            if (pszFile != NULL) {
                dstPath = new UTLString(pszFile);
            }
        }
    }

    return dstPath;
}

UTLString* atr_BuildCommandLine(const wchar_t* pszProcessFilePath, const wchar_t* pszProcessArguments)
{
    UTLString* utlCmdLine;

    if (pszProcessFilePath == NULL) {
        return NULL;
    }

    utlCmdLine = new UTLString(pszProcessFilePath);
    if (pszProcessArguments != NULL
        && !utlCmdLine->Append(L" ")
        || !utlCmdLine->Append(pszProcessArguments)) {
        if (utlCmdLine != NULL) {
            delete utlCmdLine;
        }
        utlCmdLine = NULL;
    }

    return utlCmdLine;
}

bool atr_DoesAutorunExist(HANDLE* phMutex)
{
    bool bExists = false;
    HANDLE hMutex;

    hMutex = CreateMutexW(NULL, NULL, L"Microsoft.Windows.Setup.Autorun-F2F632C7-AC94-4B7A-D06B-97D56ED9DC8B");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        bExists = true;
        CloseHandle(hMutex);
        hMutex = NULL;
    }
    if (phMutex != NULL) {
        *phMutex = hMutex;
        hMutex = NULL;
    }
    if (hMutex != NULL) {
        CloseHandle(hMutex);
    }

    return bExists;
}

BOOL CALLBACK atr_SetAsForegroundCB(HWND hwnd, LPARAM lParam)
{
    DWORD dwPid;
    BOOL bResult = TRUE;

    GetWindowThreadProcessId(hwnd, &dwPid);
    if (~GetWindowLong(hwnd, GWL_USERDATA) == dwPid)
    {
        SetForegroundWindow(hwnd);
        lParam = 1;
        ShowWindow(hwnd, 9);
        bResult = FALSE;
    }

    return bResult;
}

DWORD atr_RunAutorun(HINSTANCE hInstance, const wchar_t* lpDllPath, const wchar_t* lpCmdLine)
{
    HMODULE hAutorunDll;
    FARPROC StartAutorun;
    bool bDllFound = false;
    DWORD dwResult;
    UTLString utlMsg;

    hAutorunDll = LoadLibraryEx(lpDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hAutorunDll != NULL) {
        StartAutorun = GetProcAddress(hAutorunDll, "StartAutorun");
        if (StartAutorun != NULL) {
            dwResult = ((int (__thiscall*)(FARPROC, const wchar_t*))StartAutorun)(StartAutorun, lpCmdLine);
            bDllFound = true;
        } else {
            dwResult = GetLastError();
        }
        FreeLibrary(hAutorunDll);
    } else {
        dwResult = GetLastError();
    }
    if (bDllFound == false) {
        utlMsg.FormatMessageW(hInstance, IDS_NOTFOUND, L"autorun.dll", dwResult);
        atr_MsgBox(hInstance, utlMsg.m_lpsz);
        utlMsg.v_Free();
    }

    return dwResult;
}

HRESULT LaunchExeAndWait(const wchar_t* pszCommandLine, PDWORD pdwExitCode) {
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION pi;
    HRESULT hrResult = S_OK;

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    if (CreateProcess(NULL, (LPWSTR)pszCommandLine, NULL, NULL, NULL, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo, &pi))
    {
        if (WaitForSingleObject(pi.hProcess, INFINITE)) {
            hrResult = E_UNEXPECTED;
        } else if (pdwExitCode != NULL) {
            if (GetExitCodeProcess(pi.hProcess, pdwExitCode) == NULL) {
                hrResult = GetLastError();
                if (hrResult > S_OK) {
                    hrResult = hrResult | 0x80070000; // todo: figure this out
                }
            }
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        hrResult = GetLastError();
        if (hrResult > S_OK) {
            hrResult = hrResult | 0x80070000; // todo: figure this out
        }
    }
    return hrResult;
}

HRESULT CheckMultiArchAutoRunPath(const wchar_t* pszModulePath, const wchar_t* pszModuleFileName, const wchar_t* lpszCmdLine, PDWORD pdwExitCode)
{
    SYSTEM_INFO SysInfo;
    DWORD dwFileAttributes;
    HRESULT hrResult;
    UTLString* utlCommandLine;
    wchar_t* pszProcessArguments;
    const wchar_t* pszProcessorArchitecture;
    wchar_t szAutoRunArchExePath[MAX_PATH];

    pszProcessArguments = (wchar_t*)lpszCmdLine;
    if (pszModulePath == NULL || *pszModulePath == NULL
        || pszModuleFileName == NULL || *pszModuleFileName == NULL) {
        return E_INVALIDARG;
    }

    ZeroMemory(&SysInfo, sizeof(SysInfo));
    GetNativeSystemInfo(&SysInfo);
    if (SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        pszProcessorArchitecture = L"x86";
    } else if (SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        pszProcessorArchitecture = L"x64";
    } else {
        return E_UNEXPECTED;
    }

    ZeroMemory(szAutoRunArchExePath, sizeof(szAutoRunArchExePath));

    if (pszModulePath[wcslen(pszModulePath) - 1] == '\\') {
        hrResult = StringCchPrintfW(szAutoRunArchExePath, MAX_PATH, L"%s%s\\%s", pszModulePath, pszProcessorArchitecture, pszModuleFileName);
    } else {
        hrResult = StringCchPrintfW(szAutoRunArchExePath, MAX_PATH, L"%s\\%s\\%s", pszModulePath, pszProcessorArchitecture, pszModuleFileName);
    }

    if (hrResult >= S_OK) {
        dwFileAttributes = GetFileAttributes(szAutoRunArchExePath);
        if (dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
            hrResult = GetLastError();
            if (hrResult > S_OK) {
                hrResult = hrResult | 0x80070000; // what error is this ? can't figure it out from winerror
            }
            return hrResult;
        }
        if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            return ERROR_BAD_PATHNAME;
        }
        utlCommandLine = atr_BuildCommandLine(szAutoRunArchExePath, pszProcessArguments);
        if (utlCommandLine == NULL) {
            hrResult = GetLastError();
            if (hrResult > S_OK) {
                hrResult = hrResult | 0x80070000; // what error is this ? can't figure it out from winerror
            }
        } else {
            LaunchExeAndWait(utlCommandLine->m_lpsz, pdwExitCode);
        }
    }

    return hrResult;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    wchar_t wCmdLine[512];
    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, wCmdLine, 512);
    int iResult;
    DWORD dwRetCode;
    HANDLE hMutex;

    UTLString* utlModPath;
    UTLString* utlModFile;

    hMutex = NULL;
    if (atr_DoesAutorunExist(NULL)) {
        EnumWindows(atr_SetAsForegroundCB, (LPARAM)&hMutex);
        return ERROR_ALREADY_EXISTS;
    }

    utlModPath = atr_GetModPath();
    utlModFile = atr_GetModFile();

    if (utlModPath != NULL && utlModFile != NULL)
    {
        wchar_t* pwWorkingDir = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(wchar_t));
if (pwWorkingDir) {
    GetCurrentDirectoryW(MAX_PATH, pwWorkingDir);
}
        if (pwWorkingDir != NULL) {
            SetEnvironmentVariable(L"ORIGINAL_SETUP_WORKINGDIR_ENV_VAR", pwWorkingDir);
            HeapFree(GetProcessHeap(), NULL, pwWorkingDir);
        }

        if (CheckMultiArchAutoRunPath(utlModPath->m_lpsz, utlModFile->m_lpsz, wCmdLine, &dwRetCode) < S_OK) {
            if (atr_DoesAutorunExist(&hMutex)) {
                utlModFile = NULL;
                EnumWindows(atr_SetAsForegroundCB, (LPARAM)&utlModFile);
                dwRetCode = ERROR_ALREADY_EXISTS;
            } else {
                utlModPath->Append(L"Sources\\");
                SetCurrentDirectory(utlModPath->m_lpsz);
                utlModPath->Append(L"autorun.dll");
                dwRetCode = atr_RunAutorun(hInstance, utlModPath->m_lpsz, wCmdLine);
            }

        }
        delete utlModPath;
        iResult = dwRetCode;
    }
    else {
        iResult = GetLastError();
    }
    if (hMutex != NULL) {
        CloseHandle(hMutex);
        iResult = dwRetCode;
    }

    return iResult;
}




