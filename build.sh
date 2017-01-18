#!/bin/bash

set -e

mkdir -p build

OPTIMIZE=false
RUN=false
OPENMP=true
OPENGL=false

LEAKCHECK=true
BUILD_INTERNAL=true
BUILD_SLOW=true

if [ "$1" = "run" ]; then
    # OPTIMIZE=true
    RUN=true
fi

CFLAGS="-g -fno-exceptions\
        -Wall -Wextra -Wno-write-strings -Wno-missing-field-initializers\
        -Wno-missing-braces -Wno-unused-parameter -Wno-unused -Werror\
        -DBUILD_INTERNAL=$BUILD_INTERNAL\
        -DBUILD_SLOW=$BUILD_SLOW\
        -DED_LINUX_OPENGL=$OPENGL\
        -DED_LEAKCHECK=$LEAKCHECK\
        "

LFLAGS="$(pkg-config --cflags --libs x11) -lGL -ldl -lpthread -lm"

if $OPTIMIZE; then
    CFLAGS="$CFLAGS -O3"
fi

if $OPENMP; then
    CFLAGS="$CFLAGS -fopenmp=libiomp5"
fi

# g++ --std=c++11 -Isrc/ $CFLAGS src/ED_linux.cpp $LFLAGS -o build/editor
clang++-4.0 -stdlib=libc++ --std=c++11 -Isrc/ $CFLAGS src/ED_linux.cpp $LFLAGS -o build/editor

if $RUN; then
    cd build
    ./editor
fi

