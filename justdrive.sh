#!/bin/sh
# Straight into free driving: unlimited practice, car already spawned.
#   ./justdrive.sh              Sports Car
#   ./justdrive.sh "Police Car" any name from the vehicle list
#
# Vehicles: Sports Car, Rocket Car, Sedan, Minivan, Police Car, Motorcycle,
#           Tank, UFO, X Fighter
set -e

VEHICLE=${1:-Sports Car}

cd "$(dirname "$0")/DRIVE"
DIR=$(pwd)

if [ ! -x ./drive ] || [ ! -x ./drive_server ]; then
    case "$(uname)" in
    Darwin) ../build.macos.sh ;;
    *)      ../build.linux.sh ;;
    esac
fi

pkill -x drive_server 2>/dev/null || true
sleep 1

if [ "$(uname)" = Darwin ]; then
    osascript -e "tell application \"Terminal\" to do script \"cd '$DIR' && DRIVE_PRACTICE_MODE=1 ./drive_server\"" \
              -e 'tell application "Terminal" to activate' >/dev/null
elif command -v x-terminal-emulator >/dev/null; then
    DRIVE_PRACTICE_MODE=1 x-terminal-emulator -e "$DIR/drive_server" &
elif command -v xterm >/dev/null; then
    DRIVE_PRACTICE_MODE=1 xterm -e "$DIR/drive_server" &
else
    DRIVE_PRACTICE_MODE=1 ./drive_server >/dev/null 2>&1 &
fi

sleep 3
echo "Practice mode, no time limit.  Driving a $VEHICLE."
echo "Mouse inside the window: up = throttle, down = brake.  'r' respawns."
DRIVE_AUTOSTART_MODE=1 DRIVE_VEHICLE="$VEHICLE" ./drive localhost
