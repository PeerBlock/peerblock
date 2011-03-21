@ECHO OFF
SETLOCAL
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

REM check for the help switches
IF /I "%1"=="help"   GOTO SHOWHELP
IF /I "%1"=="/help"  GOTO SHOWHELP
IF /I "%1"=="-help"  GOTO SHOWHELP
IF /I "%1"=="--help" GOTO SHOWHELP
IF /I "%1"=="/?"     GOTO SHOWHELP
GOTO CHECK


:SHOWHELP
TITLE "%~nx0 %1"
ECHO.
ECHO Usage:  %~nx0 [Clean^|Build^|Rebuild]
ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0 Rebuild"
ECHO.
ENDLOCAL
EXIT /B


:CHECK
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

IF NOT DEFINED VS90COMNTOOLS (
  TITLE Compiling PeerBlock [ERROR]
  COLOR 0C
  ECHO Visual Studio 2008 NOT FOUND!
  GOTO ErrorDetected
)

REM Check for the switches
IF "%1" == "" (
  SET "BUILDTYPE=Rebuild"
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
REM Compile PeerBlock with MSVC 2008
CALL "%VS90COMNTOOLS%vsvars32.bat" >NUL

FOR %%A IN ("Win32" "x64"
) DO (
CALL :SubMSVC "Release" %%A
CALL :SubMSVC "Release_(Vista)" %%A
)

IF /I "%BUILDTYPE%" == "Clean" GOTO END

REM Sign driver and program
IF DEFINED PB_CERT (
  TITLE Signing the driver and the program...
  FOR %%F IN (
  "bin\Win32\Release" "bin\Win32\Release_(Vista)" "bin\x64\Release" "bin\x64\Release_(Vista)"
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

FOR /F "delims=" %%a IN (
  'REG QUERY "%U_%\Inno Setup 5_is1" /v "Inno Setup: App Path"2^>Nul^|FIND "REG_"') DO (
  SET "InnoSetupPath=%%a" & CALL :SubInnoSetupPath %%InnoSetupPath:*Z=%%)

IF NOT DEFINED InnoSetupPath (
  ECHO. & ECHO.
  ECHO Inno Setup IS NOT INSTALLED! The installer won't be compiled.
  GOTO CreateZips
)

PUSHD setup
TITLE Compiling installer...
ECHO.
ECHO Compiling installer...
ECHO.

"%InnoSetupPath%\iscc.exe" /SStandard="cmd /c "..\..\..\bin\windows\sign_driver.cmd" $f "^
 /Q /O"..\..\..\distribution" "setup.iss"

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

MD "..\..\distribution" >NUL 2>&1

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
ENDLOCAL
PAUSE
EXIT /B


:SubMSVC
TITLE Compiling PeerBlock with MSVC 2008 - %~1^|%~2...
devenv /nologo PeerBlock.sln /%BUILDTYPE% "%~1|%~2"
IF %ERRORLEVEL% NEQ 0 GOTO ErrorDetected
EXIT /B


:SubZipFiles
TITLE Creating ZIP files - %~2 %1...
MD "temp_zip" >NUL 2>&1
COPY "bin\%1\%~2\peerblock.exe" "temp_zip\" /Y /V
COPY "bin\%1\%~2\pbfilter.sys" "temp_zip\" /Y /V
COPY "..\..\license.txt" "temp_zip\" /Y /V
COPY "..\..\doc\readme.rtf" "temp_zip\" /Y /V

PUSHD "temp_zip"
START "" /B /WAIT "..\..\..\bin\windows\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__%1_%~2.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf" >NUL
IF %ERRORLEVEL% NEQ 0 GOTO ErrorDetected

ECHO PeerBlock_r%buildnum%__%1_%~2.zip created successfully!
MOVE /Y "PeerBlock_r%buildnum%__%1_%~2.zip" "..\..\..\distribution" >NUL 2>&1
ECHO.
POPD
RD /S /Q "temp_zip" >NUL 2>&1
EXIT /B


:SubInnoSetupPath
SET InnoSetupPath=%*
EXIT /B


:SubRevNumber
SET buildnum=%*
EXIT /B


:ErrorDetected
ECHO. & ECHO.
ECHO Compilation FAILED!
ECHO. & ECHO.
ENDLOCAL
PAUSE
EXIT
