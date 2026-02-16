// init.cpp - Control Panel compatibility layer
// Handles mapping of legacy Control Panel applet names to modern implementations

#include <windows.h>
#include <memsafe.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <objbase.h>
#include <strsafe.h>
#include <stdlib.h>

// Forward declarations
wchar_t* NextToken(wchar_t **ppszTokens);
bool GetNameAndPage(wchar_t *pszCommandLine, wchar_t *pszPage, unsigned int cchPage, wchar_t **ppszName);
int GetControlPanelViewAndPath(CPVIEW *pView, wchar_t *pszPath, unsigned int cchPath);
unsigned long GetRegisteredCplPath(wchar_t *pszCplName, wchar_t *pszPath, unsigned int cchPath);
int OpenControlPanel(wchar_t *pszName, wchar_t *pszPage);
int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *lpCmdLine, int nShowCmd);

wchar_t* c_szDCUsersSnapin = L"%SystemRoot%\\system32\\dsa.msc";
wchar_t* c_szNetplwiz = L"%SystemRoot%\\system32\\netplwiz.exe";
wchar_t* c_szRunDLL32 = L"%SystemRoot%\\system32\\rundll32.exe";
wchar_t* c_szScannersAndCameras = L"%ProgramFiles%\\Windows Photo Viewer\\ImagingDevices.exe";
wchar_t* c_szSchedTasksSnapin = L"%SystemRoot%\\system32\\taskschd.msc";
wchar_t* c_szSystemComputerName = L"%SystemRoot%\\system32\\SystemPropertiesComputerName.exe";
wchar_t* c_szUsersSnapin = L"%SystemRoot%\\system32\\lusrmgr.msc";

// Command mapping structure for legacy Control Panel applets
typedef struct _COMPATCPL {
    wchar_t *pszOldForm;           // Legacy applet name (e.g., "DESKTOP", "MOUSE")
    DWORD dwOS;                     // OS version requirement (0xFFFFFFFF = any)
    BOOL fUseRunDllShell32;         // Whether to use rundll32.exe
    BOOL fIsNamedApplet;            // Whether this is a named applet (modern)
    wchar_t *pszFile;                // File to execute
    wchar_t *pszParameters;          // Parameters to pass
} COMPATCPL;

#define OS_ANY          ((DWORD)-1)

// Command mapping table
COMPATCPL c_aCommandMap[] = {
    // pszOldForm          dwOS        fUseRunDllShell32 fIsNamedApplet pszFile                          pszParameters
    { L"DESKTOP",           OS_ANY, FALSE,            TRUE,          L"Microsoft.Personalization",   NULL },
    { L"COLOR",             OS_ANY, TRUE,             FALSE,         L"desk.cpl",                    L",@Advanced" },
    { L"DATE/TIME",         OS_ANY, FALSE,            TRUE,          L"Microsoft.DateAndTime",       NULL },
    { L"PORTS",             OS_ANY, FALSE,            FALSE,         c_szSystemComputerName, NULL },
    { L"INTERNATIONAL",     OS_ANY, FALSE,            TRUE,          L"Microsoft.RegionAndLanguage", NULL },
    { L"MOUSE",             OS_ANY, FALSE,            TRUE,          L"Microsoft.Mouse",             NULL },
    { L"KEYBOARD",          OS_ANY, FALSE,            TRUE,          L"Microsoft.Keyboard",          NULL },
    { L"PRINTERS",          OS_ANY, FALSE,            TRUE,          L"Microsoft.DevicesAndPrinters", NULL },
    { L"FONTS",             OS_ANY, FALSE,            TRUE,          L"Microsoft.Fonts",             NULL },
    { L"FOLDERS",           OS_ANY, FALSE,            TRUE,          L"Microsoft.FolderOptions",     NULL },
    { L"TELEPHONY",         OS_ANY, FALSE,            TRUE,          L"Microsoft.PhoneAndModem",     NULL },
    { L"ADMINTOOLS",        OS_ANY, FALSE,            TRUE,          L"Microsoft.AdministrativeTools", NULL },
    { L"SCHEDTASKS",        OS_ANY, FALSE,            FALSE,         c_szSchedTasksSnapin, NULL },
    { L"INFRARED",          OS_ANY, FALSE,            TRUE,          L"Microsoft.Infrared",          NULL },
    { L"USERPASSWORDS",     OS_DOMAINMEMBER, FALSE,            FALSE,         c_szDCUsersSnapin, NULL },
    { L"USERPASSWORDS",     OS_ANYSERVER,       FALSE,            FALSE,         c_szUsersSnapin, NULL },
    { L"USERPASSWORDS",     OS_WHISTLERORGREATER, FALSE,            TRUE,          L"Microsoft.UserAccounts",      NULL },
    { L"USERPASSWORDS2",    OS_ANY, FALSE,            FALSE,         c_szNetplwiz, NULL },
    { L"SCANNERCAMERA",     OS_ANY, FALSE,            FALSE,         c_szScannersAndCameras, NULL },
    { L"SYSTEM",            OS_ANY, FALSE,            TRUE,          L"Microsoft.System",            NULL },
    { L"UPDATE",            OS_ANY, FALSE,            TRUE,          L"Microsoft.WindowsUpdate",     NULL }
};

int GetControlPanelViewAndPath(CPVIEW *pView, wchar_t *pszPath, unsigned int cchPath)
{
    HRESULT hr;
    IOpenControlPanel *pOpenControlPanel = NULL;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&pOpenControlPanel));
        if (SUCCEEDED(hr))
        {
            hr = pOpenControlPanel->GetCurrentView(pView);
            if (SUCCEEDED(hr))
            {
                // GetPath expects a LPWSTR and a UINT* for the size
                UINT cchPathSize = cchPath;
                hr = pOpenControlPanel->GetPath(NULL, pszPath, cchPath);
            }
            pOpenControlPanel->Release();
        }
        CoUninitialize();
    }
    
    return hr;
}

bool GetNameAndPage(wchar_t *pszCommandLine, wchar_t *pszPage, unsigned int cchPage, wchar_t **ppszName)
{
    bool bFound = false;
    wchar_t *pszTokens = pszCommandLine;
    wchar_t *pszNameAllocated = NULL;
    
    *ppszName = NULL;
    pszPage[0] = L'\0';
    
    // Allocate string from command line using _AllocStringWorker
    HRESULT hr = _AllocStringWorker<CTCoAllocPolicy>(GetProcessHeap(), 
                                    HEAP_ZERO_MEMORY, 
                                    pszCommandLine, 
                                    wcslen(pszCommandLine) * sizeof(wchar_t), 
                                    (wcslen(pszCommandLine) + 1) * sizeof(wchar_t), 
                                    &pszNameAllocated);
    
    if (SUCCEEDED(hr))
    {
        wchar_t *pszToken = NextToken(&pszTokens);
        
        if (pszToken != NULL)
        {
            if ((StrCmpICW(pszToken, L"/name") == 0 || 
                 StrCmpICW(pszToken, L"-name") == 0))
            {
                wchar_t *pszName = NextToken(&pszTokens);
                if (pszName != NULL)
                {
                    // Store the name in the allocated buffer
                    *ppszName = pszNameAllocated;
                    bFound = true;
                    
                    wchar_t *pszNextToken = NextToken(&pszTokens);
                    if (pszNextToken != NULL &&
                        (StrCmpICW(pszNextToken, L"/page") == 0 || 
                         StrCmpICW(pszNextToken, L"-page") == 0))
                    {
                        wchar_t *pszPageToken = NextToken(&pszTokens);
                        if (pszPageToken != NULL)
                        {
                            // Use _AllocStringWorker for page parameter as well
                            _AllocStringWorker<CTCoAllocPolicy>(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              pszPageToken,
                                              wcslen(pszPageToken) * sizeof(wchar_t),
                                              (wcslen(pszPageToken) + 1) * sizeof(wchar_t),
                                              &pszPage);
                        }
                    }
                }
            }
        }
        
        CoTaskMemFree(pszNameAllocated);
    }
    
    return bFound;
}

unsigned long GetRegisteredCplPath(wchar_t *pszCplName, wchar_t *pszPath, unsigned int cchPath)
{
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    LONG lResult;
    unsigned long uResult = ERROR_SUCCESS;
    wchar_t szSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls";
    wchar_t szValueName[260];
    wchar_t szData[520];
    DWORD cbData = sizeof(szData);
    DWORD cchValueName = 260;
    DWORD dwType;
    
    *pszPath = L'\0';
    
    if (pszCplName != NULL)
    {
        // Try both 32-bit and 64-bit registry views
        for (int iView = 0; iView < 2; iView++)
        {
            lResult = RegOpenKeyExW((iView == 0) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                                    szSubKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
            
            if (lResult == ERROR_SUCCESS)
            {
                dwIndex = 0;
                while (TRUE)
                {
                    cbData = sizeof(szData);
                    cchValueName = 260;
                    
                    lResult = RegEnumValueW(hKey, dwIndex, szValueName, &cchValueName,
                                           NULL, &dwType, (LPBYTE)szData, &cbData);
                    dwIndex++;
                    
                    if (lResult != ERROR_SUCCESS)
                        break;
                    
                    if (cbData < 3) // Need at least 3 characters for a valid path
                        continue;
                    
                    if (StrCmpICW(pszCplName, szValueName) == 0)
                    {
                        if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
                        {
                            DWORD cchExpanded = ExpandEnvironmentStringsW(szData, pszPath + 1, cchPath - 1);
                            if (cchExpanded > 0 && cchExpanded < cchPath - 1)
                            {
                                *pszPath = L'\"';
                                pszPath[cchExpanded] = L'\"';
                                pszPath[cchExpanded + 1] = L'\0';
                            }
                            else
                            {
                                uResult = ERROR_BAD_PATHNAME;
                            }
                        }
                        else
                        {
                            uResult = ERROR_BAD_PATHNAME;
                        }
                        break;
                    }
                }
                
                RegCloseKey(hKey);
            }
        }
    }
    
    return uResult;
}

wchar_t* NextToken(wchar_t **ppszTokens)
{
    wchar_t *pszCurrent = *ppszTokens;
    
    if (pszCurrent == NULL)
        return NULL;
    
    // Skip leading spaces
    while (*pszCurrent == L' ')
        pszCurrent++;
    
    if (*pszCurrent == L'\0')
    {
        *ppszTokens = NULL;
        return NULL;
    }
    
    wchar_t *pszToken = pszCurrent;
    
    // Handle quoted tokens
    if (*pszCurrent == L'\"')
    {
        pszCurrent++;
        pszToken = pszCurrent;
        
        while (*pszCurrent != L'\0' && *pszCurrent != L'\"')
            pszCurrent++;
        
        if (*pszCurrent == L'\0')
        {
            *ppszTokens = NULL;
            return NULL;
        }
        
        // Null terminate at the closing quote
        *pszCurrent = L'\0';
        *ppszTokens = pszCurrent + 1;
    }
    else
    {
        // Find end of token (space or null)
        while (*pszCurrent != L' ' && *pszCurrent != L'\0')
            pszCurrent++;
        
        if (*pszCurrent == L'\0')
        {
            *ppszTokens = NULL;
            return pszToken;
        }
        
        *pszCurrent = L'\0';
        *ppszTokens = pszCurrent + 1;
    }
    
    return pszToken;
}

int OpenControlPanel(wchar_t *pszName, wchar_t *pszPage)
{
    HRESULT hr;
    IOpenControlPanel *pOpenControlPanel = NULL;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&pOpenControlPanel));
        if (SUCCEEDED(hr))
        {
            // Open expects a LPCWSTR for name and page, and a HWND as the third parameter
            hr = pOpenControlPanel->Open(pszName, pszPage, NULL);
            pOpenControlPanel->Release();
        }
        CoUninitialize();
    }
    
    return hr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    wchar_t *pszCommandLine = GetCommandLineW();
    wchar_t *pszArgs = pszCommandLine;
    
    // Skip executable name (handles quoted paths)
    if (*pszArgs == L'\"')
    {
        pszArgs++;
        while (*pszArgs != L'\0' && *pszArgs != L'\"')
            pszArgs++;
        if (*pszArgs == L'\"')
            pszArgs++;
    }
    else
    {
        while (*pszArgs > L' ')
            pszArgs++;
    }
    
    // Skip whitespace
    while (*pszArgs == L' ')
        pszArgs++;
    
    StrTrimW(pszArgs, L" \t");
    
    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    GetStartupInfoW(&si);
    
    HINSTANCE hShowCmd;
    if ((si.dwFlags & STARTF_USESHOWWINDOW) == 0)
        hShowCmd = (HINSTANCE)10; // SW_SHOWDEFAULT
    else
        hShowCmd = (HINSTANCE)(unsigned int)si.wShowWindow;
    
    GetModuleHandleW(NULL); // Get our module handle
    
    // Fix the HINSTANCE vs wchar_t* issue - hInstance is being misused as a string pointer
    // In WinMain, hInstance is a handle, not a string. We need to use pszArgs instead.
    return WinMainT((HINSTANCE)pszArgs, hShowCmd, NULL, (int)&si);
}

int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *lpCmdLine, int nShowCmd)
{
    SHELLEXECUTEINFOW sei = {0};
    wchar_t szModuleName[MAX_PATH];
    wchar_t szCommand[780];
    wchar_t szPath[MAX_PATH];
    wchar_t szPage[42] = {0};
    wchar_t *pszNameAllocated = NULL;
    COMPATCPL *pCommandMap;
    bool bFound;
    int iResult;
    
    wchar_t szShellRunDLL[] = L"Shell32.dll,Control_RunDLL ";
    
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    
    // Use lpCmdLine if provided, otherwise fall back to hInstance
    wchar_t *pszCmdLine = lpCmdLine;
    if (pszCmdLine == NULL || *pszCmdLine == L'\0')
        pszCmdLine = (wchar_t*)hInstance;
    
    // Handle empty command line
    if (*pszCmdLine == L'\0' || StrCmpICW(pszCmdLine, L"PANEL") == 0)
    {
        iResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(iResult))
        {
            IOpenControlPanel *pOpenControlPanel = NULL;
            iResult = CoCreateInstance(CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(&pOpenControlPanel));
            if (SUCCEEDED(iResult))
            {
                iResult = pOpenControlPanel->Open(NULL, NULL, NULL);
                pOpenControlPanel->Release();
            }
            CoUninitialize();
            
            if (SUCCEEDED(iResult))
                return 1;
        }
        return 0;
    }
    
    // Try to parse as named applet
    bFound = GetNameAndPage(pszCmdLine, szPage, _countof(szPage), &pszNameAllocated);
    if (bFound)
    {
        iResult = OpenControlPanel(pszCmdLine, szPage);
        if (pszNameAllocated)
            HeapFree(GetProcessHeap(), 0, pszNameAllocated);
        return (iResult >= 0) ? 1 : 0;
    }
    
    // Search command map for legacy applet name - process in order
    wchar_t *pszFile = NULL;
    wchar_t *pszParameters = NULL;
    BOOL bFoundInMap = FALSE;
    
    for (DWORD dwIndex = 0; dwIndex < _countof(c_aCommandMap); dwIndex++)
    {
        pCommandMap = &c_aCommandMap[dwIndex];
        
        if (StrCmpICW(pCommandMap->pszOldForm, pszCmdLine) == 0)
        {
            // Check OS condition
            if (pCommandMap->dwOS == 0xFFFFFFFF || IsOS(pCommandMap->dwOS))
            {
                bFoundInMap = TRUE;
                
                if (pCommandMap->fUseRunDllShell32)
                {
                    pszFile = c_szRunDLL32;
                    
                    if (pCommandMap->pszParameters == NULL)
                    {
                        iResult = StringCchPrintfW(szCommand, _countof(szCommand),
                                                  L"%s%s", szShellRunDLL, pCommandMap->pszFile);
                    }
                    else
                    {
                        iResult = StringCchPrintfW(szCommand, _countof(szCommand),
                                                  L"%s%s %s", szShellRunDLL, 
                                                  pCommandMap->pszFile, pCommandMap->pszParameters);
                    }
                    
                    if (iResult < 0)
                        return 0;
                    
                    pszParameters = szCommand;
                }
                else if (pCommandMap->fIsNamedApplet)
                {
                    iResult = OpenControlPanel(pCommandMap->pszFile, pCommandMap->pszParameters);
                    return (iResult >= 0) ? 1 : 0;
                }
                else
                {
                    pszFile = pCommandMap->pszFile;
                    pszParameters = pCommandMap->pszParameters;
                }
                break;  // Stop at first match
            }
        }
    }
    
    // Handle NETCONNECTIONS/ncpa.cpl special case
    if (pszFile == NULL)
    {
        if (StrCmpICW(L"NETCONNECTIONS", pszCmdLine) == 0 ||
            StrCmpICW(L"ncpa.cpl", pszCmdLine) == 0)
        {
            wchar_t szNetworkFolder[] = L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}";
            
            iResult = GetControlPanelViewAndPath(NULL, szPath, _countof(szPath));
            if (iResult >= 0)
            {
                iResult = StringCchPrintfW(szCommand, _countof(szCommand),
                                          L"%s\\%s", szPath, szNetworkFolder);
                if (iResult >= 0)
                {
                    pszFile = szCommand;
                    pszParameters = NULL;  // Clear parameters for this case
                }
            }
            
            if (iResult < 0)
                return 0;
        }
        else if (pszFile == NULL)
        {
            // Try to find in registered CPLs
            wchar_t szCplName[MAX_PATH];
            wchar_t szRegisteredPath[MAX_PATH * 2] = {0};  // Separate buffer for registered path
            
            iResult = StringCchCopyW(szCplName, _countof(szCplName), pszCmdLine);
            if (iResult >= 0)
            {
                unsigned long uResult = GetRegisteredCplPath(szCplName, szRegisteredPath, _countof(szRegisteredPath));
                if (uResult == ERROR_SUCCESS)
                {
                    // Build the full command with rundll32 and the registered path
                    pszFile = c_szRunDLL32;
                    
                    iResult = StringCchPrintfW(szCommand, _countof(szCommand),
                                              L"%s%s", szShellRunDLL, szRegisteredPath);
                    if (iResult >= 0)
                    {
                        pszParameters = szCommand;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    // Not registered - try direct execution
                    pszFile = pszCmdLine;
                    pszParameters = NULL;
                }
            }
            else
            {
                return 0;
            }
        }
    }
    
    // Make sure we have something to execute
    if (pszFile == NULL)
        return 0;
    
    // Execute the Control Panel applet
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"open";
    sei.lpFile = pszFile;
    sei.lpParameters = pszParameters;
    sei.nShow = (hPrevInstance == NULL) ? SW_SHOWDEFAULT : (int)(unsigned int)hPrevInstance;
    sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOZONECHECKS | SEE_MASK_DOENVSUBST;
    
    AllowSetForegroundWindow(ASFW_ANY);
    
    if (ShellExecuteExW(&sei))
        return 1;
    
    return 0;
}

