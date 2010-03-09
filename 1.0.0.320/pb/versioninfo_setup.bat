@ECHO OFF
DEL /Q "versioninfo_setup.h" >NUL 2>&1
FINDSTR /I /L /C:"define PB_VER_MAJOR" versioninfo_parsed.h > versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_MINOR" versioninfo_parsed.h >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUGFIX" versioninfo_parsed.h >> versioninfo_setup.h
FINDSTR /I /L /C:"define PB_VER_BUILDNUM" versioninfo_parsed.h >> versioninfo_setup.h
