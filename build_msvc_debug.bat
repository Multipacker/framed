@echo off

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310

set additional_includes=-I../vendor/
set opts=-DCONSOLE=1 -DENABLE_ASSERT=1
set compiler_flags=%opts% -Zi -nologo -FC -Od -W4 -fsanitize=address -WX %disabled_warnings% %additional_includes%
set libs=user32.lib kernel32.lib winmm.lib gdi32.lib
set linker_flags=%libs%
set src_files=../src/main.c

if not exist build mkdir build
pushd build

cl %src_files% %compiler_flags% -link %linker_flags%

popd