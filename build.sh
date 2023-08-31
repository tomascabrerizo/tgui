#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

clang -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -std=c99 -g -I./thirdparty -I./code ./thirdparty/stb_truetype.c ./code/main.c -o ./build/app -lm -lX11 -lGL -lXcursor -Wno-implicit-fallthrough
