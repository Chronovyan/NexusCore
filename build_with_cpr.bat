@echo off
setlocal

echo Building AI-First TextEditor with CPR...

REM Create a build directory
if not exist build_main mkdir build_main
cd build_main

REM Configure with CMake
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug ^
      -DCPR_FORCE_USE_SYSTEM_CURL=OFF ^
      -DBUILD_SHARED_LIBS=ON ^
      -DCPR_BUILD_TESTS=OFF ^
      -DCPR_BUILD_TESTS_SSL=OFF ^
      -DCPR_ENABLE_CERTIFICATE_OPTIMIZATION=OFF ^
      ..

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

echo Build completed successfully!
cd ..

echo Done. 