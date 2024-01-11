#!/bin/sh

mode=${1:-debug}

mkdir -p build

src_files="src/image_test.c"
errors="-Werror -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-override-init-side-effects -Wno-unused-but-set-variable -Wconversion"
common_flags="-Isrc -Ivendor -o build/out -DRENDERER_OPENGL=1 -pthread"
linker_flags="-fuse-ld=mold -lm -lSDL2 build/freetype/freetype"

if [ "$mode" == "debug" ]; then
	compiler_flags="-g -fsanitize=address -DENABLE_ASSERT=1 -DBUILD_MODE_DEBUG=1"
elif [ "$mode" == "optimized" ]; then
	compiler_flags="-g -O3 -fsanitize=address -DENABLE_ASSERT=1 -DBUILD_MODE_OPTIMIZED=1"
elif [ "$mode" == "release" ]; then
	compiler_flags="-O3 -DBUILD_MODE_RELEASE=1"
else
	echo "ERROR: Unknown build type."
	exit 1
fi

gcc $src_files $errors $common_flags $compiler_flags $linker_flags
