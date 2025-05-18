@echo off
setlocal

REM Find Visual Studio installation (adjust version as needed)
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set VSINSTALL=%%i
)

if "%VSINSTALL%"=="" (
  echo Error: Visual Studio installation not found.
  exit /b 1
)

REM Setup Visual Studio environment
echo Setting up Visual Studio environment...
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"

REM Clean build directory if it exists
if exist build_msvc (
  echo Cleaning build directory...
  rmdir /s /q build_msvc
)

REM Create build directory
mkdir build_msvc
cd build_msvc

REM Configure with CMake using MSVC
echo Configuring with CMake using MSVC...
cmake -G "Visual Studio 17 2022" -A x64 ..

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed.
  exit /b %ERRORLEVEL%
)

REM Build the project
echo Building project...
cmake --build . --config Debug

if %ERRORLEVEL% neq 0 (
  echo Build failed.
  exit /b %ERRORLEVEL%
)

REM Run the tests
echo Running AIAgentOrchestratorTest...
if exist tests\Debug\AIAgentOrchestratorTest.exe (
  tests\Debug\AIAgentOrchestratorTest.exe
) else (
  echo Test executable not found. Check build results.
)

cd ..

echo Done. 