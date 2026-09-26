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

if [ ! -x ./drive ] || [ ! -x ./drive_server ]; then
    case "$(uname)" in
    Darwin) ../build.macos.sh ;;
    *)      ../build.linux.sh ;;
    esac
fi

echo "Practice mode, no time limit.  Driving a $VEHICLE."
echo "Mouse inside the window: up = throttle, down = brake.  'r' respawns."
DRIVE_LOCAL_SERVER=1 DRIVE_PRACTICE_MODE=1 \
DRIVE_AUTOSTART_MODE=1 DRIVE_VEHICLE="$VEHICLE" exec ./drive localhost
