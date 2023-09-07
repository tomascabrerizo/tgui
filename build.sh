#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

clang -pedantic -D_GNU_SOURCE -Wall -Wextra -Werror -std=c99 -g -I./thirdparty -I./code ./thirdparty/stb_truetype.c \
    ./code/main.c ./code/tgui.c ./code/tgui_memory.c ./code/tgui_gfx.c ./code/tgui_os.c \
    ./code/tgui_painter.c ./code/tgui_geometry.c ./code/tgui_docker.c ./code/tgui_serializer.c \
    -o ./build/app -lm -lX11 -lGL -lXcursor -Wno-implicit-fallthrough
