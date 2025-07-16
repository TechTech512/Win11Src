// xpathexception.cpp

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

    Exception(String* msg, Exception* inner = nullptr)
        : m_Message(msg), m_Inner(inner) {}

    virtual ~Exception() {
        delete m_Message;
        delete m_Inner;
    }

    const wchar_t* Message() const {
        return m_Message ? m_Message->CStr() : L"(null)";
    }
};

class SystemException : public Exception {
protected:
    unsigned long m_HResult;

public:
    SystemException()
        : Exception(), m_HResult(0x80131501) {}

    SystemException(String* msg, Exception* inner = nullptr)
        : Exception(msg, inner), m_HResult(0x80131501) {}

    unsigned long HResult() const { return m_HResult; }
};

// === XPathException ===

class XPathException : public SystemException {
private:
    int m_LineNumber;

public:
    XPathException(String* msg, Exception* inner, int line)
        : SystemException(msg, inner), m_LineNumber(line) {}

    int GetLineNumber() const {
        return m_LineNumber;
    }
};

} // namespace UnBCL

