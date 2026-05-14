// The CPL basics
#include "precomp.h"

// Prototypes

LONG OnCPlDblClk(int i, HWND hwndParent, LPTSTR pszCmdLine);

// Global Variables


// DllMain
//
// This is the DLL entry point, called whenever the DLL is loaded.

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

// CPlApplet
//
// This is the main entry point for a CPl applet. This exported function
// is called by the control panel.

LONG APIENTRY CPlApplet(
    HWND    hwndCPl,
    UINT    uMsg,
    LPARAM  lParam1,
    LPARAM  lParam2
)
{
    LONG lResult = 0;
    int iSystemMetric = 0;

    switch (uMsg)
    {
    case CPL_INIT:  // CPL_INIT
        iSystemMetric = GetSystemMetrics(0x43);  // SM_CLEANBOOT
        return (LONG)(iSystemMetric == 0);

    case CPL_GETCOUNT:  // CPL_GETCOUNT
        return 1;

    case CPL_INQUIRE:  // CPL_INQUIRE
        ((CPLINFO*)lParam2)->lData = 0;
        ((CPLINFO*)lParam2)->idIcon = 100;
        ((CPLINFO*)lParam2)->idName = 1;
        ((CPLINFO*)lParam2)->idInfo = 2;
        lResult = 0;
        break;

    case CPL_DBLCLK:  // CPL_STOP
        lParam2 = 0;
        lResult = 0;
        break;

    case CPL_STOP:  // CPL_EXIT
    case CPL_EXIT:  // CPL_STARTWPARMS
    case CPL_STARTWPARMSW: // Some custom message
        SetProcessDPIAware();
        lResult = OnCPlDblClk((int)lParam1, hwndCPl, (LPTSTR)lParam2);
        break;

    default:
        lResult = 0;
        break;
    }

    return lResult;
}

// OnCPlDblClk
//
// This message is sent whenever our CPl is selected. In response we display
// our UI and handle input. This is also used when we are started with parameters
// in which case we get passed a command line.

typedef LONG (WINAPI *INTERNALCONFIGPROC)(void);

LONG OnCPlDblClk(int i, HWND hwndCPl, LPTSTR pszCmdLine)
{
    HMODULE hTapi = NULL;
    INTERNALCONFIGPROC pfnInternalConfig = NULL;
    
    hTapi = LoadLibraryW(L"TAPI32.DLL");
    
    if (hTapi != NULL)
    {
        pfnInternalConfig = (INTERNALCONFIGPROC)GetProcAddress(hTapi, "internalConfig");
        if (pfnInternalConfig != NULL)
        {
            pfnInternalConfig();
        }
    }
    
    return 1;
}

