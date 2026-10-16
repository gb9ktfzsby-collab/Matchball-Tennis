@echo off
rem Matchball Tennis - Schiedsrichter-Stimme mit ElevenLabs erzeugen (README 16.2)
rem Ruft Tools\elevenlabs-stimme.ps1 auf: fragt den Schluessel ab (wird nicht gespeichert)
rem und erzeugt nur die fehlenden Bausteine. Beispiele fuer Parameter:
rem   -Liste                  nur anzeigen, was fehlt
rem   -Namen "Meier,Huber"    zusaetzliche Namen
rem   -StimmeId VOICE_ID      andere Stimme (Standard: Alice)
rem Hinweis: ~dp0 (Ordner dieser Datei) endet mit einem Backslash, daher direkt "Tools\..." anhaengen.
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\elevenlabs-stimme.ps1" %*
set "MB_EXITCODE=%ERRORLEVEL%"
echo.
if not "%MB_EXITCODE%"=="0" echo Es ist ein Fehler aufgetreten - Exit-Code %MB_EXITCODE%. Die Meldungen stehen oben.
pause
exit /b %MB_EXITCODE%
