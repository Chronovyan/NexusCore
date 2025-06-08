@echo off
setlocal

:: Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Set Google Test include and lib paths
set GTEST_INCLUDE=C:\Users\HydraPony\dev\AI-First TextEditor\external\googletest\googletest\include
set GTEST_LIB=C:\Users\HydraPony\dev\AI-First TextEditor\build\lib\Debug

:: Compile the test with Google Test using C++17 and necessary libraries
cl /EHsc /std:c++17 /I"%GTEST_INCLUDE%" /I"%GTEST_INCLUDE%\.." minimal_gtest.cpp ^
    /link /LIBPATH:"%GTEST_LIB%" gtest.lib gtest_main.lib ^
    /NODEFAULTLIB:libcpmt ^
    /DEFAULTLIB:msvcrt.lib ^
    /DEFAULTLIB:msvcrtd.lib ^
    /DEFAULTLIB:ucrt.lib ^
    /DEFAULTLIB:vcruntime.lib ^
    /DEFAULTLIB:vcruntimed.lib ^
    /DEFAULTLIB:ucrtd.lib
if %ERRORLEVEL% neq 0 (
    echo Compilation failed.
    exit /b %ERRORLEVEL%
)

:: Run the test
minimal_gtest.exe

endlocal
