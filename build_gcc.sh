#!/bin/sh

mode=${1:-debug}

mkdir -p build

src_files="src/main.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
common_flags="-Isrc -Ivendor -o build/out"
linker_flags="-lm"

if [ "$mode" == "debug" ]; then
	compiler_flags="-g -fsanitize=address -DBUILD_MODE_DEBUG=1"
elif [ "$mode" == "optimized"]; then
	compiler_flags="-g -fsanitize=address -DBUILD_MODE_OPTIMIZED=1"
elif [ "$mode" == "release"]; then
	compiler_flags="-DBUILD_MODE_RELEASE=1"
else
	echo "ERROR: Unknown build type."
	exit 1
fi

gcc $src_files $errors $common_flags $compiler_flags $linker_flags
