#!/bin/sh
# Build DRIVE (client + server) for modern Linux (gcc, system GLFW).
# Debian/Ubuntu deps: build-essential libglfw3-dev libgl1-mesa-dev libncurses-dev
set -e

cd "$(dirname "$0")"
ROOT=$(pwd)

LEGACY="-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration \
 -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types"

echo "==> libjpeg"
cd "$ROOT/JPEG"
[ -f Makefile ] || CC=gcc CFLAGS="-O2 -fPIC $LEGACY" ./configure
make libjpeg.a

echo "==> libpng helpers"
cd "$ROOT/LIBPNG"
mkdir -p OBJ
for f in readpng writepng; do
    gcc -O2 -fPIC $LEGACY -DPNG_LITTLE_ENDIAN -c $f.c -o OBJ/$f.o
done

echo "==> HoverWare"
cd "$ROOT/HW"
mkdir -p Objs Objs_d Common/Objs Objects/Objs GUI/Objs GLES/Objs Null/Objs
make -f Linux_GLFW.mk default

echo "==> DRIVE"
cd "$ROOT/DRIVE"
make -f Linux_GLFW.mk all

echo
echo "Built $ROOT/DRIVE/drive and $ROOT/DRIVE/drive_server"
