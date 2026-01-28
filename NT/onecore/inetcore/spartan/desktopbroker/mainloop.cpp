// mainloop.cpp
#include "exe/precomp.h"

// Global variables
bool g_fMayExitProcess = TRUE;
volatile long g_ServerRefCount = 0;
volatile bool g_fForceServerExit = FALSE;
void* g_hQuitEvent = nullptr;
bool g_fMultiUse = FALSE;
volatile bool g_fNothingYetAllocated = TRUE;
volatile unsigned long g_dwTimer = 0;
unsigned long g_ShutdownWaitInMilleseconds = 0xEA60;

// External declarations
extern "C" {
    int __stdcall CoInitializeEx(void* pvReserved, unsigned long dwCoInit);
    unsigned long __stdcall CoCreateInstance(const GUID& rclsid, void* pUnkOuter, unsigned long dwClsContext, const GUID& riid, void** ppv);
    unsigned long __stdcall CoInitializeSecurity(void* pSecDesc, long cAuthSvc, void* asAuthSvc, void* pReserved1, unsigned long dwAuthnLevel, unsigned long dwImpLevel, void* pAuthList, unsigned long dwCapabilities, void* pReserved3);
    void __stdcall CoUninitialize();
    int __stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(const wchar_t* StringSecurityDescriptor, unsigned long StringSDRevision, void** SecurityDescriptor, unsigned long* SecurityDescriptorSize);
}

unsigned int IncrementServerRefCountProtected();
int WaitToCheckForServerShutdown();

const GUID CLSID_GlobalOptions = 
    {0x0000034B, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

const GUID __GUID_0000015b_0000_0000_c000_000000000046 = 
    {0x0000015B, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

struct ProviderStruct {
    void* RegHandle[2];
    unsigned long LevelPlus1;
    void* ProviderMetadataPtr;
    void* EnableCallback;
    void* CallbackContext;
};

ProviderStruct g_Provider = {0};

void _TlgDefineProvider_annotation() {
    return;
}

unsigned int CheckForServerShutdown() {
    if (g_fMayExitProcess && (g_ServerRefCount < 1 || g_fForceServerExit)) {
        g_fForceServerExit = true;
        if (g_hQuitEvent != nullptr) {
            SetEvent(g_hQuitEvent);
        }
        return 0;
    }
    return 1;
}

unsigned int COMServerCallback(unsigned long dwReason) {
    if (dwReason == 1) {
        return IncrementServerRefCountProtected();
    } else if (dwReason == 2) {
        g_fMayExitProcess = true;
        g_fForceServerExit = true;
        SetEvent(g_hQuitEvent);
    }
    return 0x80004005;
}

long DecrementServerRefCount() {
    int newCount;
    int oldCount;
    
    newCount = g_ServerRefCount - 1;
    
    oldCount = g_ServerRefCount - 1;
    g_ServerRefCount = newCount;
    
    if (oldCount < 1 && g_fMayExitProcess && !g_fForceServerExit) {
        if (g_fMultiUse) {
            WaitToCheckForServerShutdown();
        } else {
            CheckForServerShutdown();
        }
    }
    return oldCount;
}

unsigned int HelperFunction() {
    return 0;
}

long IncrementServerRefCount() {
    g_fNothingYetAllocated = false;
    g_ServerRefCount = g_ServerRefCount + 1;
    return g_ServerRefCount;
}

unsigned int IncrementServerRefCountProtected() {
    if (!g_fForceServerExit && (g_fMultiUse || g_ServerRefCount > 0 || g_fNothingYetAllocated)) {
        g_fNothingYetAllocated = false;
        g_ServerRefCount = g_ServerRefCount + 1;
        return 0;
    }
    return 1;
}

unsigned long __stdcall ThreadProc_WaitToCheckForServerShutdown(void* lpParameter) {
    unsigned long timerState1;
    unsigned long timerState2;
    
    do {
        timerState1 = g_dwTimer;
        if (g_dwTimer == 2) {
            g_dwTimer = 1;
            timerState1 = 2;
        }
        
        timerState2 = g_dwTimer;
        
        while (timerState1 == 2) {
            g_dwTimer = timerState2;
            WaitForSingleObject(g_hQuitEvent, g_ShutdownWaitInMilleseconds);
            
            timerState1 = g_dwTimer;
            if (g_dwTimer == 2) {
                g_dwTimer = 1;
                timerState1 = 2;
            }
            
            timerState2 = g_dwTimer;
        }
        
        if (timerState2 == 1) {
            g_dwTimer = 0;
            timerState2 = 1;
        }
    } while (timerState2 != 1);
    
    CheckForServerShutdown();
    return 0;
}

int WaitToCheckForServerShutdown() {
    unsigned long timerState;
    int threadHandle;
    
    if (!g_fMayExitProcess || g_fForceServerExit) {
        return 0;
    }
    
    timerState = g_dwTimer;
    if (g_dwTimer == 1) {
        g_dwTimer = 2;
        timerState = 1;
    }
    
    if (timerState != 1) {
        timerState = g_dwTimer;
        if (g_dwTimer == 0) {
            g_dwTimer = 2;
            timerState = 0;
        }
        
        if (timerState != 0) {
            return 0;
        }
        
        threadHandle = (int)CreateThread(nullptr, 0, ThreadProc_WaitToCheckForServerShutdown, nullptr, 0, nullptr);
        if (threadHandle == 0) {
            CheckForServerShutdown();
            g_dwTimer = 0;
            threadHandle = HelperFunction();
            return threadHandle;
        }
        CloseHandle((void*)threadHandle);
    }
    return 1;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int result;
    wchar_t* token;
    int cmpResult;
    void* funcPtr;
    unsigned int lastError;
    unsigned int comResult;
    GUID* pGuid;
    int* pInterface;
    int tempInt;
    void* tempPtr;
    int** ppTemp;
    int* pTemp;
    unsigned int tempUint;
    wchar_t* pTokenStr;
    int** ppArg;
    int* pArg;
    unsigned int stackUint;
    unsigned int securityCookie;
    
    unsigned int sdSize1;
    unsigned int sdSize2;
    unsigned int sdSize3;
    unsigned int sdSize4;
    unsigned int sdSize5;
    unsigned int sdSize6;
    char tempBuf1[4];
    unsigned int securityDesc;
    int* pLocalVar;
    char flag;
    char msgBuf[12];
    unsigned int metadata1;
    unsigned int metadata2;
    unsigned int metadata3;
    unsigned int metadata4;
    char sdBuffer1[100];
    char sdBuffer2[100];
    char sdBuffer3[100];
    char sdBuffer4[100];
    unsigned int tokContext1;
    unsigned int tokContext2;
    unsigned short tokContext3;
    unsigned int cookieCheck;

    // Security initialization
    cookieCheck = 0;
    
    metadata1 = *(unsigned int*)((BYTE*)g_Provider.ProviderMetadataPtr - 8);
    lastError = 0x80070057;
    metadata2 = *(unsigned int*)((BYTE*)g_Provider.ProviderMetadataPtr - 6);
    metadata3 = *(unsigned int*)((BYTE*)g_Provider.ProviderMetadataPtr - 4);
    metadata4 = *(unsigned int*)((BYTE*)g_Provider.ProviderMetadataPtr - 2);
    
    g_Provider.EnableCallback = nullptr;
    g_Provider.CallbackContext = nullptr;
    
    result = EventRegister(nullptr, nullptr, nullptr, (PREGHANDLE)g_Provider.RegHandle);
    if (result == 0) {
        EventSetInformation((REGHANDLE)g_Provider.RegHandle[0], (EVENT_INFO_CLASS)0, nullptr, 0);
    }
    
    SetErrorMode(0);
    securityCookie = 1;
    HeapSetInformation(nullptr, (HEAP_INFORMATION_CLASS)0, nullptr, 0);
    
    result = 0;
    tokContext1 = 0x2c0020;
    tokContext2 = 0xa0009;
    tokContext3 = 0;
    pArg = (int*)tempBuf1;
    ppArg = (int**)&tokContext1;
    flag = 0;
    pTokenStr = lpCmdLine;
    
    token = wcstok_s(lpCmdLine, L" ", (wchar_t**)&tempBuf1);
    if (token != nullptr) {
        do {
            cmpResult = _wcsnicmp(token, L"/Embedding", 11);
            if (cmpResult == 0) {
                flag = 1;
                g_fMultiUse = false;
                g_fMayExitProcess = true;
                g_ShutdownWaitInMilleseconds = 30000;
            }
            
            cmpResult = _wcsnicmp(token, L"/MultiUse", 10);
            if (cmpResult == 0) {
                flag = 1;
                g_fMultiUse = true;
                g_fMayExitProcess = true;
                token = wcstok_s(nullptr, L" ", (wchar_t**)&tempBuf1);
                if (token == nullptr) {
                    g_ShutdownWaitInMilleseconds = 300000;
                } else {
                    wchar_t* endPtr;
                    long timeout = wcstol(token, &endPtr, 10);
                    if (endPtr == token) {
                        result = 99;
                    }
                    g_ShutdownWaitInMilleseconds = timeout * 1000;
                }
            }
            
            cmpResult = _wcsnicmp(token, L"/Automation", 10);
            if (cmpResult == 0) {
                flag = 1;
                g_fMultiUse = true;
                g_fMayExitProcess = false;
            }
            
            result++;
            token = wcstok_s(nullptr, L" ", (wchar_t**)&tempBuf1);
        } while (token != nullptr);
        
        if (result == 1 && flag != 0) {
            g_hQuitEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (g_hQuitEvent == nullptr) {
                lastError = GetLastError();
                if (lastError > 0) {
                    lastError = (lastError & 0xFFFF) | 0x80070000;
                }
            } else {
                comResult = CoInitializeEx(nullptr, 2);
                if (comResult <= 0x7FFFFFFF) {
                    void* pGlobalOptions = nullptr;
                    comResult = CoCreateInstance(CLSID_GlobalOptions, nullptr, 1, 
                                                 __GUID_0000015b_0000_0000_c000_000000000046, 
                                                 &pGlobalOptions);
                    if (comResult <= 0x7FFFFFFF) {
                        // Call interface methods
                        funcPtr = GetProcAddress(nullptr, "");
                        if (funcPtr) {
                            comResult = ((unsigned int(__stdcall*)(void))funcPtr)();
                        }
                        
                        funcPtr = GetProcAddress(nullptr, "");
                        if (funcPtr) {
                            ((void(__stdcall*)(void))funcPtr)();
                        }
                        
                        if (comResult <= 0x7FFFFFFF) {
                            securityDesc = 0;
                            sdSize6 = 0;
                            result = ConvertStringSecurityDescriptorToSecurityDescriptorW(
                                L"O:BAG:BAD:(A;;0x3;;;AU)(A;;0x3;;;BG)(A;;0x3;;;AC)", 1, 
                                (void**)&securityDesc, nullptr);
                            if (result == 0) {
                                lastError = GetLastError();
                                if (lastError > 0) {
                                    lastError = (lastError & 0xFFFF) | 0x80070000;
                                }
                            } else {
                                sdSize1 = 100;
                                sdSize2 = 100;
                                sdSize3 = 0;
                                sdSize4 = 100;
                                sdSize5 = 100;
                                result = MakeAbsoluteSD((PSECURITY_DESCRIPTOR)securityDesc, sdBuffer4, (LPDWORD)&sdSize1, 
                                                       (PACL)sdBuffer1, (LPDWORD)&sdSize2, nullptr, (LPDWORD)&sdSize3,
                                                       sdBuffer2, (LPDWORD)&sdSize4, sdBuffer3, (LPDWORD)&sdSize5);
                                if (result == 0) {
                                    lastError = GetLastError();
                                    if (lastError > 0) {
                                        lastError = (lastError & 0xFFFF) | 0x80070000;
                                    }
                                } else {
                                    comResult = CoInitializeSecurity(sdBuffer4, -1, nullptr, 
                                                                    nullptr, 2, 3, nullptr, 0, nullptr);
                                }
                                LocalFree((void*)securityDesc);
                            }
                            
                            if (comResult <= 0x7FFFFFFF) {
                                void* hLib = LoadLibraryExW(L"", nullptr, 0);
                                if (hLib == nullptr) {
                                    lastError = GetLastError();
                                    if (lastError > 0) {
                                        lastError = (lastError & 0xFFFF) | 0x80070000;
                                    }
                                } else {
                                    unsigned long flags = 0x401;
                                    if (!g_fMultiUse) {
                                        flags |= 0;
                                    } else {
                                        flags |= 0x100;
                                    }
                                    if (!g_fMayExitProcess) {
                                        flags |= 0;
                                    } else {
                                        flags |= 0x200;
                                    }
                                    
                                    comResult = 0x8000FFFF;
                                    void* hModule = GetModuleHandleW(L"browserbroker.dll");
                                    if (hModule != nullptr) {
										typedef unsigned int (__stdcall *REGISTER_CALLBACKS_FN)(
											unsigned long (*pIncrement)(void),
											unsigned long (*pDecrement)(void), 
											unsigned int (*pCOMCallback)(unsigned long),
											unsigned long flags);
											
                                        REGISTER_CALLBACKS_FN pfnRegister = (REGISTER_CALLBACKS_FN)GetProcAddress((HMODULE)hModule, "DllGetClassObject");
                                        if (pfnRegister != nullptr) {
                                            comResult = pfnRegister(
												(unsigned long (*)(void))IncrementServerRefCount,
												(unsigned long (*)(void))DecrementServerRefCount,
												COMServerCallback,
												flags);
                                        }
                                    }
                                    
                                    if (comResult <= 0x7FFFFFFF) {
                                        WaitToCheckForServerShutdown();
                                        void* handles[1] = {g_hQuitEvent};
                                        int waitResult = MsgWaitForMultipleObjects(1, (const HANDLE*)handles, 
                                                                                   FALSE, 0xFFFFFFFF, 0x1CFF);
                                        while (waitResult != -1 && waitResult != 0) {
                                            int msgResult = PeekMessageW((LPMSG)msgBuf, nullptr, 0, 0, 1);
                                            while (msgResult != 0) {
                                                TranslateMessage((const MSG*)msgBuf);
                                                DispatchMessageW((const MSG*)msgBuf);
                                                msgResult = PeekMessageW((LPMSG)msgBuf, nullptr, 0, 0, 1);
                                            }
                                            waitResult = MsgWaitForMultipleObjects(1, (const HANDLE*)handles, 
                                                                                   FALSE, 0xFFFFFFFF, 0x1CFF);
                                        }
                                        
                                        comResult = 0x8000FFFF;
                                        hModule = GetModuleHandleW(L"browserbroker.dll");
                                        if (hModule != nullptr) {
                                            funcPtr = GetProcAddress((HMODULE)hModule, "DllCanUnloadNow");
                                            if (funcPtr != nullptr) {
                                                comResult = ((unsigned int(__stdcall*)(void))funcPtr)();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        CoUninitialize();
                    }
                }
                
                CloseHandle(g_hQuitEvent);
            }
        }
    }
    
    EventUnregister((REGHANDLE)g_Provider.RegHandle[0]);
    g_Provider.RegHandle[0] = nullptr;
    g_Provider.RegHandle[1] = nullptr;
    g_Provider.LevelPlus1 = 0;
    
    __security_check_cookie(securityCookie);
    return lastError;
}



