#!/bin/sh
# Straight into free driving: unlimited practice, car already spawned.
# The server runs hidden inside the game, so there is only the one window.
#   ./justdrive.sh              Sports Car
#   ./justdrive.sh "Police Car" any name from the vehicle list
#
# Vehicles: Sports Car, Rocket Car, Sedan, Minivan, Police Car, Motorcycle,
#           Tank, UFO, X Fighter
set -e

VEHICLE=${1:-Sports Car}

cd "$(dirname "$0")/DRIVE"

case "$(uname)" in
Darwin)       BUILD=../build.macos.sh ;;
MINGW*|MSYS*) BUILD=../build.windows.sh; EXE=.exe ;;
*)            BUILD=../build.linux.sh ;;
esac

if [ ! -x ./drive$EXE ] || [ ! -x ./drive_server$EXE ]; then
    $BUILD
fi

echo "Practice mode, no time limit.  Driving a $VEHICLE."
echo "Mouse inside the window: up = throttle, down = brake.  'r' respawns."
DRIVE_LOCAL_SERVER=1 DRIVE_PRACTICE_MODE=1 \
DRIVE_AUTOSTART_MODE=1 DRIVE_VEHICLE="$VEHICLE" exec ./drive$EXE localhost
