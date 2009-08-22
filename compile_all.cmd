@ECHO OFF
SETLOCAL && CLS
TITLE Compiling PeerBlock...

REM Check if Windows DDK 6.1 is present in PATH
IF NOT EXIST %PB_DDK_DIR% (
ECHO:Windows DDK 6.1 path NOT FOUND!!!&&(GOTO END)
)

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="x64"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerGuardian2.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="x64"
ECHO.
ECHO.


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

CD setup
IF DEFINED InnoSetupPath ("%InnoSetupPath%\iscc.exe" /Q "setup.iss"&&(
ECHO:Installer compiled successfully!)) ELSE (ECHO:%M_%)


:END
ECHO.
ENDLOCAL && PAUSE

:Sub
SET InnoSetupPath=%*
