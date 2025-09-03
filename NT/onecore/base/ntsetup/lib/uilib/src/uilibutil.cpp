#include "pch.h"

typedef unsigned int uint;
typedef unsigned char byte;

unsigned int __cdecl IsCheckBoxCtrl(HWND__* hWnd, int* isCheckBox)
{
    unsigned int result = 0;
    wchar_t classNameBuffer[260] = {0};
    
    if (hWnd == nullptr || isCheckBox == nullptr) {
        return 0x80070057; // E_INVALIDARG
    }
    
    *isCheckBox = 0;
    
    int classNameLength = GetClassNameW(hWnd, classNameBuffer, 260);
    if (classNameLength == 0 || classNameBuffer[0] == 0) {
        result = GetLastError();
        if (result > 0) {
            result = result & 0xFFFF | 0x80070000;
        }
        if ((int)result < 0) {
            result = GetLastError();
            if (result > 0) {
                result = result & 0xFFFF | 0x80070000;
            }
        } else {
            result = 0x80004005; // E_FAIL
        }
        return result;
    }
    
    int compareResult = CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, 
                                     classNameBuffer, -1, L"Button", -1);
    if (compareResult != CSTR_EQUAL) {
        return 0x80004005; // E_FAIL
    }
    
    LONG style = GetWindowLongW(hWnd, GWL_STYLE);
    if (style == 0) {
        result = GetLastError();
        if (result > 0) {
            result = result & 0xFFFF | 0x80070000;
        }
        return result;
    }
    
    unsigned char buttonType = static_cast<unsigned char>(style) & 0xF;
    if (buttonType == BS_CHECKBOX || buttonType == BS_AUTOCHECKBOX || 
        buttonType == BS_3STATE || buttonType == BS_AUTO3STATE) {
        *isCheckBox = 1;
    } else {
        *isCheckBox = 0;
    }
    
    return 0; // S_OK
}

