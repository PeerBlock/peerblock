@ECHO OFF
DEL /Q "versioninfo_setup.h" >NUL 2>&1
ECHO:#pragma once > versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_A" versioninfo_parsed.h >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_B" versioninfo_parsed.h >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_C" versioninfo_parsed.h >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_BLDNUM" versioninfo_parsed.h >> versioninfo_setup.h
