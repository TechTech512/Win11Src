#include "stdafx.h"

LONG CALLBACK CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    switch(uMsg)
    {
        case 1:  // CPL_INIT
        case 4:  // CPL_GETCOUNT
        case 5:  // CPL_INQUIRE
        case 6:  // CPL_DBLCLK
        case 7:  // CPL_STOP
        case 10: // CPL_EXIT
            return 1;
        default:
            return 0;
    }
}

