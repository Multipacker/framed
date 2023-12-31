@echo off

rem 4201: nonstandard extension used: nameless struct/union
rem 4152: nonstandard extension used: function/data ptr conversion in expression
rem 4100: unreferenced formal parameter
rem 4189: local variable is initialized but not referenced
rem 4101: unreferenced local variable
rem 4310: cast truncates constant value
rem 4061: enum case not explicitly handled
rem 4820: n bytes padding added after data member x
rem 5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
rem 4711: x selected for automatic inline expansion
rem 4710: function not inlined

rem -- Common flags --

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310 -wd4061 -wd4820 -wd4191 -wd5045 -wd4711 -wd4710
set additional_includes=-I../vendor/
set opts=-DENABLE_ASSERT=1 -DRENDERER_OPENGL=1
set compiler_flags=%opts% -nologo -FC -Wall -WX %disabled_warnings% %additional_includes% -Fe:main
set libs=user32.lib kernel32.lib winmm.lib gdi32.lib shcore.lib
set linker_flags=%libs% -incremental:no
set src_files=../src/ui_test.c

rem -- Debug build flags --

set debug_compiler_flags=-RTC1 -MTd -Zi -Od -DCONSOLE=1 -DBUILD_MODE_DEBUG=1
set debug_linker_flags=-subsystem:console freetype_debug.lib

rem -- Optimized build flags --

set optimized_compiler_flags=-MTd -Zi -fsanitize=address -O2 -Oi -fp:fast -GS- -DCONSOLE=1 -DBUILD_MODE_OPTIMIZED=1
set optimized_linker_flags=-subsystem:console freetype_debug.lib

rem -- Release build flags --

set release_compiler_flags=-O2 -Oi -EHsc -fp:fast -GS- -DBUILD_MODE_RELEASE=1
set release_linker_flags=-fixed -opt:icf -opt:ref -subsystem:windows libvcruntime.lib freetype.lib

set arg0="%1%"

if %arg0% == "debug" (
	echo Debug Build
	set compiler_flags=%compiler_flags% %debug_compiler_flags%
	set linker_flags=%linker_flags% %debug_linker_flags%
) else if %arg0% == "optimized" (
	echo Optimized Build
	set compiler_flags=%compiler_flags% %optimized_compiler_flags%
	set linker_flags=%linker_flags% %optimized_linker_flags%
) else if %arg0% == "release" (
	echo Release Build
	set compiler_flags=%compiler_flags% %release_compiler_flags%
	set linker_flags=%linker_flags% %release_linker_flags%
)

if not exist build mkdir build
pushd build

cl %src_files% %compiler_flags% -link %linker_flags%

popd