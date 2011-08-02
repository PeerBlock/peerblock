@ECHO OFF
SETLOCAL

SET "SUBWCREV=SubWCRev.exe"

"%SUBWCREV%" "..\.." "version.h" "version_parsed.h" -f
IF %ERRORLEVEL% NEQ 0 GOTO NoSubWCRev

ENDLOCAL
EXIT /B


:NoSubWCRev
ECHO. & ECHO SubWCRev, which is part of TortoiseSVN, wasn't found!
ECHO You should (re)install TortoiseSVN.
ECHO I'll use PB_VER_BUILDNUM=9999 for now.

ECHO #define PB_VER_MAJOR 1 > version_parsed.h
ECHO #define PB_VER_MINOR 1 >> version_parsed.h
ECHO #define PB_VER_BUGFIX 0 >> version_parsed.h
ECHO #define PB_VER_BUILDNUM 9999 >> version_parsed.h

ENDLOCAL
EXIT /B
