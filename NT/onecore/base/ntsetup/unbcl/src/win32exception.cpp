// win32exception.cpp

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_string.h"
#include "smartptr.h"
#include "exception.h"

namespace UnBCL {

class ExternalException : public Exception {
public:
    ExternalException(String* msg) : Exception(msg) {}
    ExternalException(String* msg, Exception* inner) : Exception(msg, inner) {}
};

// Helper to return a clean Win32 message
String* pWin32ErrorText(unsigned long errorCode) {
    wchar_t* buffer = nullptr;

    DWORD len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    if (!len || !buffer) {
        return new String(L"Unknown Win32 error");
    }

    // Strip control characters
    for (wchar_t* p = buffer; *p; ++p) {
        if (*p < 0x20 && *p != L'\t') {
            *p = L' ';
        }
    }

    String* result = new String(buffer);
    LocalFree(buffer);
    return result;
}

class Win32Exception : public ExternalException {
private:
    ULONG m_Code;

    static String* GetWin32Message(ULONG code) {
        return pWin32ErrorText(code);
    }

public:
    Win32Exception()
        : Win32Exception(GetLastError()) {}

    Win32Exception(ULONG code)
        : ExternalException(String::Format(GetWin32Message(code)->CStr(), nullptr, code)), m_Code(code) {}

    Win32Exception(ULONG code, const wchar_t* user)
        : ExternalException(String::Format(GetWin32Message(code)->CStr(), user, code)), m_Code(code) {}

    Win32Exception(ULONG code, String* user)
        : ExternalException(String::Format(GetWin32Message(code)->CStr(), user ? user->CStr() : nullptr, code)), m_Code(code) {}

    Win32Exception(ULONG code, const wchar_t* user, Exception* inner)
        : ExternalException(String::Format(GetWin32Message(code)->CStr(), user, code), inner), m_Code(code) {}

    Win32Exception(ULONG code, String* user, Exception* inner)
        : ExternalException(String::Format(GetWin32Message(code)->CStr(), user ? user->CStr() : nullptr, code), inner), m_Code(code) {}

    ULONG Code() const { return m_Code; }
};

} // namespace UnBCL

