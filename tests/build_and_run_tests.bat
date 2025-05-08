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
    
    :: Build the FutureFeatureTests.cpp with MSVC
    echo Building FutureFeatureTests...
    cl.exe /std:c++17 /EHsc /W4 /Fe:bin\FutureFeatureTests.exe FutureFeatureTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /I..\src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: MSVC build failed for FutureFeatureTests.
        exit /B 1
    )
    
    :: Build the UndoRedoTest.cpp with MSVC
    echo Building UndoRedoTest...
    cl.exe /std:c++17 /EHsc /W4 /Fe:bin\UndoRedoTest.exe UndoRedoTest.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /I..\src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: MSVC build failed for UndoRedoTest.
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
    
    :: Build the FutureFeatureTests.cpp with GCC
    echo Building FutureFeatureTests...
    g++ -std=c++17 -Wall -Wextra -o bin/FutureFeatureTests.exe FutureFeatureTests.cpp ../src/Editor.cpp ../src/TextBuffer.cpp -I../src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: GCC build failed for FutureFeatureTests.
        exit /B 1
    )
    
    :: Build the UndoRedoTest.cpp with GCC
    echo Building UndoRedoTest...
    g++ -std=c++17 -Wall -Wextra -o bin/UndoRedoTest.exe UndoRedoTest.cpp ../src/Editor.cpp ../src/TextBuffer.cpp -I../src
    
    :: Check if compilation was successful
    if %ERRORLEVEL% NEQ 0 (
        echo Error: GCC build failed for UndoRedoTest.
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

:: Run the future feature tests
echo.
echo Running future feature tests...
echo ===========================
bin\FutureFeatureTests.exe

:: Check test execution status
if %ERRORLEVEL% NEQ 0 (
    echo Future feature tests failed with exit code %ERRORLEVEL%
    exit /B %ERRORLEVEL%
)

:: Run the undo/redo tests
echo.
echo Running undo/redo tests...
echo ===========================
bin\UndoRedoTest.exe

:: Check test execution status
if %ERRORLEVEL% NEQ 0 (
    echo Undo/redo tests failed with exit code %ERRORLEVEL%
    exit /B %ERRORLEVEL%
)

echo.
echo All tests completed successfully.

endlocal 