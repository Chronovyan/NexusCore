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
    echo Please verify the VS_ROOT path in build.bat.
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

REM Check if we only want to set up the environment
if "%1"=="setup_only" goto :eof

REM Create output directories if they don't exist
if not exist .\bin mkdir .\bin
if not exist .\obj mkdir .\obj

REM Source files and output
set "MAIN_SRC=src\main.cpp"
set "BUFFER_SRC=src\TextBuffer.cpp"
set "EDITOR_SRC=src\Editor.cpp"
set "SYNTAX_SRC=src\SyntaxHighlighter.cpp"
set "SYNTAX_MGR_SRC=src\SyntaxHighlightingManager.cpp"
set "MAIN_OBJ=obj\main.obj"
set "BUFFER_OBJ=obj\TextBuffer.obj"
set "EDITOR_OBJ=obj\Editor.obj"
set "SYNTAX_OBJ=obj\SyntaxHighlighter.obj"
set "SYNTAX_MGR_OBJ=obj\SyntaxHighlightingManager.obj"
set "OUTPUT_EXE=bin\CustomTextEditor.exe"

REM Compiler options: /EHsc (exception handling), /std:c++17, /W4 (warnings), /Zi (debug info), /nologo
set "COMPILE_FLAGS=/EHsc /std:c++17 /W4 /Zi /nologo"
REM To treat warnings as errors, add /WX: set "COMPILE_FLAGS=/EHsc /std:c++17 /W4 /WX /Zi /nologo"

REM Compile main.cpp
echo Compiling %MAIN_SRC%...
cl.exe %COMPILE_FLAGS% /c %MAIN_SRC% /Fo%MAIN_OBJ%
if errorlevel 1 (
    echo ERROR: Compilation of %MAIN_SRC% failed with error code %ERRORLEVEL%
    goto :eof
)

REM Compile TextBuffer.cpp
echo Compiling %BUFFER_SRC%...
cl.exe %COMPILE_FLAGS% /c %BUFFER_SRC% /Fo%BUFFER_OBJ%
if errorlevel 1 (
    echo ERROR: Compilation of %BUFFER_SRC% failed with error code %ERRORLEVEL%
    goto :eof
)

REM Compile Editor.cpp
echo Compiling %EDITOR_SRC%...
cl.exe %COMPILE_FLAGS% /c %EDITOR_SRC% /Fo%EDITOR_OBJ%
if errorlevel 1 (
    echo ERROR: Compilation of %EDITOR_SRC% failed with error code %ERRORLEVEL%
    goto :eof
)

REM Compile SyntaxHighlighter.cpp
echo Compiling %SYNTAX_SRC%...
cl.exe %COMPILE_FLAGS% /c %SYNTAX_SRC% /Fo%SYNTAX_OBJ%
if errorlevel 1 (
    echo ERROR: Compilation of %SYNTAX_SRC% failed with error code %ERRORLEVEL%
    goto :eof
)

REM Compile SyntaxHighlightingManager.cpp
echo Compiling %SYNTAX_MGR_SRC%...
cl.exe %COMPILE_FLAGS% /c %SYNTAX_MGR_SRC% /Fo%SYNTAX_MGR_OBJ%
if errorlevel 1 (
    echo ERROR: Compilation of %SYNTAX_MGR_SRC% failed with error code %ERRORLEVEL%
    goto :eof
)

REM Link object files
echo Linking to create %OUTPUT_EXE%...
link.exe /NOLOGO /OUT:%OUTPUT_EXE% %MAIN_OBJ% %BUFFER_OBJ% %EDITOR_OBJ% %SYNTAX_OBJ% %SYNTAX_MGR_OBJ% /DEBUG
if errorlevel 1 (
    echo ERROR: Linking failed with error code %ERRORLEVEL%
    goto :eof
)

echo.
echo Build successful! Output: %OUTPUT_EXE%
echo To run the program, execute: %OUTPUT_EXE%
echo.

goto :eof

:eof
endlocal 