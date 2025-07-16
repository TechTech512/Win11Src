// sehexception.cpp

#include <windows.h>

namespace UnBCL {

// === Base Types ===

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
        wcscpy_s(m_Data, len + 1, str);
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
        : m_Message(new String(msg ? msg->CStr() : L"Unknown exception")), m_Inner(nullptr) {}

    Exception(String* msg, Exception* inner)
        : m_Message(new String(msg ? msg->CStr() : L"Unknown exception")), m_Inner(inner) {}

    virtual ~Exception() {
        delete m_Message;
        delete m_Inner;
    }
};

class SystemException : public Exception {
public:
    using Exception::Exception;
};

class ExternalException : public SystemException {
public:
    using SystemException::SystemException;
};

// === SEHException ===

class SEHException : public ExternalException {
private:
    unsigned long m_Code;

public:
    SEHException()
        : ExternalException(), m_Code(0) {}

    SEHException(unsigned long code)
        : ExternalException(), m_Code(code) {}

    unsigned long GetCode() const {
        return m_Code;
    }

    void SetCode(unsigned long code) {
        m_Code = code;
    }
};

} // namespace UnBCL

