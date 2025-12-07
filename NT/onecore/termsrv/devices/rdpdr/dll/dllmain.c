/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    This module implements the initialization routines for RDP mini redirector network
    provider router interface DLL

Author:

    Joy Chik    1/17/2000

--*/

#include <windows.h>
#include <winternl.h>

// Global variables (likely defined elsewhere)
extern wchar_t ProviderName[260];
extern UNICODE_STRING DrProviderName;

int __cdecl DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        HKEY hKey;
        DWORD dwDataSize;
        LONG lResult;

        DisableThreadLibraryCalls(hInstance);
        
        ProviderName[0] = L'\0';
        
        lResult = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\RDPNP\\NetworkProvider",
            0,
            KEY_READ,
            &hKey
        );
        
        if (lResult == ERROR_SUCCESS)
        {
            dwDataSize = sizeof(ProviderName);
            lResult = RegQueryValueExW(
                hKey,
                L"Name",
                0,
                NULL,
                (LPBYTE)ProviderName,
                &dwDataSize
            );
            
            RegCloseKey(hKey);
            
            if (lResult == ERROR_SUCCESS)
            {
                ProviderName[259] = L'\0';
            }
            else
            {
                ProviderName[0] = L'\0';
            }
        }
        else
        {
            ProviderName[0] = L'\0';
        }
        
        // Calculate string length
        wchar_t* pCurrent = ProviderName;
        while (*pCurrent != L'\0')
        {
            pCurrent++;
        }
        
        // Setup UNICODE_STRING structure
        DrProviderName.Buffer = ProviderName;
        DrProviderName.Length = (USHORT)((pCurrent - ProviderName) * sizeof(wchar_t));
        DrProviderName.MaximumLength = DrProviderName.Length + sizeof(wchar_t);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        return 1;
    }
    
    return 1;
}

