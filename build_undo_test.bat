@echo off
setlocal

REM --- User Configuration: Adjust this path to your Visual Studio installation ---
REM Common paths:
REM VS 2022 Community: "C:\Program Files\Microsoft Visual Studio\2022\Community"
REM VS 2019 Community: "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
REM VS Build Tools: "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools" (or similar for 2022)
set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
REM --- End User Configuration ---

set "VCVARS_SCRIPT=%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"

if NOT EXIST "%VCVARS_SCRIPT%" (
    echo ERROR: Could not find "%VCVARS_SCRIPT%"
    echo Please verify the VS_ROOT path in build_undo_test.bat.
    echo Current VS_ROOT: "%VS_ROOT%"
    goto :eof
)

REM Call vcvars64.bat to set up the environment
echo Setting up Visual Studio x64 environment...
call "%VCVARS_SCRIPT%"
if errorlevel 1 (
    echo ERROR: Failed to setup Visual Studio environment.
    goto :eof
)
echo Visual Studio environment has been set up.
echo.

REM Create output directories if they don't exist
if not exist .\bin mkdir .\bin

REM Source files and output
set "TEST_SRC=ManualUndoRedoTest.cpp"
set "EDITOR_SRC=..\src\Editor.cpp"
set "BUFFER_SRC=..\src\TextBuffer.cpp"
set "OUTPUT_EXE=bin\UndoRedoTest.exe"

REM Compiler options: /EHsc (exception handling), /std:c++17, /W4 (warnings), /Zi (debug info), /nologo
set "COMPILE_FLAGS=/EHsc /std:c++17 /W4 /Zi /nologo /I..\src"

REM Compile and link in one step
echo Compiling and linking %TEST_SRC%...
cl.exe %COMPILE_FLAGS% %TEST_SRC% %EDITOR_SRC% %BUFFER_SRC% /Fe%OUTPUT_EXE%
if errorlevel 1 (
    echo ERROR: Compilation failed with error code %ERRORLEVEL%
    goto :eof
)

echo.
echo Build successful! Output: %OUTPUT_EXE%
echo Running undo/redo test...
echo.

REM Run the test
%OUTPUT_EXE%

goto :eof

:eof
endlocal 