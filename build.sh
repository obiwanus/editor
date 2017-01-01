#!/bin/bash

set -e

CFLAGS="-g -std=c++11 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -Wall -Wextra -Wno-write-strings -Wno-missing-field-initializers -Wno-unused -Werror"
LFLAGS="$(pkg-config --cflags --libs x11) -ldl -lpthread"

mkdir -p build

# FILES_TO_COMPILE="src/ED_linux.cpp \
#     src/ED_core.cpp \
#     src/ED_math.cpp \
#     src/ED_ui.cpp \
#     src/ED_model.cpp \
#     "

OPTIMIZE=false
RUN=false

if [ "$1" = "run" ]; then
    # OPTIMIZE=true
    RUN=true
fi

if $OPTIMIZE; then
    CFLAGS="$CFLAGS -O2"
fi

g++ --std=c++11 -Isrc/ $CFLAGS src/ED_linux.cpp $LFLAGS -o build/editor

if $RUN; then
    cd build
    ./editor
fi

