@echo off
setlocal

REM First, set up the Visual Studio environment using the main build script
cd ..
call build.bat setup_only

REM Change back to the tests directory
cd tests

REM Create bin directory if it doesn't exist
if not exist bin mkdir bin

REM Compile the manual undo/redo test
echo Compiling ManualUndoRedoTest.cpp...
cl.exe /EHsc /std:c++17 /W4 /I..\src ManualUndoRedoTest.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp /Febin\UndoRedoTest.exe

if errorlevel 1 (
    echo ERROR: Failed to compile ManualUndoRedoTest.cpp
    exit /b 1
)

echo Running undo/redo test...
bin\UndoRedoTest.exe

endlocal 