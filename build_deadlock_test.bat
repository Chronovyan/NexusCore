@echo off
setlocal

echo Building Deadlock Verification Test

REM Setup Visual Studio environment if needed
if not defined VSCMD_VER (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if errorlevel 1 (
        echo Failed to setup Visual Studio environment.
        exit /b 1
    )
)

REM Create output directory if it doesn't exist
if not exist bin mkdir bin

REM Compile the deadlock test
echo Compiling DeadlockTest.cpp...
cl.exe /std:c++17 /EHsc /W4 /Zi /Fmbin\DeadlockTest.map ^
    /Febin\DeadlockTest.exe DeadlockTest.cpp ^
    src\Editor.cpp src\TextBuffer.cpp src\SyntaxHighlighter.cpp src\SyntaxHighlightingManager.cpp ^
    /I. /Fo"bin\\" /Fd"bin\\"

if errorlevel 1 (
    echo Compilation failed.
    exit /b 1
)

REM Clean up temporary files
if exist *.obj del *.obj

echo.
echo Compilation successful.
echo Running deadlock verification test...
echo.

REM Run the test
bin\DeadlockTest.exe

if errorlevel 1 (
    echo.
    echo Deadlock test failed.
    exit /b 1
) else (
    echo.
    echo Deadlock test passed.
)

endlocal 