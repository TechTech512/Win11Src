// mutex.cpp

#include <windows.h>

namespace UnBCL {

// === Base Types ===

class IDisposable {
public:
    virtual void Dispose() { /* Default: do nothing */ }
    virtual ~IDisposable() {}
};

class WaitHandle : public IDisposable {
protected:
    HANDLE m_Handle;

public:
    WaitHandle() : m_Handle(INVALID_HANDLE_VALUE) {}

    virtual void Dispose() override {
        if (m_Handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
        }
    }

    virtual ~WaitHandle() {
        Dispose();
    }
};

// === Mutex Class ===

class Mutex : public WaitHandle {
public:
    Mutex();
    // Additional wait/lock functions could be added
};

Mutex::Mutex()
{
    m_Handle = CreateMutexW(nullptr, FALSE, nullptr);

    if (!m_Handle)
        OutputDebugStringW(L"[UnBCL::Mutex] Failed to create mutex\n");
    else
        OutputDebugStringW(L"[UnBCL::Mutex] Mutex created\n");
}

} // namespace UnBCL



