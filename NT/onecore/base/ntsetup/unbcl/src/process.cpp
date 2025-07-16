// process.cpp

#include <windows.h>

namespace UnBCL {

class IDisposable {
public:
    virtual void Dispose() = 0;
};

class Process : public IDisposable {
private:
    HANDLE m_hProcess;
    bool m_OwnHandle;

public:
    Process()
        : m_hProcess(INVALID_HANDLE_VALUE), m_OwnHandle(false)
    {
    }

    ~Process() {
        Close();
    }

    void Close() {
        if (m_OwnHandle && m_hProcess && m_hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hProcess);
            m_hProcess = INVALID_HANDLE_VALUE;
        }
    }

    void Dispose() override {
        Close();
    }

    void SetHandle(HANDLE h, bool owns = true) {
        Close(); // Close any previous handle
        m_hProcess = h;
        m_OwnHandle = owns;
    }

    HANDLE GetHandle() const {
        return m_hProcess;
    }

    bool IsValid() const {
        return m_hProcess && m_hProcess != INVALID_HANDLE_VALUE;
    }

    // Optional: could add Wait(), Terminate(), etc.
};

} // namespace UnBCL

