@echo off
setlocal enabledelayedexpansion

echo TextEditor Test Build Script
echo ===========================

:: Create build directory for tests if it doesn't exist
if not exist bin mkdir bin

:: Check if we should use MSVC or GCC
where cl.exe >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC compiler...
    
    :: Setup MSVC environment if needed (uncommment if not running from Developer Command Prompt)
    :: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    
    :: Build the BasicEditorTests.cpp with MSVC
    echo Building BasicEditorTests...
    cl.exe /std:c++17 /EHsc /W4 /Fe:bin\BasicEditorTests.exe BasicEditorTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /I..\src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: MSVC build failed for BasicEditorTests.
        exit /B 1
    )
    
    :: Build the AdvancedEditorTests.cpp with MSVC
    echo Building AdvancedEditorTests...
    cl.exe /std:c++17 /EHsc /W4 /Fe:bin\AdvancedEditorTests.exe AdvancedEditorTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /I..\src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: MSVC build failed for AdvancedEditorTests.
        exit /B 1
    )
) else (
    echo Using GCC compiler...
    
    :: Build the BasicEditorTests.cpp with GCC
    echo Building BasicEditorTests...
    g++ -std=c++17 -Wall -Wextra -o bin/BasicEditorTests.exe BasicEditorTests.cpp ../src/Editor.cpp ../src/TextBuffer.cpp -I../src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: GCC build failed for BasicEditorTests.
        exit /B 1
    )
    
    :: Build the AdvancedEditorTests.cpp with GCC
    echo Building AdvancedEditorTests...
    g++ -std=c++17 -Wall -Wextra -o bin/AdvancedEditorTests.exe AdvancedEditorTests.cpp ../src/Editor.cpp ../src/TextBuffer.cpp -I../src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: GCC build failed for AdvancedEditorTests.
        exit /B 1
    )
)

:: Clean up temporary files from MSVC if they exist
if exist *.obj del *.obj

:: Run the basic tests
echo.
echo Running basic tests...
echo ===========================
bin\BasicEditorTests.exe

:: Check test execution status
if %ERRORLEVEL% NEQ 0 (
    echo Basic tests failed with exit code %ERRORLEVEL%
    exit /B %ERRORLEVEL%
)

:: Run the advanced tests
echo.
echo Running advanced tests...
echo ===========================
bin\AdvancedEditorTests.exe

:: Check test execution status
if %ERRORLEVEL% NEQ 0 (
    echo Advanced tests failed with exit code %ERRORLEVEL%
    exit /B %ERRORLEVEL%
)

echo.
echo All tests completed successfully.

endlocal 