@echo off
rem Straight into free driving: unlimited practice, car already spawned.
rem The server runs hidden inside the game, so there is only the one window.
rem   justdrive.bat               Sports Car
rem   justdrive.bat "Police Car"  any name from the vehicle list
rem
rem Vehicles: Sports Car, Rocket Car, Sedan, Minivan, Police Car, Motorcycle,
rem           Tank, UFO, X Fighter
setlocal
set VEHICLE=%~1
if "%VEHICLE%"=="" set VEHICLE=Sports Car

cd /d "%~dp0DRIVE"
if not exist drive.exe (
    echo Not built yet.  Run build.windows.sh in an MSYS2 MINGW64 shell.
    pause
    exit /b 1
)

set DRIVE_LOCAL_SERVER=1
set DRIVE_PRACTICE_MODE=1
set DRIVE_AUTOSTART_MODE=1
set DRIVE_VEHICLE=%VEHICLE%
start "" drive.exe localhost
