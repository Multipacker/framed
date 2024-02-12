@echo off

set disabled_warnings=-wd4201 -wd4152 -wd4100 -wd4189 -wd4101 -wd4310 -wd4061 -wd4820 -wd4191 -wd5045 -wd4711 -wd4710
set additional_includes=-I../
set compiler_flags=%opts% -nologo -Zi -Od -FC -Wall -MP -WX %disabled_warnings% %additional_includes% -Fe:test
set libs=
set linker_flags=%libs% -incremental:no 
set src_files=../src/tests/test.c

if not exist build mkdir build
pushd build

cl %src_files% %compiler_flags% -link %linker_flags%

popd