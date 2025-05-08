@echo off
setlocal

echo Building Syntax Highlighting Test...

if "%1"=="gcc" (
    echo Using GCC compiler...
    g++ -std=c++17 -Wall -Wextra -o syntax_test tests/SyntaxHighlightingTest.cpp src/Editor.cpp src/TextBuffer.cpp src/SyntaxHighlighter.cpp
) else (
    echo Using MSVC compiler...
    
    rem Check for Visual Studio installation
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
        echo Found Visual Studio 2019 Community
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" (
        echo Found Visual Studio 2019 Enterprise
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" (
        echo Found Visual Studio 2019 Professional
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
        echo Found Visual Studio 2017 Community
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        echo Found Visual Studio 2022 Community
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    ) else (
        echo Visual Studio installation not found, trying to use cl.exe directly
    )
    
    echo Compiling with MSVC...
    cl.exe /EHsc /std:c++17 /Fe:syntax_test.exe tests/SyntaxHighlightingTest.cpp src/Editor.cpp src/TextBuffer.cpp src/SyntaxHighlighter.cpp /I.
)

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Running Syntax Highlighting Test...
syntax_test.exe

echo Done.
endlocal 