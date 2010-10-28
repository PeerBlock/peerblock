@ECHO OFF
SETLOCAL

REM   PeerBlock copyright (C) 2009-2010 PeerBlock, LLC

REM   This software is provided 'as-is', without any express or implied
REM   warranty.  In no event will the authors be held liable for any damages
REM   arising from the use of this software.

REM   Permission is granted to anyone to use this software for any purpose,
REM   including commercial applications, and to alter it and redistribute it
REM   freely, subject to the following restrictions:

REM   1. The origin of this software must not be misrepresented; you must not
REM      claim that you wrote the original software. If you use this software
REM      in a product, an acknowledgment in the product documentation would be
REM      appreciated but is not required.
REM   2. Altered source versions must be plainly marked as such, and must not be
REM      misrepresented as being the original software.
REM   3. This notice may not be removed or altered from any source distribution.

REM   $Id$

IF /I "%1"=="help" GOTO :showhelp
IF /I "%1"=="-help" GOTO :showhelp
IF /I "%1"=="--help" GOTO :showhelp
GOTO :start

:showhelp
TITLE build.bat %1
ECHO.
ECHO:Usage:  build.bat [Clean^|Build^|Rebuild]
ECHO.
ECHO:Executing "build.bat" will use the defaults: "build.bat Rebuild"
ECHO.
ENDLOCAL
EXIT /B

:start
TITLE Compiling PeerBlock...
REM Check if Windows DDK is present in PATH
IF NOT DEFINED PB_DDK_DIR (
COLOR 0C
ECHO:Windows DDK path NOT FOUND!!!
ECHO:Install the Windows DDK and set an environment variable named "PB_DDK_DIR"
ECHO:pointing to the Windows DDK installation path.
ECHO:Example: H:\progs\WinDDK\6001.18002
GOTO :ErrorDetected
)

IF NOT DEFINED VS90COMNTOOLS (
COLOR 0C
ECHO:Visual Studio 2008 NOT FOUND!!!
GOTO :ErrorDetected
)

IF "%1" == "" (
SET BUILDTYPE=Rebuild
) ELSE (
SET BUILDTYPE=%1
)

REM Compile PeerBlock with MSVC 2008
TITLE Compiling PeerBlock with MSVC 2008...

CALL "%VS90COMNTOOLS%vsvars32.bat" >NUL

FOR %%A IN ("Win32" "x64"
) DO (
CALL :SubMSVC "Release" %%A
CALL :SubMSVC "Release_(Vista)" %%A
)

IF /I "%1" == "Clean" GOTO :END

REM Sign driver and program
IF DEFINED PB_CERT (
TITLE Signing driver and program...
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
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

SET "I_=Inno Setup"
SET "A_=%I_% 5"
FOR /f "delims=" %%a IN (
	'REG QUERY "%U_%\%A_%_is1" /v "%I_%: App Path"2^>Nul^|FIND "REG_"') DO (
	SET "InnoSetupPath=%%a"&Call :SubISPath %%InnoSetupPath:*Z=%%)

IF NOT DEFINED InnoSetupPath (
ECHO. && ECHO.
ECHO:Inno Setup IS NOT INSTALLED!!! The installer won't be compiled.
GOTO :CreateZips
)

PUSHD setup
TITLE Compiling installer...
ECHO.
ECHO:Compiling installer...
ECHO.

"%InnoSetupPath%\iscc.exe" /SStandard="cmd /c "..\..\..\bin\windows\sign_driver.cmd" $f "^
 /Q /O"..\..\..\distribution" "setup.iss"

IF %ERRORLEVEL% NEQ 0 (
GOTO :ErrorDetected
) ELSE (
ECHO:Installer compiled successfully!
)
POPD


:CreateZips
REM Create all the zip files ready for distribution
TITLE Creating ZIP files...

REM Get the revision number
FOR /f "tokens=3,4 delims= " %%K IN (
	'FINDSTR /I /L /C:"define PB_VER_BUILDNUM" "..\..\src\peerblock\version_parsed.h"') DO (
	SET "buildnum=%%K"&Call :SubRevNumber %%buildnum:*Z=%%)
ECHO. && ECHO.

MD "..\..\distribution" >NUL 2>&1

FOR %%L IN (
"Release" "Release_(Vista)"
) DO (
CALL :SubZipFiles Win32 %%L
CALL :SubZipFiles x64 %%L
)

GOTO :END


:END
TITLE Compiling PeerBlock - Finished!
ECHO. && ECHO.
ENDLOCAL && PAUSE
EXIT


:SubMSVC
devenv /nologo PeerBlock.sln /%BUILDTYPE% "%~1|%~2"
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
GOTO :EOF

:SubZipFiles
MD "temp_zip" >NUL 2>&1
COPY "bin\%1\%~2\peerblock.exe" "temp_zip\" /Y /V
COPY "bin\%1\%~2\pbfilter.sys" "temp_zip\" /Y /V
COPY "..\..\license.txt" "temp_zip\" /Y /V
COPY "..\..\doc\readme.rtf" "temp_zip\" /Y /V

PUSHD "temp_zip"
START "" /B /WAIT "..\..\..\bin\windows\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__%1_%~2.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf" >NUL
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected

ECHO:PeerBlock_r%buildnum%__%1_%~2.zip created successfully!
MOVE /Y "PeerBlock_r%buildnum%__%1_%~2.zip" "..\..\..\distribution" >NUL 2>&1
ECHO.
POPD
RD /S /Q "temp_zip" >NUL 2>&1
GOTO :EOF

:SubISPath
SET InnoSetupPath=%*
GOTO :EOF

:SubRevNumber
SET buildnum=%*
GOTO :EOF

:ErrorDetected
ECHO. && ECHO.
ECHO:Compilation FAILED!!!
ECHO. && ECHO.
ENDLOCAL
PAUSE
EXIT /B