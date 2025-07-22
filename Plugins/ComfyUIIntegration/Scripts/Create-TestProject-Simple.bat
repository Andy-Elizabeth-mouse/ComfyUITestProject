@echo off
chcp 65001 > nul
title ComfyUI Integration Test Project Creator
echo ==========================================
echo ComfyUI Integration Test Project Creator
echo ==========================================
echo.
echo This script will create a UE5 test project with the ComfyUI Integration plugin
echo.

:: Check PowerShell
echo Checking PowerShell...
powershell -Command "Write-Host 'PowerShell is available' -ForegroundColor Green"
if errorlevel 1 (
    echo ERROR: PowerShell is not available or not working properly
    echo Please ensure PowerShell is installed and accessible
    pause
    exit /b 1
)

:: Set execution policy
echo Setting PowerShell execution policy...
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force" 2>nul

:: Run the simplified script
echo.
echo Running project creation script...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0Create-TestProject-Simple.ps1"

echo.
echo Batch script completed!
pause
