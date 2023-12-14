#!/bin/sh

mkdir -p build

src_files="src/main.c"
errors="-Werror -Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
compiler_flags="-Isrc -g -fsanitize=address"
linker_flags="-lm"

gcc $src_files $compiler_flags $linker_flags $errors -o build/out
