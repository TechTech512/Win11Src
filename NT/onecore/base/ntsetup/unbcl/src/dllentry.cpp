// dllentry.cpp

#include <windows.h>

namespace UnBCL {

class Allocator {
public:
    static void InitTerm(int mode) {
        if (mode)
            OutputDebugStringW(L"[UnBCL] Allocator initialized\n");
        else
            OutputDebugStringW(L"[UnBCL] Allocator terminated\n");
    }
};

class SbRegistrationList {
public:
    static void RegisterStaticTypes(int mode) {
        if (mode)
            OutputDebugStringW(L"[UnBCL] Static types registered\n");
        else
            OutputDebugStringW(L"[UnBCL] Static types unregistered\n");
    }
};

class Environment {
public:
    static void Done() {
        OutputDebugStringW(L"[UnBCL] Environment cleaned up\n");
    }
};

class String {
public:
    static void Done() {
        OutputDebugStringW(L"[UnBCL] String system cleaned up\n");
    }
};

} // namespace UnBCL

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        UnBCL::Allocator::InitTerm(1);
        UnBCL::SbRegistrationList::RegisterStaticTypes(1);
        break;

    case DLL_PROCESS_DETACH:
        UnBCL::SbRegistrationList::RegisterStaticTypes(0);
        UnBCL::Environment::Done();
        UnBCL::String::Done();
        UnBCL::Allocator::InitTerm(0);
        break;
    }

    return TRUE;
}

