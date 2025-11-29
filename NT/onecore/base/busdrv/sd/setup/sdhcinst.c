
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ci.c

Abstract:

    Battery Class Installer

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>

DWORD __cdecl SdClassCoInstaller(
    DWORD InstallFunction,
    PVOID Reserved,
    PSP_DEVINFO_DATA DeviceInfoData,
    PCOINSTALLER_CONTEXT_DATA ContextData
)
{
    return NO_ERROR;
}

DWORD __cdecl SdClassInstall(
    DWORD InstallFunction,
    PVOID Reserved,
    PSP_DEVINFO_DATA DeviceInfoData
)
{
    return ERROR_DI_DO_DEFAULT;
}

