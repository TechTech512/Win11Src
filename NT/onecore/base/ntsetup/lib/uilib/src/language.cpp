#include "pch.h"
#include "utlstring.h"
#include <winnt.h>

typedef unsigned int uint;
typedef unsigned long ulong;

#define MULTI_UI_LANGUAGES 0x8

// External function declarations
extern "C" uint GetSupportedUILangugages_ini(uint param1, wchar_t* param2, uint param3, wchar_t* param4);
extern "C" void GetResourceFontName(UTLString* returnStorage, wchar_t* param2, wchar_t* param3, uint param4);
extern "C" int IsLanguageEnabledEx(wchar_t* param1, wchar_t* param2);

unsigned int __cdecl GetSetupDefaultLanguage(wchar_t* outputBuffer, unsigned long bufferSize, wchar_t* param_3, unsigned int param_4)
{
    uint result = 0;
    wchar_t languageListBuffer[520] = {0};
    
    if (outputBuffer == nullptr) {
        return 0x80070057; // E_INVALIDARG
    }
    
    uint supportedLanguages = GetSupportedUILangugages_ini(0, languageListBuffer, param_4, param_3);
    if (supportedLanguages == 0) {
        return 0x80004005; // E_FAIL
    }
    
    for (uint i = 0; i < supportedLanguages; i++) {
        wchar_t languageCodeBuffer[520] = {0};
        wchar_t fontNameBuffer[520] = {0};
        
        uint languageResult = GetSupportedUILangugages_ini(0x104, languageCodeBuffer, 0x104, languageListBuffer);
        if (languageResult != 0 && (languageResult & 1) != 0) {
            UTLString resourceFontName;
            GetResourceFontName(&resourceFontName, reinterpret_cast<wchar_t*>(bufferSize), fontNameBuffer, reinterpret_cast<uint>(languageCodeBuffer));
            
            int isEnabled = IsLanguageEnabledEx(fontNameBuffer, reinterpret_cast<wchar_t*>(bufferSize));
            if (isEnabled != 0) {
                HRESULT copyResult = StringCchCopyW(outputBuffer, bufferSize, reinterpret_cast<wchar_t*>(bufferSize));
                resourceFontName.v_Free();
                result = copyResult;
                goto cleanup;
            }
            resourceFontName.v_Free();
        }
    }
    
    result = StringCchCopyW(outputBuffer, bufferSize, L"en-US");

cleanup:
    return result;
}

unsigned int __cdecl GetVistaThreadLanguage(wchar_t* languageBuffer, unsigned long bufferSize)
{
    uint result = 0;
    uint securityCookie = 0;
    
    if (languageBuffer == nullptr) {
        return 0x80070057; // E_INVALIDARG
    }
    
    *languageBuffer = 0;
    
    HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
    if (kernel32 == nullptr) {
        result = GetLastError();
        if (result > 0) {
            result = result & 0xFFFF | 0x80070000;
        }
        goto cleanup;
    }
    
    FARPROC getThreadPreferredUILanguages = GetProcAddress(kernel32, "GetThreadPreferredUILanguages");
    if (getThreadPreferredUILanguages == nullptr) {
        result = GetLastError();
        if (result > 0) {
            result = result & 0xFFFF | 0x80070000;
        }
        goto cleanup;
    }
    
    DWORD numLanguages = 0;
    BOOL success = reinterpret_cast<BOOL(WINAPI*)(DWORD, DWORD*, wchar_t*, DWORD*)>(getThreadPreferredUILanguages)(
        MULTI_UI_LANGUAGES, &numLanguages, nullptr, nullptr);
    
    if (success != TRUE) {
        result = GetLastError();
        if (result > 0) {
            result = result & 0xFFFF | 0x80070000;
        }
    }

cleanup:
    if (kernel32 != nullptr) {
        FreeLibrary(kernel32);
    }
    return result;
}

