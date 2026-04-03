#pragma warning (disable:4530)

#include "stdafx.h"
#include <set>
#include <vector>

// Global data references (defined elsewhere)
unsigned long DAT_100053f4 = 0;
unsigned long DAT_10005400 = 0;
unsigned int ret = 0;

// Type definitions
struct CGamePanelExternalHook {
    HINSTANCE* _hDll;
    HWND* _hWnd;
    HWND* _hCallback;
    bool _intercept;
    short _padding_;
    std::vector<HHOOK> _hHooks;
};

struct _EnumWindowsLParam {
    std::set<unsigned long> threadIds;
    unsigned long processId;
};

// Static instance
static CGamePanelExternalHook instance;

// Forward declarations
class CGamePanelExternalHookClass {
public:
    static __declspec(dllexport) CGamePanelExternalHook* __cdecl GetInstance(void);
    static int __cdecl EnumWindowsProc(HWND hwnd, LPARAM lParam);
    static LRESULT __cdecl GetMsgProc(int code, WPARAM wParam, LPARAM lParam);
    static __declspec(dllexport) unsigned long __cdecl GPHHookWindowPointerDown(void);
    static unsigned long __cdecl GPHSetIntercept(void);
    static __declspec(dllexport) void __thiscall Hook(CGamePanelExternalHook* thisPtr, HWND* targetWnd);
    static __declspec(dllexport) void __thiscall SetIntercept(CGamePanelExternalHook* thisPtr, bool intercept, HWND* callbackWnd);
    static __declspec(dllexport) void __thiscall Unhook(CGamePanelExternalHook* thisPtr);
};

// Destructor for static instance
void __cdecl dynamic_atexit_destructor_for_instance(void) {
    if (!instance._hHooks.empty()) {
        instance._hHooks.clear();
    }
}

// GetInstance export
__declspec(dllexport) CGamePanelExternalHook* __cdecl CGamePanelExternalHookClass::GetInstance(void) {
    if ((DAT_10005400 & 1) == 0) {
        DAT_10005400 = DAT_10005400 | 1;
        instance._hDll = NULL;
        instance._hWnd = NULL;
        instance._hCallback = NULL;
        instance._intercept = false;
        instance._hHooks.clear();
        atexit(dynamic_atexit_destructor_for_instance);
    }
    return &instance;
}

// EnumWindowsProc callback
int __cdecl CGamePanelExternalHookClass::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (lParam == 0) {
        return 0;
    }
    
    _EnumWindowsLParam* pParam = (_EnumWindowsLParam*)lParam;
    
    unsigned long processId = 0;
    unsigned long threadId = GetWindowThreadProcessId(hwnd, &processId);
    if (processId == pParam->processId) {
        pParam->threadIds.insert(threadId);
    }
    return 1;
}

// GetMsgProc hook procedure
LRESULT __cdecl CGamePanelExternalHookClass::GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
    if ((code == 0) && (lParam != 0)) {
        CGamePanelExternalHook* pThis = GetInstance();
        unsigned long message = *(unsigned long*)(lParam + 4);
        unsigned long interceptMsg = CGamePanelExternalHookClass::GPHSetIntercept();
        
        if (message == interceptMsg) {
            pThis->_intercept = (*(int*)(lParam + 8) != 0);
            pThis->_hCallback = *(HWND**)(lParam + 12);
        }
        else if (pThis->_intercept != false) {
            if (message == 0xff) {
                if (DAT_100053f4 == 0) {
                    DAT_100053f4 = RegisterWindowMessageW(L"{AF451219-E48A-4E61-B08A-B699B0DA2339}");
                }
                *(int*)(lParam + 4) = DAT_100053f4;
            }
            else if ((message == 0x246) && (IsWindow((HWND)pThis->_hCallback) != 0)) {
                unsigned long wParamMsg = *(unsigned long*)(lParam + 8);
                unsigned long lParamMsg = *(unsigned long*)(lParam + 12);
                unsigned long hookMsg = CGamePanelExternalHookClass::GPHHookWindowPointerDown();
                PostMessageW((HWND)pThis->_hCallback, hookMsg, wParamMsg, lParamMsg);
            }
        }
    }
    return CallNextHookEx(0, code, wParam, lParam);
}

// GPHHookWindowPointerDown export
__declspec(dllexport) unsigned long __cdecl CGamePanelExternalHookClass::GPHHookWindowPointerDown(void) {
    if (ret == 0) {
        ret = RegisterWindowMessageW(L"{455ed0d7-0f66-41a2-93bc-babb428fcfc3}");
    }
    return ret;
}

// GPHSetIntercept (internal)
unsigned long __cdecl CGamePanelExternalHookClass::GPHSetIntercept(void) {
    static unsigned long msgRet = 0;
    if (msgRet == 0) {
        msgRet = RegisterWindowMessageW(L"{B9CA1334-AD80-4490-B294-F089D6AB0F85}");
    }
    return msgRet;
}

// Hook export
__declspec(dllexport) void __thiscall CGamePanelExternalHookClass::Hook(CGamePanelExternalHook* thisPtr, HWND* targetWnd) {
    if (thisPtr->_hWnd != targetWnd) {
        Unhook(thisPtr);
        thisPtr->_hWnd = targetWnd;
        
        _EnumWindowsLParam enumParam;
        enumParam.processId = 0;
        
        GetWindowThreadProcessId((HWND)thisPtr->_hWnd, &enumParam.processId);
        EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&enumParam);
        
        for (std::set<unsigned long>::iterator it = enumParam.threadIds.begin(); it != enumParam.threadIds.end(); ++it) {
            HHOOK hook = SetWindowsHookExW(WH_GETMESSAGE, (HOOKPROC)GetMsgProc, (HINSTANCE)thisPtr->_hDll, *it);
            if (hook != 0) {
                thisPtr->_hHooks.push_back(hook);
            }
        }
    }
}

// SetIntercept export
__declspec(dllexport) void __thiscall CGamePanelExternalHookClass::SetIntercept(CGamePanelExternalHook* thisPtr, bool intercept, HWND* callbackWnd) {
    thisPtr->_intercept = intercept;
    if (thisPtr->_hWnd != NULL) {
        unsigned long interceptMsg = GPHSetIntercept();
        PostMessageW((HWND)thisPtr->_hWnd, interceptMsg, (WPARAM)intercept, (LPARAM)callbackWnd);
    }
}

// Unhook export
__declspec(dllexport) void __thiscall CGamePanelExternalHookClass::Unhook(CGamePanelExternalHook* thisPtr) {
    for (unsigned int i = 0; i < thisPtr->_hHooks.size(); ++i) {
        UnhookWindowsHookEx(thisPtr->_hHooks[i]);
    }
    thisPtr->_hHooks.clear();
    thisPtr->_hWnd = NULL;
    thisPtr->_hCallback = NULL;
    thisPtr->_intercept = false;
}

// DllMain
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        CGamePanelExternalHook* pInstance = CGamePanelExternalHookClass::GetInstance();
        pInstance->_hDll = (HINSTANCE*)hinstDLL;
    }
    return TRUE;
}

