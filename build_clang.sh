#!/bin/sh

mkdir -p build

src_files="src/linux_test.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-newline-eof -Wno-unused-variable -Wno-unused-function -Wconversion"
compiler_flags="-Isrc -g -fsanitize=address"
linker_flags=""

clang $src_files $compiler_flags $linker_flags $errors -o build/out
