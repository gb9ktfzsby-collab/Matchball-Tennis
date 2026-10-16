@echo off
rem Matchball Tennis - Spiel fuer Freunde packen (README 15.4, 19.3)
rem Baut das Spiel (Shipping) und legt "Matchball Tennis VERSION (BUILD).zip" auf den Desktop.
rem Ruft Tools\package.ps1 -Zip auf; weitere Parameter werden durchgereicht (z. B. -NurNachbearbeitung).
rem Hinweis: ~dp0 (Ordner dieser Datei) endet mit einem Backslash, daher direkt "Tools\..." anhaengen.
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\package.ps1" -Zip %*
set "MB_EXITCODE=%ERRORLEVEL%"
echo.
if not "%MB_EXITCODE%"=="0" echo Es ist ein Fehler aufgetreten - Exit-Code %MB_EXITCODE%. Die Meldungen stehen oben.
pause
exit /b %MB_EXITCODE%
