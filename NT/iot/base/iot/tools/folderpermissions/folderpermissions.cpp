/*
 * folderpermissions.c
 *
 * Utility to manage APPX (ALL APPLICATION PACKAGES) permissions on a folder.
 * Allows enabling or removing GENERIC_READ|GENERIC_WRITE (0xC0000000) access
 * for the well-known SID S-1-15-2-1.
 */

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <aclapi.h>
#include <sddl.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

// Access mask used by the original tool: GENERIC_READ | GENERIC_WRITE (0xC0000000)
#define APPX_ACCESS_MASK 0xC0000000

// Forward declarations
static void Usage(void);
static BOOL FolderExists(const wchar_t* path);
static BOOL FolderHasAppxPermissions(const wchar_t* folder);
static BOOL SetFolderPermissions(const wchar_t* folder, BOOL enable);

// ------------------------------------------------------------------
// Usage
// ------------------------------------------------------------------
static void Usage(void)
{
    wprintf(
        L"Usage: FolderPermissions <Folder> [-e | -r]\n"
        L"Where [-e] will enable APPX access to a folder\n"
        L"and [-r] will remove APPX access to a folder\n"
        L"FolderPermissions <Folder> will display current APPX access permissions\n"
        L"\n"
    );
}

// ------------------------------------------------------------------
// Check if a folder exists
// ------------------------------------------------------------------
static BOOL FolderExists(const wchar_t* path)
{
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return FALSE;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// ------------------------------------------------------------------
// Check if the folder has APPX permissions (0xC0000000 mask)
// Returns TRUE if the ACE for ALL APPLICATION PACKAGES grants the exact mask.
// ------------------------------------------------------------------
static BOOL FolderHasAppxPermissions(const wchar_t* folder)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl = NULL;
    BOOL bResult = FALSE;
    PSID pAppxSid = NULL;

    // Create the APPX SID using CreateWellKnownSid (as in original)
    DWORD sidSize = SECURITY_MAX_SID_SIZE;
    pAppxSid = (PSID)LocalAlloc(LPTR, sidSize);
    if (!pAppxSid) return FALSE;
    if (!CreateWellKnownSid(WinBuiltinAnyPackageSid, NULL, pAppxSid, &sidSize)) {
        LocalFree(pAppxSid);
        return FALSE;
    }

    DWORD dwRes = GetNamedSecurityInfoW(
        folder,
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, &pSD
    );
    if (dwRes != ERROR_SUCCESS) {
        LocalFree(pAppxSid);
        return FALSE;
    }

    if (pDacl) {
        ACL_SIZE_INFORMATION aclInfo;
        if (GetAclInformation(pDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation)) {
            for (DWORD i = 0; i < aclInfo.AceCount; i++) {
                LPVOID pAce;
                if (GetAce(pDacl, i, &pAce)) {
                    ACE_HEADER* pHeader = (ACE_HEADER*)pAce;
                    if (pHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ||
                        pHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) {
                        PSID pSid = NULL;
                        if (pHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) {
                            pSid = (PSID)&((ACCESS_ALLOWED_ACE*)pAce)->SidStart;
                        } else {
                            pSid = (PSID)&((ACCESS_ALLOWED_OBJECT_ACE*)pAce)->SidStart;
                        }
                        if (pSid && EqualSid(pSid, pAppxSid)) {
                            DWORD mask = (pHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) ?
                                ((ACCESS_ALLOWED_ACE*)pAce)->Mask :
                                ((ACCESS_ALLOWED_OBJECT_ACE*)pAce)->Mask;
                            // Check for exactly the mask used by the original (0xC0000000)
                            if ((mask & APPX_ACCESS_MASK) == APPX_ACCESS_MASK) {
                                bResult = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    LocalFree(pAppxSid);
    LocalFree(pSD);
    return bResult;
}

// ------------------------------------------------------------------
// Set or revoke APPX permissions on a folder
// enable = TRUE  => add ACE with mask 0xC0000000
// enable = FALSE => remove all ACEs for the APPX SID
// ------------------------------------------------------------------
static BOOL SetFolderPermissions(const wchar_t* folder, BOOL enable)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl = NULL;
    PACL pNewDacl = NULL;
    BOOL bResult = FALSE;
    PSID pAppxSid = NULL;

    // Create the APPX SID (as in original)
    DWORD sidSize = SECURITY_MAX_SID_SIZE;
    pAppxSid = (PSID)LocalAlloc(LPTR, sidSize);
    if (!pAppxSid) {
        wprintf(L"Failed to allocate APPX SID\n");
        return FALSE;
    }
    if (!CreateWellKnownSid(WinBuiltinAnyPackageSid, NULL, pAppxSid, &sidSize)) {
        wprintf(L"CreateWellKnownSid failed with error %lu\n", GetLastError());
        LocalFree(pAppxSid);
        return FALSE;
    }

    // Retrieve current DACL
    DWORD dwRes = GetNamedSecurityInfoW(
        folder,
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, &pSD
    );
    if (dwRes != ERROR_SUCCESS) {
        wprintf(L"GetNamedSecurityInfo failed with error %lu\n", dwRes);
        LocalFree(pAppxSid);
        return FALSE;
    }

    // Build the EXPLICIT_ACCESS structure exactly as the original decompiled code does.
    // The original uses an array of 5 DWORDs, but we'll use the proper structure.
    // We need to set:
    //   grfAccessPermissions = 0xC0000000
    //   grfAccessMode = SET_ACCESS (2) or REVOKE_ACCESS (4)
    //   grfInheritance = 3 (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)
    //   Trustee.TrusteeForm = TRUSTEE_IS_SID (5)
    //   Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN (0)
    //   Trustee.ptstrName = (LPWSTR)pAppxSid
    //   pMultipleTrustee = NULL
    //   MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE (0)
    EXPLICIT_ACCESSW ea = {0};
    ea.grfAccessPermissions = APPX_ACCESS_MASK;          // 0xC0000000
    ea.grfAccessMode = enable ? SET_ACCESS : REVOKE_ACCESS;  // 2 or 4
    ea.grfInheritance = 3;  // OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE
    ea.Trustee.ptstrName = (LPWSTR)pAppxSid;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;             // 5
    ea.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;         // 0
    ea.Trustee.pMultipleTrustee = NULL;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE; // 0

    dwRes = SetEntriesInAclW(1, &ea, pDacl, &pNewDacl);
    if (dwRes == ERROR_SUCCESS) {
        // Apply the new DACL
        dwRes = SetNamedSecurityInfoW(
            (LPWSTR)folder,
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL, NULL, pNewDacl, NULL
        );
        if (dwRes == ERROR_SUCCESS) {
            bResult = TRUE;
        } else {
            wprintf(L"SetNamedSecurityInfo failed with error %lu\n", dwRes);
        }
    } else {
        wprintf(L"SetEntriesInAcl failed with error %lu\n", dwRes);
    }

    if (pNewDacl) LocalFree(pNewDacl);
    LocalFree(pSD);
    LocalFree(pAppxSid);
    return bResult;
}

// ------------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------------
int __cdecl wmain(int argc, wchar_t* argv[])
{
    wprintf(L"APPX Folder Permissions\n");

    if (argc == 1) {
        Usage();
        return 0;
    }

    // Check if folder exists
    const wchar_t* folder = argv[1];
    BOOL isDir = FolderExists(folder);
    if (!isDir) {
        wprintf(L"Folder %s does not exist\n", folder);
        return 1;
    }

    // Show current status if only folder given
    if (argc == 2) {
        BOOL hasPerm = FolderHasAppxPermissions(folder);
        if (hasPerm) {
            wprintf(L"Folder %s has APPX R/W permissions\n", folder);
        } else {
            wprintf(L"Folder %s does not have APPX R/W Permissions\n", folder);
        }
        return 0;
    }

    // One option expected
    if (argc == 3) {
        if (_wcsicmp(argv[2], L"-e") == 0) {
            // Enable
            if (FolderHasAppxPermissions(folder)) {
                wprintf(L"APPX permissions already set on folder %s\n", folder);
                return 0;
            }
            wprintf(L"Setting APPX R/W Permissions on folder %s\n", folder);
            if (SetFolderPermissions(folder, TRUE)) {
                wprintf(L"Success - APPX R/W Permissions now set on folder %s\n", folder);
                return 0;
            } else {
                wprintf(L"Something went wrong, APPX R/W permissions not set on folder %s\n", folder);
                return 1;
            }
        } else if (_wcsicmp(argv[2], L"-r") == 0) {
            // Revoke
            if (!FolderHasAppxPermissions(folder)) {
                wprintf(L"Folder %s does not have APPX R/W permissions\n", folder);
                return 0;
            }
            if (SetFolderPermissions(folder, FALSE)) {
                wprintf(L"Folder %s APPX R/W permissions have been revoked\n", folder);
                return 0;
            } else {
                wprintf(L"Something went wrong, APPX R/W permissions not revoked on folder %s\n", folder);
                return 1;
            }
        } else {
            wprintf(L"Unknown paramater - Usage is displayed below...\n");
            Usage();
            return 1;
        }
    }

    // Too many arguments -> show usage
    Usage();
    return 1;
}

