@echo off
chcp 65001 > nul
echo ===================================
echo ComfyUI Integration Plugin Test Project Creation
echo ===================================
echo.

:: Check if PowerShell is available
powershell -Command "Write-Host 'PowerShell Available' -ForegroundColor Green"
if errorlevel 1 (
    echo Error: PowerShell is not available
    pause
    exit /b 1
)

:: Set execution policy (may require admin privileges)
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force"

:: Run PowerShell script
powershell -ExecutionPolicy Bypass -File "%~dp0Create-TestProject.ps1"

echo.
echo Script execution complete!
pause
