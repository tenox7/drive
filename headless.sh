#!/bin/sh
# Build and run DRIVE in a Linux container on a virtual X display, and save a
# screenshot.  Nothing appears on the host desktop.
#
#   ./headless.sh out.png                 just start up and capture
#   ./headless.sh out.png 160 465         click window coords x,y first
#   VEHICLE="Tank" ./headless.sh out.png  pick a different vehicle
#
# First run builds the image and the game, which takes a few minutes.
set -e

cd "$(dirname "$0")"
OUT=${1:-frame.png}
CLICK_X=$2
CLICK_Y=$3
IMG=drive-test
VOL=drivebuild

command -v docker >/dev/null || { echo "docker required"; exit 1; }

if ! docker image inspect $IMG >/dev/null 2>&1; then
    echo "==> building container image"
    docker build -q -t $IMG - <<'DOCKER'
FROM debian:bookworm
RUN apt-get update -qq && apt-get install -y -qq \
      build-essential libglfw3-dev libgl1-mesa-dev libncurses-dev \
      xvfb x11-utils xdotool imagemagick libgl1-mesa-dri mesa-utils rsync \
    && rm -rf /var/lib/apt/lists/*
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV DISPLAY=:99
DOCKER
fi

docker volume create $VOL >/dev/null

docker run --rm -e DISPLAY=:99 -e VEHICLE="${VEHICLE:-Sports Car}" \
    -e CLICK_X="$CLICK_X" -e CLICK_Y="$CLICK_Y" \
    -v "$PWD:/srcro:ro" -v $VOL:/build -v "$PWD:/out" $IMG sh -c '
set -e
export DISPLAY=:99

# Host build artifacts must never leak into the Linux tree.
rsync -a --delete --exclude ".git" --exclude "*.o" --exclude "*.a" \
    --exclude "*.png" --exclude "DRIVE/drive" --exclude "DRIVE/drive_server" \
    --exclude "JPEG/Makefile" --exclude "JPEG/jconfig.h" \
    /srcro/ /build/src/
cd /build/src
rm -f DRIVE/drive DRIVE/drive_server JPEG/Makefile JPEG/jconfig.h
find . \( -name "*.o" -o -name "*.a" \) -delete
./build.linux.sh >/tmp/build.log 2>&1 || { tail -25 /tmp/build.log; exit 1; }

Xvfb :99 -screen 0 1400x1100x24 >/dev/null 2>&1 &
sleep 2
cd DRIVE
# The server console is curses, so it needs a pty.
TERM=xterm script -q -c "DRIVE_PRACTICE_MODE=1 ./drive_server" /tmp/srv.pty \
    >/dev/null 2>&1 &
sleep 3
DRIVE_AUTOSTART_MODE=1 DRIVE_VEHICLE="$VEHICLE" ./drive localhost \
    >/tmp/cli.log 2>&1 &
sleep 10

WID=$(xdotool search --name "^Drive$" | head -1)
[ -n "$WID" ] || { echo "no window"; head -5 /tmp/cli.log; exit 1; }
if [ -n "$CLICK_X" ]; then
    xdotool mousemove --window $WID "$CLICK_X" "$CLICK_Y" click 1
    sleep 2
fi
import -window "$WID" /out/CAPTURE.png
' 

mv -f CAPTURE.png "$OUT"
echo "wrote $OUT"
