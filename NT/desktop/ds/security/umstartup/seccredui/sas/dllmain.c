#include <windows.h>
#include <winternl.h>

extern int __cdecl WlSecureDesktopSimulateSAS(void);

typedef BOOL (WINAPI *WmsgSendMessageProc)(ULONG SessionId, ULONG Message, WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, PVOID lpvReserved)
{
    if ((fdwReason != DLL_PROCESS_DETACH) && (fdwReason == DLL_PROCESS_ATTACH)) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

void __cdecl SendSAS(int param_1)
{
    if (param_1 == 0) {
        DWORD sessionId = 0;
        ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);

        /* If we are not in session 0, target the active console session. */
        if (sessionId != 0) {
            sessionId = WTSGetActiveConsoleSessionId();
        }

        HMODULE wmsgapi = LoadLibraryA("wmsgapi.dll");
        if (wmsgapi != NULL) {
            WmsgSendMessageProc WmsgSendMessage =
                (WmsgSendMessageProc)GetProcAddress(wmsgapi, "WmsgSendMessage");

            if (WmsgSendMessage != NULL) {
                DWORD sasInfo = 0;
                WmsgSendMessage(sessionId, 0x208, 0, (LPARAM)&sasInfo);
            }
            FreeLibrary(wmsgapi);
        }
    } else {
        WlSecureDesktopSimulateSAS();
    }
}

