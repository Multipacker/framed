@echo off

:: 4201: nonstandard extension used: nameless struct/union
:: 4152: nonstandard extension used: function/data ptr conversion in expression
:: 4100: unreferenced formal parameter
:: 4189: local variable is initialized but not referenced
:: 4101: unreferenced local variable
:: 4310: cast truncates constant value
:: 4061: enum case not explicitly handled
:: 4820: n bytes padding added after data member x
:: 5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
:: 4711: x selected for automatic inline expansion
:: 4710: function not inlined

:: -- Common flags --

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310 -wd4061 -wd4820 -wd4191 -wd5045 -wd4711 -wd4710 -wd4200
set additional_includes=-I../vendor/ -I../src/ -I../
set opts=-DENABLE_ASSERT=1 -DRENDERER_D3D11=1
set compiler_flags=%opts% -nologo -MT -FC -Wall -MP -WX %disabled_warnings% %additional_includes% -Fe:framed
set linker_flags=-incremental:no user32.lib kernel32.lib winmm.lib shell32.lib shcore.lib opengl32.lib gdi32.lib
set src_files=../src/framed/framed_main.c

:: NOTE(hampus): We're compiling everything with the non-debug crt static library

:: -- Debug build flags --

set debug_compiler_flags=-RTC1 -Zi -Od -fsanitize=address -DCONSOLE=1 -DBUILD_MODE_DEBUG=1
set debug_linker_flags=-subsystem:console

:: -- Optimized build flags --

set optimized_compiler_flags=-Zi -O2 -Oi -fp:fast -GS- -DCONSOLE=1 -DBUILD_MODE_OPTIMIZED=1
set optimized_linker_flags=-subsystem:console

:: -- Release build flags --

set release_compiler_flags=-O2 -Oi -EHsc -fp:fast -GS- -DBUILD_MODE_RELEASE=1
set release_linker_flags=-fixed -opt:icf -opt:ref -subsystem:windows libvcruntime.lib

set build_mode="%1%"

if %build_mode% == "debug" (
    echo [debug build]
    set compiler_flags=%compiler_flags% %debug_compiler_flags%
    set linker_flags=%linker_flags% %debug_linker_flags%
) else if %build_mode% == "optimized" (
    echo [optimized build]
    set compiler_flags=%compiler_flags% %optimized_compiler_flags%
    set linker_flags=%linker_flags% %optimized_linker_flags%
) else if %build_mode% == "release" (
    echo [release build]
    set compiler_flags=%compiler_flags% %release_compiler_flags%
    set linker_flags=%linker_flags% %release_linker_flags%
) else if %build_mode% == "examples" (
    echo [examples build]
) else (
    echo [release build]
    set compiler_flags=%compiler_flags% %release_compiler_flags%
    set linker_flags=%linker_flags% %release_linker_flags%
)

if %build_mode% == "examples" (

    echo ---- Building examples ----

    if not exist examples\build mkdir examples\build
    pushd examples\build

    cl -nologo ..\recursion.c -I..\..\
    cl -nologo ..\simple_init_and_send.c -I..\..\
    cl -nologo ..\auto_closing_zones.cpp -I..\..\

	popd

) else (

    if exist build\*.pdb del build\*.pdb

    if not exist build mkdir build
    pushd build

	echo ---- Building meta program ----
    cl -nologo -FC -O2 -I../src/ -I../vendor/ ../src/meta/meta.c -Fe:meta -link user32.lib kernel32.lib winmm.lib shell32.lib shcore.lib

    popd

    echo ---- Running meta program ----

    build\meta.exe

    echo ---- Building shaders ----

    fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh src/render/d3d11/d3d11_vshader.h /Vn d3d11_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv src/render/d3d11/d3d11_shader.hlsl
    fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh src/render/d3d11/d3d11_pshader.h /Vn d3d11_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv src/render/d3d11/d3d11_shader.hlsl

    if not exist build mkdir build
    pushd build

    echo ---- Building Framed ----

    cl %src_files% %compiler_flags% -link %linker_flags%

    popd

)

