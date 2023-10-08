#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MSYS2_ARCH="i686"
else
    export MSYS2_ARCH="x86_64"
fi

pacman --noconfirm -Suy

pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-glib2 \
    mingw-w64-$MSYS2_ARCH-libjpeg-turbo \
    mingw-w64-$MSYS2_ARCH-libpng \
    mingw-w64-$MSYS2_ARCH-libtiff \
    mingw-w64-$MSYS2_ARCH-meson \
    mingw-w64-$MSYS2_ARCH-toolchain

meson setup --buildtype debug \
    -Dpng=enabled \
    -Djpeg=enabled \
    -Dtiff=enabled \
    -Dman=false \
    _build

meson compile -C _build
meson test -C _build
