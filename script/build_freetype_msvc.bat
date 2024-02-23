@echo off

set ft_root=../vendor/freetype

set freetype_additional_includes=-I../vendor/ -I../vendor/freetype/include

set ft_files=%ft_root%\src\base\ftsystem.c
set ft_files=%ft_files% %ft_root%\src\base\ftinit.c
set ft_files=%ft_files% %ft_root%\src\base\ftdebug.c
set ft_files=%ft_files% %ft_root%\src\base\ftbase.c
set ft_files=%ft_files% %ft_root%\src\base\ftbitmap.c
set ft_files=%ft_files% %ft_root%\src\base\ftbbox.c
set ft_files=%ft_files% %ft_root%\src\base\ftglyph.c
set ft_files=%ft_files% %ft_root%\src\base\ftmm.c

set ft_files=%ft_files% %ft_root%\src\truetype\truetype.c
set ft_files=%ft_files% %ft_root%\src\sfnt\sfnt.c
set ft_files=%ft_files% %ft_root%\src\raster\raster.c
set ft_files=%ft_files% %ft_root%\src\psnames\psnames.c
set ft_files=%ft_files% %ft_root%\src\gzip\ftgzip.c
set ft_files=%ft_files% %ft_root%\src\smooth\smooth.c

set freetype_compile_flags=-c -FC -nologo %ft_files% -I%ft_root% %freetype_additional_includes% -DFT_CONFIG_OPTION_ERROR_STRINGS -DFT_CONFIG_MODULES_H="\"ftmodule.h\"" -DFT2_BUILD_LIBRARY
set freetype_lib_flags=*.obj -nologo

set freetype_build_mode="%1%"

if not exist build mkdir build
pushd build

cl %freetype_compile_flags%
lib %freetype_lib_flags% -OUT:freetype.lib

del *.obj

popd
