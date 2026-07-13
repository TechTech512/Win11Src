/*
 * modernapiexthost.cxx
 *
 * Provides modern API extensions for legacy applications,
 * wrapping PSM (Process State Manager) functions from twinapi.appcore.dll.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

// ------------------------------------------------------------------
// Forward declarations of types used
// ------------------------------------------------------------------
typedef enum _PSM_CURRENT_APP_STATE {
    PsmAppStateUnknown = 0,
    PsmAppStateActive = 1,
    PsmAppStateQuiescing = 2,
    PsmAppStateBackground = 3,
    PsmAppStateResuming = 4,
    PsmAppMaxStates = 5
} PSM_CURRENT_APP_STATE;

typedef enum _PSM_APPSTATE_CHANGE_ROUTINE_CATEGORY {
    StateChangeCategorySystem = 0,
    StateChangeCategoryLibrary = 1,
    StateChangeCategoryFramework = 2,
    StateChangeCategoryApplication = 3,
    StateChangeCategoryMaximum = 4
} PSM_APPSTATE_CHANGE_ROUTINE_CATEGORY;

struct _PSM_APPSTATE_REGISTRATION {
    PVOID reserved[4];
};

typedef _PSM_APPSTATE_REGISTRATION PSM_APPSTATE_REGISTRATION;

typedef enum _PROCESS_UICONTEXT {
    PROCESS_UICONTEXT_DESKTOP = 0,
    PROCESS_UICONTEXT_IMMERSIVE = 1,
    PROCESS_UICONTEXT_IMMERSIVE_BROKER = 2,
    PROCESS_UICONTEXT_IMMERSIVE_BROWSER = 3
} PROCESS_UICONTEXT;

typedef enum _PROCESS_UI_FLAGS {
    PROCESS_UIF_NONE = 0,
    PROCESS_UIF_AUTHORING_MODE = 1,
    PROCESS_UIF_RESTRICTIONS_DISABLED = 2
} PROCESS_UI_FLAGS;

typedef struct _PROCESS_UICONTEXT_INFORMATION {
    PROCESS_UICONTEXT processUIContext;
    PROCESS_UI_FLAGS flags;
} PROCESS_UICONTEXT_INFORMATION;

// ------------------------------------------------------------------
// Function pointer types for PSM functions
// ------------------------------------------------------------------
typedef LONG (__stdcall *PFN_PsmRegisterAppStateChangeNotification)(
    VOID (__stdcall *pfnRoutine)(UCHAR, PVOID, PVOID),
    PSM_APPSTATE_CHANGE_ROUTINE_CATEGORY category,
    ULONG flags,
    PVOID pContext,
    PUCHAR pRegistrationCookie,
    PSM_APPSTATE_REGISTRATION** ppRegistration
);

typedef VOID (__stdcall *PFN_PsmUnregisterAppStateChangeNotification)(
    PSM_APPSTATE_REGISTRATION* pRegistration
);

typedef VOID (__stdcall *PFN_PsmShutdownApplication)(VOID);

typedef VOID (__stdcall *PFN_PsmBlockAppStateChangeCompletion)(
    PSM_APPSTATE_REGISTRATION* pRegistration
);

typedef VOID (__stdcall *PFN_PsmUnblockAppStateChangeCompletion)(
    PSM_APPSTATE_REGISTRATION* pRegistration
);

typedef VOID (__stdcall *PFN_PsmWaitForAppResume)(
    PSM_APPSTATE_REGISTRATION* pRegistration
);

typedef PSM_CURRENT_APP_STATE (__stdcall *PFN_PsmQueryCurrentAppState)(VOID);

// ------------------------------------------------------------------
// Global function pointers
// ------------------------------------------------------------------
HMODULE g_hModTwinApiAppCore = NULL;

PFN_PsmRegisterAppStateChangeNotification g_pfnPsmRegisterAppStateChangeNotification = NULL;
PFN_PsmUnregisterAppStateChangeNotification g_pfnPsmUnregisterAppStateChangeNotification = NULL;
PFN_PsmShutdownApplication g_pfnPsmShutdownApplication = NULL;
PFN_PsmBlockAppStateChangeCompletion g_pfnPsmBlockAppStateChangeCompletion = NULL;
PFN_PsmUnblockAppStateChangeCompletion g_pfnPsmUnblockAppStateChangeCompletion = NULL;
PFN_PsmWaitForAppResume g_pfnPsmWaitForAppResume = NULL;
PFN_PsmQueryCurrentAppState g_pfnPsmQueryCurrentAppState = NULL;

// ------------------------------------------------------------------
// DllMain – Entry point
// ------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_hModTwinApiAppCore) {
            FreeLibrary(g_hModTwinApiAppCore);
            g_hModTwinApiAppCore = NULL;
        }
    }
    return TRUE;
}

// ------------------------------------------------------------------
// LoadTwinApiAppCore – loads twinapi.appcore.dll and resolves exports
// ------------------------------------------------------------------
BOOL LoadTwinApiAppCore(VOID)
{
    if (g_hModTwinApiAppCore)
        return TRUE;

    g_hModTwinApiAppCore = LoadLibraryExW(L"twinapi.appcore.dll", NULL, 0);
    if (!g_hModTwinApiAppCore)
        return FALSE;

    g_pfnPsmRegisterAppStateChangeNotification =
        (PFN_PsmRegisterAppStateChangeNotification)GetProcAddress(
            g_hModTwinApiAppCore, "PsmRegisterAppStateChangeNotification");
    g_pfnPsmUnregisterAppStateChangeNotification =
        (PFN_PsmUnregisterAppStateChangeNotification)GetProcAddress(
            g_hModTwinApiAppCore, "PsmUnregisterAppStateChangeNotification");
    g_pfnPsmShutdownApplication =
        (PFN_PsmShutdownApplication)GetProcAddress(
            g_hModTwinApiAppCore, "PsmShutdownApplication");
    g_pfnPsmBlockAppStateChangeCompletion =
        (PFN_PsmBlockAppStateChangeCompletion)GetProcAddress(
            g_hModTwinApiAppCore, "PsmBlockAppStateChangeCompletion");
    g_pfnPsmUnblockAppStateChangeCompletion =
        (PFN_PsmUnblockAppStateChangeCompletion)GetProcAddress(
            g_hModTwinApiAppCore, "PsmUnblockAppStateChangeCompletion");
    g_pfnPsmWaitForAppResume =
        (PFN_PsmWaitForAppResume)GetProcAddress(
            g_hModTwinApiAppCore, "PsmWaitForAppResume");
    g_pfnPsmQueryCurrentAppState =
        (PFN_PsmQueryCurrentAppState)GetProcAddress(
            g_hModTwinApiAppCore, "PsmQueryCurrentAppState");

    // Verify all required functions are present
    if (!g_pfnPsmRegisterAppStateChangeNotification ||
        !g_pfnPsmUnregisterAppStateChangeNotification ||
        !g_pfnPsmShutdownApplication ||
        !g_pfnPsmBlockAppStateChangeCompletion ||
        !g_pfnPsmUnblockAppStateChangeCompletion ||
        !g_pfnPsmWaitForAppResume ||
        !g_pfnPsmQueryCurrentAppState) {
        FreeLibrary(g_hModTwinApiAppCore);
        g_hModTwinApiAppCore = NULL;
        return FALSE;
    }

    return TRUE;
}

// ------------------------------------------------------------------
// PSM wrapper functions
// ------------------------------------------------------------------
LONG __stdcall PsmRegisterAppStateChangeNotification(
    VOID (__stdcall *pfnRoutine)(UCHAR, PVOID, PVOID),
    PSM_APPSTATE_CHANGE_ROUTINE_CATEGORY category,
    ULONG flags,
    PVOID pContext,
    PUCHAR pRegistrationCookie,
    PSM_APPSTATE_REGISTRATION** ppRegistration)
{
    if (!LoadTwinApiAppCore())
        return ERROR_PROC_NOT_FOUND;  // 0x7E

    return g_pfnPsmRegisterAppStateChangeNotification(
        pfnRoutine, category, flags, pContext, pRegistrationCookie, ppRegistration);
}

VOID __stdcall PsmUnregisterAppStateChangeNotification(
    PSM_APPSTATE_REGISTRATION* pRegistration)
{
    if (LoadTwinApiAppCore() && g_pfnPsmUnregisterAppStateChangeNotification) {
        g_pfnPsmUnregisterAppStateChangeNotification(pRegistration);
    }
}

VOID __stdcall PsmShutdownApplication(VOID)
{
    if (LoadTwinApiAppCore() && g_pfnPsmShutdownApplication) {
        g_pfnPsmShutdownApplication();
    }
}

VOID __stdcall PsmBlockAppStateChangeCompletion(
    PSM_APPSTATE_REGISTRATION* pRegistration)
{
    if (LoadTwinApiAppCore() && g_pfnPsmBlockAppStateChangeCompletion) {
        g_pfnPsmBlockAppStateChangeCompletion(pRegistration);
    }
}

VOID __stdcall PsmUnblockAppStateChangeCompletion(
    PSM_APPSTATE_REGISTRATION* pRegistration)
{
    if (LoadTwinApiAppCore() && g_pfnPsmUnblockAppStateChangeCompletion) {
        g_pfnPsmUnblockAppStateChangeCompletion(pRegistration);
    }
}

VOID __stdcall PsmWaitForAppResume(
    PSM_APPSTATE_REGISTRATION* pRegistration)
{
    if (LoadTwinApiAppCore() && g_pfnPsmWaitForAppResume) {
        g_pfnPsmWaitForAppResume(pRegistration);
    }
}

PSM_CURRENT_APP_STATE __stdcall PsmQueryCurrentAppState(VOID)
{
    if (!LoadTwinApiAppCore() || !g_pfnPsmQueryCurrentAppState) {
        return PsmAppStateUnknown;
    }
    return g_pfnPsmQueryCurrentAppState();
}

// ------------------------------------------------------------------
// Stub functions – always return default values (as per original)
// ------------------------------------------------------------------
BOOL __stdcall DoesLinkTrackingExist(VOID)
{
    return TRUE;
}

BOOL __stdcall DriveIsMounted(LPCWSTR pszDrive)
{
    return TRUE;
}

BOOL __stdcall FileTypeHasPackageManagerHandler(LPCWSTR pszFileType)
{
    return FALSE;
}

BOOL __stdcall IsCurrentNetworkNotTracked(VOID)
{
    return FALSE;
}

BOOL __stdcall IsCurrentNetworkOffTrack(VOID)
{
    return FALSE;
}

BOOL __stdcall IsCurrentNetworkOverLimit(VOID)
{
    return FALSE;
}

BOOL __stdcall ItemIsUnderPhoneRestrictedFolder(LPCWSTR pszPath)
{
    return FALSE;
}

BOOL __stdcall IsImmersiveProcess(LPCWSTR pszProcessName)
{
    return FALSE;
}

BOOL __stdcall GetProcessUIContextInformation(
    HANDLE hProcess,
    PROCESS_UICONTEXT_INFORMATION* pInfo)
{
    if (pInfo) {
        pInfo->processUIContext = PROCESS_UICONTEXT_DESKTOP;
        pInfo->flags = PROCESS_UIF_NONE;
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------
// Stubs for GetCurrentPackageId / GetCurrentPackageInfo
// (left empty as per original)
// ------------------------------------------------------------------
VOID GetCurrentPackageId(VOID)
{
    return;
}

VOID GetCurrentPackageInfo(VOID)
{
    return;
}

