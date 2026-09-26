#!/bin/sh
# Build DRIVE (client + server) for Windows in an MSYS2 MINGW64 shell.
# Needs: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw make
set -e

cd "$(dirname "$0")"
ROOT=$(pwd)

LEGACY="-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration \
 -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types"

echo "==> libjpeg"
cd "$ROOT/JPEG"
# Its configure predates the .exe suffix; the ANSI makefile needs neither.
cp makefile.ansi Makefile
cp jconfig.dj jconfig.h
make libjpeg.a CC=gcc CFLAGS="-O2 $LEGACY"

echo "==> libpng helpers"
cd "$ROOT/LIBPNG"
mkdir -p OBJ
for f in readpng writepng; do
    gcc -O2 $LEGACY -DPNG_LITTLE_ENDIAN -c $f.c -o OBJ/$f.o
done

echo "==> PDCurses"
cd "$ROOT/PDCURSES/wincon"
make pdcurses.a

echo "==> HoverWare"
cd "$ROOT/HW"
mkdir -p Objs Objs_d Common/Objs Objects/Objs GUI/Objs GLES/Objs Null/Objs
make -f Mingw_GLFW.mk default

echo "==> DRIVE"
cd "$ROOT/DRIVE"
make -f Mingw_GLFW.mk all

echo
echo "Built $ROOT/DRIVE/drive.exe and $ROOT/DRIVE/drive_server.exe"
