@echo off

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310

set opts=-DCONSOLE=1
set compiler_flags=%opts% -RTC1 -MTd -Zi -nologo -FC -Od -W4 -WX -fsanitize=address %disabled_warnings%
set libs=user32.lib kernel32.lib winmm.lib gdi32.lib
set linker_flags=%libs% -incremental:no
set src_files=../src/main.c

if not exist build mkdir build
pushd build

cl %src_files% %compiler_flags% -link %linker_flags%

popd