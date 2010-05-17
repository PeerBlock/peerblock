@ECHO OFF

"%PB_DDK_DIR%\bin\SelfSign\signtool.exe" sign /v /ac %PB_CERT% /s my^
 /n "PeerBlock, LLC" /t http://timestamp.globalsign.com/scripts/timstamp.dll %1

:END
ECHO. && ECHO.
ECHO:Done!
ECHO.
