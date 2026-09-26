#!/bin/sh
# Build DRIVE (client + server) for modern macOS (arm64/x86_64, clang, Homebrew GLFW).
set -e

cd "$(dirname "$0")"
ROOT=$(pwd)

LEGACY="-fcommon -std=gnu89 -Wno-implicit-int -Wno-implicit-function-declaration \
 -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types \
 -Wno-deprecated-non-prototype -Wno-deprecated-declarations"

command -v brew >/dev/null || { echo "Homebrew required"; exit 1; }
brew --prefix glfw >/dev/null 2>&1 || brew install glfw

echo "==> libjpeg"
cd "$ROOT/JPEG"
[ -f Makefile ] || CC=clang CFLAGS="-O2 $LEGACY" ./configure
make libjpeg.a

echo "==> libpng helpers"
cd "$ROOT/LIBPNG"
mkdir -p OBJ
for f in readpng writepng; do
    clang -O2 $LEGACY -DMAC -DPNG_LITTLE_ENDIAN -c $f.c -o OBJ/$f.o
done

echo "==> HoverWare"
cd "$ROOT/HW"
mkdir -p Objs Objs_d Common/Objs Objects/Objs GUI/Objs GLES/Objs Null/Objs
make -f OSX_GLFW.mk default

echo "==> DRIVE"
cd "$ROOT/DRIVE"
make -f OSX_GLFW.mk all

echo
echo "Built $ROOT/DRIVE/drive and $ROOT/DRIVE/drive_server"
