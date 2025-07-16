// type.cpp

#include <windows.h>

namespace UnBCL {

// === Base Object ===
class Object {
public:
    virtual ~Object() {}
};

// === Simple String ===
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

    virtual ~String() {
        delete[] m_Data;
    }

    const wchar_t* CStr() const {
        return m_Data;
    }
};

// === Type ===

class Type : public Object {
private:
    String* m_Name;

public:
    Type(String* name)
        : m_Name(name) {}

    virtual ~Type() {
        delete m_Name;
        m_Name = nullptr;
    }

    const String* GetName() const {
        return m_Name;
    }
};

} // namespace UnBCL

