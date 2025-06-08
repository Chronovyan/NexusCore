@echo off
echo AI-First TextEditor Workspace Cleanup
echo =====================================
echo.

:: Check if PowerShell is available
where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo PowerShell is not available. Please install PowerShell to run this script.
    exit /b 1
)

echo This script will help clean up your workspace and identify potential issues.
echo.
echo Available options:
echo 1. Clean up build artifacts
echo 2. Identify dead/unused code
echo 3. Perform all cleanup tasks
echo 4. Exit
echo.

:menu
set /p choice="Enter your choice (1-4): "

if "%choice%"=="1" (
    call :cleanup_artifacts
    goto menu
) else if "%choice%"=="2" (
    call :identify_dead_code
    goto menu
) else if "%choice%"=="3" (
    call :cleanup_artifacts
    call :identify_dead_code
    goto menu
) else if "%choice%"=="4" (
    echo Exiting...
    exit /b 0
) else (
    echo Invalid choice. Please try again.
    goto menu
)

exit /b 0

:cleanup_artifacts
echo.
echo Cleaning up build artifacts...
echo.

echo Removing temporary build files...
for %%d in (build build_fast build_clean build_test build_simple build_msvc build_new build_di build2 build_focused) do (
    if exist %%d\CMakeFiles rd /s /q %%d\CMakeFiles
    if exist %%d\Debug rd /s /q %%d\Debug
    if exist %%d\Testing rd /s /q %%d\Testing
    if exist %%d\Release rd /s /q %%d\Release
    if exist %%d\x64 rd /s /q %%d\x64
)

echo Removing build artifacts outside build directories...
del /s /q *.obj *.o *.pdb *.ilk *.exp *.idb *.tlog *.log *.lastbuildstate 2>nul

echo Removing backup files...
del /s /q *.bak *.tmp *.temp *.old *.orig *.copy *~ 2>nul

echo Build artifacts cleanup complete!
echo.
exit /b 0

:identify_dead_code
echo.
echo Identifying potential dead code and technical debt...
echo.

:: Create a simple report of commented-out code blocks, TODOs, and deprecated code
echo # Dead Code and Technical Debt Report > dead_code_report.md
echo. >> dead_code_report.md
echo ## Summary >> dead_code_report.md
echo. >> dead_code_report.md

:: Find large commented code blocks (very simplified, might miss some)
echo ### Commented Code Blocks >> dead_code_report.md
echo. >> dead_code_report.md
powershell -Command "Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp | Select-String -Pattern '^\s*//\s*\w+' -Context 0,4 | ForEach-Object { $file = $_.Path; $line = $_.LineNumber; $match = $_.Line; if ($_.Context.PostContext.Count -ge 4) { Add-Content -Path dead_code_report.md -Value \"- **$file** (starting at line $line): ``$match``\" } }"

:: Find deprecated code
echo. >> dead_code_report.md
echo ### Deprecated Code >> dead_code_report.md
echo. >> dead_code_report.md
powershell -Command "Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp | Select-String -Pattern '@deprecated|DEPRECATED|DO_NOT_USE|obsolete' | ForEach-Object { Add-Content -Path dead_code_report.md -Value \"- **$($_.Path)** (line $($_.LineNumber)): ``$($_.Line.Trim())``\" }"

:: Find TODO/FIXME comments
echo. >> dead_code_report.md
echo ### TODO/FIXME Items >> dead_code_report.md
echo. >> dead_code_report.md
powershell -Command "Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp | Select-String -Pattern 'TODO|FIXME|XXX' | ForEach-Object { Add-Content -Path dead_code_report.md -Value \"- **$($_.Path)** (line $($_.LineNumber)): ``$($_.Line.Trim())``\" }"

:: Find backup/duplicate files
echo. >> dead_code_report.md
echo ### Backup/Duplicate Files >> dead_code_report.md
echo. >> dead_code_report.md
powershell -Command "Get-ChildItem -Recurse -Include *.bak,*.old,*.backup,*.tmp,*.temp,*.copy,*~ | ForEach-Object { Add-Content -Path dead_code_report.md -Value \"- **$($_.FullName)** ($([math]::Round($_.Length / 1KB, 2)) KB) - Last Modified: $($_.LastWriteTime)\" }"

:: Find temp/fixed files
echo. >> dead_code_report.md
echo ### Temp/Fixed Files >> dead_code_report.md
echo. >> dead_code_report.md
powershell -Command "Get-ChildItem -Recurse | Where-Object { $_.Name -match '\.fixed\.|\.temp\.|temp\.|fixed\.' } | ForEach-Object { Add-Content -Path dead_code_report.md -Value \"- **$($_.FullName)** ($([math]::Round($_.Length / 1KB, 2)) KB) - Last Modified: $($_.LastWriteTime)\" }"

echo Dead code identification complete!
echo Results saved to dead_code_report.md
echo.
exit /b 0 