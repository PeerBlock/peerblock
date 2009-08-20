@ECHO OFF
TITLE Compiling PeerBlock...
SETLOCAL && CLS	

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


REM Set the path of Inno Setup and compile installer
SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
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
@ECHO OFF
TITLE Compiling PeerBlock...
SETLOCAL && CLS	

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="x64"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="x64"
ECHO.
ECHO.


REM Set the path of Inno Setup and compile installer
SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
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
@ECHO OFF
TITLE Compiling PeerBlock...
SETLOCAL && CLS	

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


REM Set the path of Inno Setup and compile installer
SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
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
@ECHO OFF
TITLE Compiling PeerBlock...
SETLOCAL && CLS	

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="Win32"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration=Release /p:Platform="x64"
ECHO.
ECHO.

"%WINDIR%\Microsoft.NET\Framework\v3.5\MSBuild.exe" PeerBlock.sln^
 /t:Rebuild /p:Configuration="Release (Vista)" /p:Platform="x64"
ECHO.
ECHO.


REM Set the path of Inno Setup and compile installer
SET "U_=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
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
