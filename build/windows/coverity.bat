@ECHO OFF
REM
REM Licensed to whatever license PeerBlock is using.
REM


SETLOCAL

PUSHD %~dp0

SET COVDIR=H:\progs\thirdparty\cov-analysis-win64-6.6.1
SET "PB_DDK_DIR=H:\WinDDK\7600.16385.1"

CALL "%VS110COMNTOOLS%..\..\VC\vcvarsall.bat" x86

SET MSBUILD_SWITCHES=/nologo /consoleloggerparameters:Verbosity=minimal /maxcpucount^
 /nodeReuse:true /target:Rebuild /property:Configuration="Release_(Vista)";Platform=Win32

"%COVDIR%\bin\cov-build.exe" --dir cov-int MSBuild "PeerBlock.sln" %MSBUILD_SWITCHES%

:tar
IF EXIST "PeerBlock.tgz" DEL "PeerBlock.tgz"
tar --version 1>&2 2>NUL || (ECHO. & ECHO ERROR: tar not found & GOTO SevenZip)
tar czvf PeerBlock.tgz cov-int
GOTO End

:SevenZip
IF NOT EXIST "%PROGRAMFILES%\7za.exe" (ECHO. & ECHO ERROR: "%PROGRAMFILES%\7za.exe" not found & GOTO End)
IF EXIST "PeerBlock.tar" DEL "PeerBlock.tar"
IF EXIST "PeerBlock.tgz" DEL "PeerBlock.tgz"
"%PROGRAMFILES%\7za.exe" a -ttar PeerBlock.tar cov-int
"%PROGRAMFILES%\7za.exe" a -tgzip PeerBlock.tgz PeerBlock.tar
IF EXIST "PeerBlock.tar" DEL "PeerBlock.tar"


:End
POPD
ECHO. & ECHO Press any key to close this window...
PAUSE >NUL
ENDLOCAL
EXIT /B
