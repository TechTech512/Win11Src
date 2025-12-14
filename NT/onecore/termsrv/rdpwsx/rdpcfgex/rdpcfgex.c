/*******************************************************************************
*
* rdpcfgex.c
*
* WinCfg extension DLL
*
* Copyright (c) 1997, Microsoft Corporation
* All rights reserved.
*
*******************************************************************************/

#include <windows.h>
#include <ntverp.h>
#include "rdpcfgex.h"

HINSTANCE g_hInst;

struct _EncLevel {
    int RegistryValue;
    int StringID;
    int DescrptID;
};

struct _SecLayer {
    int RegistryValue;
    int StringID;
    int DescrptID;
};

struct _EncLevelEx {
    struct _EncLevel eL;
    int DescrptID;
};

//
// This global variable is returned to TSCFG and is used to populate the
// Encryption Level field.
//
struct _EncLevelEx EncryptionLevelsEx[4] = {
    {{1, IDS_LOW, IDS_LOW_DESCR}, IDS_LOW_DESCR},
    {{2, IDS_COMPATIBLE, IDS_COMPATIBLE_DESCR}, IDS_COMPATIBLE_DESCR},
    {{3, IDS_HIGH, IDS_HI_DESCR}, IDS_HI_DESCR},
    {{4, IDS_FIPS, IDS_FIPS_DESCR}, IDS_FIPS_DESCR}
};

struct _EncLevel EncryptionLevels[4] = {
    {1, IDS_LOW, IDS_LOW_DESCR},
    {2, IDS_COMPATIBLE, IDS_COMPATIBLE_DESCR},
    {3, IDS_HIGH, IDS_HI_DESCR},
    {4, IDS_FIPS, IDS_FIPS_DESCR}
};

// Security Layers (RDP, Negotiate, SSL)
struct _SecLayer SecurityLayers[3] = {
    {0, IDS_RDPSECLAYER, IDS_RDPSECDESCR},
    {1, IDS_NEGOTIATE, IDS_NEGOTDESC},
    {2, IDS_SSL, IDS_SSLDESC}
};

/////////////////////////////////////////////////////////////////////////////
// DllMain
//
// Main entry point of the DLL
//
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        g_hInst = hinstDLL;
    }
    return TRUE;
}

//-------------------------------------------------------------------------
// VER_PRODUCTVERSION_DW defined in ntverp.h
//-------------------------------------------------------------------------
DWORD __cdecl ExGetCfgVersionInfo(void)
{
    return VER_PRODUCTVERSION_DW;
}

/////////////////////////////////////////////////////////////////////////////
// ExtEncryptionLevels
//
// Provide array of encryption levels for this protocol
// Returns the number of encryption levels in the array
//
LONG __cdecl ExtEncryptionLevels(wchar_t (*levelNames)[33], struct _EncLevel **levelList)
{
    *levelList = EncryptionLevels;
    return 4;
}

/////////////////////////////////////////////////////////////////////////////
// ExtGetCapabilities
//
// This routine returns a ULONG which contains a mask of the different
// Client settings that the RDP protocol supports.
//
DWORD __cdecl ExtGetCapabilities(void)
{
    return 0x7F;
}

LONG __cdecl ExtGetEncryptionLevelAndDescrEx(
    int levelIndex,
    wchar_t *levelName,
    DWORD nameBufferSize,
    wchar_t *description,
    DWORD descBufferSize,
    DWORD *registryValue,
    DWORD *requiredBufferSize
)
{
    int loadResult;
    
    if (levelIndex < 0 || levelIndex > 3) {
        return -1;
    }
    
    if (levelName == NULL) {
        if (description == NULL) {
            if (requiredBufferSize != NULL) {
                *requiredBufferSize = 4;
                return 0;
            }
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        loadResult = LoadStringW(g_hInst, EncryptionLevelsEx[levelIndex].eL.StringID, levelName, nameBufferSize);
        if (loadResult == 0) {
            memset(levelName, 0, nameBufferSize * sizeof(wchar_t));
        }
        if (description == NULL) goto SetRegistryValue;
    }
    
    loadResult = LoadStringW(g_hInst, EncryptionLevelsEx[levelIndex].DescrptID, description, descBufferSize);
    if (loadResult == 0) {
        memset(description, 0, descBufferSize * sizeof(wchar_t));
    }
    
SetRegistryValue:
    if (registryValue != NULL) {
        *registryValue = EncryptionLevelsEx[levelIndex].eL.RegistryValue;
    }
    return 0;
}

//-------------------------------------------------------------------------
// We need to be compatible with citrix, modifying EncryptionLevel struct
// would cause some undesirable results on a metaframe server.  Currently
// the MS ext will support description for the encryption levels.
// When TSCC obtains the extension config dll it will getproc this method
// failure indicates that we have a non-MS cfgdll
//
LONG __cdecl ExtGetEncryptionLevelDescr(int levelIndex, int *stringID)
{
    switch (levelIndex) {
        case 1:
            *stringID = IDS_LOW_DESCR;
            break;
        case 2:
            *stringID = IDS_COMPATIBLE_DESCR;
            break;
        case 3:
            *stringID = IDS_HI_DESCR;
            break;
        case 4:
            *stringID = IDS_FIPS_DESCR;
            break;
        default:
            *stringID = 0;
            break;
    }
    return (*stringID != 0) - 1;
}

LONG __cdecl ExtGetSecurityLayerDescrString(int layerIndex, wchar_t *description, int bufferSize)
{
    int loadResult = 0;
    int stringID = 0;
    
    if (description == NULL) {
        return -1;
    }
    if (bufferSize > 0x100) {
        return -1;
    }
    
    memset(description, 0, bufferSize * sizeof(wchar_t));
    
    switch (layerIndex) {
        case 0:
            stringID = IDS_RDPSECDESCR;
            break;
        case 1:
            stringID = IDS_NEGOTDESC;
            break;
        case 2:
            stringID = IDS_SSLDESC;
            break;
        default:
            return -1;
    }
    
    loadResult = LoadStringW(g_hInst, stringID, description, bufferSize);
    return (loadResult != 0) - 1;
}

LONG __cdecl ExtGetSecurityLayerName(int layerIndex, wchar_t *layerName, int bufferSize)
{
    int loadResult = 0;
    int stringID = 0;
    
    if (layerName == NULL) {
        return -1;
    }
    if (bufferSize > 0x100) {
        return -1;
    }
    
    memset(layerName, 0, bufferSize * sizeof(wchar_t));
    
    switch (layerIndex) {
        case 0:
            stringID = IDS_RDPSECLAYER;
            break;
        case 1:
            stringID = IDS_NEGOTIATE;
            break;
        case 2:
            stringID = IDS_SSL;
            break;
        default:
            return -1;
    }
    
    loadResult = LoadStringW(g_hInst, stringID, layerName, bufferSize);
    return (loadResult != 0) - 1;
}

LONG __cdecl ExtSecurityLayers(wchar_t (*layerNames)[33], struct _SecLayer **layerList)
{
    if (layerList == NULL) {
        return -1;
    }
    *layerList = SecurityLayers;
    return 3;
}

/////////////////////////////////////////////////////////////////////////////
// ExtStart
//
// WinCfg calls this function immediately after loading the DLL
// Put any global initialization stuff here
//
void __cdecl ExtStart(wchar_t (*param_1)[33])
{
    return;
}

/////////////////////////////////////////////////////////////////////////////
// ExtEnd
//
// WinCfg calls this function when exiting
// Put any global cleanup stuff here
//
void __cdecl ExtEnd(wchar_t (*param_1)[33])
{
    return;
}

