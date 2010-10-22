@ECHO OFF

ECHO:#pragma once> versioninfo_setup.h
ECHO.>> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_MAJOR" "..\pb\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_MINOR" "..\pb\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUGFIX" "..\pb\versioninfo_parsed.h" >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUILDNUM" "..\pb\versioninfo_parsed.h" >> versioninfo_setup.h
