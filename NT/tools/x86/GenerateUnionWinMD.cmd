@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this source code is subject to the terms of the Microsoft
@REM premium shared source license agreement under which you licensed
@REM this source code. If you did not accept the terms of the license
@REM agreement, you are not authorized to use this source code.
@REM For the terms of the license, please see the license agreement
@REM signed by you and Microsoft.
@REM THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
@REM
@REM ============================================================
@REM  Script to generate merged winmd file during installation
@REM  or uninstallation of ExtensionSDKs
@REM ============================================================

@echo off
SETLOCAL

REM Copyright display
ECHO Microsoft (R) Generate UnionWinMD Tool version 10.0.2
ECHO Copyright (c) Microsoft Corporation
ECHO All rights reserved.

REM Show usage text
set SHOW_HELP=
if /i "%~1" == "/?" set SHOW_HELP=1
if /i "%~1" == "-?" set SHOW_HELP=1
if /i "%~1" == "/help" set SHOW_HELP=1
if /i "%~1" == "-help" set SHOW_HELP=1
if /i "%SHOW_HELP%" == "1" (
    ECHO.
    ECHO GenerateUnionWinMD.cmd unifies all existing contract winmd files
    ECHO under the "<SDKRoot>\<Version>\References\" into a single union winmd. 
    ECHO This union winmd will be named Windows.winmd and generated under
    ECHO "<SDKRoot>\<Version>\UnionMetadata\Windows.winmd". This command line
    ECHO utility does not take any arguments as input
    ECHO.
    ECHO Must be run from an elevated command prompt.
    EXIT /B 0
)

REM Check for elevation
fltmc >nul 2>&1
if ERRORLEVEL 1 (
    fsutil dirty query %systemdrive% >nul 2>&1
    if ERRORLEVEL 1 (
        ECHO Error: You must run this script from an elevated command prompt.
        EXIT /B 5
    ) else (
        ECHO Confirmed running as administrator.
    )
) else (
    ECHO Confirmed running as administrator.
)

ECHO.
ECHO Generating a Union WinMD file of all winmd files in the SDK. 

REM Get SDK install folder from the registry
set SDKInstallFolder = ""
for /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKLM\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0" /v InstallationFolder') DO SET SDKInstallFolder=%%B

if NOT EXIST "%SDKInstallFolder%" (
    for /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0" /v InstallationFolder') DO SET SDKInstallFolder=%%B
)

REM Exit if you can't find the SDK install folder
if NOT EXIST "%SDKInstallFolder%" (
    ECHO Error: Can't find the SDK install folder: "%SDKInstallFolder%". Please install the Windows SDK before running this tool.
    EXIT /B 3
)

echo An SDK was found at the following location: %SDKInstallFolder%

set SDKVersion=10.0.26100.0
set MDFolder=%temp%\WindowsSDK\UnionWinmdWorkingFolder
set RandomName=%RANDOM%
set MDFileName=%RandomName%
set MDLogFileName=%MDFolder%\Logs\%MDFileName%-MDMerge
set MDMergeLogFileName=%MDLogFileName%.log
set MDMergeErrFileName=%MDLogFileName%.err

set MDFullPath=%MDFolder%\%RandomName%
set MDFullVersionedPath=%MDFullPath%\%SDKVersion%
set ReferencesFolder=%SDKInstallFolder%References
set ReferencesVersionedFolder=%SDKInstallFolder%References\%SDKVersion%
set UMDFolder=%SDKInstallFolder%UnionMetadata
set UMDVersionedFolder=%SDKInstallFolder%UnionMetadata\%SDKVersion%
set ToolsFolder=%SDKInstallFolder%bin\x86
set ToolsVersionedFolder=%SDKInstallFolder%bin\%SDKVersion%\x86

ECHO Deleting all temp folders
if EXIST "%MDFullPath%" (
    if EXIST "%MDFullVersionedPath%" (
        rmdir /S /Q "%MDFullVersionedPath%"
    )
    rmdir /S /Q "%MDFullPath%"
)

echo Deleted all the temp folders

echo Re-creating temp folders

md "%MDFullPath%\WinMDs"
ECHO Created WinMDs folder in filepath :%MDFullPath%

md "%MDFullVersionedPath%\WinMDs"
ECHO Created WinMDs versioned folder in filepath :%MDFullVersionedPath%

if NOT EXIST "%MDFolder%\Logs" (
    md "%MDFolder%\Logs"
    ECHO Created Logs folder in path "%MDFolder%\Logs"
)

echo creating UnionMetadata folder if does not already exist
if NOT EXIST "%UMDFolder%" (
    md "%UMDFolder%"    
    ECHO Created folder "%UMDFolder%"
)
if NOT EXIST "%UMDVersionedFolder%"  (
    md "%UMDVersionedFolder%" 
    ECHO Created folder "%UMDVersionedFolder%"
)

REM Locate latest version of a contract winmd
REM Contract winmds are versioned using x.x.x.x folder names, where x is a decimal number
if NOT EXIST "%ReferencesFolder%" (
    ECHO Error: Can't find the SDK Contract References folder: "%ReferencesFolder%". Please install/repair the Windows SDK before running this tool.
    EXIT /B 2
)

ECHO Creating UnionWinMD using mdmerge tool

REM Generate UnionWinMD for Nonversioned references
call :GenerateWinMD "%ReferencesFolder%" "%MDFullPath%" "%ToolsFolder%" "%UMDFolder%" "%MDLogFileName%"

REM Generate UnionWinMD  for versioned references
if EXIST "%ReferencesVersionedFolder%" (
    call :GenerateWinMD "%ReferencesVersionedFolder%" "%MDFullVersionedPath%" "%ToolsVersionedFolder%" "%UMDVersionedFolder%" "%MDLogFileName%"
)

IF NOT EXIST "%UMDVersionedFolder%\Windows.winmd" (
   IF NOT EXIST "%UMDFolder%\Windows.winmd" (
        ECHO Error: Unable to find "%UMDVersionedFolder%\Windows.winmd" and "%UMDFolder%\Windows.winmd". See MDMerge tool logs at %MDMergeLogFileName% and %MDMergeErrFileName%
        EXIT /B 2
   )   
   ECHO Error: Unable to find "%UMDFolder%\Windows.winmd". See MDMerge tool logs at %MDMergeLogFileName% and %MDMergeErrFileName%
)

ECHO Clean up temp files
if EXIST "%MDFullVersionedPath%\WinMDs" (
    rmdir /S /Q "%MDFullVersionedPath%\WinMDs"
)
if EXIST "%MDFullVersionedPath%" (
    rmdir /S /Q "%MDFullVersionedPath%"
)
if EXIST "%MDFullPath%\WinMDs" (
    rmdir /S /Q "%MDFullPath%\WinMDs"
)
if EXIST "%MDFullPath%" (
    rmdir /S /Q "%MDFullPath%"
)

EXIT /B 0

ENDLOCAL

:GenerateWinMD
set lclrefFolder=%~1
set lclWinMDFolder=%~2
set lcltoolFolder=%~3
set lclUMDFolder=%~4
set lclFileName=%~5
set lclLogFileName=%lclFileName%.log
set lclErrFilename=%lclFileName%.err

REM Generating UnionWinMD for References folder
ECHO Locating WinMDs in "%lclrefFolder%" folder
setlocal enableextensions enabledelayedexpansion
for /f "tokens=*" %%G in ('dir /b /ON /AD "%lclrefFolder%"') do (
    set /A LoopCount = 0
    REM skip version folders
    echo "%%G" | findstr /si "Windows">nul
    IF ERRORLEVEL 1 (
       echo "skipping %lclrefFolder%\%%G"
       set /A LoopCount += 1
    )
    
    REM List all folders in reverse sorted order (so that latest version is listed first)
    for /f "tokens=*" %%J in ('dir /b /s /AD /O-N "%lclrefFolder%\%%G"') do (
        REM Use only the first item from the list (i.e. the latest version) and it winmd does not exist, go to the next
        if !LoopCount! == 0 (
           If EXIST "%%J\*.winmd" (
                REM Copy WinMD files to a temp folder as mdmerge takes a folder as input
                copy "%%J\*.winmd" "%lclWinMDFolder%\WinMDs\" >> "%lclLogFileName%" 2>>"%lclErrFilename%"
                set /A LoopCount += 1
            )
        )
    )
    if !LoopCount! == 0 (
       ECHO "No winmds found for contract %%G" >> "%lclLogFileName%" 2>>"%lclErrFilename%"
    )
)
endlocal

REM Delete Windows.winmd as we don't want to include that in our union winmd
if EXIST "%lclWinMDFolder%\WinMDs\Windows.winmd" (
    del "%lclWinMDFolder%\WinMDs\Windows.winmd"
)
ECHO Removed Windows.winmd as we don't want to include that in our union winmd

REM Run MDMerge tool passing in the temp folder containing winmd files
REM Assumes run out of X86 folder
ECHO Running "%lcltoolFolder%\MDMerge.exe"
if NOT EXIST "%lcltoolFolder%\MDMerge.exe" echo Skipping call to MDMerge because "%lcltoolFolder%\MDMerge.exe"  does not exist.  >>"%lclErrFilename%"
if EXIST "%lcltoolFolder%\MDMerge.exe" "%lcltoolFolder%\MDMerge.exe" -n:1 -v -i "%lclWinMDFolder%\WinMDs" -o "%lclUMDFolder%" >> "%lclLogFileName%" 2>>"%lclErrFilename%"


if ERRORLEVEL 1 (
    ECHO MDMerge failed to generate UnionWinMD for References folder. Please check See MDMerge tool logs at %lclLogFileName% and %lclErrFilename% 
)
goto :EOF