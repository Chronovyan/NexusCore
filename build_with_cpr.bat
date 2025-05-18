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
      -DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=POST_BUILD ^
      ..

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed.
  exit /b %ERRORLEVEL%
)

REM Build only the main application targets, skipping test discovery
echo Building main project targets...
cmake --build . --config Debug --target AITextEditor TextEditor OpenAIClientTest

if %ERRORLEVEL% neq 0 (
  echo Build failed.
  exit /b %ERRORLEVEL%
)

echo Build completed successfully!
cd ..

echo Done. 