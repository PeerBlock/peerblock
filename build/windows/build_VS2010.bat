@ECHO OFF
SETLOCAL ENABLEEXTENSIONS
CD /D %~dp0

REM  PeerBlock copyright (C) 2009-2011 PeerBlock, LLC

REM  This software is provided 'as-is', without any express or implied
REM  warranty.  In no event will the authors be held liable for any damages
REM  arising from the use of this software.

REM  Permission is granted to anyone to use this software for any purpose,
REM  including commercial applications, and to alter it and redistribute it
REM  freely, subject to the following restrictions:

REM  1. The origin of this software must not be misrepresented; you must not
REM     claim that you wrote the original software. If you use this software
REM     in a product, an acknowledgment in the product documentation would be
REM     appreciated but is not required.
REM  2. Altered source versions must be plainly marked as such, and must not be
REM     misrepresented as being the original software.
REM  3. This notice may not be removed or altered from any source distribution.

REM  $Id$

REM You can set here the Inno Setup path if for example you have Inno Setup Unicode
REM installed and you want to use the ANSI Inno Setup which is in another location
REM SET "InnoSetupPath="
REM You can set here the path to the Windows DDK if you don't feel like adding a new
REM environment variable or if you are lazy like I am ;)
REM SET "PB_DDK_DIR=C:\WinDDK\7600.16385.1"

REM If you define BUILD_ONLY_PB=True then only the main program will be compiled
REM without the installer and the ZIP packages
REM SET BUILD_ONLY_PB=True

REM check for the help switches
IF /I "%1"=="help"   GOTO SHOWHELP
IF /I "%1"=="/help"  GOTO SHOWHELP
IF /I "%1"=="-help"  GOTO SHOWHELP
IF /I "%1"=="--help" GOTO SHOWHELP
IF /I "%1"=="/?"     GOTO SHOWHELP


REM Check if Windows DDK is present in PATH
IF NOT DEFINED PB_DDK_DIR (
  TITLE Compiling PeerBlock [ERROR]
  COLOR 0C
  ECHO Windows DDK path NOT FOUND!
  ECHO Install the Windows DDK and set an environment variable named "PB_DDK_DIR"
  ECHO pointing to the Windows DDK installation path.
  ECHO Example: C:\WinDDK\6001.18002
  GOTO ErrorDetected
)

IF NOT DEFINED VS100COMNTOOLS (
  TITLE Compiling PeerBlock [ERROR]
  COLOR 0C
  ECHO Visual Studio 2010 NOT FOUND!
  GOTO ErrorDetected
)

REM Check for the switches
IF "%1" == "" (
  SET "BUILDTYPE=Build"
) ELSE (
  IF /I "%1" == "Build"     SET "BUILDTYPE=Build"   & GOTO START
  IF /I "%1" == "/Build"    SET "BUILDTYPE=Build"   & GOTO START
  IF /I "%1" == "-Build"    SET "BUILDTYPE=Build"   & GOTO START
  IF /I "%1" == "--Build"   SET "BUILDTYPE=Build"   & GOTO START
  IF /I "%1" == "Clean"     SET "BUILDTYPE=Clean"   & GOTO START
  IF /I "%1" == "/Clean"    SET "BUILDTYPE=Clean"   & GOTO START
  IF /I "%1" == "-Clean"    SET "BUILDTYPE=Clean"   & GOTO START
  IF /I "%1" == "--Clean"   SET "BUILDTYPE=Clean"   & GOTO START
  IF /I "%1" == "Rebuild"   SET "BUILDTYPE=Rebuild" & GOTO START
  IF /I "%1" == "/Rebuild"  SET "BUILDTYPE=Rebuild" & GOTO START
  IF /I "%1" == "-Rebuild"  SET "BUILDTYPE=Rebuild" & GOTO START
  IF /I "%1" == "--Rebuild" SET "BUILDTYPE=Rebuild" & GOTO START

  ECHO.
  ECHO Unsupported commandline switch!
  ECHO Run "%~nx0 help" for details about the commandline switches.
  GOTO EndWithError
)


:START
SET START_TIME=%DATE%-%TIME%

REM Compile PeerBlock with MSVC 2010
CALL "%VS100COMNTOOLS%vsvars32.bat" >NUL

FOR %%A IN ("Win32" "x64"
) DO (
CALL :SubMSVC "Release" %%A
CALL :SubMSVC "Release_(Vista)" %%A
)

IF /I "%BUILDTYPE%" == "Clean" GOTO END

IF /I "%BUILD_ONLY_PB%" == "True" GOTO END

REM Sign driver and program
IF DEFINED PB_CERT (
  TITLE Signing the driver and the program...
  FOR %%F IN (
  "bin10\Win32\Release" "bin10\Win32\Release_(Vista)" "bin10\x64\Release" "bin10\x64\Release_(Vista)"
  ) DO (
  PUSHD %%F
  CALL ..\..\..\..\..\bin\windows\sign_driver.cmd pbfilter.sys
  CALL ..\..\..\..\..\bin\windows\sign_driver.cmd peerblock.exe
  POPD
  )
)


:BuildInstaller
REM Detect if we are running on 64bit WIN and use Wow6432Node, set the path
REM of Inno Setup accordingly and compile the installer
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (
  SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
  SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

IF NOT DEFINED InnoSetupPath (
  FOR /F "delims=" %%a IN (
    'REG QUERY "%U_%\Inno Setup 5_is1" /v "Inno Setup: App Path"2^>Nul^|FIND "REG_"') DO (
    SET "InnoSetupPath=%%a" & CALL :SubInnoSetupPath %%InnoSetupPath:*Z=%%)
)

IF NOT EXIST "%InnoSetupPath%" (
  ECHO. & ECHO.
  ECHO Inno Setup wasn't found! The installer won't be compiled.
  GOTO CreateZips
)

PUSHD "setup"
TITLE Compiling installer...
ECHO.
ECHO Compiling installer...
ECHO.

"%InnoSetupPath%\iscc.exe" /SStandard="cmd /c "..\..\..\bin\windows\sign_driver.cmd" $f "^
 /Q /O"..\..\..\distribution" "setup.iss" /DVS2010build

IF %ERRORLEVEL% NEQ 0 (
  GOTO ErrorDetected
) ELSE (
  ECHO Installer compiled successfully!
)
POPD


:CreateZips
REM Create all the zip files ready for distribution

REM Get the revision number
FOR /f "tokens=3,4 delims= " %%K IN (
  'FINDSTR /I /L /C:"define PB_VER_BUILDNUM" "..\..\src\peerblock\version_parsed.h"') DO (
  SET "buildnum=%%K" & Call :SubRevNumber %%buildnum:*Z=%%)
ECHO. & ECHO.

IF NOT EXIST "..\..\distribution" MD "..\..\distribution"

FOR %%L IN (
"Release" "Release_(Vista)"
) DO (
CALL :SubZipFiles Win32 %%L
CALL :SubZipFiles x64 %%L
)

GOTO END


:END
TITLE Compiling PeerBlock - Finished!
ECHO. & ECHO.
ECHO PeerBlock's compilation started on %START_TIME%
ECHO and completed on %DATE%-%TIME%
ECHO.
ENDLOCAL
PAUSE
EXIT /B


:SubMSVC
TITLE Compiling PeerBlock with MSVC 2010 - %~1^|%~2...
"%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" PeerBlock_VS2010.sln^
 /t:%BUILDTYPE% /p:Configuration=%1 /p:Platform=%2 /maxcpucount^
 /consoleloggerparameters:DisableMPLogging;Summary;Verbosity=minimal
IF %ERRORLEVEL% NEQ 0 GOTO ErrorDetected
EXIT /B


:SubZipFiles
TITLE Creating ZIP files - %~2 %1...
IF NOT EXIST "temp_zip" MD "temp_zip"
COPY /Y /V "bin10\%1\%~2\peerblock.exe" "temp_zip\"
COPY /Y /V "bin10\%1\%~2\pbfilter.sys"  "temp_zip\"
COPY /Y /V "..\..\license.txt"          "temp_zip\"
COPY /Y /V "..\..\doc\readme.rtf"       "temp_zip\"

PUSHD "temp_zip"
START "" /B /WAIT "..\..\..\bin\windows\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__%1_%~2_VS2010.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf" >NUL
IF %ERRORLEVEL% NEQ 0 GOTO ErrorDetected

ECHO PeerBlock_r%buildnum%__%1_%~2_VS2010.zip created successfully!
MOVE /Y "PeerBlock_r%buildnum%__%1_%~2_VS2010.zip" "..\..\..\distribution" >NUL
ECHO.
POPD
IF EXIST "temp_zip" RD /S /Q "temp_zip"
EXIT /B


:SubInnoSetupPath
SET InnoSetupPath=%*
EXIT /B


:SubRevNumber
SET buildnum=%*
EXIT /B


:SHOWHELP
TITLE "%~nx0 %1"
ECHO.
ECHO Usage:  %~nx0 [Clean^|Build^|Rebuild]
ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0 Build"
ECHO.
ENDLOCAL
EXIT /B


:ErrorDetected
ECHO. & ECHO.
ECHO Compilation FAILED!
ECHO. & ECHO.
ENDLOCAL
PAUSE
EXIT
