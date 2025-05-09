@echo off
setlocal

REM Setup Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

echo TextEditor Test Script (MSVC Only)
echo ===========================

REM Create build directory for tests if it doesn't exist
if not exist tests\bin mkdir tests\bin

cd tests

REM Build with MSVC
echo Building BasicEditorTests...
cl.exe /std:c++17 /EHsc /W4 /Fe:bin\BasicEditorTests.exe BasicEditorTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp ..\src\SyntaxHighlighter.cpp ..\src\SyntaxHighlightingManager.cpp /I..\src

echo Building AdvancedEditorTests...
cl.exe /std:c++17 /EHsc /W4 /Fe:bin\AdvancedEditorTests.exe AdvancedEditorTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp ..\src\SyntaxHighlighter.cpp ..\src\SyntaxHighlightingManager.cpp /I..\src

echo Building FutureFeatureTests...
cl.exe /std:c++17 /EHsc /W4 /Fe:bin\FutureFeatureTests.exe FutureFeatureTests.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp ..\src\SyntaxHighlighter.cpp ..\src\SyntaxHighlightingManager.cpp /I..\src

echo Building UndoRedoTest...
cl.exe /std:c++17 /EHsc /W4 /Fe:bin\UndoRedoTest.exe UndoRedoTest.cpp ..\src\Editor.cpp ..\src\TextBuffer.cpp ..\src\SyntaxHighlighter.cpp ..\src\SyntaxHighlightingManager.cpp /I..\src

REM Clean up temporary files
if exist *.obj del *.obj

REM Run the tests
echo.
echo Running basic tests...
echo ===========================
bin\BasicEditorTests.exe

echo.
echo Running advanced tests...
echo ===========================
bin\AdvancedEditorTests.exe

echo.
echo Running future feature tests...
echo ===========================
bin\FutureFeatureTests.exe

echo.
echo Running undo/redo tests...
echo ===========================
bin\UndoRedoTest.exe

echo.
echo All tests completed.

cd ..
endlocal 