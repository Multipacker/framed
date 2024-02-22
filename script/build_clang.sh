#!/bin/sh

mode=${1:-release}

if [ ! -f build/freetype/freetype ]; then
    source "script/build_freetype_clang.sh"
fi

mkdir -p build

src_files="src/framed/framed_main.c"
out_file="build/framed"
errors="-Werror -Wall -Wno-missing-braces -Wno-newline-eof -Wno-unused-variable -Wno-unused-function -Wconversion -Wno-initializer-overrides -Wno-unused-but-set-variable -Wshadow -Wsign-compare -Wsign-conversion -Wtautological-compare -Wtype-limits"
common_flags="-I. -Isrc -Ivendor -Ivendor/freetype/include -o $out_file -DRENDERER_OPENGL=1 -pthread"
linker_flags="-lSDL2 build/freetype/freetype -lm"

if [ "$mode" == "debug" ]; then
    compiler_flags="-g -fsanitize=address -DENABLE_ASSERT=1 -DBUILD_MODE_DEBUG=1"
elif [ "$mode" == "optimized" ]; then
    compiler_flags="-g -O3 -march=native -fsanitize=address -DENABLE_ASSERT=1 -DBUILD_MODE_OPTIMIZED=1"
elif [ "$mode" == "release" ]; then
    compiler_flags="-O3 -s -march=native -DBUILD_MODE_RELEASE=1 -Wl,--gc-sections -fdata-sections -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -Wa,--noexecstack"
else
    echo "ERROR: Unknown build type."
    exit 1
fi

clang $src_files $errors $common_flags $compiler_flags $linker_flags

if [ "$mode" == "release" ]; then
    strip -R .comment $out_file
fi
