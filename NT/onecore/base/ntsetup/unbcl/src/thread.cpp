// thread.cpp

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // for malloc, free, _wtoi, _ultow
#include <stdint.h>     // for int64_t, uint8_t, etc.
#include <process.h>

namespace UnBCL {

// === Base Types ===

class Object {
public:
    virtual ~Object() {}
};

class ThreadStartDelegate {
public:
    virtual void Invoke() = 0;
    virtual ~ThreadStartDelegate() {}
};

// === Win32Exception stub ===
class Win32Exception {
public:
    static void ThrowLastError(const wchar_t* /*wsrc*/, const char* /*src*/, int /*line*/) {
        DWORD err = GetLastError();
        wchar_t buf[256];
        swprintf(buf, L"[Win32Exception] LastError: %lu\n", err);
        OutputDebugStringW(buf);
        ExitProcess(err); // simulate throw
    }
};

// === Internal thread launcher ===
DWORD WINAPI StartAddress(LPVOID param) {
    ThreadStartDelegate* del = reinterpret_cast<ThreadStartDelegate*>(param);
    if (del) {
        del->Invoke();
    }
    return 0;
}

// === Thread class ===

class Thread : public Object {
private:
    ThreadStartDelegate* m_Start;
    bool m_Started;
    HANDLE m_Handle;
    DWORD m_Id;

public:
    // Constructor from existing handle
    Thread(void* existingHandle)
        : m_Start(nullptr),
          m_Started(true),
          m_Handle(static_cast<HANDLE>(existingHandle)),
          m_Id(0)
    {}

    // Constructor from delegate (creates thread)
    Thread(ThreadStartDelegate* start)
        : m_Start(start),
          m_Started(false),
          m_Handle(nullptr),
          m_Id(0)
    {
        m_Handle = CreateThread(
            nullptr,         // default security
            0,               // default stack
            StartAddress,    // start address
            start,           // parameter
            0,               // no CREATE_SUSPENDED
            &m_Id            // thread ID
        );

        if (!m_Handle) {
            Win32Exception::ThrowLastError(
                L"onecore\\base\\ntsetup\\unbcl\\src\\thread.cpp",
                "onecore\\base\\ntsetup\\unbcl\\src\\thread.cpp",
                27
            );
        }
    }

    ~Thread() {
        if (m_Handle) {
            CloseHandle(m_Handle);
            m_Handle = nullptr;
        }
    }

    DWORD GetThreadId() const {
        return m_Id;
    }

    HANDLE GetHandle() const {
        return m_Handle;
    }
};

} // namespace UnBCL

