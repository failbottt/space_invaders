@echo off
setlocal enabledelayedexpansion

if exist build rmdir /s /q "./build"

if not exist build mkdir build

set cl_common=     /I..\src\ /nologo /FC /Z7
set cl_debug=      call cl /Od /Ob1 /DBUILD_DEBUG=1 /W2 /DBUILD_INTERNAL=1 %cl_common%

pushd build
echo [building...] && %cl_debug% ../src/main.c ../src/gfx.c ../src/platform/windows.c /link opengl32.lib user32.lib gdi32.lib
popd
