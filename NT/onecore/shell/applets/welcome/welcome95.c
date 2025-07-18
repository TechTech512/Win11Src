//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "welcome95.h"	
#include "resource95.h"
#include <regstr.h>
#include "..\..\inc\help.h"


//---------------------------------------------------------------------------
// Global to this file only...

//---------------------------------------------------------------------------
// Global to the app...
HINSTANCE g_hinst;
HBRUSH  g_hbrInfoBk = NULL;
BOOL    g_fForceShowNextTime = FALSE;

TCHAR const FAR   g_szRegTips[]  = REGSTR_PATH_EXPLORER TEXT("\\Tips");
TCHAR const FAR   g_szRegTour[]  = REGSTR_PATH_SETUP TEXT("\\SETUP\\optionalComponents\\tour");
TCHAR const FAR   g_szRegOLRKey[] = REGSTR_PATH_SETUP;
TCHAR const FAR   g_szRegOLR[]  =  TEXT("REGDONE");
TCHAR const FAR   g_szRegSetup[] = REGSTR_PATH_SETUP TEXT("\\SETUP");
TCHAR const FAR   g_szRegWiz[] = TEXT("Software\\Microsoft\\Shared Tools\\Registration Wizard\\1.0");

TCHAR const FAR   g_szWelcomeNext[] = TEXT("Next");
TCHAR const FAR   g_szWelcomeShow[] = TEXT("Show");
TCHAR const FAR   g_szInstalled[] = TEXT("installed");
TCHAR const FAR   g_szAlibaba[] = TEXT("Expostrt");
TCHAR const FAR   g_szLocation[] = TEXT("Location");
TCHAR const FAR   g_szTourPath[] = TEXT("TourPath");

//---------------------------------------------------------------------------
// Helper function used to remove unwanted buttons...
void PASCAL ExchangeWindowPos(HWND hwnd0, HWND hwnd1)
{
	HWND hParent;
	RECT rc[2];

	hParent = GetParent(hwnd0);
	Assert(hParent == GetParent(hwnd1));

	GetWindowRect(hwnd0, &rc[0]);
	GetWindowRect(hwnd1, &rc[1]);

	MapWindowPoints(HWND_DESKTOP, hParent, (LPPOINT)rc, 4);

	SetWindowPos(hwnd0, NULL, rc[1].left, rc[1].top, 0, 0,
		SWP_NOZORDER|SWP_NOSIZE);
	SetWindowPos(hwnd1, NULL, rc[0].left, rc[0].top, 0, 0,
		SWP_NOZORDER|SWP_NOSIZE);
}

//---------------------------------------------------------------------------
BOOL NEAR PASCAL InitApplication(HINSTANCE hInstance)
{
    return TRUE;
}


//---------------------------------------------------------------------------
BOOL NEAR PASCAL ShowNextTip(HWND hwndDlg)
{
    HKEY    hkeyUser;
    HKEY    hkey;
    DWORD   cbData;
    DWORD   dwType;
    BOOL    fShow = TRUE;

    //
    // Now to get the tip text and options.  Note: the tip text is global
    // for the machine, so it is under HKEY_LOCAL_MACHINE, whereas the
    // options and the next tip to show are per user, so we store these
    // options under HKEY_CURRENT_USER.  First get the Per user options...

    if (RegCreateKey(HKEY_CURRENT_USER, g_szRegTips, &hkeyUser) == ERROR_SUCCESS)
    {
        SHORT   sNext;
        TCHAR    szKeyName[8];
        TCHAR    szTipText[256];
        LONG     lResult;

        cbData = sizeof(sNext);

        // Get the index...
        if (RegQueryValueEx(hkeyUser, (LPTSTR)g_szWelcomeNext, NULL, &dwType, (LPBYTE)&sNext,
                &cbData) != ERROR_SUCCESS)
            sNext = 0;

        wsprintf(szKeyName, TEXT("%d"), (int)sNext);

        // Now lets open the tips key under the local machine...

        if (RegOpenKey(HKEY_LOCAL_MACHINE, g_szRegTips, &hkey) == ERROR_SUCCESS)
        {
            cbData=ARRAYSIZE(szTipText) * sizeof(TCHAR);
            lResult = RegQueryValueEx(hkey, szKeyName, NULL, &dwType, (LPBYTE)szTipText,
                                      &cbData);

            if (lResult == ERROR_FILE_NOT_FOUND) {
                sNext=0;
                wsprintf(szKeyName, TEXT("%d"), (int)sNext);

                lResult = RegQueryValueEx(hkey, szKeyName, NULL, &dwType, (LPBYTE)szTipText,
                                          &cbData);
            }

            if (lResult == ERROR_SUCCESS) {
                SetDlgItemText(hwndDlg, IDC_WELCOME_TIPS, szTipText);
                sNext++;
            } else {
                sNext=0;
            }
        }

        // Setup for the next one.
        RegSetValueEx(hkeyUser, g_szWelcomeNext, 0, REG_BINARY, (CONST LPBYTE)
                &sNext, (DWORD)sizeof(sNext));

        // Get the Show field from the registry
        cbData = sizeof(fShow);

        // Get the index...
        if (RegQueryValueEx(hkeyUser, (LPTSTR)g_szWelcomeShow, NULL, &dwType, (LPBYTE)&fShow,
                &cbData) != ERROR_SUCCESS)
            fShow = TRUE;

        RegCloseKey(hkeyUser);
    }

    return(fShow);
}
//---------------------------------------------------------------------------

BOOL ExecApplication(LPTSTR lpApp)
{
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Result;

    //
    // Initialize process startup info
    //
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.lpDesktop = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;


    //
    // Start the app
    //
    Result = CreateProcess(
                      NULL,   // Image name
                      lpApp,   // Command line
                      NULL,  // Default process protection
                      NULL,  // Default thread protection
                      FALSE, // Don't inherit handles
                      NORMAL_PRIORITY_CLASS,
                      NULL,  // Inherit environment
                      NULL,  // Inherit current directory
                      &si,
                      &ProcessInformation
                      );

    if (Result) {

        //
        // Close our handles to the process and thread
        //

        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);

    }

    return(Result);
}

//---------------------------------------------------------------------------
//
void NEAR PASCAL CheckOptionalComponentAndRemoveButton(HWND hwndDlg,
        LPCTSTR szComponentKey, LPCTSTR szComponentValueName,
        BOOL fInstalled, int idcComponent)
{
    HKEY hkey;
    DWORD   cbData;
    DWORD   dwType;


    // See if we need to remove the Tour button from the dialog.
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szComponentKey, &hkey)
            == ERROR_SUCCESS)
    {
        TCHAR szTemp[10];

        cbData=ARRAYSIZE(szTemp) * sizeof(TCHAR);
        if (RegQueryValueEx(hkey, szComponentValueName, NULL, &dwType,
                    (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS)
        {
            if (szTemp[0] == TEXT('1'))
                fInstalled = !fInstalled;
        }

        RegCloseKey(hkey);
    }

    if (!fInstalled)
    {
        HWND hwndComponent;
        HWND hwndT;
        int idc;
        // If more buttons installed make this a loop...

        hwndComponent = GetDlgItem(hwndDlg, idcComponent);

        // Some items may only exist in of the two dialogs...
        if (!hwndComponent)
            return;

        for (idc=idcComponent + 1; idc <= IDC_WELCOME_NEXTTIP; idc++)
        {
            hwndT = GetDlgItem(hwndDlg, idc);
            if (hwndT)
                ExchangeWindowPos(hwndComponent, hwndT);
        }

        // And destroy the tour window...
        DestroyWindow(hwndComponent);
    }
}

void NEAR PASCAL _InitRedCarpetDialog(HWND hwndDlg)
{
    HBITMAP hbm;
    HWND    hwndCtl;
    NONCLIENTMETRICS ncm;
    BOOL    fShow = TRUE;
    BOOL    fFirst = FALSE;
    COLORREF colorref;
    HDC hdc;
    BOOL    fTourInstalled = FALSE;
    TCHAR    szRelPath[MAX_PATH];



    if (hbm = LoadImage(g_hinst, MAKEINTRESOURCE(IDB_WELCOME_TITLE),
            IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS))
    {
        SendDlgItemMessage(hwndDlg, IDC_WELCOME_TITLE, STM_SETIMAGE, 0,
                (LPARAM)hbm);
    }

    colorref =   GetSysColor(COLOR_INFOBK);

    g_hbrInfoBk = CreateSolidBrush(colorref);

    if (hwndCtl = GetDlgItem(hwndDlg, IDC_WELCOME_MON))
    {
        COLORMAP cmWelcome[1];
        RECT rc;
        fFirst = TRUE;

        cmWelcome[0].from = RGB(128,0,0);
        cmWelcome[0].to = colorref;

        // set the position so that dithered colors won't leave a border.
        GetWindowRect(hwndCtl, &rc);
        MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
        if (rc.left & 1) {
            rc.left--;
            rc.right--;
        }

        if (rc.top & 1) {
            rc.top--;
            rc.bottom--;
        }

        SetWindowPos(hwndCtl,NULL,
                     rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top , SWP_NOZORDER | SWP_NOACTIVATE);

        hbm = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_WELCOME_MON));
        if (hbm)
        {
            HBITMAP hbmT;
            HBRUSH hbrT;
            BITMAP bm;

            hdc = CreateCompatibleDC(NULL);
            hbmT = SelectObject(hdc, hbm);
            hbrT = SelectObject(hdc, g_hbrInfoBk);
            GetObject(hbm, sizeof(bm), &bm);

            ExtFloodFill(hdc, 0, bm.bmHeight-1,
                GetPixel(hdc, 0, bm.bmHeight-1), FLOODFILLSURFACE);

            SelectObject(hdc, hbrT);
            SelectObject(hdc, hbmT);
            DeleteDC(hdc);

            SendMessage(hwndCtl, STM_SETIMAGE, 0, (LPARAM)hbm);
        }

    }

    // Make the You know font to be bold...
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        HFONT hfontTemp;

        // make the bold font
        ncm.lfMenuFont.lfWeight = FW_BOLD;
        hfontTemp = CreateFontIndirect(&ncm.lfMenuFont);
        if (hfontTemp)
        {
            SendDlgItemMessage(hwndDlg, IDC_WELCOME_YOUKNOW,
                    WM_SETFONT, (WPARAM)hfontTemp, 0);
        }
    }

    // Note: these tests need to be run in the numerical order of the
    // controls...

#ifndef WINNT
    //
    // NT won't have the tour option
    //

    // See if we need to remove the Tour button from the dialog.
    LoadString(g_hinst, IDS_TOURRELPATH, szRelPath, ARRAYSIZE(szRelPath));
    {
        // Later may want to remove the old Install test...
        CheckOptionalComponentAndRemoveButton(hwndDlg, g_szRegTour,
                g_szInstalled, FALSE, IDC_WELCOME_TOUR);
    }
#endif

    // Likewise online registration...
    CheckOptionalComponentAndRemoveButton(hwndDlg, g_szRegOLRKey,

#ifdef WINNT

            //
            // When NT is ready for Online registration,
            // remove this section and the button will
            // reappear
            //

            g_szRegOLR, FALSE, IDC_WELCOME_REGISTER);
#else
            g_szRegOLR, TRUE, IDC_WELCOME_REGISTER);
#endif
    // And last but not least Alibaba...
    CheckOptionalComponentAndRemoveButton(hwndDlg, g_szRegSetup,
            g_szAlibaba, FALSE, IDC_WELCOME_MSPRODS);


    //
    // Now to get the tip text and options.  Note: the tip text is global
    // for the machine, so it is under HKEY_LOCAL_MACHINE, whereas the
    // options and the next tip to show are per user, so we store these
    // options under HKEY_CURRENT_USER.  First get the Per user options...

    if (!fFirst)
        fShow = ShowNextTip(hwndDlg);

    // Now see if we need to initialize the show next time item or not.
    if (!fFirst && (fShow || g_fForceShowNextTime))
        SendDlgItemMessage(hwndDlg, IDC_WELCOME_SHOWNEXTTIME,
                BM_SETCHECK, TRUE, 0);
}

BOOL CALLBACK RedCarpetDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        _InitRedCarpetDialog(hwnd);
        break;

    case WM_DESTROY:
    	break;

    case WM_CTLCOLORSTATIC:
        {
            int id = GetWindowLong((HWND)lParam, GWL_ID);

            if ((id != IDC_WELCOME_TITLE) && (id != IDC_WELCOME_SHOWNEXTTIME))
            {
                SetWindowLong(hwnd, DWL_MSGRESULT,
                        (DWORD)g_hbrInfoBk);

                // A couple of our controls want to be painted transperent..
                if ((id == IDC_WELCOME_YOUKNOW) || (id == IDC_WELCOME_TIPS))
                {
                    SetBkMode((HDC)wParam, TRANSPARENT);
                }

                SetTextColor((HDC)wParam, GetSysColor(COLOR_INFOTEXT));

                // user wants the return to be the brush also..
                return (BOOL)g_hbrInfoBk;
            }
            else
                return(FALSE);  // did not process it.
        }
        break;


    case WM_COMMAND:
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

void NEAR PASCAL RollOutRedCarpet(LPTSTR pszCmdLine)
{
    // Lets try having a way to expose the CDROM and override capabilities that are
    // used to launch the tour as a command line, such that we may be able to use it
    // in shortcuts to the tour and or bambi...
    if ( ((*pszCmdLine == TEXT('-')) || (*pszCmdLine == TEXT('/'))) &&
            ((*(pszCmdLine+1) == TEXT('x')) || (*(pszCmdLine+1) == TEXT('X'))))
    {
        return;
    }

    // This is real primative, but if it works who cares...
    if ((lstrcmpi(pszCmdLine, TEXT("-t")) == 0) || (lstrcmpi(pszCmdLine, TEXT("/t")) == 0))
        g_fForceShowNextTime = TRUE;

    if ((lstrcmpi(pszCmdLine, TEXT("-f")) == 0) || (lstrcmpi(pszCmdLine, TEXT("/f")) == 0))
        DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_WELCOME_FIRST_BOOT), NULL, RedCarpetDlgProc, TRUE);
    else
        DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_WELCOME), NULL, RedCarpetDlgProc, FALSE);

    // And Cleanup
    if (g_hbrInfoBk)
        DeleteObject(g_hbrInfoBk);

}


//---------------------------------------------------------------------------

int PASCAL WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{

#ifdef DEBUG
	if (GetAsyncKeyState(VK_MENU) < 0)
		DebugBreak();       // Need to debug...
#endif

    g_hinst = hInstance;
    if (InitApplication(hInstance))
    {
        RollOutRedCarpet(lpCmdLine);
    }
    return TRUE;
}

//---------------------------------------------------------------------------

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
    	     != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
    	    pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
    	    pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;	// We never comes here.
}
