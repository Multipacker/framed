@echo off

set ft_root=../../vendor/freetype

set additional_includes=-I../../vendor/

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

if not exist build\freetype mkdir build\freetype
pushd build\freetype

cl -c -nologo -O2 -Oi -fp:fast -GS- %ft_files% -I%ft_root% %additional_includes% -DFT_CONFIG_OPTION_ERROR_STRINGS -DFT2_BUILD_LIBRARY -link -opt:icf -opt:ref libvcruntime.lib -fixed

lib *.obj -OUT:freetype.lib

del *.obj

cl -c -nologo -Od %ft_files% -MTd -I%ft_root% %additional_includes% -DFT_CONFIG_OPTION_ERROR_STRINGS -DFT2_BUILD_LIBRARY

lib *.obj -OUT:freetype_debug.lib

del *.obj

popd