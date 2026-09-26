@echo off
rem Run DRIVE on Windows: the server console in its own window, then the
rem client.  The normal race cycle, not the endless practice justdrive.bat
rem gives you.
rem   run.bat              server + client
rem   run.bat server       just the server, in this window
rem   run.bat client HOST  just the client (default host: localhost)
setlocal
cd /d "%~dp0DRIVE"

if not exist drive.exe goto nobuild
if /i "%~1"=="server" goto server
if /i "%~1"=="client" goto client

start "DRIVE server" drive_server.exe
rem let the server bind its socket before the client comes looking
timeout /t 3 /nobreak >nul
echo Server console is in the other window.
echo Press Return there on NEXT STATE to start the race.
start "" drive.exe localhost
goto :eof

:server
drive_server.exe
goto :eof

:client
set HOST=%~2
if "%HOST%"=="" set HOST=localhost
start "" drive.exe %HOST%
goto :eof

:nobuild
echo Not built yet.  Run build.windows.sh in an MSYS2 MINGW64 shell.
pause
exit /b 1
