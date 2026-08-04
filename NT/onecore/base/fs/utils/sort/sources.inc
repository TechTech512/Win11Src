!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=sort
TARGETPATH=obj
TARGETTYPE=PROGRAM
EXPECTED_WINVER=10.00

LINKENTRY=mainCRTStartup

SOURCES=..\sort.c ..\sort.rc ..\sortmsg.mc

# Fusionized
SXS_MANIFEST_RESOURCE_ID=1
SXS_APPLICATION_MANIFEST=Sort.Manifest
SXS_ASSEMBLY_NAME=Microsoft.Windows.Filesystem.Sort
SXS_NO_BINPLACE=1
SXS_MANIFEST_IN_RESOURCES=1

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
INCLUDES=$(O)

USE_MSVCRT=1

UMTYPE=console
UMENTRY=main
TARGETLIBS=\
        $(SDK_LIB_PATH)\msvcrt_enforcement.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\ucrt.lib \
        $(SDK_LIB_PATH)\ntdll.lib \
        $(SDK_LIB_PATH)\kernl32p.lib     

