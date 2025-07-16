// xmlexception.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.

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

    static String* Format(unsigned long hr, int line, int pos, const wchar_t* msg) {
        wchar_t buffer[512];
        swprintf(buffer, L"hResult = 0x%08X, Line = %d, Position = %d; %s.",
                   hr, line, pos, msg ? msg : L"(null)");
        return new String(buffer);
    }
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

    void SetMessage(String* msg) {
        delete m_Message;
        m_Message = msg;
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

// === XmlException ===

class XmlException : public SystemException {
private:
    int m_LineNumber;
    int m_LinePosition;

public:
    XmlException(String* userMessage, Exception* inner, int line, int pos, int hresult)
        : SystemException(nullptr, inner), m_LineNumber(line), m_LinePosition(pos)
    {
        SetMessage(String::Format(hresult, line, pos, userMessage ? userMessage->CStr() : L""));
        m_HResult = static_cast<unsigned long>(hresult);
    }

    int GetLineNumber() const { return m_LineNumber; }
    int GetLinePosition() const { return m_LinePosition; }
};

} // namespace UnBCL

