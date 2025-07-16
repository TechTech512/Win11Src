// serializationexception.cpp

#include <windows.h>

namespace UnBCL {

// === Supporting types ===

class Object {
public:
    virtual ~Object() {}
};

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
        : m_Message(msg ? new String(msg->CStr()) : new String(L"Unknown exception")), m_Inner(nullptr) {}

    Exception(String* msg, Exception* inner)
        : m_Message(msg ? new String(msg->CStr()) : new String(L"Unknown exception")), m_Inner(inner) {}

    virtual ~Exception() {
        delete m_Message;
        delete m_Inner;
    }
};

class SystemException : public Exception {
public:
    using Exception::Exception;
};

// === SerializationException ===

class SerializationException : public SystemException {
public:
    SerializationException()
        : SystemException() {}

    SerializationException(const wchar_t* msg)
        : SystemException(msg) {}

    SerializationException(String* msg)
        : SystemException(msg) {}

    SerializationException(String* msg, Exception* inner)
        : SystemException(msg, inner) {}
};

} // namespace UnBCL

