@echo off

REM For some dumb fucking reason, Microsoft doesn't setup running their
REM C compiler from the command line automatically. So we have to do this during
REM setup each time. This is located in the Visual Studio install path,
REM so it will vary based on which version you have. Add the location of it
REM to the path before running
REM The DevEnvDir is a variable set by vcvars, so only call it if it
REM hasn't been called before
if not defined DevEnvDir (
    call vcvars64.bat
)

