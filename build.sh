#!/bin/bash

set -e

CFLAGS="-g -std=c++11 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -Wno-write-strings"
LFLAGS="$(pkg-config --cflags --libs x11) -ldl -lpthread"

mkdir -p build

g++ --std=c++11 $CFLAGS base/ED_linux.cpp base/ED_core.cpp base/ED_math.cpp $LFLAGS -o build/editor

build/editor

