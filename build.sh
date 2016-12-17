#!/bin/bash

set -e

CFLAGS="-g -std=c++11 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -Wno-write-strings"
LFLAGS="$(pkg-config --cflags --libs x11) -ldl -lpthread"

mkdir -p build

FILES_TO_COMPILE="src/ED_linux.cpp \
    src/ED_core.cpp \
    src/ED_math.cpp \
    src/ED_ui.cpp \
    src/raytrace/ED_raytrace.cpp \
    src/ED_model.cpp \
    "

OPTIMIZE=true

if $OPTIMIZE; then
    CFLAGS="$CFLAGS -O2"
fi

g++ --std=c++11 -Isrc/ $CFLAGS $FILES_TO_COMPILE $LFLAGS -o build/editor

if [ "$1" = "run" ]; then
    cd build
    ./editor
fi

