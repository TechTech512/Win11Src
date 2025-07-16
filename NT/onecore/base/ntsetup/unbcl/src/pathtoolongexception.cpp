// pathtoolongexception.cpp

#include <windows.h>

namespace UnBCL {

// === Supporting Classes ===

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

    String(const String& other) {
        size_t len = wcslen(other.m_Data);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, other.m_Data);
    }

    ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const { return m_Data; }
};

class Exception {
protected:
    String* m_Message;
    Exception* m_Inner;

public:
    Exception()
        : m_Message(new String(L"Unknown exception")), m_Inner(nullptr) {}

    Exception(const wchar_t* msg)
        : m_Message(new String(msg)), m_Inner(nullptr) {}

    Exception(String* msg)
        : m_Message(new String(msg ? msg->CStr() : L"Unknown exception")), m_Inner(nullptr) {}

    virtual ~Exception() {
        delete m_Message;
        delete m_Inner;
    }
};

class SystemException : public Exception {
protected:
    unsigned long m_HResult;

public:
    using Exception::Exception;

    SystemException()
        : Exception(), m_HResult(0) {}

    SystemException(const wchar_t* msg)
        : Exception(msg), m_HResult(0) {}

    SystemException(String* msg)
        : Exception(msg), m_HResult(0) {}

    virtual ~SystemException() {}
};

// === PathTooLongException ===

class PathTooLongException : public SystemException {
public:
    PathTooLongException()
        : SystemException()
    {
        m_HResult = 0x800700ce; // ERROR_FILENAME_EXCED_RANGE
    }

    PathTooLongException(const wchar_t* msg)
        : SystemException(msg)
    {
        m_HResult = 0x800700ce;
    }

    PathTooLongException(String* msg)
        : SystemException(msg)
    {
        m_HResult = 0x800700ce;
    }
};

} // namespace UnBCL

