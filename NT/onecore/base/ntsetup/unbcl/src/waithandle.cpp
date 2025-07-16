// waithandle.cpp

#include <windows.h>

namespace UnBCL {

// === Base Classes ===

class Object {
public:
    virtual ~Object() {}
};

class IDisposable {
public:
    virtual void Dispose() = 0;
};

// === WaitHandle Class ===

class WaitHandle : public Object, public IDisposable {
protected:
    HANDLE m_Handle;

public:
    WaitHandle()
        : m_Handle(INVALID_HANDLE_VALUE) {}

    virtual ~WaitHandle() {
        if (m_Handle && m_Handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
        }
    }

    virtual void Dispose() override {
        if (m_Handle && m_Handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE GetHandle() const {
        return m_Handle;
    }

    void SetHandle(HANDLE handle) {
        if (m_Handle && m_Handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Handle);
        }
        m_Handle = handle;
    }

    bool IsValid() const {
        return (m_Handle && m_Handle != INVALID_HANDLE_VALUE);
    }
};

} // namespace UnBCL
