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
set freetype_link_flags=
set freetype_lib_flags=*.obj

set freetype_build_mode="%1%"

if not exist build mkdir build
pushd build

if %freetype_build_mode% == "debug" (
	set freetype_compile_flags=%freetype_compile_flags% -Od -MTd
	set freetype_link_flags=%freetype_link_flags%
	set freetype_lib_flags=%freetype_lib_flags% -OUT:freetype_debug.lib
) else if %freetype_build_mode% == "release" (
	set freetype_compile_flags=%freetype_compile_flags% -O2 -Oi -fp:fast -GS-
	set freetype_link_flags=%freetype_link_flags% -opt:icf -opt:ref libvcruntime.lib -fixed
	set freetype_lib_flags=%freetype_lib_flags% -OUT:freetype.lib
)

cl %freetype_compile_flags% -link %freetype_link_flags%
lib %freetype_lib_flags%
	
del *.obj

popd
