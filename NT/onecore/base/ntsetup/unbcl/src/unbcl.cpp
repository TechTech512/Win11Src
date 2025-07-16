// unbcl.cpp

#include <windows.h>

namespace UnBCL {

class SystemInfo {
public:
    virtual ~SystemInfo() {}
};

class OperatingSystem {
public:
    virtual ~OperatingSystem() {}
};

class Environment {
public:
    static SystemInfo* m_SystemInfo;
    static OperatingSystem* m_OSVersion;
};

// Static member definitions
SystemInfo* Environment::m_SystemInfo = nullptr;
OperatingSystem* Environment::m_OSVersion = nullptr;

} // namespace UnBCL

// === Global Cleanup ===

extern "C" void __cdecl UnBCLReleaseResources(void) {
    if (UnBCL::Environment::m_SystemInfo) {
        delete UnBCL::Environment::m_SystemInfo;
        UnBCL::Environment::m_SystemInfo = nullptr;
    }

    if (UnBCL::Environment::m_OSVersion) {
        delete UnBCL::Environment::m_OSVersion;
        UnBCL::Environment::m_OSVersion = nullptr;
    }
}

