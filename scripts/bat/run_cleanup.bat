@echo off
echo AI-First TextEditor Workspace Cleanup Suite
echo ==========================================
echo.

echo This script will help you run all the cleanup and organization tools.
echo.

set /p confirm="Would you like to proceed with the full cleanup process? (y/n): "
if /i NOT "%confirm%"=="y" goto :end

echo.
echo --- Phase 1: Code & Artifact Cleanup ---
echo.

echo Running artifact cleanup script...
call powershell -ExecutionPolicy Bypass -File cleanup_artifacts.ps1

echo.
echo Running dead code identification script...
call powershell -ExecutionPolicy Bypass -File identify_dead_code.ps1

echo.
echo --- Phase 2: Documentation Organization ---
echo.

echo Running documentation organization script...
call powershell -ExecutionPolicy Bypass -File organize_docs.ps1

echo.
echo --- Phase 3: Generating Summary Report ---
echo.

echo Updating CHRONOLOG.MD with cleanup summary...
powershell -Command "(Get-Content CHRONOLOG.MD) -replace '\`date \+\"%Y-\%m-\%d\"\`', (Get-Date -Format 'yyyy-MM-dd') | Set-Content CHRONOLOG.MD"
echo Summary report has been added to CHRONOLOG.MD

echo.
echo Cleanup process completed!
echo Please review the generated reports:
echo - dead_code_report.md (if generated)
echo - documentation_report.md (if generated)
echo - CHRONOLOG.MD (updated with summary)
echo.

:end
echo.
if /i NOT "%confirm%"=="y" echo Cleanup process canceled.
pause 