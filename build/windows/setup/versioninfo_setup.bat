@ECHO OFF

ECHO:#pragma once> versioninfo_setup.h
ECHO.>> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_MAJOR" "..\..\..\src\peerblock\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_MINOR" "..\..\..\src\peerblock\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUGFIX" "..\..\..\src\peerblock\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUILDNUM" "..\..\..\src\peerblock\versioninfo_parsed.h" >> versioninfo_setup.h
