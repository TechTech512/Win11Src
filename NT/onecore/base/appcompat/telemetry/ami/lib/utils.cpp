#include <windows.h>
#include <sddl.h>
#include <strsafe.h>
#include <stdio.h>
#include <accctrl.h>
#include <winternl.h>

// Logging function
extern "C" void __cdecl AslLogCallPrintf(int level, const char* function, unsigned long line, const char* format, ...);

// Security cookie functions (if needed)
#ifdef _MSC_VER
extern "C" uintptr_t __security_cookie;
#endif

// Registry helper function (alternative to RegDeleteKeyValueW)
bool DeleteRegistryValue(HKEY hKey, LPCWSTR subKey, LPCWSTR valueName)
{
    HKEY hSubKey = NULL;
    if (RegOpenKeyExW(hKey, subKey, 0, KEY_SET_VALUE, &hSubKey) == ERROR_SUCCESS) {
        LSTATUS result = RegDeleteValueW(hSubKey, valueName);
        RegCloseKey(hSubKey);
        return result == ERROR_SUCCESS;
    }
    return false;
}

// AmiRegGetStoreFullPath - cleaned up version
unsigned long __cdecl AmiRegGetStoreFullPath(wchar_t* outputPath, unsigned long bufferSize)
{
    wchar_t systemPath[MAX_PATH] = {0};
    wchar_t overridePath[MAX_PATH] = {0};
    const wchar_t* registryPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags";
    const wchar_t* valueName = L"AmiOverridePath";

    // Try to get override path from registry
    DWORD pathSize = sizeof(overridePath);
    LONG result = RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, valueName, 
                             RRF_RT_REG_SZ, NULL, overridePath, &pathSize);

    if (result == ERROR_SUCCESS) {
        if (GetFileAttributesW(overridePath) != INVALID_FILE_ATTRIBUTES) {
            if (SUCCEEDED(StringCchCopyW(outputPath, bufferSize, overridePath))) {
                return ERROR_SUCCESS;
            }
            AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 361, "Buffer overflow");
            return ERROR_INSUFFICIENT_BUFFER;
        }
        
        // Path doesn't exist - remove invalid registry entry
        DeleteRegistryValue(HKEY_LOCAL_MACHINE, registryPath, valueName);
    }

    // Fall back to system directory
    UINT sysDirLen = GetSystemWindowsDirectoryW(systemPath, MAX_PATH);
    if (sysDirLen == 0 || sysDirLen > MAX_PATH) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 335, "GetSystemWindowsDirectory failed [%d]", error);
        return error;
    }

    // Construct default path
    if (FAILED(StringCchPrintfW(outputPath, bufferSize, L"%s\\AppCompat\\Programs", systemPath))) {
        AslLogCallPrintf(1, "AmiRegGetStoreFullPath", 347, "Path construction failed");
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return ERROR_SUCCESS;
}

// AmiUtilityAcquireMutex - cleaned up version
unsigned long __cdecl AmiUtilityAcquireMutex(HANDLE* mutexHandle, wchar_t* mutexName)
{
    wchar_t fullMutexName[MAX_PATH];
    DWORD pid = GetCurrentProcessId();
    swprintf_s(fullMutexName, MAX_PATH, L"Local\\%s_%d", mutexName ? mutexName : L"AmiSharedMutex", pid);

    HANDLE mutex = CreateMutexW(NULL, FALSE, fullMutexName);
    if (mutex == NULL) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityAcquireMutex", 613, "Failed to create mutex [%d]", error);
        return error;
    }

    DWORD waitResult = WaitForSingleObject(mutex, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
        DWORD error = GetLastError();
        CloseHandle(mutex);
        AslLogCallPrintf(1, "AmiUtilityAcquireMutex", 622, "Failed to acquire mutex [%d]", error);
        return error ? error : ERROR_ACCESS_DENIED;
    }

    *mutexHandle = mutex;
    return ERROR_SUCCESS;
}

// AmiUtilityExceedQuota - cleaned up version
int __cdecl AmiUtilityExceedQuota(wchar_t* filePath)
{
    HANDLE file = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityExceedQuota", 421, "Could not open file %S [%d]", filePath, error);
        return 0;
    }

    LARGE_INTEGER fileSize;
    int result = 0;
    
    if (GetFileSizeEx(file, &fileSize)) {
        if (fileSize.QuadPart < 100 * 1024 * 1024) { // 100MB limit
            result = 1;
        } else {
            AslLogCallPrintf(2, "AmiUtilityExceedQuota", 439, "Store size exceeded the limit");
        }
    } else {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityExceedQuota", 430, "Could not get file size for %S [%d]", filePath, error);
    }

    CloseHandle(file);
    return result;
}

// AmiUtilityInitSecurityDescriptor - cleaned up version

unsigned long __cdecl AmiUtilityInitSecurityDescriptor(SECURITY_DESCRIPTOR* securityDescriptor, ACL** acl)
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID sid = NULL;
    unsigned long error = 0;

    // Allocate and initialize the SID
    if (!AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 
                                0, 0, 0, 0, 0, 0, &sid)) {
        error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 140, 
                        "AllocateAndInitializeSid failed [%d]", error);
        goto cleanup;
    }

    // Set up the explicit access structure
    EXPLICIT_ACCESS_W ea = {0};
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPWSTR)sid;

    // Dynamically load SetEntriesInAclW
    HMODULE hAdvapi32 = LoadLibraryW(L"advapi32.dll");
    if (!hAdvapi32) {
        error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 150, 
                        "Failed to load advapi32.dll [%d]", error);
        goto cleanup;
    }

    typedef DWORD (WINAPI *SetEntriesInAclWFunc)(ULONG, PEXPLICIT_ACCESS_W, PACL, PACL*);
    SetEntriesInAclWFunc pSetEntriesInAclW = (SetEntriesInAclWFunc)GetProcAddress(hAdvapi32, "SetEntriesInAclW");
    
    if (!pSetEntriesInAclW) {
        error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 155, 
                        "Failed to get SetEntriesInAclW [%d]", error);
        FreeLibrary(hAdvapi32);
        goto cleanup;
    }

    // Call the function through the pointer
    error = pSetEntriesInAclW(1, &ea, NULL, acl);
    FreeLibrary(hAdvapi32);

    if (error != ERROR_SUCCESS) {
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 175, "SetEntriesInAcl failed [%d]", error);
        goto cleanup;
    }

    // Initialize the security descriptor
    if (!InitializeSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
        error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 182, 
                        "InitializeSecurityDescriptor failed [%d]", error);
        goto cleanup;
    }

    // Set the DACL
    if (!SetSecurityDescriptorDacl(securityDescriptor, TRUE, *acl, FALSE)) {
        error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilityInitSecurityDescriptor", 191, 
                        "SetSecurityDescriptorDacl failed [%d]", error);
    }

cleanup:
    if (sid) FreeSid(sid);
    return error;
}

// AmiUtilityGivePrivilegesToServices - cleaned up version
unsigned long __cdecl AmiUtilityGivePrivilegesToServices(HKEY registryKey)
{
    SECURITY_DESCRIPTOR sd;
    ACL* acl = NULL;
    
    unsigned long error = AmiUtilityInitSecurityDescriptor(&sd, &acl);
    if (error != 0) {
        AslLogCallPrintf(1, "AmiUtilityGivePrivilegesToServices", 233, 
                        "AmiUtilityInitSecurityDescriptor failed [%d]", error);
        return error;
    }

    error = RegSetKeySecurity(registryKey, DACL_SECURITY_INFORMATION, &sd);
    if (error != 0) {
        AslLogCallPrintf(1, "AmiUtilityGivePrivilegesToServices", 242, 
                        "RegSetKeySecurity failed [%d]", error);
    }

    return error;
}

// AmiUtilitySetPrivilege - cleaned up version
unsigned long __cdecl AmiUtilitySetPrivilege(unsigned long privilegeId, unsigned char enable)
{
    unsigned long error = RtlNtStatusToDosError(privilegeId);
    AslLogCallPrintf(1, "AmiUtilitySetPrivilege", 59, "Failed to set privilege [%d]", error);
    return error;
}

// AmiUtilitySharedBlockAlloc - cleaned up version
void* __cdecl AmiUtilitySharedBlockAlloc(wchar_t* name, unsigned int size)
{
    wchar_t mappingName[MAX_PATH];
    DWORD pid = GetCurrentProcessId();
    swprintf_s(mappingName, MAX_PATH, L"Local\\%s_%d", name ? name : L"AmiSharedFileMapping", pid);

    HANDLE hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, mappingName);
    if (hMapping == NULL) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilitySharedBlockAlloc", 698, "Failed to create file mapping [%d]", error);
        return NULL;
    }

    void* pBlock = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (pBlock == NULL) {
        DWORD error = GetLastError();
        AslLogCallPrintf(1, "AmiUtilitySharedBlockAlloc", 709, 
                        "Failed to map view of file [%d]", error);
        CloseHandle(hMapping);
        return NULL;
    }

    // First DWORD is handle, second is refcount, then data
    if (*reinterpret_cast<HANDLE*>(pBlock) == NULL) {
        *reinterpret_cast<HANDLE*>(pBlock) = hMapping;
    }
    reinterpret_cast<ULONG*>(pBlock)[1]++;

    // Return pointer to data area (after handle and refcount)
    return reinterpret_cast<BYTE*>(pBlock) + sizeof(HANDLE) + sizeof(ULONG);
}

