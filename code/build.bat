@echo off

REM Setup Windows C compiler for terminal each time lol
call ..\misc\shell.bat

REM Create build dir at root of project and set
REM current working dir to build

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM Compiles the source code using MSVC
REM -D - Custom compiler flags that can be referenced in the code
REM
REM     HANDMADE_WIN32 - Flag that is set when we want to indicate
REM     we are building in windows mode
REM
REM     HANDMADE_INTERNAL - Used when we only want something to show up
REM     on our local build for dev purposes
REM
REM     HANDMADE_SLOW - Used to determine if slow code is allowed to execute
REM                     Useful if we want to do performance measurements
REM
REM -Zi - used to compile debug info into a seperate .pdb file
REM -FC - gives the full path of files during compilation
REM /std:c++17 - Needed to allow init statements in if/switch
REM Dumps the exe in the build folder at the root level of
REM the project
REM user32.lib - GUI library for windows
REM Gdi32.lib - For windows Graphical Device Interface

cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Zi /std:c++17 ..\code\win32_handmade.cpp user32.lib Gdi32.lib

REM Pop the build dir so we go back to our original dir
popd
