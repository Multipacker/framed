#!/bin/sh

mode=${1:-debug}

mkdir -p build

src_files="src/linux_test.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-newline-eof -Wno-unused-variable -Wno-unused-function -Wconversion"
common_flags="-Isrc -Ivendor -o build/out"
linker_flags=""

if [ "$mode" == "debug" ]; then
	compiler_flags="-g -fsanitize=address -DBUILD_MODE=0"
elif [ "$mode" == "optimized"]; then
	compiler_flags="-g -O3 -fsanitize=address -DBUILD_MODE=1"
elif [ "$mode" == "release"]; then
	compiler_flags="-DBUILD_MODE=2"
else
	echo "ERROR: Unknown build type."
	exit 1
fi

clang $src_files $errors $common_flags $compiler_flags $linker_flags
