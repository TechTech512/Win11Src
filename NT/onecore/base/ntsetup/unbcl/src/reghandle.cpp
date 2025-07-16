// reghandle.cpp

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

class RegHandle : public Object, public IDisposable {
private:
    HKEY m_hKey;
    bool m_OwnHandle;

public:
    RegHandle()
        : m_hKey(nullptr), m_OwnHandle(false) {}

    RegHandle(HKEY hKey, bool owns)
        : m_hKey(hKey), m_OwnHandle(owns) {}

    ~RegHandle() {
        Close();
    }

    void Dispose() override {
        Close();
    }

    void Close() {
        if (m_OwnHandle && m_hKey) {
            RegCloseKey(m_hKey);
            m_hKey = nullptr;
            m_OwnHandle = false;
        }
    }

    HKEY Get() const {
        return m_hKey;
    }

    void Set(HKEY hKey, bool owns) {
        Close();
        m_hKey = hKey;
        m_OwnHandle = owns;
    }

    bool IsValid() const {
        return m_hKey != nullptr;
    }
};

} // namespace UnBCL

