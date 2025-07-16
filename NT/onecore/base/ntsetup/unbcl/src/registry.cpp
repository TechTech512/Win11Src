// registry.cpp

#include <windows.h>

namespace UnBCL {

class Object {
public:
    virtual ~Object() {}
};

class Registry : public Object {
public:
    static HKEY ClassesRoot()       { return HKEY_CLASSES_ROOT; }
    static HKEY CurrentUser()       { return HKEY_CURRENT_USER; }
    static HKEY LocalMachine()      { return HKEY_LOCAL_MACHINE; }
    static HKEY Users()             { return HKEY_USERS; }
    static HKEY CurrentConfig()     { return HKEY_CURRENT_CONFIG; }
    static HKEY PerformanceData()   { return HKEY_PERFORMANCE_DATA; }

    // Add other root keys if needed (e.g., HKEY_DYN_DATA if supported)
};

} // namespace UnBCL

