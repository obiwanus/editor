#!/bin/bash

echo "Starting build"

CFLAGS="-g -std=c++11 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -Wno-write-strings"
LFLAGS="$(pkg-config --cflags --libs x11) -ldl -lpthread"

mkdir -p build

cd build

gcc $CFLAGS ../base/ED_linux.cpp $LFLAGS -o editor

cd ..
