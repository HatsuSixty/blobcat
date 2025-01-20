#!/bin/sh

PKGS=raylib
CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb $(pkg-config --cflags $PKGS)"
LIBS="$(pkg-config --libs raylib)"

cc $CFLAGS -o main ./src/main.c ./src/ffmpeg.c $LIBS
