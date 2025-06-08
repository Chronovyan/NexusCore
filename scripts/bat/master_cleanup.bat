@echo off
echo AI-First TextEditor Master Cleanup Suite
echo ======================================
echo.

echo This script will run all cleanup tools in sequence for a thorough workspace cleanup.
echo.

:: Check if PowerShell is available
where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo PowerShell is not available. Please install PowerShell to run this script.
    exit /b 1
)

set /p confirm="Would you like to proceed with the complete cleanup process? (y/n): "
if /i NOT "%confirm%"=="y" goto :end

:menu
echo.
echo Cleanup Options:
echo 1. Run Basic Artifact Cleanup (remove build artifacts)
echo 2. Run Advanced Trash Cleanup (remove temp files, backup dirs, etc.)
echo 3. Consolidate Build Directories (merge multiple build dirs)
echo 4. Run Documentation Cleanup (analyze and fix docs)
echo 5. Run Dead Code Identification (find unused code)
echo 6. Find Duplicate Files (scan for and remove duplicates)
echo 7. Run Performance Analysis (identify performance bottlenecks)
echo 8. Analyze Dependencies (find unused and problematic dependencies)
echo 9. Generate Summary Report (create comprehensive cleanup report)
echo 10. Run Complete Cleanup (all of the above)
echo 11. Exit
echo.

set /p choice="Enter your choice (1-11): "

if "%choice%"=="1" (
    call :run_basic_cleanup
    goto menu
) else if "%choice%"=="2" (
    call :run_advanced_cleanup
    goto menu
) else if "%choice%"=="3" (
    call :consolidate_builds
    goto menu
) else if "%choice%"=="4" (
    call :organize_docs
    goto menu
) else if "%choice%"=="5" (
    call :identify_dead_code
    goto menu
) else if "%choice%"=="6" (
    call :find_duplicates
    goto menu
) else if "%choice%"=="7" (
    call :analyze_performance
    goto menu
) else if "%choice%"=="8" (
    call :analyze_dependencies
    goto menu
) else if "%choice%"=="9" (
    call :generate_summary
    goto menu
) else if "%choice%"=="10" (
    call :run_complete_cleanup
    goto menu
) else if "%choice%"=="11" (
    goto end
) else (
    echo Invalid choice. Please try again.
    goto menu
)

goto :end

:run_basic_cleanup
echo.
echo --- Running Basic Artifact Cleanup ---
echo.
powershell -ExecutionPolicy Bypass -File cleanup_artifacts.ps1
exit /b 0

:run_advanced_cleanup
echo.
echo --- Running Advanced Trash Cleanup ---
echo.
powershell -ExecutionPolicy Bypass -File advanced_cleanup.ps1
exit /b 0

:consolidate_builds
echo.
echo --- Running Build Directory Consolidation ---
echo.
powershell -ExecutionPolicy Bypass -File consolidate_builds.ps1
exit /b 0

:organize_docs
echo.
echo --- Running Documentation Organization ---
echo.
powershell -ExecutionPolicy Bypass -File organize_docs.ps1
exit /b 0

:identify_dead_code
echo.
echo --- Running Dead Code Identification ---
echo.
powershell -ExecutionPolicy Bypass -File identify_dead_code.ps1
exit /b 0

:find_duplicates
echo.
echo --- Running Duplicate File Finder ---
echo.
powershell -ExecutionPolicy Bypass -File find_duplicates.ps1
exit /b 0

:analyze_performance
echo.
echo --- Running Performance Analysis ---
echo.
powershell -ExecutionPolicy Bypass -File analyze_performance.ps1
exit /b 0

:analyze_dependencies
echo.
echo --- Running Dependency Analysis ---
echo.
powershell -ExecutionPolicy Bypass -File analyze_dependencies.ps1
exit /b 0

:generate_summary
echo.
echo --- Generating Comprehensive Summary Report ---
echo.
powershell -ExecutionPolicy Bypass -File generate_summary.ps1
exit /b 0

:run_complete_cleanup
echo.
echo === Running Complete Cleanup Process ===
echo.

echo Step 1: Basic Artifact Cleanup
call :run_basic_cleanup
echo.

echo Step 2: Advanced Trash Cleanup
call :run_advanced_cleanup
echo.

echo Step 3: Build Directory Consolidation
call :consolidate_builds
echo.

echo Step 4: Documentation Organization
call :organize_docs
echo.

echo Step 5: Dead Code Identification
call :identify_dead_code
echo.

echo Step 6: Find Duplicate Files
call :find_duplicates
echo.

echo Step 7: Performance Analysis
call :analyze_performance
echo.

echo Step 8: Dependency Analysis
call :analyze_dependencies
echo.

echo Step 9: Generate Summary Report
call :generate_summary
echo.

echo.
echo Complete cleanup process finished!
echo Please review the generated reports for any remaining issues.
echo.

exit /b 0

:end
echo.
if /i NOT "%confirm%"=="y" echo Cleanup process canceled.
echo Exiting...
exit /b 0 