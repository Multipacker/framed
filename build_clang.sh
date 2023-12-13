#!/bin/sh

src_files="src/clang_compiler_test.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-newline-eof -Wno-unused-variable -Wno-unused-function"
compiler_flags="-Isrc -g -fsanitize=address"

clang $src_files $compiler_flags $errors -o build/out
