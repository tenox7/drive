#!/bin/sh
# Build a Linux tarball that runs on any reasonably current distribution.
#
#   ./package.linux.sh        build/DRIVE-linux-x86_64.tar.gz
#
# Everything happens in an Ubuntu 20.04 container, so the binaries need
# glibc 2.31 or newer -- Ubuntu 20.04, Debian 11, RHEL 9 and up.  GLFW and
# curses are linked statically; libGL and libX11 deliberately are not,
# because those have to be the host's own.
set -e

cd "$(dirname "$0")"
IMG=drive-package
GLFW=3.4

command -v docker >/dev/null || { echo "docker required"; exit 1; }

if ! docker image inspect $IMG >/dev/null 2>&1; then
    echo "==> building container image"
    docker build -q --platform linux/amd64 -t $IMG - <<DOCKER
FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y -qq \
      build-essential cmake curl rsync pkg-config libgl1-mesa-dev \
      libncurses-dev libx11-dev libxrandr-dev libxinerama-dev \
      libxcursor-dev libxi-dev \
    && rm -rf /var/lib/apt/lists/*
# GLFW ships no static library on Ubuntu, so build one.  X11 only: Wayland
# desktops run it through XWayland, and that is one less thing to depend on.
RUN curl -sSL -o /tmp/glfw.tar.gz \
      https://github.com/glfw/glfw/archive/refs/tags/$GLFW.tar.gz \
 && cd /tmp && tar xzf glfw.tar.gz && cd glfw-$GLFW \
 && cmake -S . -B b -DGLFW_BUILD_SHARED_LIBRARY=OFF -DGLFW_BUILD_WAYLAND=OFF \
      -DGLFW_BUILD_X11=ON -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF \
      -DGLFW_BUILD_DOCS=OFF -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
 && cmake --build b -j\$(nproc) >/dev/null && cmake --install b \
 && rm -rf /tmp/glfw*
DOCKER
fi

docker run --rm --platform linux/amd64 -v "$PWD:/srcro:ro" -v "$PWD/build:/out" \
    $IMG sh -c '
set -e
# Host build artifacts must never leak into the Linux tree.
rsync -a --delete --exclude ".git" --exclude "*.o" --exclude "*.a" \
    --exclude "build" --exclude "DRIVE/drive" --exclude "DRIVE/drive_server" \
    --exclude "JPEG/Makefile" --exclude "JPEG/jconfig.h" /srcro/ /src/
cd /src
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
export GLFW_LIB="/usr/local/lib/libglfw3.a $(pkg-config --static --libs-only-l glfw3 | sed s/-lglfw3//)"
export CURSES_LIB="-Wl,-Bstatic -lncurses -ltinfo -Wl,-Bdynamic"
./build.linux.sh >/tmp/build.log 2>&1 || { tail -25 /tmp/build.log; exit 1; }
make tar
cp build/*.tar.gz /out/
echo "=== ldd drive ==="; ldd DRIVE/drive
'

ls -lh build/*.tar.gz
