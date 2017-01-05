#!/bin/bash

set -e

mkdir -p build

OPTIMIZE=false
RUN=false
LEAKCHECK=true
OPENMP=true
OPENGL=false

if [ "$1" = "run" ]; then
    # OPTIMIZE=true
    RUN=true
fi

CFLAGS="-g -std=c++11\
        -Wall -Wextra -Wno-write-strings -Wno-missing-field-initializers -Wno-unused -Werror\
        -DBUILD_INTERNAL=1\
        -DBUILD_SLOW=1\
        -DED_LINUX_OPENGL=$OPENGL\
        -DED_LEAKCHECK=$LEAKCHECK\
        "

LFLAGS="$(pkg-config --cflags --libs x11) -lGL -ldl -lpthread"

if $OPTIMIZE; then
    CFLAGS="$CFLAGS -O2"
fi

if $OPENMP; then
    CFLAGS="$CFLAGS -fopenmp"
fi

g++ --std=c++11 -Isrc/ $CFLAGS src/ED_linux.cpp $LFLAGS -o build/editor

if $RUN; then
    cd build
    ./editor
fi

