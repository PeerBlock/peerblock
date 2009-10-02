@ECHO OFF
SETLOCAL && CLS
TITLE Compiling PeerBlock...

REM Check if Windows DDK 6.1 is present in PATH
IF NOT EXIST %PB_DDK_DIR% (
ECHO:Windows DDK 6.1 path NOT FOUND!!!&&(GOTO :END)
)

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="Win32"
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="Win32"
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="x64"
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="x64"
IF %ERRORLEVEL% NEQ 0 GOTO :ErrorDetected
ECHO.
ECHO.


REM Sign drivers and programs
IF NOT DEFINED PB_CERT (
ECHO:Certificate not found!!&&(GOTO :BuildInstaller)
)

PUSHD "x64\Release (Vista)"
CALL ..\..\tools\sign_driver.cmd pbfilter.sys 
CALL ..\..\tools\sign_driver.cmd peerblock.exe
POPD

PUSHD "x64\Release"
CALL ..\..\tools\sign_driver.cmd pbfilter.sys 
CALL ..\..\tools\sign_driver.cmd peerblock.exe
POPD

PUSHD "Win32\Release (Vista)"
CALL ..\..\tools\sign_driver.cmd pbfilter.sys 
CALL ..\..\tools\sign_driver.cmd peerblock.exe
POPD

PUSHD "Win32\Release"
CALL ..\..\tools\sign_driver.cmd pbfilter.sys 
CALL ..\..\tools\sign_driver.cmd peerblock.exe
POPD


:BuildInstaller
REM Detect if we are running on 64bit WIN and use Wow6432Node, set the path
REM of Inno Setup accordingly and compile installer
IF "%PROGRAMFILES(x86)%zzz"=="zzz" (SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
) ELSE (
SET "U_=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
)

SET "I_=Inno Setup"
SET "A_=%I_% 5"
SET "M_=Inno Setup IS NOT INSTALLED!!!"
FOR /f "delims=" %%a IN (
	'REG QUERY "%U_%\%A_%_is1" /v "%I_%: App Path"2^>Nul^|FIND "REG_"') DO (
	SET "InnoSetupPath=%%a"&Call :Sub %%InnoSetupPath:*Z=%%)

PUSHD setup
ECHO:Compiling installer...
ECHO.
IF DEFINED InnoSetupPath ("%InnoSetupPath%\iscc.exe" /Q "setup.iss"&&(
ECHO:Installer compiled successfully!)) ELSE (ECHO:%M_%)
POPD


REM Sign installer
IF NOT DEFINED PB_CERT (
ECHO:Certificate not found!!&&(GOTO :CreateZips)
)

PUSHD "Distribution"
FOR %%A IN (*.exe) DO CALL ..\tools\sign_driver.cmd %%A 
POPD


REM Create all the zip files ready for distribution
:CreateZips
MD "Distribution" >NUL 2>&1
DEL "tools\buildnum_parsed.txt" >NUL 2>&1
ECHO.
"tools\SubWCRev.exe" "." "tools\buildnum.txt" "tools\buildnum_parsed.txt"

FOR /f "delims=" %%G IN (
	'FINDSTR ".*" "tools\buildnum_parsed.txt"') DO (
	SET "buildnum=%%G"&Call :Sub2 %%buildnum:*Z=%%)

ECHO.
MD "temp_zip" >NUL 2>&1
COPY "Win32\Release\peerblock.exe" "temp_zip\" /Y
COPY "Win32\Release\pbfilter.sys" "temp_zip\" /Y
COPY "license.txt" "temp_zip\" /Y
COPY "setup\readme.rtf" "temp_zip\" /Y
PUSHD "temp_zip"
START "" /B /WAIT "..\tools\7za\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__Win32_Release.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf">NUL&&(
	ECHO:PeerBlock_r%buildnum%__Win32_Release.zip created successfully!)
MOVE /Y "PeerBlock_r%buildnum%__Win32_Release.zip" "..\Distribution" >NUL 2>&1
POPD
RD /S /Q "temp_zip" >NUL 2>&1


ECHO.
MD "temp_zip" >NUL 2>&1
COPY "Win32\Release (Vista)\peerblock.exe" "temp_zip\" /Y
COPY "Win32\Release (Vista)\pbfilter.sys" "temp_zip\" /Y
COPY "license.txt" "temp_zip\" /Y
COPY "setup\readme.rtf" "temp_zip\" /Y
PUSHD "temp_zip"
START "" /B /WAIT "..\tools\7za\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__Win32_Release_(Vista).zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf">NUL&&(
	ECHO:PeerBlock_r%buildnum%__Win32_Release_Vista.zip created successfully!)
MOVE /Y "PeerBlock_r%buildnum%__Win32_Release_(Vista).zip" "..\Distribution" >NUL 2>&1
POPD
RD /S /Q "temp_zip" >NUL 2>&1


ECHO.
MD "temp_zip" >NUL 2>&1
COPY "x64\Release\peerblock.exe" "temp_zip\" /Y
COPY "x64\Release\pbfilter.sys" "temp_zip\" /Y
COPY "license.txt" "temp_zip\" /Y
COPY "setup\readme.rtf" "temp_zip\" /Y
PUSHD "temp_zip"
START "" /B /WAIT "..\tools\7za\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__x64_Release.zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf">NUL&&(
	ECHO:PeerBlock_r%buildnum%__x64_Release.zip created successfully!)
MOVE /Y "PeerBlock_r%buildnum%__x64_Release.zip" "..\Distribution" >NUL 2>&1
POPD
RD /S /Q "temp_zip" >NUL 2>&1


ECHO.
MD "temp_zip" >NUL 2>&1
COPY "x64\Release (Vista)\peerblock.exe" "temp_zip\" /Y
COPY "x64\Release (Vista)\pbfilter.sys" "temp_zip\" /Y
COPY "license.txt" "temp_zip\" /Y
COPY "setup\readme.rtf" "temp_zip\" /Y
PUSHD "temp_zip"
START "" /B /WAIT "..\tools\7za\7za.exe" a -tzip -mx=9^
 "PeerBlock_r%buildnum%__x64_Release_(Vista).zip" "peerblock.exe" "pbfilter.sys"^
 "license.txt" "readme.rtf">NUL&&(
	ECHO:PeerBlock_r%buildnum%__x64_Release_Vista.zip created successfully!)
MOVE /Y "PeerBlock_r%buildnum%__x64_Release_(Vista).zip" "..\Distribution" >NUL 2>&1
POPD
RD /S /Q "temp_zip" >NUL 2>&1


GOTO :AllOK

:ErrorDetected
ECHO: ERROR:  Failed to build PeerBlock!

:AllOK

:END
ECHO.
ECHO.
ENDLOCAL && PAUSE

:Sub
SET InnoSetupPath=%*

:Sub2
SET buildnum=%*