@echo off

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310

set additional_includes=-I../vendor/
set opts=-DCONSOLE=0
set compiler_flags=%opts% -nologo -GS- -FC -O2 -W4 -WX %disabled_warnings% %additional_includes%
set libs=user32.lib kernel32.lib winmm.lib gdi32.lib
set linker_flags=%libs% -fixed -incremental:no -opt:icf -opt:ref
set src_files=../src/main.c

if not exist build mkdir build
pushd build

cl %src_files% %compiler_flags% -link %linker_flags%

popd