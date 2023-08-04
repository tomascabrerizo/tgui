#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

clang -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -std=c99 -g -I./code ./code/main.c -o ./build/app -lX11
