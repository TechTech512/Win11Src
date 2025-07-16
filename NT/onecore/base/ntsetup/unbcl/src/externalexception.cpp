// externalexception.cpp

#include <windows.h>

namespace UnBCL {

// === Base classes ===

class Object {
public:
    virtual ~Object() {}
};

class String : public Object {
private:
    wchar_t* m_Data;

public:
    String(const wchar_t* text)
    {
        size_t len = (text != nullptr) ? wcslen(text) : 0;
        m_Data = new wchar_t[len + 1];
        if (text)
            wcscpy_s(m_Data, len + 1, text);
        else
            m_Data[0] = L'\0';
    }

    String(const String& other)
    {
        size_t len = wcslen(other.m_Data);
        m_Data = new wchar_t[len + 1];
        wcscpy_s(m_Data, len + 1, other.m_Data);
    }

    ~String()
    {
        delete[] m_Data;
    }

    const wchar_t* CStr() const { return m_Data; }
};

class Exception : public Object {
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

    Exception(const wchar_t* msg, Exception* inner)
        : m_Message(new String(msg)), m_Inner(inner) {}

    Exception(String* msg, Exception* inner)
        : m_Message(new String(msg ? msg->CStr() : L"Unknown exception")), m_Inner(inner) {}

    virtual ~Exception()
    {
        delete m_Message;
        delete m_Inner;
    }
};

class SystemException : public Exception {
public:
    using Exception::Exception;
};

// === ExternalException ===

class ExternalException : public SystemException {
public:
    ExternalException()
        : SystemException() {}

    ExternalException(const wchar_t* msg)
        : SystemException(msg) {}

    ExternalException(String* msg)
        : SystemException(msg) {}

    ExternalException(const wchar_t* msg, Exception* inner)
        : SystemException(msg, inner) {}

    ExternalException(String* msg, Exception* inner)
        : SystemException(msg, inner) {}
};

} // namespace UnBCL

