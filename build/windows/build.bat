@ECHO OFF
SETLOCAL && CLS
TITLE Compiling PeerBlock...

REM Check if Windows DDK 6.1 is present in PATH
IF NOT DEFINED PB_DDK_DIR (
COLOR 0C
ECHO:Windows DDK 6.1 path NOT FOUND!!!
ECHO:Install the Windows DDK 6.1 and set an environment variable named "PB_DDK_DIR"
ECHO:pointing to the Windows DDK 6.1 installation path.
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
FOR %%A IN ("Win32" "x64"
) DO (
CALL :SubMSVC "Release" %%A
CALL :SubMSVC "Release (Vista)" %%A
)

IF /I "%1" == "Clean" GOTO :END

REM Sign driver and program
IF DEFINED PB_CERT (
TITLE Signing driver and program...
  FOR %%F IN (
  "Win32\Release" "Win32\Release (Vista)" "x64\Release" "x64\Release (Vista)"
  ) DO (
  PUSHD %%F
  CALL ..\..\..\..\bin\windows\sign_driver.cmd pbfilter.sys
  CALL ..\..\..\..\bin\windows\sign_driver.cmd peerblock.exe
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
"Release" "Release (Vista)"
) DO (
CALL :SubZipFiles Win32 %%L
CALL :SubZipFiles x64 %%L
)

GOTO :AllOK


:AllOK
PUSHD "..\..\distribution"
DEL/f/a "PeerBlock_r%buildnum%*_Release_(Vista).zip" >NUL 2>&1
REN "PeerBlock_r%buildnum%*_Release (Vista).zip"^
 "PeerBlock_r%buildnum%*_Release_(Vista).zip" >NUL 2>&1
POPD
GOTO :END


:END
TITLE Compiling PeerBlock - Finished!
ECHO. && ECHO.
ENDLOCAL && PAUSE
EXIT


:SubMSVC
"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:%BUILDTYPE% /p:Configuration=%1 /p:Platform=%2
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
GOTO :EOF

:SubZipFiles
MD "temp_zip" >NUL 2>&1
COPY "%1\%~2\peerblock.exe" "temp_zip\" /Y /V
COPY "%1\%~2\pbfilter.sys" "temp_zip\" /Y /V
COPY "..\..\license.txt" "temp_zip\" /Y /V
COPY "..\..\doc\readme.rtf" "temp_zip\" /Y /V

PUSHD "temp_zip"
START "" /B /WAIT "..\..\..\bin\windows\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__%1_%~2.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf" >NUL
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected

ECHO:PeerBlock_r%buildnum%__%1_^%~2.zip created successfully!
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