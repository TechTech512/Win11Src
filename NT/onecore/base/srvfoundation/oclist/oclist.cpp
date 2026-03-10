// oclist.cpp - Windows Optional Component List Utility
// Lists server roles and optional features that can be installed with Ocsetup.exe

#pragma warning (disable:4530)

// Define WPP control GUIDs before including the TMH file
#define WPP_MACRO_USE_KM_VERSION_FOR_UM
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(OptionalComponent, (81b20fea,73a8,4b62,95bc,354477c97a6f), \
        WPP_DEFINE_BIT(ERROR) \
        WPP_DEFINE_BIT(WARNING) \
        WPP_DEFINE_BIT(INFO) \
        WPP_DEFINE_BIT(VERBOSE) \
    )


#include "oclist.tmh"
#include <windows.h>
#include <comdef.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <wchar.h>
#include <malloc.h>
#include "resource.h"

// Add CBS COM interface forward declarations and GUIDs
struct ICbsIdentity : public IUnknown
{
    virtual HRESULT GetCanonicalName(wchar_t** ppszCanonicalName) = 0;
};

struct ICbsPackage : public IUnknown
{
    virtual HRESULT GetProperty(int propertyId, wchar_t** ppszPropertyValue) = 0;
    virtual HRESULT IsInstalled(BOOL* pbInstalled) = 0;
};

struct ICbsSession : public IUnknown
{
    virtual HRESULT Initialize(DWORD flags, const wchar_t* callerName, void* pReserved1, void* pReserved2) = 0;
    virtual HRESULT GetEnumPackages(DWORD flags, IUnknown** ppEnum) = 0;
    virtual HRESULT OpenPackageByIdentity(DWORD flags, ICbsIdentity* pIdentity, DWORD reserved, ICbsPackage** ppPackage) = 0;
};

struct IEnumCbsIdentity : public IUnknown
{
    virtual HRESULT Next(ULONG celt, ICbsIdentity** rgelt, ULONG* pceltFetched) = 0;
};

// CBS GUIDs
const GUID CLSID_CbsSession = {0x752073a1, 0x23f2, 0x4396, {0x85, 0xf0, 0x8f, 0xdb, 0x87, 0x9e, 0xd0, 0xed}};
const GUID IID_ICbsSession = {0x75207391, 0x23f2, 0x4396, {0x85, 0xf0, 0x8f, 0xdb, 0x87, 0x9e, 0xd0, 0xed}};

// Global variables
HINSTANCE__* m_module = nullptr;
wchar_t m_szResourceNotFound[128];
wchar_t* m_lpszResourceNotFound = nullptr;
wchar_t m_szUpdateInstalled[128];
wchar_t m_szUpdateNotInstalled[128];
wchar_t m_szUpdateNameMissing[128];
wchar_t m_szMemoryAllocationFailure[128];
wchar_t m_szCbsWrapperConstructionFailure[128];
wchar_t m_szInvalidPackageNameFailure[128];
wchar_t m_szUsage[1024];
wchar_t m_szFailedAdminGroupSID[128];
wchar_t m_szFailedCheckTokenMembership[128];
wchar_t m_szFailedAdminPriv[128];

ICbsSession* g_pCbsSession = nullptr;

// Function declarations
long __cdecl LoadResourceStrings(void);
long __cdecl OcsIsUserAdmin(int* isAdmin);
long __cdecl OpenCbsPackage(ICbsIdentity* identity, ICbsPackage** package);
long __cdecl OpenCbsSession(void);
unsigned long __cdecl OutputString(unsigned long stdHandle, const wchar_t* text);
void __cdecl PrintError(long errorCode);
long __cdecl PrintFoundationPackage(void);
long __cdecl PrintPackage(ICbsIdentity* identity);
long __cdecl PrintUpdateTree(ICbsPackage* package);

void WppCleanupUm(void)
{
    void** pCurrent = (void**)WPP_GLOBAL_Control;
    
    if (WPP_GLOBAL_Control != (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control) {
        while (pCurrent != nullptr) {
            if (pCurrent[1] != nullptr) {
                UnregisterTraceGuids((TRACEGUID_HANDLE)nullptr);
                pCurrent[1] = nullptr;
            }
            pCurrent = (void**)*pCurrent;
        }
        WPP_GLOBAL_Control = (WPP_PROJECT_CONTROL_BLOCK*)&WPP_GLOBAL_Control;
    }
    
    return;
}

long __cdecl LoadResourceStrings(void)
{
    m_module = (HINSTANCE__*)GetModuleHandleW(nullptr);
    if (m_module == nullptr) {
        DWORD lastError = GetLastError();
        PrintError(lastError);
        exit(1);
    }

    // Load IDS_RESOURCENOTFOUND (200)
    if (LoadStringW(m_module, IDS_RESOURCENOTFOUND, m_szResourceNotFound, 128) == 0) {
        wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"Resource not found: %d\r\n", IDS_RESOURCENOTFOUND);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }
    m_lpszResourceNotFound = m_szResourceNotFound;

    // Load IDS_INSTALLEDUPDATE (100)
    if (LoadStringW(m_module, IDS_INSTALLEDUPDATE, m_szUpdateInstalled, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_INSTALLEDUPDATE);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_NOTINSTALLEDUPDATE (101)
    if (LoadStringW(m_module, IDS_NOTINSTALLEDUPDATE, m_szUpdateNotInstalled, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_NOTINSTALLEDUPDATE);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_MISSINGNAME (102)
    if (LoadStringW(m_module, IDS_MISSINGNAME, m_szUpdateNameMissing, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_MISSINGNAME);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_MEMALLOCFAILURE (104)
    if (LoadStringW(m_module, IDS_MEMALLOCFAILURE, m_szMemoryAllocationFailure, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_MEMALLOCFAILURE);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_CANNOTOBTAINCBSINFO (110)
    if (LoadStringW(m_module, IDS_CANNOTOBTAINCBSINFO, m_szCbsWrapperConstructionFailure, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_CANNOTOBTAINCBSINFO);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_INVALIDPACKNAME (105)
    if (LoadStringW(m_module, IDS_INVALIDPACKNAME, m_szInvalidPackageNameFailure, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_INVALIDPACKNAME);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_USEOCSETUPNAMES (106)
    if (LoadStringW(m_module, IDS_USEOCSETUPNAMES, m_szUsage, 1024) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_USEOCSETUPNAMES);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_FAILEDTOINITADMINGRPSID (107)
    if (LoadStringW(m_module, IDS_FAILEDTOINITADMINGRPSID, m_szFailedAdminGroupSID, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_FAILEDTOINITADMINGRPSID);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_FAILEDTOKENMEMBERSHIP (108)
    if (LoadStringW(m_module, IDS_FAILEDTOKENMEMBERSHIP, m_szFailedCheckTokenMembership, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_FAILEDTOKENMEMBERSHIP);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    // Load IDS_RUNOCLISTASADMIN (109)
    if (LoadStringW(m_module, IDS_RUNOCLISTASADMIN, m_szFailedAdminPriv, 128) == 0) {
		wchar_t errorMsg[256];
		swprintf_s(errorMsg, 256, L"%s%d\r\n", m_lpszResourceNotFound, IDS_RUNOCLISTASADMIN);
		OutputString(STD_ERROR_HANDLE, errorMsg);
        return -1;
    }

    return 0;
}

long __cdecl OcsIsUserAdmin(int* isAdmin)
{
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID administratorsGroup = nullptr;
    BOOL isMember = FALSE;

    if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        OutputString(STD_ERROR_HANDLE, m_szFailedAdminGroupSID);
        OutputString(STD_ERROR_HANDLE, L"\r\n");
        return -1;
    }

    if (!CheckTokenMembership(nullptr, administratorsGroup, &isMember)) {
        if (administratorsGroup != nullptr) {
            FreeSid(administratorsGroup);
        }
        OutputString(STD_ERROR_HANDLE, m_szFailedCheckTokenMembership);
        OutputString(STD_ERROR_HANDLE, L"\r\n");
        return -1;
    }

    *isAdmin = isMember;
    
    if (administratorsGroup != nullptr) {
        FreeSid(administratorsGroup);
    }
    
    return 0;
}

long __cdecl OpenCbsPackage(ICbsIdentity* identity, ICbsPackage** package)
{
    ICbsPackage* pPackage = nullptr;
    HRESULT hr = g_pCbsSession->OpenPackageByIdentity(0, identity, 0, &pPackage);
    
    if (SUCCEEDED(hr)) {
        // Since we already have an ICbsPackage from OpenPackageByIdentity,
        // we can just return it directly without QueryInterface
        *package = pPackage;
        return 0;
    }
    
    PrintError(hr);
    return hr;
}

long __cdecl OpenCbsSession(void)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    
    if (SUCCEEDED(hr) || hr == S_FALSE) {
        hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, 
                                   RPC_C_AUTHN_LEVEL_DEFAULT, 
                                   RPC_C_IMP_LEVEL_IMPERSONATE, 
                                   nullptr, EOAC_NONE, nullptr);
        
        if (SUCCEEDED(hr)) {
            hr = CoCreateInstance(CLSID_CbsSession, nullptr, CLSCTX_INPROC_SERVER, 
                                   IID_ICbsSession, (void**)&g_pCbsSession);
            
            if (SUCCEEDED(hr)) {
                hr = g_pCbsSession->Initialize(0, L"oclist", nullptr, nullptr);
                if (SUCCEEDED(hr)) {
                    return 0;
                }
                
                if (g_pCbsSession != nullptr) {
                    g_pCbsSession->Release();
                    g_pCbsSession = nullptr;
                }
            } else {
                // Class not registered - but we can still show usage
                OutputString(STD_OUTPUT_HANDLE, L"===========================================================================\r\n");
                OutputString(STD_OUTPUT_HANDLE, L"This tool is deprecated in this version of Windows.\r\n");
                CoUninitialize();
                return hr;
            }
        }
    }
    
    CoUninitialize();
    PrintError(hr);
    return hr;
}

unsigned long __cdecl OutputString(unsigned long stdHandle, const wchar_t* text)
{
    DWORD bytesWritten = 0;
    HANDLE hConsole = GetStdHandle(stdHandle);
    DWORD fileType = GetFileType(hConsole);
    
    // Check if it's a console
    if ((fileType & FILE_TYPE_CHAR) != 0) {
        DWORD consoleMode;
        if (GetConsoleMode(hConsole, &consoleMode)) {
            // Console output - use WriteConsoleW
            WriteConsoleW(hConsole, text, (DWORD)wcslen(text), &bytesWritten, nullptr);
            return bytesWritten;
        }
    }
    
    // Not a console or can't get console mode - use WriteFile with conversion
    UINT consoleCP = GetConsoleOutputCP();
    int bufferSize = WideCharToMultiByte(consoleCP, 0, text, -1, nullptr, 0, nullptr, nullptr);
    char* mbBuffer = (char*)malloc(bufferSize);
    
    if (mbBuffer != nullptr) {
        WideCharToMultiByte(consoleCP, 0, text, -1, mbBuffer, bufferSize, nullptr, nullptr);
        WriteFile(hConsole, mbBuffer, bufferSize - 1, &bytesWritten, nullptr);
        free(mbBuffer);
    }
    
    return bytesWritten;
}

void __cdecl PrintError(long errorCode)
{
    if (errorCode == 0) return;
    
    wchar_t* errorMessage = nullptr;
    
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, errorCode, 0, (LPWSTR)&errorMessage, 0, nullptr);
    
    if (errorMessage != nullptr) {
        OutputString(STD_ERROR_HANDLE, errorMessage);
        OutputString(STD_ERROR_HANDLE, L"\r\n");
        LocalFree(errorMessage);
    }
}

long __cdecl PrintFoundationPackage(void)
{
    HRESULT hr;
    IEnumCbsIdentity* enumIdentity = nullptr;
    
    hr = g_pCbsSession->GetEnumPackages(0x10, (IUnknown**)&enumIdentity);
    if (FAILED(hr)) {
        PrintError(hr);
        return hr;
    }
    
    while (true) {
        ICbsIdentity* identity = nullptr;
        ULONG fetched = 0;
        
        hr = enumIdentity->Next(1, &identity, &fetched);
        if (FAILED(hr) || fetched == 0) {
            break;
        }
        
        ICbsPackage* package = nullptr;
        long result = OpenCbsPackage(identity, &package);
        
        if (SUCCEEDED(result)) {
            wchar_t* packageName = nullptr;
            hr = package->GetProperty(8, &packageName);
            
            if (package != nullptr) {
                package->Release();
                package = nullptr;
            }
            
            if (SUCCEEDED(hr) && packageName != nullptr) {
                // Check if this is the Foundation package
                if (_wcsnicmp(packageName, L"Foundation", 10) == 0) {
                    PrintPackage(identity);
                }
                
                if (packageName != nullptr) {
                    CoTaskMemFree(packageName);
                }
            }
        }
        
        if (identity != nullptr) {
            identity->Release();
            identity = nullptr;
        }
    }
    
    if (enumIdentity != nullptr) {
        enumIdentity->Release();
    }
    
    return 0;
}

long __cdecl PrintPackage(ICbsIdentity* identity)
{
    wchar_t* canonicalName = nullptr;
    ICbsPackage* package = nullptr;
    
    HRESULT hr = identity->GetCanonicalName(&canonicalName);
    if (FAILED(hr)) {
        PrintError(hr);
        return hr;
    }
    
    hr = OpenCbsPackage(identity, &package);
    if (FAILED(hr)) {
        if (canonicalName != nullptr) {
            CoTaskMemFree(canonicalName);
        }
        PrintError(hr);
        return hr;
    }
    
    // Find the position of '~' in the canonical name
    size_t tildePos = wcscspn(canonicalName, L"~");
    if (tildePos == 0) {
        OutputString(STD_ERROR_HANDLE, m_szInvalidPackageNameFailure);
		OutputString(STD_ERROR_HANDLE, L"\r\n");
        if (canonicalName != nullptr) {
            CoTaskMemFree(canonicalName);
        }
        if (package != nullptr) {
            package->Release();
        }
        return -1;
    }
    
    // Extract the package name (everything before the first '~')
    size_t nameLength = wcslen(canonicalName);
    wchar_t* packageDisplayName = new (std::nothrow) wchar_t[nameLength + 1];
    
    if (packageDisplayName == nullptr) {
        OutputString(STD_ERROR_HANDLE, m_szMemoryAllocationFailure);
		OutputString(STD_ERROR_HANDLE, L"\r\n");
        if (canonicalName != nullptr) {
            CoTaskMemFree(canonicalName);
        }
        if (package != nullptr) {
            package->Release();
        }
        return -1;
    }
    
    wcsncpy_s(packageDisplayName, nameLength + 1, canonicalName, tildePos);
    
    // Output the package information
    OutputString(STD_OUTPUT_HANDLE, m_szUsage);
    OutputString(STD_OUTPUT_HANDLE, L"\r\n\r\n");
    OutputString(STD_OUTPUT_HANDLE, L"===========================================================================\r\n");
    OutputString(STD_OUTPUT_HANDLE, packageDisplayName);
    OutputString(STD_OUTPUT_HANDLE, L"\r\n");
    
    // Print the update tree for this package
    long result = PrintUpdateTree(package);
    
    delete[] packageDisplayName;
    
    if (canonicalName != nullptr) {
        CoTaskMemFree(canonicalName);
    }
    
    if (package != nullptr) {
        package->Release();
    }
    
    return result;
}

long __cdecl PrintUpdateTree(ICbsPackage* package)
{
    // This function would traverse and print the update hierarchy
    // Simplified for reconstruction
    
    // Check if the package has updates installed
    BOOL isInstalled = FALSE;
    package->IsInstalled(&isInstalled);
    
    if (isInstalled) {
        OutputString(STD_OUTPUT_HANDLE, L"    Installed:\r\n");
    } else {
        OutputString(STD_OUTPUT_HANDLE, L"Not Installed:\r\n");
    }
    
    // Additional update tree traversal would go here
    // This is a simplified version
    
    return 0;
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
    // Set thread UI language
    SetThreadPreferredUILanguages(MUI_CONSOLE_FILTER, nullptr, nullptr);
    
    // Set locale to OEM CP for console output
    _wsetlocale(LC_ALL, L".OCP");
    _wsetlocale(LC_CTYPE, L".OCP");
    _wsetlocale(LC_MONETARY, L".OCP");
    _wsetlocale(LC_NUMERIC, L".OCP");
    _wsetlocale(LC_TIME, L".OCP");
    
    // Load resource strings
    long result = LoadResourceStrings();
    if (result < 0) {
        PrintError(result);
        exit(1);
    }
    
    // Always show usage message first (this matches official behavior)
    OutputString(STD_OUTPUT_HANDLE, m_szUsage);
    OutputString(STD_OUTPUT_HANDLE, L"\r\n\r\n");
    
    // Check if user is admin
    int isAdmin = 0;
    result = OcsIsUserAdmin(&isAdmin);
    if (result < 0) {
        PrintError(result);
        exit(1);
    }
    
    if (isAdmin == 0) {
        OutputString(STD_OUTPUT_HANDLE, m_szFailedAdminPriv);
        OutputString(STD_OUTPUT_HANDLE, L"\r\n");
        exit(1);
    }
    
    // Open CBS session
    result = OpenCbsSession();
    if (result < 0) {
        // Already showed usage message, just exit
        exit(1);
    }
    
    // Print Foundation packages
    PrintFoundationPackage();
    
    // Cleanup
    if (g_pCbsSession != nullptr) {
        g_pCbsSession->Release();
        g_pCbsSession = nullptr;
    }
    
    CoUninitialize();
    WppCleanupUm();
    
    return 0;
}

