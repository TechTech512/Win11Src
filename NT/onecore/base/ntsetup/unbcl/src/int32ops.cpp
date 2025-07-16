// int32ops.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.

namespace UnBCL {

// === Minimal String class ===

class String {
private:
    wchar_t* m_Data;

public:
    String(const wchar_t* str) {
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const { return m_Data; }

    static String* Format(const wchar_t* format, int value) {
        wchar_t buffer[64];
        _snwprintf(buffer, 64, format, value);
        return new String(buffer);
    }
};

// === Int32Ops namespace ===

class Int32Ops {
public:
    static String* ToString(int value) {
        return String::Format(L"%i", value);
    }
};

} // namespace UnBCL

