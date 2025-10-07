#pragma warning (disable:4996)

#include <windows.h>
#include <stdio.h>

typedef struct _CompatCheckContext {
    void *Reserved;
} CompatCheckContext;

int OSUpgradeADMTv3ComplianceCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    BOOL bVersionCheckFailed = FALSE;
    BOOL bAdmtv3Detected = FALSE;
    HKEY hKeyUninstall = NULL;
    HKEY hKeyAdmt = NULL;
    DWORD dwType = 0;
    DWORD cbData = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
    DWORD dwBuildNumber = 0;
    wchar_t szVersion[520];
    DWORD dwVersionSize = sizeof(szVersion);
    SYSTEM_INFO systemInfo;
    wchar_t* pszAdmtKeyPath = NULL;
    int n1 = 0, n2 = 0, n3 = 0, n4 = 0;

    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (!GetVersionExW((LPOSVERSIONINFOW)&osvi))
    {
        bVersionCheckFailed = TRUE;
    }
    else if (osvi.wProductType == VER_NT_DOMAIN_CONTROLLER && 
             osvi.dwMajorVersion == 5 && 
             osvi.dwMinorVersion == 2)
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ADMT",
                         0,
                         KEY_READ,
                         &hKeyUninstall) == ERROR_SUCCESS)
        {
            if (RegQueryValueExW(hKeyUninstall, L"UninstallString", NULL, &dwType, NULL, &cbData) == ERROR_SUCCESS &&
                (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
            {
                GetSystemInfo(&systemInfo);
                
                if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
                    systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
                {
                    pszAdmtKeyPath = L"SOFTWARE\\WOW6432Node\\Microsoft\\ADMT";
                }
                else
                {
                    pszAdmtKeyPath = L"SOFTWARE\\Microsoft\\ADMT";
                }

                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, pszAdmtKeyPath, 0, KEY_READ, &hKeyAdmt) == ERROR_SUCCESS)
                {
                    dwVersionSize = sizeof(szVersion);
                    if (RegQueryValueExW(hKeyAdmt, L"Version", NULL, &dwType, (LPBYTE)szVersion, &dwVersionSize) == ERROR_SUCCESS &&
                        dwType == REG_SZ)
                    {
                        if (swscanf_s(szVersion, L" %d.%d.%d.%d ", &n1, &n2, &n3, &n4) == 4)
                        {
                            if (n1 == 5 && n2 == 3)
                            {
                                bAdmtv3Detected = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (hKeyUninstall != NULL)
    {
        RegCloseKey(hKeyUninstall);
    }
    
    if (hKeyAdmt != NULL)
    {
        RegCloseKey(hKeyAdmt);
    }

    if (bVersionCheckFailed)
    {
        return 0;
    }

    if (bAdmtv3Detected)
    {
        pCallback(pContext, 0, 0x1771);
    }

    return 1;
}

