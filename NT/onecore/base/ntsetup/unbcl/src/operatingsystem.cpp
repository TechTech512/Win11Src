// operatingsystem.cpp

#include <windows.h>

namespace UnBCL {

// === Base Framework ===

template <typename T = void>
class SmartPtr {
public:
    T* m_pObj;

    SmartPtr() : m_pObj(nullptr) {}
    SmartPtr(T* p) : m_pObj(p) {}
    ~SmartPtr() { delete m_pObj; }

    T* operator->() const { return m_pObj; }
    T*& GetRef() { return m_pObj; }
    operator bool() const { return m_pObj != nullptr; }
};

// === PlatformID Enum ===

enum PlatformID {
    PlatformID_Win32S = 0,
    PlatformID_Win32Windows = 1,
    PlatformID_Win32NT = 2,
    PlatformID_WinCE = 3
};

// === Exception Types ===

class ArgumentException {
public:
    ArgumentException(const wchar_t* msg) {
        OutputDebugStringW(L"[ArgumentException] ");
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
    }
};

class ArgumentNullException : public ArgumentException {
public:
    ArgumentNullException(const wchar_t* msg) : ArgumentException(msg) {}
};

class ArgumentOutOfRangeException : public ArgumentException {
public:
    ArgumentOutOfRangeException(const wchar_t* msg) : ArgumentException(msg) {}
};

template <typename T>
T* AddStackTraceToException(T* ex, const char*) {
    return ex;
}

// === Version ===

class Version {
private:
    int m_Major;
    int m_Minor;

public:
    Version() : m_Major(0), m_Minor(0) {}
    Version(const Version* other) {
        if (other) {
            m_Major = other->m_Major;
            m_Minor = other->m_Minor;
        } else {
            m_Major = m_Minor = 0;
        }
    }
};

// === OperatingSystem ===

class OperatingSystem {
private:
    PlatformID m_Platform;
    unsigned long m_Architecture;
    SmartPtr<Version> m_Version;

public:
    // Default constructor
    OperatingSystem()
        : m_Platform(PlatformID_Win32NT),
          m_Architecture(0),
          m_Version(nullptr)
    {
        OutputDebugStringW(L"[OperatingSystem] Default constructed\n");
    }

    // Copy constructor
    OperatingSystem(const OperatingSystem& other)
        : m_Platform(other.m_Platform),
          m_Architecture(other.m_Architecture)
    {
        if (other.m_Version)
            m_Version = new Version(other.m_Version.m_pObj);
        else
            m_Version = nullptr;

        OutputDebugStringW(L"[OperatingSystem] Copied\n");
    }

    // Full constructor
    OperatingSystem(PlatformID platform, unsigned long arch, Version* version)
        : m_Platform(platform), m_Architecture(arch), m_Version(version)
    {
        if (platform < 0 || platform > PlatformID_WinCE) {
            auto* ex = new ArgumentOutOfRangeException(L"invalid platform to OperatingSystem constructor");
            throw AddStackTraceToException(ex, nullptr);
        }

        if (!version) {
            auto* ex = new ArgumentNullException(L"null version to OperatingSystem constructor");
            throw AddStackTraceToException(ex, nullptr);
        }

        OutputDebugStringW(L"[OperatingSystem] Constructed with platform/version\n");
    }

    // Platform-only constructor with default arch
    OperatingSystem(PlatformID platform, Version* version)
        : OperatingSystem(platform, 0xFFFF, version)
    {
        OutputDebugStringW(L"[OperatingSystem] Constructed with platform only\n");
    }

    ~OperatingSystem() {
        OutputDebugStringW(L"[OperatingSystem] Destroyed\n");
    }
};

} // namespace UnBCL

