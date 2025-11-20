#include "enumreg.h"
#include <comdef.h>
#include <initguid.h>
#include <strsafe.h>

// Global variables
HINSTANCE hInputLib = NULL;
HINSTANCE hMsCtfMig = NULL;
bool CMyRegKey::vftable = false;

class IMigrationContext;
HRESULT __cdecl LogMessage(IMigrationContext* pMigrationContext, UINT dwLogLevel, wchar_t* pszFormat, ...);
int __cdecl DisableObsoleteCHSIMEs(IMigrationContext* pMigrationContext, wchar_t* pszParam);
UINT __cdecl RemoveObsoleteCHSHKLMRegkey(IMigrationContext* pMigrationContext);

// Function pointer types
typedef int (__cdecl *INSTALL_LAYOUT_OR_TIP_USER_REG)(wchar_t*, wchar_t*, wchar_t*, wchar_t*, ULONG);
typedef HRESULT (__cdecl *LOAD_BIN_FROM_ENABLED_LAYOUT_OR_TIP_FILE)(IMigrationContext*, void*, ULONG, ULONG*, wchar_t*, wchar_t*);
typedef HRESULT (__cdecl *LOAD_REG_FROM_FILE)(IMigrationContext*, HKEY*, wchar_t*, wchar_t*);
typedef HRESULT (__cdecl *LOG_MESSAGE_LAYOUT_OR_TIP)(IMigrationContext*, UINT, wchar_t*, wchar_t*, void*);
typedef HRESULT (__cdecl *SAVE_KEYS_TO_FILE)(IMigrationContext*, HKEY*, wchar_t*, void*, wchar_t*);

// Function pointers
INSTALL_LAYOUT_OR_TIP_USER_REG fpInstallLayoutOrTipUserReg = NULL;
LOAD_BIN_FROM_ENABLED_LAYOUT_OR_TIP_FILE fpLoadBinFromEnabledLayoutOrTipFile = NULL;
LOAD_REG_FROM_FILE fpLoadRegFromFile = NULL;
LOG_MESSAGE_LAYOUT_OR_TIP fpLogMessageLayoutOrTip = NULL;
SAVE_KEYS_TO_FILE fpSaveKeysToFile = NULL;

extern DWORD __cdecl TransNum(wchar_t* pszString, wchar_t** ppszEndPtr);

struct LAYOUTORTIPPROFILE {
    DWORD dwProfileType;
    DWORD dwFlags;
    GUID  guidProfile;
    wchar_t szId[256];
};
HRESULT __cdecl InternalLogMessageLayoutOrTip(IMigrationContext* pMigrationContext, UINT dwLogLevel, wchar_t* pszMessage, wchar_t* pszParam1, LAYOUTORTIPPROFILE* pProfile);
HRESULT __cdecl InternalLoadBinFromEnabledLayoutOrTipFile(IMigrationContext* pMigrationContext, void** ppBuffer, ULONG dwReserved, ULONG* pcbBuffer, wchar_t* pszParam1, wchar_t* pszParam2);
int __cdecl IsLayoutOrTipForTableDrivenIMM32(LAYOUTORTIPPROFILE* pProfile, ULONG* pdwIndex);
UINT __cdecl SaveE0xxxx04IMERegToFile_ForE0xxxx04(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, wchar_t* pszParam);
int __cdecl SaveKeysToFile_ForE0xxxx04(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, wchar_t* pszParam);

struct IME_MAPPING {
    DWORD dwLayout;
    WORD langid;
    wchar_t** ppszModules;
    wchar_t** ppszSharedModules;
};

struct TIP_MAPPING {
    WORD langid;
    GUID guidProfile;
};

struct MAPPING_TABLE_ENTRY {
    union {
        IME_MAPPING ime;
        TIP_MAPPING tip;
    };
};

MAPPING_TABLE_ENTRY rgMappingTable[] = {
    // Entry 0 - IME
    { { 0, 0, NULL, NULL } },
    // Entry 1 - IME  
    { { 0, 0, NULL, NULL } },
    // Entry 2 - TIP
    { { 0, 0 } },
    // Entry 3 - IME
    { { 0, 0, NULL, NULL } }
};

// Global instance
HINSTANCE ghInstance = NULL;

// Constants
const wchar_t* c_rgszObsoleteCHSIMEsList[] = {
    L"E0200804",
    L"E0210804", 
    L"E0220804"
};

// IMigrationContext interface
class IMigrationContext {
public:
    _bstr_t __thiscall GetWorkingDir(IMigrationContext* pThis);
    _bstr_t __thiscall GetUserNameW(IMigrationContext* pThis);
    _bstr_t GetDomain(IMigrationContext* pThis);
    int __thiscall SendLogMessage(UINT dwLogLevel, _bstr_t bstrMessage);
    _bstr_t __thiscall GetUserSidString(IMigrationContext* pThis);
};

int __cdecl _InternalInstallLayoutOrTipUserReg(IMigrationContext* pMigrationContext, wchar_t* pszUserReg, wchar_t* pszSystemReg, wchar_t* pszLayoutTip, wchar_t* pszFlags, ULONG dwFlags)
{
    if (hInputLib == NULL) {
        hInputLib = LoadLibraryExW(L"input.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    
    if (fpInstallLayoutOrTipUserReg == NULL) {
        fpInstallLayoutOrTipUserReg = (INSTALL_LAYOUT_OR_TIP_USER_REG)GetProcAddress(hInputLib, "InstallLayoutOrTipUserReg");
        if (fpInstallLayoutOrTipUserReg == NULL) {
            DWORD dwError = GetLastError();
            LogMessage(pMigrationContext, 2, L"GetProcAddress(InstallLayoutOrTipUserReg) error hInputLib=%08x, gle=%d", hInputLib, dwError);
            return 0;
        }
    }
    
    return fpInstallLayoutOrTipUserReg(pszUserReg, NULL, NULL, pszLayoutTip, dwFlags);
}

HINSTANCE __cdecl _LoadMigrationLibrary(IMigrationContext* pMigrationContext, wchar_t* pszLibraryName)
{
    wchar_t szModulePath[MAX_PATH] = {0};
    wchar_t szDirectory[MAX_PATH] = {0};
    wchar_t szFullPath[MAX_PATH * 2] = {0};
    HINSTANCE hLibrary = NULL;
    
    if (GetModuleFileNameW(NULL, szModulePath, MAX_PATH) != 0) {
        if (GetFullPathNameW(szModulePath, MAX_PATH, szDirectory, NULL) != 0 && szDirectory[0] != L'\0') {
            wchar_t* pLastBackslash = (wchar_t*)wcsrchr(szDirectory, L'\\');
            if (pLastBackslash != NULL) {
                *pLastBackslash = L'\0';
                
                StringCchCopyW(szFullPath, MAX_PATH * 2, szDirectory);
                StringCchCatW(szFullPath, MAX_PATH * 2, L"\\");
                StringCchCatW(szFullPath, MAX_PATH * 2, pszLibraryName);
                
                hLibrary = LoadLibraryExW(szFullPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                if (hLibrary == NULL) {
                    DWORD dwError = GetLastError();
                    
                    UINT uSysDirLen = GetSystemDirectoryW(szModulePath, MAX_PATH);
                    if (uSysDirLen != 0 && uSysDirLen < MAX_PATH) {
                        if (szModulePath[uSysDirLen - 1] != L'\\') {
                            StringCchCatW(szModulePath, MAX_PATH, L"\\");
                        }
                        StringCchCatW(szModulePath, MAX_PATH, pszLibraryName);
                        
                        hLibrary = LoadLibraryW(szModulePath);
                        if (hLibrary == NULL) {
                            DWORD dwError2 = GetLastError();
                            LogMessage(pMigrationContext, 2, L"LoadLibraryEx error (%s) gle=%d", szFullPath, dwError);
                            LogMessage(pMigrationContext, 2, L"LoadLibrary error (%s) gle=%d", szModulePath, dwError2);
                        } else {
                            LogMessage(pMigrationContext, 0, L"LoadLibrary succeed (%s)", szModulePath);
                        }
                    }
                } else {
                    LogMessage(pMigrationContext, 0, L"LoadLibraryEx succeed (%s)", szFullPath);
                }
            }
        }
    }
    
    return hLibrary;
}

int __cdecl ApplySuccess(IMigrationContext* pMigrationContext)
{
    LogMessage(pMigrationContext, 0, L"TableTextServiceMig!ApplySuccess() start ...");
    
    _bstr_t bstrUserSid;
    pMigrationContext->GetUserSidString((IMigrationContext*)&bstrUserSid);
    
    int iResult = -0x7fffbffb; // E_FAIL
    
    if (bstrUserSid.length() == 0) {
        DisableObsoleteCHSIMEs(pMigrationContext, NULL);
        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!ApplySuccess() remove HKLM Registries");
        iResult = RemoveObsoleteCHSHKLMRegkey(pMigrationContext);
        if (iResult < 0) {
            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!ApplySuccess() remove HKLM Registries error");
        }
    } else {
        DisableObsoleteCHSIMEs(pMigrationContext, NULL);
    }
    
    LogMessage(pMigrationContext, 0, L"TableTextServiceMig!ApplySuccess() end ...");
    return iResult;
}

int __cdecl ApplyUserRegWithFilter(IMigrationContext* pMigrationContext, wchar_t* pszWorkingDir, wchar_t* pszUserSid)
{
    void* pBuffer = NULL;
    ULONG cbBuffer = 0;
    int iResult = 0;
    
    HRESULT hr = InternalLoadBinFromEnabledLayoutOrTipFile(pMigrationContext, &pBuffer, 0, &cbBuffer, NULL, NULL);
    
    if (SUCCEEDED(hr)) {
        if (cbBuffer == 0) {
            LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: no enabled install layout");
        } else {
            LAYOUTORTIPPROFILE* pProfiles = (LAYOUTORTIPPROFILE*)pBuffer;
            DWORD dwProfileCount = cbBuffer / sizeof(LAYOUTORTIPPROFILE);
            
            for (DWORD i = 0; i < dwProfileCount; i++) {
                LAYOUTORTIPPROFILE* pProfile = &pProfiles[i];
                
                int bIsTableDriven = IsLayoutOrTipForTableDrivenIMM32(pProfile, NULL);
                if (bIsTableDriven) {
                    if (pProfile->dwProfileType == 0) {
                        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: This layout has been skiped by obsolete.");
                        
                        iResult = _InternalInstallLayoutOrTipUserReg(pMigrationContext, NULL, NULL, pProfile->szId, L"1", 0);
                        if (iResult == 0) {
                            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg(uninstall obsolete layout) error");
                        } else {
                            LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg(uninstall obsolete layout) succeeded to install");
                        }
                    } else {
                        iResult = _InternalInstallLayoutOrTipUserReg(pMigrationContext, NULL, NULL, pProfile->szId, L"1", 0);
                        if (iResult == 0) {
                            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg(uninstall legacy layout) error");
                        } else {
                            LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg(uninstall legacy layout) succeeded to install");
                        }
                        
                        wchar_t szNewLayoutTip[260];
                        StringCchPrintfW(szNewLayoutTip, 260, L"%04x:%s%s", 
                                        pProfile->dwFlags, 
                                        pProfile->szId, 
                                        L"");
                        
                        iResult = _InternalInstallLayoutOrTipUserReg(pMigrationContext, NULL, NULL, szNewLayoutTip, L"2", 0);
                        if (iResult == 0) {
                            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg error");
                        } else {
                            LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: InstallLayoutOrTipUserReg succeeded to install");
                        }
                    }
                }
            }
            
            delete[] pBuffer;
        }
    }
    
    return iResult;
}

int __cdecl CLSIDToStringW(GUID* pGuid, wchar_t* pszString)
{
    if (!pGuid || !pszString) return 0;
    
    StringCchPrintfW(pszString, 39, 
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        pGuid->Data1, pGuid->Data2, pGuid->Data3,
        pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
        pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]);
    
    return 1;
}

int __cdecl DisableObsoleteCHSIMEs(IMigrationContext* pMigrationContext, wchar_t* pszParam)
{
    for (int i = 0; i < 3; i++) {
        int iResult = _InternalInstallLayoutOrTipUserReg(pMigrationContext, NULL, NULL, (wchar_t*)c_rgszObsoleteCHSIMEsList[i], (wchar_t*)L"1", 0);
        if (iResult != 0) {
            LogMessage(pMigrationContext, 0, L"DisableObsoleteCHSIMEs : InstallLayoutOrTipUserReg succeeded to uninstall %s", c_rgszObsoleteCHSIMEsList[i]);
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        ghInstance = hInstance;
    }
    return TRUE;
}

int __cdecl Gather(IMigrationContext* pMigrationContext)
{
    LogMessage(pMigrationContext, 0, L"TableTextServiceMig!Gather() start ...");
    
    _bstr_t bstrWorkingDir;
    _bstr_t bstrUserSid;
    
    pMigrationContext->GetWorkingDir((IMigrationContext*)&bstrWorkingDir);
    pMigrationContext->GetUserSidString((IMigrationContext*)&bstrUserSid);
    
    int iResult = 0;
    
    if (bstrUserSid.length() == 0) {
        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!Gather :: System context");
        
        wchar_t szRegFilePath[MAX_PATH] = {0};
        StringCchCopyW(szRegFilePath, MAX_PATH, bstrWorkingDir);
        StringCchCatW(szRegFilePath, MAX_PATH, L"\\");
        StringCchCatW(szRegFilePath, MAX_PATH, L"HKLM-E0xxxx04IME.reg");
        
        iResult = SaveE0xxxx04IMERegToFile_ForE0xxxx04(pMigrationContext, NULL, szRegFilePath, NULL);
    }
    
    return iResult;
}

int __cdecl HexStringToDword(wchar_t** ppszString, DWORD* pdwValue, int cchMax, wchar_t chTerminator)
{
    if (!ppszString || !pdwValue) return 0;
    
    *pdwValue = 0;
    int cchProcessed = 0;
    
    while (cchProcessed < cchMax && **ppszString != L'\0') {
        wchar_t ch = **ppszString;
        DWORD dwDigitValue = 0;
        
        if (ch >= L'0' && ch <= L'9') {
            dwDigitValue = ch - L'0';
        } else if (ch >= L'A' && ch <= L'F') {
            dwDigitValue = ch - L'A' + 10;
        } else if (ch >= L'a' && ch <= L'f') {
            dwDigitValue = ch - L'a' + 10;
        } else {
            return 0;
        }
        
        *pdwValue = (*pdwValue << 4) | dwDigitValue;
        (*ppszString)++;
        cchProcessed++;
        
        if (chTerminator != 0 && **ppszString == chTerminator) {
            (*ppszString)++;
            return 1;
        }
    }
    
    return 1;
}

HRESULT __cdecl InternalLoadBinFromEnabledLayoutOrTipFile(IMigrationContext* pMigrationContext, void** ppBuffer, ULONG dwReserved, ULONG* pcbBuffer, wchar_t* pszParam1, wchar_t* pszParam2)
{
    if (hMsCtfMig == NULL) {
        hMsCtfMig = _LoadMigrationLibrary(pMigrationContext, L"msctfmig.dll");
    }
    
    if (fpLoadBinFromEnabledLayoutOrTipFile == NULL) {
        fpLoadBinFromEnabledLayoutOrTipFile = (LOAD_BIN_FROM_ENABLED_LAYOUT_OR_TIP_FILE)GetProcAddress(hMsCtfMig, "LoadBinFromEnabledLayoutOrTipFile");
        if (fpLoadBinFromEnabledLayoutOrTipFile == NULL) {
            DWORD dwError = GetLastError();
            LogMessage(pMigrationContext, 2, L"GetProcAddress(LoadBinFromEnabledLayoutOrTipFile) error hMsCtfMig=%08x, gle=%d", hMsCtfMig, dwError);
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    
    return fpLoadBinFromEnabledLayoutOrTipFile(pMigrationContext, ppBuffer, dwReserved, pcbBuffer, pszParam1, pszParam2);
}

HRESULT __cdecl InternalLoadRegFromFile(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, wchar_t* pszSubKey)
{
    if (hMsCtfMig == NULL) {
        hMsCtfMig = _LoadMigrationLibrary(pMigrationContext, L"msctfmig.dll");
    }
    
    if (fpLoadRegFromFile == NULL) {
        fpLoadRegFromFile = (LOAD_REG_FROM_FILE)GetProcAddress(hMsCtfMig, "LoadRegFromFile");
        if (fpLoadRegFromFile == NULL) {
            DWORD dwError = GetLastError();
            LogMessage(pMigrationContext, 2, L"GetProcAddress(LoadRegFromFile) error hMsCtfMig=%08x, gle=%d", hMsCtfMig, dwError);
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    
    return fpLoadRegFromFile(pMigrationContext, phKey, pszFileName, pszSubKey);
}

HRESULT __cdecl InternalLogMessageLayoutOrTip(IMigrationContext* pMigrationContext, UINT dwLogLevel, wchar_t* pszMessage, wchar_t* pszParam1, LAYOUTORTIPPROFILE* pProfile)
{
    if (hMsCtfMig == NULL) {
        hMsCtfMig = _LoadMigrationLibrary(pMigrationContext, L"msctfmig.dll");
    }
    
    if (fpLogMessageLayoutOrTip == NULL) {
        fpLogMessageLayoutOrTip = (LOG_MESSAGE_LAYOUT_OR_TIP)GetProcAddress(hMsCtfMig, "LogMessageLayoutOrTip");
        if (fpLogMessageLayoutOrTip == NULL) {
            DWORD dwError = GetLastError();
            LogMessage(pMigrationContext, 2, L"GetProcAddress(LogMessageLayoutOrTip) error hMsCtfMig=%08x, gle=%d", hMsCtfMig, dwError);
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    
    return fpLogMessageLayoutOrTip(pMigrationContext, dwLogLevel, pszMessage, pszParam1, pProfile);
}

HRESULT __cdecl InternalSaveKeysToFile(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, void* pvReserved, wchar_t* pszSubKey)
{
    if (hMsCtfMig == NULL) {
        hMsCtfMig = _LoadMigrationLibrary(pMigrationContext, L"msctfmig.dll");
    }
    
    if (fpSaveKeysToFile == NULL) {
        fpSaveKeysToFile = (SAVE_KEYS_TO_FILE)GetProcAddress(hMsCtfMig, "SaveKeysToFile");
        if (fpSaveKeysToFile == NULL) {
            DWORD dwError = GetLastError();
            LogMessage(pMigrationContext, 2, L"GetProcAddress(SaveKeysToFile) error hMsCtfMig=%08x, gle=%d", hMsCtfMig, dwError);
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    
    return fpSaveKeysToFile(pMigrationContext, phKey, pszFileName, pvReserved, pszSubKey);
}

int __cdecl IsFileExist(wchar_t* pszFilePath)
{
    HANDLE hFile = CreateFileW(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    CloseHandle(hFile);
    return 1;
}

int __cdecl IsLayoutOrTipForTableDrivenIMM32(LAYOUTORTIPPROFILE* pProfile, ULONG* pdwIndex)
{
    if (pProfile->dwProfileType == 2) {
        for (int i = 0; i < 4; i++) {
            if (rgMappingTable[i].tip.langid != 0) {
                if (pProfile->guidProfile == rgMappingTable[i].tip.guidProfile) {
                    if (pdwIndex) *pdwIndex = i;
                    return 1;
                }
            }
        }
    } else if (pProfile->dwProfileType == 0) {
        wchar_t* pszLayout = pProfile->szId;
        DWORD dwLayout = wcstoul(pszLayout, NULL, 16);
        
        for (int i = 0; i < 4; i++) {
            if (rgMappingTable[i].ime.dwLayout != 0) {
                if (dwLayout == rgMappingTable[i].ime.dwLayout) {
                    if (pdwIndex) *pdwIndex = i;
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

HRESULT __cdecl LogMessage(IMigrationContext* pMigrationContext, UINT dwLogLevel, wchar_t* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    
    wchar_t szMessage[512];
    StringCchVPrintfW(szMessage, 512, pszFormat, args);
    
    va_end(args);
    
	_bstr_t bstrMessage(szMessage);
    return pMigrationContext->SendLogMessage(dwLogLevel, bstrMessage);
}

UINT __cdecl PreApply(IMigrationContext* pMigrationContext)
{
    LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply() start ...");
    
    _bstr_t bstrWorkingDir;
    _bstr_t bstrUserSid;
    
    pMigrationContext->GetWorkingDir((IMigrationContext*)&bstrWorkingDir);
    pMigrationContext->GetUserSidString((IMigrationContext*)&bstrUserSid);
    
    UINT uResult = 0;
    
    if (bstrUserSid.length() == 0) {
        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: System context");
        
        wchar_t szRegFilePath[MAX_PATH] = {0};
        StringCchCopyW(szRegFilePath, MAX_PATH, bstrWorkingDir);
        StringCchCatW(szRegFilePath, MAX_PATH, L"\\");
        StringCchCatW(szRegFilePath, MAX_PATH, L"HKLM-E0xxxx04IME.reg");
        
        HKEY hTempKey = NULL;
        DWORD dwDisposition = 0;
        
        LONG lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, 
                                      L"System\\CurrentControlSet\\Control\\Keyboard Layouts MigrationTemp",
                                      0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hTempKey, &dwDisposition);
        
        if (lResult == ERROR_SUCCESS) {
            HRESULT hr = InternalLoadRegFromFile(pMigrationContext, &hTempKey, szRegFilePath, NULL);
            if (SUCCEEDED(hr)) {
                CEnumRegKey enumKey(hTempKey, NULL);
                wchar_t szSubKeyName[256];
                
                while (enumKey.Next(szSubKeyName, 256) == 0) {
                    DWORD dwLayout = wcstoul(szSubKeyName, NULL, 16);
                    
                    if ((dwLayout & 0xF0000000) == 0xE0000000) {
                        WORD wLayoutHigh = HIWORD(dwLayout);
                        if ((wLayoutHigh & 0xF000) == 0xE000 && 
                            (wLayoutHigh & 0x0FFF) > 0x1F && 
                            (LOWORD(dwLayout) & 0x03FF) == 0x0004) {
                            
                            wchar_t szSystemDir[MAX_PATH];
                            UINT uSysDirLen = GetSystemDirectoryW(szSystemDir, MAX_PATH);
                            
                            if (uSysDirLen > 0 && uSysDirLen < MAX_PATH) {
                                if (szSystemDir[uSysDirLen - 1] != L'\\') {
                                    StringCchCatW(szSystemDir, MAX_PATH, L"\\");
                                }
                                
                                for (int i = 0; i < 4; i++) {
                                    if (rgMappingTable[i].ime.dwLayout == dwLayout) {
                                        wchar_t** ppszModules = rgMappingTable[i].ime.ppszModules;
                                        if (ppszModules) {
                                            for (int j = 0; ppszModules[j] != NULL; j++) {
                                                wchar_t szModulePath[MAX_PATH];
                                                StringCchCopyW(szModulePath, MAX_PATH, szSystemDir);
                                                StringCchCatW(szModulePath, MAX_PATH, ppszModules[j]);
                                                
                                                if (IsFileExist(szModulePath)) {
                                                    if (DeleteFileW(szModulePath)) {
                                                        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: DeleteFile %s succeeded", szModulePath);
                                                    } else {
                                                        DWORD dwError = GetLastError();
                                                        LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: DeleteFile %s error gle=%d", szModulePath, dwError);
                                                    }
                                                }
                                            }
                                        }
                                        
                                        wchar_t** ppszSharedModules = rgMappingTable[i].ime.ppszSharedModules;
                                        if (ppszSharedModules) {
                                            for (int j = 0; ppszSharedModules[j] != NULL; j++) {
                                                wchar_t szModulePath[MAX_PATH];
                                                StringCchCopyW(szModulePath, MAX_PATH, szSystemDir);
                                                StringCchCatW(szModulePath, MAX_PATH, ppszSharedModules[j]);
                                                
                                                if (IsFileExist(szModulePath)) {
                                                    if (DeleteFileW(szModulePath)) {
                                                        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: DeleteFile %s succeeded", szModulePath);
                                                    } else {
                                                        DWORD dwError = GetLastError();
                                                        LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: DeleteFile %s error gle=%d", szModulePath, dwError);
                                                    }
                                                }
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                
                RegCloseKey(hTempKey);
                hTempKey = NULL;
                
                CMyRegKey hkLM;
                if (hkLM.Open(HKEY_LOCAL_MACHINE, L"", KEY_READ | KEY_WRITE) == ERROR_SUCCESS) {
                    hkLM.RecurseDeleteKey(L"System\\CurrentControlSet\\Control\\Keyboard Layouts MigrationTemp");
                    
                    uResult = ApplyUserRegWithFilter(pMigrationContext, bstrWorkingDir, bstrUserSid);
                    if ((int)uResult < 0) {
                        LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: ApplyUserReg(%s) error", bstrWorkingDir);
                    }
                } else {
                    LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: CMyRegKey::Open error (HKEY_LOCAL_MACHINE)");
                    uResult = 0x80070005; // E_ACCESSDENIED
                }
            } else {
                LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: LoadRegFromFile error");
                uResult = hr;
            }
            
            if (hTempKey) {
                RegCloseKey(hTempKey);
            }
        } else {
            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: RegCreateKeyExW error");
            uResult = HRESULT_FROM_WIN32(lResult);
        }
    } else {
        _bstr_t bstrUserName;
        _bstr_t bstrDomain;
        
        pMigrationContext->GetUserNameW((IMigrationContext*)&bstrUserName);
        pMigrationContext->GetDomain((IMigrationContext*)&bstrDomain);
        
        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply :: User context (%s\\%s :: %s)", 
                  (wchar_t*)bstrDomain, (wchar_t*)bstrUserName, (wchar_t*)bstrUserSid);
        
        uResult = ApplyUserRegWithFilter(pMigrationContext, bstrWorkingDir, bstrUserSid);
        if ((int)uResult < 0) {
            LogMessage(pMigrationContext, 2, L"TableTextServiceMig!PreApply :: ApplyUserReg(User context) error");
        }
    }
    
    LogMessage(pMigrationContext, 0, L"TableTextServiceMig!PreApply() end ...");
    return uResult;
}

UINT __cdecl RemoveObsoleteCHSHKLMRegkey(IMigrationContext* pMigrationContext)
{
    HKEY hKey = NULL;
    DWORD dwResult = 0;
    
    LONG lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SOFTWARE\\Microsoft\\CTF\\TIP\\{E429B25A-E5D3-4D1F-9BE3-0C608477E3A1}\\LanguageProfile\\0x00000804",
                                0, KEY_READ | KEY_WRITE, &hKey);
    
    if (lResult == ERROR_SUCCESS) {
        lResult = RegDeleteTreeW(hKey, NULL);
        LogMessage(pMigrationContext, 0, L"TableTextServiceMig!RemoveObsoleteCHSHKLMRegkey- RegDeleteTree = %d", lResult);
        RegCloseKey(hKey);
    } else {
        LogMessage(pMigrationContext, 2, L"TableTextServiceMig!RemoveObsoleteCHSHKLMRegkey - fail to open regkey = %d", lResult);
    }
    
    if (lResult != ERROR_SUCCESS) {
        dwResult = HRESULT_FROM_WIN32(lResult);
    }
    
    return dwResult;
}

UINT __cdecl SaveE0xxxx04IMERegToFile_ForE0xxxx04(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, wchar_t* pszParam)
{
    HANDLE hFile = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        LogMessage(pMigrationContext, 2, L"SaveE0xxxx04IMERegToFile_ForE0xxxx04 :: CreateFile error (%s) %d", pszFileName, dwError);
        return HRESULT_FROM_WIN32(dwError);
    }
    
    LogMessage(pMigrationContext, 0, L"SaveE0xxxx04IMERegToFile_ForE0xxxx04 :: CreateFile succeeded (%s)", pszFileName);
    
    UINT uResult = SaveKeysToFile_ForE0xxxx04(pMigrationContext, (HKEY*)HKEY_LOCAL_MACHINE, pszFileName, NULL);
    
    CloseHandle(hFile);
    return uResult;
}

int __cdecl SaveKeysToFile_ForE0xxxx04(IMigrationContext* pMigrationContext, HKEY* phKey, wchar_t* pszFileName, wchar_t* pszParam)
{
    CEnumRegKey enumKey(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Keyboard Layouts");
    wchar_t szSubKeyName[256];
    int iResult = 0;
    
    while (enumKey.Next(szSubKeyName, 256) == 0) {
        DWORD dwLayout = wcstoul(szSubKeyName, NULL, 16);
        
        if ((dwLayout & 0xF0000000) == 0xE0000000) {
            WORD wLayoutHigh = HIWORD(dwLayout);
            if ((wLayoutHigh & 0xF000) == 0xE000 && 
                (wLayoutHigh & 0x0FFF) <= 0x1F && 
                (LOWORD(dwLayout) & 0x03FF) == 0x0004) {
                
                wchar_t szFullKeyPath[512];
                StringCchCopyW(szFullKeyPath, 512, L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\");
                StringCchCatW(szFullKeyPath, 512, szSubKeyName);
                
                iResult = InternalSaveKeysToFile(pMigrationContext, (HKEY*)HKEY_LOCAL_MACHINE, pszFileName, NULL, szFullKeyPath);
                if (FAILED(iResult)) {
                    break;
                }
            }
        }
    }
    
    return iResult;
}

_bstr_t __thiscall IMigrationContext::GetDomain(IMigrationContext* pThis)
{
    void* pfnGetDomain = *((void**)pThis + 0x24);
    GUID guidResult = {0};
    
    int hr = ((int (__thiscall*)(IMigrationContext*, GUID*))pfnGetDomain)(pThis, &guidResult);
    
    if (FAILED(hr)) {
        _com_issue_errorex(hr, (IUnknown*)pThis, guidResult);
    }
    
    _bstr_t bstrResult((wchar_t*)guidResult.Data1);
    return bstrResult;
}

_bstr_t __thiscall IMigrationContext::GetUserNameW(IMigrationContext* pThis)
{
    void* pfnGetUserName = *((void**)pThis + 0x20);
    GUID guidResult = {0};
    
    int hr = ((int (__thiscall*)(IMigrationContext*, GUID*))pfnGetUserName)(pThis, &guidResult);
    
    if (FAILED(hr)) {
        _com_issue_errorex(hr, (IUnknown*)pThis, guidResult);
    }
    
    _bstr_t bstrResult((wchar_t*)guidResult.Data1);
    return bstrResult;
}

_bstr_t __thiscall IMigrationContext::GetUserSidString(IMigrationContext* pThis)
{
    void* pfnGetUserSidString = *((void**)pThis + 0x34);
    GUID guidResult = {0};
    
    int hr = ((int (__thiscall*)(IMigrationContext*, GUID*))pfnGetUserSidString)(pThis, &guidResult);
    
    if (FAILED(hr)) {
        _com_issue_errorex(hr, (IUnknown*)pThis, guidResult);
    }
    
    _bstr_t bstrResult((wchar_t*)guidResult.Data1);
    return bstrResult;
}

_bstr_t __thiscall IMigrationContext::GetWorkingDir(IMigrationContext* pThis)
{
    void* pfnGetWorkingDir = *((void**)pThis + 0x1C);
    GUID guidResult = {0};
    
    int hr = ((int (__thiscall*)(IMigrationContext*, GUID*))pfnGetWorkingDir)(pThis, &guidResult);
    
    if (FAILED(hr)) {
        _com_issue_errorex(hr, (IUnknown*)pThis, guidResult);
    }
    
    _bstr_t bstrResult((wchar_t*)guidResult.Data1);
    return bstrResult;
}

int __thiscall IMigrationContext::SendLogMessage(UINT dwLogLevel, _bstr_t bstrMessage)
{
	IMigrationContext* pThis = NULL;
	
    wchar_t* pwszMessage = NULL;
    if (bstrMessage.length() > 0) {
        pwszMessage = bstrMessage;
    }
    
    int (__thiscall *pfnSendLogMessage)(IMigrationContext*, UINT, wchar_t*) = 
        (int (__thiscall*)(IMigrationContext*, UINT, wchar_t*))(*((DWORD*)pThis) + 0x30);
    
    int hr = pfnSendLogMessage(pThis, dwLogLevel, pwszMessage);
    
    if (FAILED(hr)) {
        _com_issue_errorex(hr, (IUnknown*)pThis, IID_NULL);
    }
    
    return hr;
}

