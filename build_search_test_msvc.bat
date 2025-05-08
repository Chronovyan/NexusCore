@echo off
setlocal

echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if %ERRORLEVEL% NEQ 0 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if %ERRORLEVEL% NEQ 0 (
    echo Error: Could not find Visual Studio environment setup script.
    echo Please modify this script to point to your Visual Studio installation.
    exit /b 1
)

echo Building Search Functionality Test with MSVC...

rem Compile source files individually (excluding main.cpp)
cl.exe /c /EHsc /std:c++17 /I. /Isrc src\Editor.cpp
cl.exe /c /EHsc /std:c++17 /I. /Isrc src\TextBuffer.cpp

rem Compile and link the test
cl.exe /EHsc /std:c++17 /I. /Isrc tests\SearchFunctionalityTest.cpp Editor.obj TextBuffer.obj /Fe:search_test.exe

if %ERRORLEVEL% NEQ 0 (
    echo Error during compilation!
    exit /b %ERRORLEVEL%
)

echo Build completed successfully.
echo Running Search Functionality Test...

search_test.exe

if %ERRORLEVEL% NEQ 0 (
    echo Test failed with exit code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo All tests passed successfully!
exit /b 0 