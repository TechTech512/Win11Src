// UnattendLog.cpp
#include <windows.h>
#include <string.h>
#include <strsafe.h>

static CRITICAL_SECTION s_ULogSynchronizer;
static HINSTANCE s_hWdsCore = nullptr;

static int s_nUnattendLogInitRefCount = 0;
static bool s_bSuccessfullyInitialized = false;
static bool s_bInitializedWdsLogging = false;

using tagLOG_PARTIAL_MSG = void;

using WdsSetupLogInitFn =
    tagLOG_PARTIAL_MSG* (__cdecl*)(HINSTANCE*, unsigned long, wchar_t*);
using WdsGenericSetupLogInitFn =
    int (__cdecl*)(wchar_t*, unsigned long);
using WdsSetupLogDestroyFn =
    void (__cdecl*)();
using WdsSetupLogMessageWFn =
    int (__cdecl*)(tagLOG_PARTIAL_MSG*, unsigned long, const wchar_t*, char*, unsigned long,
                   const wchar_t*, const wchar_t*, void*, unsigned long,
                   const wchar_t*, const wchar_t*, unsigned int);
using ConstructPartialMsgVWFn =
    tagLOG_PARTIAL_MSG* (__cdecl*)(unsigned long, char*, char*);
using CurrentIPFn =
    void* (__cdecl*)();

static WdsSetupLogInitFn s_fpWdsSetupLogInit = nullptr;
static WdsGenericSetupLogInitFn s_fpWdsGenericSetupLogInit = nullptr;
static WdsSetupLogDestroyFn s_fpWdsSetupLogDestroy = nullptr;
static WdsSetupLogMessageWFn s_fpWdsSetupLogMessageW = nullptr;
static ConstructPartialMsgVWFn s_fpConstructPartialMsgVW = nullptr;
static CurrentIPFn s_fpCurrentIP = nullptr;

void InitializeLogCriticalSection() {
    InitializeCriticalSection(&s_ULogSynchronizer);
    atexit([]() {
        DeleteCriticalSection(&s_ULogSynchronizer);
    });
}

unsigned long InitFuncPointerTable() {
    if (!s_hWdsCore) return E_FAIL;

    s_fpWdsSetupLogInit = reinterpret_cast<WdsSetupLogInitFn>(
        GetProcAddress(s_hWdsCore, "WdsSetupLogInit"));
    s_fpWdsGenericSetupLogInit = reinterpret_cast<WdsGenericSetupLogInitFn>(
        GetProcAddress(s_hWdsCore, "WdsGenericSetupLogInit"));
    s_fpWdsSetupLogDestroy = reinterpret_cast<WdsSetupLogDestroyFn>(
        GetProcAddress(s_hWdsCore, "WdsSetupLogDestroy"));
    s_fpWdsSetupLogMessageW = reinterpret_cast<WdsSetupLogMessageWFn>(
        GetProcAddress(s_hWdsCore, "WdsSetupLogMessageW"));
    s_fpConstructPartialMsgVW = reinterpret_cast<ConstructPartialMsgVWFn>(
        GetProcAddress(s_hWdsCore, "ConstructPartialMsgVW"));
    s_fpCurrentIP = reinterpret_cast<CurrentIPFn>(
        GetProcAddress(s_hWdsCore, "CurrentIP"));

    if (!s_fpWdsSetupLogInit || !s_fpWdsGenericSetupLogInit || !s_fpWdsSetupLogDestroy ||
        !s_fpWdsSetupLogMessageW || !s_fpConstructPartialMsgVW || !s_fpCurrentIP) {
        DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err);
    }

    return 0;
}

void UnattendInitializeLogEx2(int, wchar_t*) {
    EnterCriticalSection(&s_ULogSynchronizer);

    ++s_nUnattendLogInitRefCount;
    if (!s_bSuccessfullyInitialized) {
        s_hWdsCore = LoadLibraryExW(L"wdscore.dll", nullptr, 0);
        if (s_hWdsCore) {
            if (SUCCEEDED(InitFuncPointerTable())) {
                s_bSuccessfullyInitialized = true;
                s_bInitializedWdsLogging = false;
            } else {
                FreeLibrary(s_hWdsCore);
                s_hWdsCore = nullptr;
            }
        }
    }

    LeaveCriticalSection(&s_ULogSynchronizer);
}

void UnattendFinalizeLog() {
    EnterCriticalSection(&s_ULogSynchronizer);

    if (--s_nUnattendLogInitRefCount == 0 && s_bSuccessfullyInitialized && s_hWdsCore) {
        if (s_bInitializedWdsLogging && s_fpWdsSetupLogDestroy) {
            s_fpWdsSetupLogDestroy();
        }
        FreeLibrary(s_hWdsCore);
        s_hWdsCore = nullptr;
        s_bSuccessfullyInitialized = false;
        s_bInitializedWdsLogging = false;
    }

    LeaveCriticalSection(&s_ULogSynchronizer);
}

void UnattendLogWV(unsigned long level, wchar_t* message, char* extraInfo) {
    if (!message || level == 0) return;

    wchar_t moduleName[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, moduleName, MAX_PATH);

    const wchar_t* baseName = wcsrchr(moduleName, L'\\');
    baseName = baseName ? baseName + 1 : moduleName;

    size_t totalSize = wcslen(baseName) + wcslen(message) + 4;
    wchar_t* formattedMessage = new wchar_t[totalSize];

    if (SUCCEEDED(StringCchPrintfW(formattedMessage, totalSize, L"[%s] %s", baseName, message))) {
        unsigned long logLevel = 0x4000000;
        if (level == 1) logLevel = 0x2000000;
        else if (level == 2) logLevel = 0x3000000;

        if (s_fpWdsSetupLogMessageW && s_fpConstructPartialMsgVW && s_fpCurrentIP) {
            void* currentIp = s_fpCurrentIP();
            tagLOG_PARTIAL_MSG* msg = s_fpConstructPartialMsgVW(logLevel, reinterpret_cast<char*>(formattedMessage), extraInfo);
            if (msg) {
                s_fpWdsSetupLogMessageW(
                    msg, logLevel, formattedMessage, extraInfo, 0,
                    L"D",
                    nullptr,
                    reinterpret_cast<void*>(0x1d6),0,
                    L"onecore\\base\\ntsetup\\lib\\unattendlog\\src\\unattendlog.cpp",
                    L"UnattendLogWV",
                    reinterpret_cast<UINT>(currentIp));
            }
        }
    }

    delete[] formattedMessage;
}

void UnattendLogW(unsigned long level, wchar_t* message) {
    UnattendLogWV(level, message, nullptr);
}

