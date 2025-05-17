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
if not exist build_test mkdir build_test
cd build_test

REM Configure with CMake
echo Configuring test build with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..

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

REM Run the test
echo Running OpenAIClientTest...
Debug\OpenAIClientTest.exe

cd ..

echo Done. 