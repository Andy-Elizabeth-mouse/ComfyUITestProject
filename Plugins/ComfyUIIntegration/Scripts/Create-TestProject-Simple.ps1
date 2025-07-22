# ComfyUI Integration Plugin Test Project Creation Script - Simplified Version
# Customized for your environment

param(
    [string]$ProjectName = "ComfyUITestProject",
    [string]$ProjectPath = "C:\UnrealProjects", 
    [string]$PluginPath = "d:\forcopilot",
    [string]$UEPath = "E:\ue\UE_5.5"
)

Write-Host "=== ComfyUI Integration Plugin Test Project Creation ===" -ForegroundColor Green
Write-Host "Project Name: $ProjectName" -ForegroundColor Cyan
Write-Host "Project Path: $ProjectPath" -ForegroundColor Cyan
Write-Host "Plugin Path: $PluginPath" -ForegroundColor Cyan
Write-Host "UE5 Path: $UEPath" -ForegroundColor Cyan
Write-Host ""

# Verify paths
if (-not (Test-Path $PluginPath)) {
    Write-Error "Plugin path does not exist: $PluginPath"
    Read-Host "Press any key to exit"
    exit 1
}

if (-not (Test-Path $UEPath)) {
    Write-Error "UE5 path does not exist: $UEPath"
    Read-Host "Press any key to exit"
    exit 1
}

# Create project directory
$FullProjectPath = Join-Path $ProjectPath $ProjectName
Write-Host "Creating project at: $FullProjectPath" -ForegroundColor Yellow

if (Test-Path $FullProjectPath) {
    Write-Host "Project directory already exists!" -ForegroundColor Yellow
    $response = Read-Host "Delete and recreate? (y/n)"
    if ($response -eq 'y' -or $response -eq 'Y') {
        Remove-Item $FullProjectPath -Recurse -Force
        Write-Host "Deleted existing project directory" -ForegroundColor Green
    } else {
        Write-Host "Operation cancelled" -ForegroundColor Red
        Read-Host "Press any key to exit"
        exit 1
    }
}

# Create directory structure
Write-Host "Creating directory structure..." -ForegroundColor Yellow
$Directories = @(
    "",
    "Content",
    "Config", 
    "Plugins",
    "Source",
    "Source\$ProjectName"
)

foreach ($dir in $Directories) {
    $fullDir = if ($dir -eq "") { $FullProjectPath } else { Join-Path $FullProjectPath $dir }
    New-Item -ItemType Directory -Path $fullDir -Force | Out-Null
}

Write-Host "Directory structure created" -ForegroundColor Green

# Create .uproject file
Write-Host "Creating .uproject file..." -ForegroundColor Yellow
$UProjectContent = @"
{
    "FileVersion": 3,
    "EngineAssociation": "5.5",
    "Category": "",
    "Description": "ComfyUI Integration Test Project",
    "Modules": [
        {
            "Name": "$ProjectName",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "ComfyUIIntegration",
            "Enabled": true
        }
    ]
}
"@

$UProjectPath = Join-Path $FullProjectPath "$ProjectName.uproject"
$UProjectContent | Out-File -FilePath $UProjectPath -Encoding UTF8 -NoNewline
Write-Host "Created: $ProjectName.uproject" -ForegroundColor Green

# Create DefaultEngine.ini
Write-Host "Creating DefaultEngine.ini..." -ForegroundColor Yellow
$DefaultEngineContent = @"
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Templates/OpenWorld

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12

[/Script/HardwareTargeting.HardwareTargetingSettings]
TargetedHardwareClass=Desktop
AppliedTargetedHardwareClass=Desktop
DefaultGraphicsPerformance=Maximum
AppliedDefaultGraphicsPerformance=Maximum

[/Script/Engine.RendererSettings]
r.GenerateMeshDistanceFields=True
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
r.Shadow.Virtual.Enable=1
r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True

[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
LogJson=Log
"@

$ConfigPath = Join-Path $FullProjectPath "Config\DefaultEngine.ini"
$DefaultEngineContent | Out-File -FilePath $ConfigPath -Encoding UTF8 -NoNewline
Write-Host "Created: DefaultEngine.ini" -ForegroundColor Green

# Create C++ module files
Write-Host "Creating C++ module files..." -ForegroundColor Yellow

# .Build.cs file
$BuildContent = @"
using UnrealBuildTool;

public class $ProjectName : ModuleRules
{
    public $ProjectName(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore" 
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
"@

$BuildPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.Build.cs"
$BuildContent | Out-File -FilePath $BuildPath -Encoding UTF8 -NoNewline

# Module .cpp file
$ModuleCppContent = @"
#include "$ProjectName.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, $ProjectName, "$ProjectName" );
"@

$ModuleCppPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.cpp"
$ModuleCppContent | Out-File -FilePath $ModuleCppPath -Encoding UTF8 -NoNewline

# Module .h file
$ModuleHeaderContent = @"
#pragma once

#include "CoreMinimal.h"
"@

$ModuleHeaderPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.h"
$ModuleHeaderContent | Out-File -FilePath $ModuleHeaderPath -Encoding UTF8 -NoNewline

# Target.cs files
$TargetContent = @"
using UnrealBuildTool;
using System.Collections.Generic;

public class ${ProjectName}Target : TargetRules
{
    public ${ProjectName}Target( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange( new string[] { "$ProjectName" } );
    }
}
"@

$TargetPath = Join-Path $FullProjectPath "Source\$ProjectName.Target.cs"
$TargetContent | Out-File -FilePath $TargetPath -Encoding UTF8 -NoNewline

$EditorTargetContent = @"
using UnrealBuildTool;
using System.Collections.Generic;

public class ${ProjectName}EditorTarget : TargetRules
{
    public ${ProjectName}EditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange( new string[] { "$ProjectName" } );
    }
}
"@

$EditorTargetPath = Join-Path $FullProjectPath "Source\${ProjectName}Editor.Target.cs"
$EditorTargetContent | Out-File -FilePath $EditorTargetPath -Encoding UTF8 -NoNewline

Write-Host "Created C++ module files" -ForegroundColor Green

# Copy plugin files
Write-Host "Copying plugin files..." -ForegroundColor Yellow
$PluginDestPath = Join-Path $FullProjectPath "Plugins\ComfyUIIntegration"

# Create plugin directory first
New-Item -ItemType Directory -Path $PluginDestPath -Force | Out-Null

# Copy files with proper error handling
try {
    Copy-Item -Path "$PluginPath\*" -Destination $PluginDestPath -Recurse -Force -ErrorAction Stop
    Write-Host "Plugin copied to: $PluginDestPath" -ForegroundColor Green
} catch {
    Write-Warning "Some files couldn't be copied: $($_.Exception.Message)"
    # Try alternative method
    robocopy "$PluginPath" "$PluginDestPath" /E /XD ".git" ".vscode" /R:3 /W:1 /NFL /NDL /NJH /NJS
    if ($LASTEXITCODE -le 1) {
        Write-Host "Plugin copied successfully using robocopy" -ForegroundColor Green
    } else {
        Write-Warning "Plugin copy may have issues"
    }
}

# Generate project files
Write-Host "Generating Visual Studio project files..." -ForegroundColor Yellow

# Try multiple possible paths for UnrealBuildTool
$PossibleUBTPaths = @(
    "$UEPath\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe",
    "$UEPath\Engine\Binaries\DotNET\UnrealBuildTool.exe",
    "$UEPath\Engine\Binaries\Win64\UnrealBuildTool.exe",
    "$UEPath\Engine\Build\BatchFiles\Build.bat"
)

$UnrealBuildTool = $null
foreach ($path in $PossibleUBTPaths) {
    if (Test-Path $path) {
        $UnrealBuildTool = $path
        break
    }
}

if ($UnrealBuildTool) {
    Write-Host "Using UnrealBuildTool at: $UnrealBuildTool" -ForegroundColor Cyan
    
    $GenerateArgs = @(
        "-projectfiles"
        "-project=`"$UProjectPath`""
        "-game"
        "-rocket"
        "-progress"
    )
    
    & $UnrealBuildTool $GenerateArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Visual Studio project files generated successfully!" -ForegroundColor Green
    } else {
        Write-Warning "Project file generation completed with warnings. Exit code: $LASTEXITCODE"
    }
} else {
    Write-Warning "UnrealBuildTool not found in any of the expected locations:"
    foreach ($path in $PossibleUBTPaths) {
        Write-Host "  - $path" -ForegroundColor Gray
    }
    Write-Host ""
    Write-Host "MANUAL STEPS:" -ForegroundColor Yellow
    Write-Host "1. Right-click on $ProjectName.uproject" -ForegroundColor White
    Write-Host "2. Select 'Generate Visual Studio project files'" -ForegroundColor White
    Write-Host "3. Then continue with normal build process" -ForegroundColor White
}

# Summary
Write-Host ""
Write-Host "=== PROJECT CREATION COMPLETE ===" -ForegroundColor Green
Write-Host "Project Location: $FullProjectPath" -ForegroundColor Cyan
Write-Host "Project File: $UProjectPath" -ForegroundColor Cyan
Write-Host "Plugin Location: $PluginDestPath" -ForegroundColor Cyan
Write-Host ""
Write-Host "NEXT STEPS:" -ForegroundColor Yellow
Write-Host "1. Open $ProjectName.sln in Visual Studio" -ForegroundColor White
Write-Host "2. Set Configuration to 'Development Editor' and Platform to 'Win64'" -ForegroundColor White
Write-Host "3. Build the solution (Ctrl+Shift+B)" -ForegroundColor White
Write-Host "4. Press F5 to start debugging" -ForegroundColor White
Write-Host "5. In UE5 Editor: Edit -> Plugins -> Enable 'ComfyUI Integration'" -ForegroundColor White
Write-Host ""

# Optional: Open Visual Studio
$response = Read-Host "Open Visual Studio now? (y/n)"
if ($response -eq 'y' -or $response -eq 'Y') {
    $SolutionPath = Join-Path $FullProjectPath "$ProjectName.sln"
    if (Test-Path $SolutionPath) {
        Write-Host "Opening Visual Studio..." -ForegroundColor Green
        Start-Process $SolutionPath
    } else {
        Write-Warning "Solution file not found. Please generate project files first."
    }
}

Write-Host "Script completed successfully!" -ForegroundColor Green
Read-Host "Press any key to exit"
