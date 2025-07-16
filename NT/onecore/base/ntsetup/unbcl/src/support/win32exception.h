#pragma once

#include <windows.h>
#include "exception.h"
#include "_string.h"

namespace UnBCL {

class Win32Exception : public Exception {
public:
    Win32Exception()
        : m_errorCode(GetLastError()) {}

    Win32Exception(DWORD error)
        : m_errorCode(error) {}

    Win32Exception(DWORD error, String* message)
        : Exception(message), m_errorCode(error) {}

    virtual ~Win32Exception() {}

    DWORD GetErrorCode() const {
        return m_errorCode;
    }

    static void ThrowLastError(String* message = nullptr, const char* file = nullptr, int line = 0) {
        DWORD err = GetLastError();
        Win32Exception* ex = new Win32Exception(err, message);
        throw ex;
    }

protected:
    DWORD m_errorCode = 0;
};

} // namespace UnBCL

