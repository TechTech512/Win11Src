#include <windows.h>
#include <winperf.h>

DWORD GetQueryType(LPWSTR lpValueName)
{
    if (lpValueName == NULL) {
        return 1; // PERF_QUERY_GLOBAL
    }
    
    if (lpValueName[0] == L'\0') {
        return 1; // PERF_QUERY_GLOBAL
    }
    
    // Check for "Global"
    LPWSTR pszGlobal = L"Global";
    LPWSTR pszCurrent = lpValueName;
    while (*pszGlobal != L'\0') {
        if (*pszCurrent != *pszGlobal) {
            break;
        }
        pszCurrent++;
        pszGlobal++;
        if (*pszCurrent == L'\0') {
            return 1; // PERF_QUERY_GLOBAL
        }
    }
    
    // Check for "Foreign"  
    LPWSTR pszForeign = L"Foreign";
    pszCurrent = lpValueName;
    while (*pszForeign != L'\0') {
        if (*pszCurrent != *pszForeign) {
            break;
        }
        pszCurrent++;
        pszForeign++;
        if (*pszCurrent == L'\0') {
            return 3; // PERF_QUERY_FOREIGN
        }
    }
    
    // Check for "Costly"
    LPWSTR pszCostly = L"Costly";
    pszCurrent = lpValueName;
    while (*pszCostly != L'\0') {
        if (*pszCurrent != *pszCostly) {
            break;
        }
        pszCurrent++;
        pszCostly++;
        if (*pszCurrent == L'\0') {
            return 4; // PERF_QUERY_COSTLY
        }
    }
    
    return 2; // PERF_QUERY_ITEMS
}

int MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION* pInstance,
    LPVOID* ppBuffer,
    DWORD dwBufferSize,
    DWORD dwInstanceName,
    DWORD dwParentInstance,
    LPWSTR pszInstanceName)
{
    DWORD dwNameLength = 0;
    DWORD dwAlignedSize = 0;
    DWORD dwCopyLength = 0;
    
    // Calculate instance name length
    if (pszInstanceName != NULL) {
        dwNameLength = lstrlenW(pszInstanceName);
    } else {
        dwNameLength = 0;
    }
    
    // Limit name length to 59 characters maximum
    if (dwNameLength > 59) {
        dwNameLength = 59;
    }
    
    // Calculate required buffer size
    dwCopyLength = dwNameLength * sizeof(WCHAR);
    dwAlignedSize = (dwCopyLength + 0x1F) & 0xFFFFFFF8; // Align to 8-byte boundary
    
    // If the name doesn't end with null terminator in the calculated length, add space for it
    if (dwCopyLength == 0 || (dwNameLength > 0 && pszInstanceName[dwNameLength - 1] != L'\0')) {
        dwAlignedSize += sizeof(WCHAR);
    }
    
    // Ensure we have at least the basic structure size
    if (dwAlignedSize < sizeof(PERF_INSTANCE_DEFINITION)) {
        dwAlignedSize = sizeof(PERF_INSTANCE_DEFINITION);
    }
    
    // Initialize the instance definition structure
    pInstance->ByteLength = dwAlignedSize;
    pInstance->ParentObjectTitleIndex = 0;
    pInstance->ParentObjectInstance = 0;
    pInstance->UniqueID = 0xFFFFFFFF; // PERF_NO_UNIQUE_ID
    pInstance->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pInstance->NameLength = dwCopyLength;
    
    // Copy the instance name if provided
    if (pszInstanceName != NULL && dwCopyLength > 0) {
        LPWSTR pNameDest = (LPWSTR)((BYTE*)pInstance + pInstance->NameOffset);
        memcpy(pNameDest, pszInstanceName, dwCopyLength);
        
        // Ensure null termination
        if (dwCopyLength >= sizeof(WCHAR)) {
            pNameDest[dwNameLength] = L'\0';
        }
    }
    
    // Update buffer pointer if provided
    if (ppBuffer != NULL) {
        *ppBuffer = (BYTE*)pInstance + pInstance->ByteLength;
    }
    
    return 0; // Success
}

