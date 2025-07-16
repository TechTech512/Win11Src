// monitor.cpp

#include <windows.h>

namespace UnBCL {

// === Monitor class ===
class Monitor
{
public:
    Monitor();
    virtual ~Monitor();
};

// === Constructor ===
Monitor::Monitor()
{
    // In real logic, this might set up a mutex or lock primitive
    OutputDebugStringW(L"[Monitor] Constructed\n");
}

// === Destructor ===
Monitor::~Monitor()
{
    OutputDebugStringW(L"[Monitor] Destroyed\n");
}

} // namespace UnBCL

