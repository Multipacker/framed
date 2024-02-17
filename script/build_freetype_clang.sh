mkdir -p build/freetype

src_files="../../vendor/freetype/src/base/ftsystem.c"
src_files+=" ../../vendor/freetype/src/base/ftinit.c"
src_files+=" ../../vendor/freetype/src/base/ftdebug.c"
src_files+=" ../../vendor/freetype/src/base/ftbase.c"
src_files+=" ../../vendor/freetype/src/base/ftbitmap.c"
src_files+=" ../../vendor/freetype/src/base/ftbbox.c"
src_files+=" ../../vendor/freetype/src/base/ftglyph.c"
src_files+=" ../../vendor/freetype/src/base/ftmm.c"
src_files+=" ../../vendor/freetype/src/truetype/truetype.c"
src_files+=" ../../vendor/freetype/src/sfnt/sfnt.c"
src_files+=" ../../vendor/freetype/src/raster/raster.c"
src_files+=" ../../vendor/freetype/src/psnames/psnames.c"
src_files+=" ../../vendor/freetype/src/gzip/ftgzip.c"
src_files+=" ../../vendor/freetype/src/smooth/smooth.c"

pushd build/freetype

clang -c -I../../vendor -I../../vendor/freetype/include -fPIC $src_files -O2 -march=native -DFT_CONFIG_OPTION_ERROR_STRINGS -DFT_CONFIG_MODULES_H='"ftmodule.h"' -DFT2_BUILD_LIBRARY

ar r freetype *.o

popd
