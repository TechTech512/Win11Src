#include <windows.h>

extern int __cdecl HexStringToDword(wchar_t** ppszString, DWORD* pdwValue, int cchMax, wchar_t chTerminator);

BOOL __cdecl IsHexNum(wchar_t ch)
{
    if ((ch >= L'0' && ch <= L'9') || 
        (ch >= L'A' && ch <= L'F') || 
        (ch >= L'a' && ch <= L'f')) {
        return TRUE;
    }
    return FALSE;
}

wchar_t* __cdecl StringToCLSIDNonNullTerminate(wchar_t* pszString, GUID* pGuid)
{
    if (!pszString || !pGuid) {
        return NULL;
    }
    
    if (pszString[0] != L'{') {
        return NULL;
    }
    
    wchar_t* pCurrent = pszString + 1;
    
    // Parse Data1 (8 hex digits)
    if (!HexStringToDword(&pCurrent, &pGuid->Data1, 8, L'-')) {
        return NULL;
    }
    
    // Parse Data2 (4 hex digits)
    if (!HexStringToDword(&pCurrent, (DWORD*)&pGuid->Data2, 4, L'-')) {
        return NULL;
    }
    
    // Parse Data3 (4 hex digits)
    if (!HexStringToDword(&pCurrent, (DWORD*)&pGuid->Data3, 4, L'-')) {
        return NULL;
    }
    
    // Parse first 2 bytes of Data4
    if (!HexStringToDword(&pCurrent, (DWORD*)&pGuid->Data4[0], 2, 0)) {
        return NULL;
    }
    
    if (!HexStringToDword(&pCurrent, (DWORD*)&pGuid->Data4[1], 2, L'-')) {
        return NULL;
    }
    
    // Parse remaining 6 bytes of Data4
    for (int i = 2; i < 8; i++) {
        if (!HexStringToDword(&pCurrent, (DWORD*)&pGuid->Data4[i], 2, (i == 7) ? L'}' : 0)) {
            return NULL;
        }
    }
    
    return pCurrent;
}

DWORD __cdecl TransNum(wchar_t* pszString, wchar_t** ppszEndPtr)
{
    if (!pszString) {
        return 0;
    }
    
    DWORD dwResult = 0;
    wchar_t* pCurrent = pszString;
    
    // Skip "0x" or "0X" prefix if present
    if (pCurrent[0] == L'0' && (pCurrent[1] == L'x' || pCurrent[1] == L'X')) {
        pCurrent += 2;
    }
    
    while (*pCurrent != L'\0') {
        wchar_t ch = *pCurrent;
        DWORD dwDigitValue = 0;
        
        if (ch >= L'0' && ch <= L'9') {
            dwDigitValue = ch - L'0';
        } else if (ch >= L'A' && ch <= L'F') {
            dwDigitValue = ch - L'A' + 10;
        } else if (ch >= L'a' && ch <= L'f') {
            dwDigitValue = ch - L'a' + 10;
        } else {
            break;
        }
        
        dwResult = (dwResult << 4) | dwDigitValue;
        pCurrent++;
    }
    
    if (ppszEndPtr != NULL) {
        *ppszEndPtr = pCurrent;
    }
    
    return dwResult;
}

