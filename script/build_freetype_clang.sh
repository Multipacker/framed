mkdir -p build/freetype

src_files="../../vendor/ft2build.h ../../vendor/freetype/src/base/ftsystem.c ../../vendor/freetype/src/base/ftinit.c ../../vendor/freetype/src/base/ftdebug.c ../../vendor/freetype/src/base/ftbase.c ../../vendor/freetype/src/base/ftbitmap.c ../../vendor/freetype/src/base/ftbbox.c ../../vendor/freetype/src/base/ftglyph.c ../../vendor/freetype/src/base/ftmm.c ../../vendor/freetype/src/truetype/truetype.c ../../vendor/freetype/src/sfnt/sfnt.c ../../vendor/freetype/src/raster/raster.c ../../vendor/freetype/src/psnames/psnames.c ../../vendor/freetype/src/gzip/ftgzip.c ../../vendor/freetype/src/smooth/smooth.c"

pushd build/freetype

clang -c -I../../vendor -I../../vendor/freetype -fPIC $src_files -O2 -DFT_CONFIG_OPTION_ERROR_STRINGS -DFT2_BUILD_LIBRARY

ar r freetype *.o

popd
