@echo off
setlocal

REM Check if the OPENAI_API_KEY environment variable is set
if "%OPENAI_API_KEY%"=="" (
    echo Error: OPENAI_API_KEY environment variable is not set.
    echo Please set your OpenAI API key as an environment variable:
    echo   setx OPENAI_API_KEY your_api_key_here
    echo Then restart your command prompt and try again.
    exit /b 1
)

REM Create a build directory for testing
if not exist build_test_cpr mkdir build_test_cpr
cd build_test_cpr

REM Configure with CMake
echo Configuring test build with CMake...
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

REM Build just the OpenAIClientTest executable
echo Building OpenAIClientTest...
cmake --build . --config Debug --target OpenAIClientTest

if %ERRORLEVEL% neq 0 (
  echo Build failed.
  exit /b %ERRORLEVEL%
)

REM Run the test with the API key set in the environment
echo Running OpenAIClientTest...
set OPENAI_API_KEY=your_api_key_here
Debug\OpenAIClientTest.exe

cd ..

echo Done. 