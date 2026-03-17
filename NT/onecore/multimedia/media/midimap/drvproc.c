/**********************************************************************

  Copyright (c) 1992-1999 Microsoft Corporation

  drvproc.c

  DESCRIPTION:
    Driver procedure for the Midi Mapper.

  HISTORY:
     06/09/93       [t-kyleb]      created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include <memory.h>

#include "midimap.h"
#include "debug.h"

//=========================== Globals ======================================
//
PCHANNEL                gapChannel[MAX_CHANNELS]    = {NULL};
WORD                    gwConfigWhere       = 0;
UINT                    gcPorts             = 0;
WORD                    gwFlags             = 0;

HINSTANCE               ghinst              = NULL;
PPORT                   gpportList          = NULL;
PINSTANCE               gpinstanceList      = NULL;
PINSTANCE               gpIoctlInstance     = NULL;
PINSTRUMENT             gpinstrumentList    = NULL;
QUEUE                   gqFreeSyncObjs;
HMIDISTRM               ghMidiStrm          = NULL;
TCHAR                   szVersion[]         = TEXT (__DATE__)
                                              TEXT ("@")
                                              TEXT (__TIME__);
DWORD                   gdwVolume           = 0xFFFFFFFFL;

HANDLE			hMutexRefCnt		= NULL;
static const TCHAR	gszRefCnt[]		= TEXT ("MidiMapper_modLongMessage_RefCnt");

HANDLE			hMutexConfig		= NULL;

static const TCHAR      gszReconfigure[]        = TEXT("MidiMapper_Reconfig");

//=========================== Prototypes ===================================
//
PRIVATE LRESULT FNLOCAL GetMapperStatus(
    LPMAPPERSTATUS          lpStat);

/* DllMain - DLL entry point */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (hMutexConfig != NULL) {
            CloseHandle(hMutexConfig);
            hMutexConfig = NULL;
        }
        if (hMutexRefCnt != NULL) {
            CloseHandle(hMutexRefCnt);
            hMutexRefCnt = NULL;
        }
        DeleteCriticalSection((LPCRITICAL_SECTION)&gqFreeSyncObjs);
        gqFreeSyncObjs.pqeFront = NULL;
        gqFreeSyncObjs.pqeRear = NULL;
        gqFreeSyncObjs.cEle = 0;
        ghinst = NULL;
    }
    else if (fdwReason == DLL_PROCESS_ATTACH) {
        ghinst = hinstDLL;
        InitializeCriticalSection((LPCRITICAL_SECTION)&gqFreeSyncObjs);
        gqFreeSyncObjs.pqeFront = NULL;
        gqFreeSyncObjs.pqeRear = NULL;
        gqFreeSyncObjs.cEle = 0;
        hMutexRefCnt = CreateMutexW(NULL, FALSE, (LPCWSTR)gszRefCnt);
        hMutexConfig = CreateMutexW(NULL, FALSE, NULL);
    }
    return TRUE;
}

/***************************************************************************

   @doc INTERNAL

   @api LRESULT | DriverProc | The entry point for an installable driver.

   @parm DWORD | dwID | For most messages, <p dwID> is the DWORD value
       that the driver returns in response to a <m DRV_OPEN> message.
       Each time that the driver is opened, through the <f DrvOpen> API,
       the driver receives a <m DRV_OPEN> message and can return an
       arbitrary, non-zero value. The installable driver interface
       saves this value and returns a unique driver handle to the
       application. Whenever the application sends a message to the
       driver using the driver handle, the interface routes the message
       to this entry point and passes the corresponding <p dwID>.
       This mechanism allows the driver to use the same or different
       identifiers for multiple opens but ensures that driver handles
       are unique at the application interface layer.

       The following messages are not related to a particular open
       instance of the driver. For these messages, the dwID will always
       be zero.

           DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN

   @parm HDRVR  | hdrvr | This is the handle returned to the application
       by the driver interface.

   @parm UINT | umsg | The requested action to be performed. Message
       values below <m DRV_RESERVED> are used for globally defined messages.
       Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
       defined driver protocols. Messages above <m DRV_USER> are used
       for driver specific messages.

   @parm LPARAM | lParam1 | Data for this message.  Defined separately for
       each message

   @parm LPARAM | lParam2 | Data for this message.  Defined separately for
       each message

   @rdesc Defined separately for each message.

****************************************************************************/

LRESULT FNEXPORT DriverProc(
    DWORD_PTR           dwID,
    HDRVR               hdrvr,
    UINT                umsg,
    LPARAM              lParam1,
    LPARAM              lParam2)
{
    //
    //  NOTE DS is not valid here.
    //
    switch (umsg)
    {
        case DRV_LOAD:
            return(1L);

        case DRV_FREE:
            return(0L);

        case DRV_OPEN:
        case DRV_CLOSE:
            return(1L);

        case DRV_ENABLE:
				// Prevent Synchronization problems
				// During Configuration
            if (NULL != hMutexConfig)
				WaitForSingleObject (hMutexConfig, INFINITE);

            SET_ENABLED;
            DPF(1, TEXT ("Enable!"));
            Configure(0);

            if (NULL != hMutexConfig)
				ReleaseMutex (hMutexConfig);
            return(0L);

        case DRV_DISABLE:
            CLR_ENABLED;
            return(0L);

        case DRV_INSTALL:
        case DRV_REMOVE:
            // If the user installs or removes the driver then let them
            // know that they will have to restart.
            //
            return((LRESULT)DRVCNF_RESTART);

        case DRV_ENABLE_DEBUG:
            return(DbgEnable((BOOL)lParam1));

        case DRV_SET_DEBUG_LEVEL:
            return(DbgSetLevel((UINT)lParam1));

        case DRV_GETMAPPERSTATUS:
            return GetMapperStatus((LPMAPPERSTATUS)lParam1);

#ifdef DEBUG
        case DRV_REGISTERDEBUGCB:
            DbgRegisterCallback((HWND)lParam1);
            return 1L;

        case DRV_GETNEXTLOGENTRY:
            return (LRESULT)DbgGetNextLogEntry((LPTSTR)lParam1, (UINT)lParam2);
#endif

            // Let the default handler handle everything else.
            //
        default:
            DPF(1, TEXT ("DriverProc unsupported=%08lX"), (DWORD)umsg);
            return(DefDriverProc(dwID, hdrvr, umsg, lParam1, lParam2));
    }
} //** DriverProc()

/***************************************************************************

   @doc internal

   @api LRESULT | GetMapperStatus | Return information about the current
    mapper setup to a debug application.

   @parm LPMAPPERSTATUS | lpStat | Pointer to a structure to receive
    the mapper information. lpStat->cbStruct must be filled in.

   @rdesc | Always returns 0;

***************************************************************************/

PRIVATE LRESULT FNLOCAL GetMapperStatus(
    LPMAPPERSTATUS          lpStat)
{
    MAPPERSTATUS            stat;


    if (lpStat->cbStruct < sizeof(stat))
        DPF(1, TEXT ("MAPPERSTATUS struct too small -- recompile MAPWATCH!!!"));

    stat.cbStruct               = lpStat->cbStruct;
//    stat.DS                     = __segname("_DATA");
    stat.ghinst                 = ghinst;
    stat.gwFlags                = gwFlags;
    stat.gwConfigWhere          = gwConfigWhere;
    stat.pgapChannel            = gapChannel;
    stat.gpportList             = gpportList;
    stat.gpinstanceList         = gpinstanceList;
    stat.gpinstrumentList       = gpinstrumentList;
    stat.lpszVersion            = szVersion;

    hmemcpy(lpStat, &stat, (UINT)lpStat->cbStruct);

    return 0;
}
