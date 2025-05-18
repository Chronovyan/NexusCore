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
if exist build (
  echo Cleaning build directory...
  rmdir /s /q build
)

REM Create build directory
mkdir build
cd build

REM Configure with CMake using Clang
echo Configuring with CMake using Clang...
cmake -G Ninja ^
      -DCMAKE_C_COMPILER=clang-cl ^
      -DCMAKE_CXX_COMPILER=clang-cl ^
      -DCMAKE_LINKER=lld-link ^
      -DCMAKE_BUILD_TYPE=Debug ^
      ..

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed.
  exit /b %ERRORLEVEL%
)

REM Build the project
echo Building project...
cmake --build .

if %ERRORLEVEL% neq 0 (
  echo Build failed.
  exit /b %ERRORLEVEL%
)

REM Run the tests
echo Running AIAgentOrchestratorTest...
if exist tests\AIAgentOrchestratorTest.exe (
  tests\AIAgentOrchestratorTest.exe
) else (
  echo Test executable not found. Check build results.
)

cd ..

echo Done. 