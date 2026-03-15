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
@REM  Script to run Feature on Demand installation
@REM ============================================================

@echo off
SETLOCAL

REM Copyright display
ECHO Microsoft (R) Feature on Demand Installation Tool version 10.0.0
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
    ECHO DismFoDInstall.cmd installs Graphics Feature on Demand 
    ECHO and exits after a timeout of 10 mins. This command line
    ECHO utility does not take any arguments as input
    ECHO.
    ECHO Must be run from an elevated command prompt.
    EXIT /B 0
)

ECHO.
ECHO Running Dism command to install Graphics Feature on Demand. 
REM Run Dism command to install Graphics Feature on Demand.
SET SystemFolder=%windir%\sysnative

REM Get PROCESSOR_ARCHITECTURE from the registry
set PROCESSOR_ARCHITECTURE = ""
for /F "tokens=2* delims=	 " %%A IN ('REG QUERY "HKLM\System\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE') DO SET PROCESSOR_ARCHITECTURE=%%B
ECHO "PROCESSOR_ARCHITECTURE: %PROCESSOR_ARCHITECTURE%"

if "%PROCESSOR_ARCHITECTURE%" == "x86" (
    SET SystemFolder=%windir%\system32
 )

ECHO "SYSTEMFOLDER:%SystemFolder%"

SET App=Dism.exe

REM MAXWAIT HERE IS MORE LIKE MINIMUM WAIT IN WINDOWS.
SET MAXWAIT=300
SET WAITCOUNT=0

ECHO "Running DismFoD command: start /b "%App%" "%SystemFolder%\%App%" /online /NoRestart /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0"
start /b "%App%" "%SystemFolder%\%App%" /online /NoRestart /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0

ECHO "Waiting for a maximum of  %MAXWAIT% seconds or for the dism command to complete"
:WAIT
IF %WAITCOUNT% GEQ %MAXWAIT% GOTO KILL_IT

REM Timeout command gives Input redirection error. Hence switching to ping instead
ping 127.0.0.1 -n 5 >nul
SET /A WAITCOUNT+=5
FOR /F "delims=" %%a IN ('TASKLIST ^| FIND /C "%App%"') DO IF %%a EQU 0 GOTO RUN_DONE
GOTO WAIT

:KILL_IT
ECHO Dism install command is being aborted as it is taking longer than 5 mins to install.
TASKKILL /IM %App% /F > NUL
:RUN_DONE

if ERRORLEVEL 1 (
    ECHO Dism install failed. Please check DISM logs in folder "%SystemFolder%\logs\dism\"
    EXIT /B %ERRORLEVEL%
) else (
    ECHO Dism install of Graphics Feature on Demand succeeded
)
EXIT /B 0

ENDLOCAL