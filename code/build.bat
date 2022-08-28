@echo off

REM Setup Windows C compiler for terminal each time lol
call ..\misc\shell.bat

REM Create build dir at root of project and set
REM current working dir to build

mkdir ..\build
pushd ..\build

REM Compiles the source code using MSVC
REM -Zi - used to compile debug info into a seperate .pdb file
REM Dumps the exe in the build folder at the root level of
REM the project
REM user32.lib - GUI library for windows

cl -Zi ..\code\win32_handmade.cpp user32.lib

REM Pop the build dir so we go back to our original dir
popd
