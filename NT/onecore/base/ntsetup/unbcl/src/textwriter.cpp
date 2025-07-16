// textwriter.cpp

#include <windows.h>

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual void Dispose() = 0;
};

// === Simulated StringPtr class ===
class StringPtr {
private:
    wchar_t* m_Data;

public:
    StringPtr() : m_Data(nullptr) {}

    StringPtr(const wchar_t* str) {
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }

    StringPtr(const StringPtr& other) {
        size_t len = wcslen(other.m_Data);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, other.m_Data);
    }

    ~StringPtr() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const {
        return m_Data;
    }

    void Set(const wchar_t* str) {
        delete[] m_Data;
        if (!str) str = L"";
        size_t len = wcslen(str);
        m_Data = new wchar_t[len + 1];
        wcscpy(m_Data, str);
    }
};

// === TextWriter base ===
class TextWriter : public Object, public IDisposable {
protected:
    StringPtr m_NewLine;

public:
    TextWriter()
        : m_NewLine(L"\r\n") {}

    virtual ~TextWriter() {}

    virtual void Dispose() override {}

    virtual void Write(const wchar_t* /*text*/) {
        // Stub
    }

    virtual void WriteLine(const wchar_t* text) {
        Write(text);
        Write(m_NewLine.CStr());
    }

    void SetNewLine(const wchar_t* newline) {
        m_NewLine.Set(newline);
    }

    const wchar_t* GetNewLine() const {
        return m_NewLine.CStr();
    }
};

} // namespace UnBCL

