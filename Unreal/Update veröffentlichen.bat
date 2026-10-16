@echo off
rem Matchball Tennis - Update veroeffentlichen (README 19.4.6)
rem Ruft Tools\release.ps1 auf (Meilenstein M10 - noch nicht umgesetzt).
rem Hinweis: ~dp0 (Ordner dieser Datei) endet mit einem Backslash, daher direkt "Tools\..." anhaengen.
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\release.ps1" %*
set "MB_EXITCODE=%ERRORLEVEL%"
echo.
if not "%MB_EXITCODE%"=="0" echo Es ist ein Fehler aufgetreten - Exit-Code %MB_EXITCODE%. Die Meldungen stehen oben.
pause
exit /b %MB_EXITCODE%
