@echo off
REM Batch script to run all automated tests
echo ====================================================
echo Automated Test Runner for AI-First TextEditor
echo ====================================================

set TESTS_DIR=%~dp0..\out\build\tests
if not exist "%TESTS_DIR%" (
    echo Error: Build directory not found at %TESTS_DIR%
    echo Please build the project before running tests
    exit /b 1
)

echo.
echo Running Automated Tests:
echo ====================================================

echo Running AutomatedUndoRedoTest...
"%TESTS_DIR%\AutomatedUndoRedoTest.exe"
if %ERRORLEVEL% neq 0 (
    echo AutomatedUndoRedoTest FAILED with code %ERRORLEVEL%
) else (
    echo AutomatedUndoRedoTest PASSED
)
echo.

echo Running AutomatedSearchTest...
"%TESTS_DIR%\AutomatedSearchTest.exe"
if %ERRORLEVEL% neq 0 (
    echo AutomatedSearchTest FAILED with code %ERRORLEVEL%
) else (
    echo AutomatedSearchTest PASSED
)
echo.

echo Running AutomatedSyntaxHighlightingTest...
"%TESTS_DIR%\AutomatedSyntaxHighlightingTest.exe"
if %ERRORLEVEL% neq 0 (
    echo AutomatedSyntaxHighlightingTest FAILED with code %ERRORLEVEL%
) else (
    echo AutomatedSyntaxHighlightingTest PASSED
)
echo.

echo Running AutomatedConcurrencyTest...
"%TESTS_DIR%\AutomatedConcurrencyTest.exe"
if %ERRORLEVEL% neq 0 (
    echo AutomatedConcurrencyTest FAILED with code %ERRORLEVEL%
) else (
    echo AutomatedConcurrencyTest PASSED
)
echo.

echo ====================================================
echo Automated Test Run Complete
echo ====================================================

REM Return success only if all tests passed
if %ERRORLEVEL% neq 0 (
    exit /b %ERRORLEVEL%
) else (
    echo All automated tests passed!
    exit /b 0
) 