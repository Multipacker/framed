#!/bin/sh

src_files="src/main.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-newline-eof -Wno-unused-variable -Wno-unused-function -Wconversion"
compiler_flags="-Isrc -g -fsanitize=address"

clang $src_files $compiler_flags $errors -o build/out
