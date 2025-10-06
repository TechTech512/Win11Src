#include "precomp.h"

DWORD NtfrsUpgpComplianceCheck(int* pResult)
{
    DWORD dwError;
    HKEY hKeyReplicaSets = NULL;
    HKEY hKeySubKey = NULL;
    DWORD dwIndex = 0;
    DWORD dwNameSize = 36;
    DWORD dwPathSize = 0;
    wchar_t* pszSubKeyName = NULL;
    wchar_t* pszFullPath = NULL;
    wchar_t szReplicaName[36];
    DWORD dwReplicaNameSize = sizeof(szReplicaName);
    DWORD dwType = 0;
    int bNonSysvolFound = 0;

    *pResult = 0;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Replica Sets",
                           0,
                           KEY_READ,
                           &hKeyReplicaSets);
    
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        dwError = ERROR_SUCCESS;
        goto Cleanup;
    }
    
    if (dwError != ERROR_SUCCESS)
        goto Cleanup;

    pszSubKeyName = (wchar_t*)malloc(72);
    if (pszSubKeyName == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwNameSize = 36;

    while ((dwError = RegEnumKeyExW(hKeyReplicaSets, dwIndex, pszSubKeyName, &dwNameSize, 
                                   NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
    {
        if (dwError == ERROR_MORE_DATA)
        {
            free(pszSubKeyName);
            dwNameSize = dwNameSize + 1;
            pszSubKeyName = (wchar_t*)malloc(dwNameSize * sizeof(wchar_t));
            if (pszSubKeyName == NULL)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            dwError = RegEnumKeyExW(hKeyReplicaSets, dwIndex, pszSubKeyName, &dwNameSize, 
                                   NULL, NULL, NULL, NULL);
        }

        if (dwError != ERROR_SUCCESS)
            goto Cleanup;

        dwPathSize = dwNameSize + 66;
        pszFullPath = (wchar_t*)malloc(dwPathSize * sizeof(wchar_t));
        if (pszFullPath == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        dwError = StringCchCopyW(pszFullPath, dwPathSize, 
                                L"SYSTEM\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Replica Sets");
        if (FAILED(dwError))
            goto Cleanup;

        dwError = StringCchCatW(pszFullPath, dwPathSize, L"\\");
        if (FAILED(dwError))
            goto Cleanup;

        dwError = StringCchCatW(pszFullPath, dwPathSize, pszSubKeyName);
        if (FAILED(dwError))
            goto Cleanup;

        dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, pszFullPath, 0, KEY_READ, &hKeySubKey);
        if (dwError == ERROR_SUCCESS)
        {
            dwReplicaNameSize = sizeof(szReplicaName);
            dwError = RegQueryValueExW(hKeySubKey, L"Replica Set Name", NULL, &dwType, 
                                      (LPBYTE)szReplicaName, &dwReplicaNameSize);
            
            if (dwError == ERROR_SUCCESS && 
                _wcsicmp(szReplicaName, L"Domain System Volume (SYSVOL share)") != 0)
            {
                bNonSysvolFound = 1;
                break;
            }

            RegCloseKey(hKeySubKey);
            hKeySubKey = NULL;
        }

        free(pszFullPath);
        pszFullPath = NULL;

        dwIndex++;
        dwNameSize = 36;
    }

    if (dwError == ERROR_NO_MORE_ITEMS)
        dwError = ERROR_SUCCESS;

    *pResult = bNonSysvolFound;

Cleanup:
    if (hKeyReplicaSets != NULL)
        RegCloseKey(hKeyReplicaSets);
        
    if (hKeySubKey != NULL)
        RegCloseKey(hKeySubKey);
        
    if (pszSubKeyName != NULL)
        free(pszSubKeyName);
        
    if (pszFullPath != NULL)
        free(pszFullPath);

    return dwError;
}

int NtfrsUpgradeComplianceCheck(void(*pCallback)(CompatCheckContext*, int, UINT), CompatCheckContext* pContext)
{
    DWORD dwError;
    int bNonSysvolFound = 0;

    if (pCallback != NULL && pContext != NULL)
    {
        dwError = NtfrsUpgpComplianceCheck(&bNonSysvolFound);
        if (dwError != ERROR_SUCCESS && bNonSysvolFound)
        {
            pCallback(pContext, 1, 0x1F43);
        }
    }

    return 1;
}

int __cdecl main(void) {
    // Dummy function to make the compiler shut the fuck up with its "libcmt.lib(crt0.obj) : error LNK2019: unresolved external symbol _main referenced in function __mainCRTStartup"
	// error.
    return 0;
}

