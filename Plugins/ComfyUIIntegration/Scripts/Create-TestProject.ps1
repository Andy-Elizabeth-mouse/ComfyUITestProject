# ComfyUI Integration Plugin Test Project Creation Script

param(
    [string]$ProjectName = "ComfyUITestProject",
    [string]$ProjectPath = "C:\UnrealProjects",
    [string]$PluginPath = "d:\forcopilot",
    [string]$UEPath = "E:\ue\UE_5.5"
)

Write-Host "=== ComfyUI Integration Plugin Test Project Creation Script ===" -ForegroundColor Green

# Auto-detect UE5 path if default doesn't exist
if (-not (Test-Path $UEPath)) {
    Write-Host "Default UE5 path not found, attempting to auto-detect..." -ForegroundColor Yellow
    
    # Common UE5 installation paths
    $CommonPaths = @(
        "C:\Program Files\Epic Games\UE_5.0",
        "C:\Program Files\Epic Games\UE_5.1", 
        "C:\Program Files\Epic Games\UE_5.2",
        "C:\Program Files\Epic Games\UE_5.3",
        "C:\Program Files\Epic Games\UE_5.4",
        "C:\Program Files\Epic Games\UE_5.5",
        "E:\ue\UE_5.0",
        "E:\ue\UE_5.1",
        "E:\ue\UE_5.2", 
        "E:\ue\UE_5.3",
        "E:\ue\UE_5.4",
        "E:\ue\UE_5.5",
        "D:\UnrealEngine\UE_5.0",
        "D:\UnrealEngine\UE_5.1",
        "D:\UnrealEngine\UE_5.2",
        "D:\UnrealEngine\UE_5.3",
        "D:\UnrealEngine\UE_5.4",
        "D:\UnrealEngine\UE_5.5"
    )
    
    foreach ($path in $CommonPaths) {
        if (Test-Path $path) {
            $UEPath = $path
            Write-Host "Found UE5 at: $UEPath" -ForegroundColor Green
            break
        }
    }
    
    if (-not (Test-Path $UEPath)) {
        Write-Host "Could not auto-detect UE5 installation." -ForegroundColor Red
        $UEPath = Read-Host "Please enter the full path to your UE5 installation"
    }
}

# Check parameters
if (-not (Test-Path $PluginPath)) {
    Write-Error "Plugin path does not exist: $PluginPath"
    exit 1
}

if (-not (Test-Path $UEPath)) {
    Write-Error "UE5 path does not exist: $UEPath"
    exit 1
}

# Create project directory
$FullProjectPath = Join-Path $ProjectPath $ProjectName
Write-Host "Creating project directory: $FullProjectPath" -ForegroundColor Yellow

if (Test-Path $FullProjectPath) {
    $response = Read-Host "Project directory already exists. Do you want to delete and recreate it? (y/n)"
    if ($response -eq 'y' -or $response -eq 'Y') {
        Remove-Item $FullProjectPath -Recurse -Force
    } else {
        Write-Host "Operation cancelled" -ForegroundColor Red
        exit 1
    }
}

New-Item -ItemType Directory -Path $FullProjectPath -Force | Out-Null

# Create basic project structure
Write-Host "Creating project structure..." -ForegroundColor Yellow

$ProjectStructure = @(
    "Content",
    "Config",
    "Plugins",
    "Source",
    "Source/$ProjectName"
)

foreach ($dir in $ProjectStructure) {
    $dirPath = Join-Path $FullProjectPath $dir
    New-Item -ItemType Directory -Path $dirPath -Force | Out-Null
}

# Create .uproject file
$UProjectContent = @"
{
    "FileVersion": 3,
    "EngineAssociation": "5.5",
    "Category": "",
    "Description": "",
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
$UProjectContent | Out-File -FilePath $UProjectPath -Encoding UTF8
Write-Host "Created .uproject file: $UProjectPath" -ForegroundColor Green

# Create DefaultEngine.ini
$DefaultEngineContent = @"
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Templates/OpenWorld

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
-D3D12TargetedShaderFormats=PCD3D_SM5
+D3D12TargetedShaderFormats=PCD3D_SM6
+D3D12TargetedShaderFormats=PCD3D_SM5
Compiler=Default
AudioSampleRate=48000
AudioCallbackBufferFrameSize=1024
AudioNumBuffersToEnqueue=1
AudioMaxChannels=0
AudioNumSourceWorkers=4
SpatializationPlugin=
SourceDataOverridePlugin=
ReverbPlugin=
OcclusionPlugin=
CompressionOverrides=(bOverrideCompressionTimes=False,DurationThreshold=5.000000,MaxNumRandomBranches=0,SoundCueQualityIndex=0)
CacheSizeKB=65536
MaxChunkSizeOverrideKB=0
bResampleForDevice=False
MaxSampleRate=48000.000000
HighSampleRate=32000.000000
MedSampleRate=24000.000000
LowSampleRate=12000.000000
MinSampleRate=8000.000000
CompressionQualityModifier=1.000000
AutoStreamingThreshold=0.000000
SoundCueCookQualityIndex=-1

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

[/Script/UnrealEd.UnrealEdEngine]
RemoveDefaultObjects=False

[/Script/Engine.Engine]
+ActiveGameNameRedirects=(OldGameName="TP_Blank",NewGameName="/Script/$ProjectName")
+ActiveGameNameRedirects=(OldGameName="/Script/TP_Blank",NewGameName="/Script/$ProjectName")
+ActiveClassRedirects=(OldClassName="TP_BlankGameModeBase",NewClassName="${ProjectName}GameModeBase")

[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]
bEnablePlugin=True
bAllowNetworkConnection=True
SecurityToken=
bIncludeInShipping=False
bAllowExternalStartInShipping=False
bCompileAFSProject=False
bUseCompression=False
bLogFiles=False
bReportStats=False
ConnectionType=USBOnly
bUseManualIPAddress=False
ManualIPAddress=

[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
LogJson=Log
"@

$ConfigPath = Join-Path $FullProjectPath "Config\DefaultEngine.ini"
$DefaultEngineContent | Out-File -FilePath $ConfigPath -Encoding UTF8
Write-Host "Created DefaultEngine.ini configuration file" -ForegroundColor Green

# Create basic C++ module files
$ModuleBuildContent = @"
using UnrealBuildTool;

public class $ProjectName : ModuleRules
{
    public $ProjectName(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        PrivateDependencyModuleNames.AddRange(new string[] {  });
    }
}
"@

$ModuleBuildPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.Build.cs"
$ModuleBuildContent | Out-File -FilePath $ModuleBuildPath -Encoding UTF8

$ModuleCppContent = @"
#include "$ProjectName.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, $ProjectName, "$ProjectName" );
"@

$ModuleCppPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.cpp"
$ModuleCppContent | Out-File -FilePath $ModuleCppPath -Encoding UTF8

$ModuleHeaderContent = @"
#pragma once

#include "CoreMinimal.h"
"@

$ModuleHeaderPath = Join-Path $FullProjectPath "Source\$ProjectName\$ProjectName.h"
$ModuleHeaderContent | Out-File -FilePath $ModuleHeaderPath -Encoding UTF8

# 创建Target.cs文件
$TargetContent = @"
using UnrealBuildTool;
using System.Collections.Generic;

public class ${ProjectName}Target : TargetRules
{
    public ${ProjectName}Target( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange( new string[] { "$ProjectName" } );
    }
}
"@

$TargetPath = Join-Path $FullProjectPath "Source\$ProjectName.Target.cs"
$TargetContent | Out-File -FilePath $TargetPath -Encoding UTF8

$EditorTargetContent = @"
using UnrealBuildTool;
using System.Collections.Generic;

public class ${ProjectName}EditorTarget : TargetRules
{
    public ${ProjectName}EditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange( new string[] { "$ProjectName" } );
    }
}
"@

$EditorTargetPath = Join-Path $FullProjectPath "Source\${ProjectName}Editor.Target.cs"
$EditorTargetContent | Out-File -FilePath $EditorTargetPath -Encoding UTF8

Write-Host "Created C++ module files" -ForegroundColor Green

# Copy plugin files
Write-Host "Copying plugin files..." -ForegroundColor Yellow
$PluginDestPath = Join-Path $FullProjectPath "Plugins\ComfyUIIntegration"
Copy-Item -Path "$PluginPath\*" -Destination $PluginDestPath -Recurse -Force
Write-Host "Plugin files copied to: $PluginDestPath" -ForegroundColor Green

# Generate project files
Write-Host "Generating Visual Studio project files..." -ForegroundColor Yellow
$UnrealBuildTool = Join-Path $UEPath "Engine\Binaries\DotNET\UnrealBuildTool.exe"

if (Test-Path $UnrealBuildTool) {
    $GenerateArgs = @(
        "-projectfiles"
        "-project=`"$UProjectPath`""
        "-game"
        "-rocket"
        "-progress"
    )
    
    & $UnrealBuildTool $GenerateArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Project files generated successfully!" -ForegroundColor Green
    } else {
        Write-Warning "Project file generation may have issues, please check output"
    }
} else {
    Write-Warning "UnrealBuildTool not found, please generate project files manually"
}

# Output summary
Write-Host "`n=== Project Creation Complete ===" -ForegroundColor Green
Write-Host "Project Path: $FullProjectPath" -ForegroundColor Cyan
Write-Host "Project File: $UProjectPath" -ForegroundColor Cyan
Write-Host "Plugin Path: $PluginDestPath" -ForegroundColor Cyan

Write-Host "`nNext Steps:" -ForegroundColor Yellow
Write-Host "1. Open $ProjectName.sln file" -ForegroundColor White
Write-Host "2. Set build configuration to 'Development Editor' and 'Win64'" -ForegroundColor White
Write-Host "3. Press F5 to start debugging" -ForegroundColor White
Write-Host "4. Enable ComfyUI Integration plugin in UE5 editor" -ForegroundColor White

# Optional: Auto-open Visual Studio
$response = Read-Host "`nWould you like to automatically open Visual Studio? (y/n)"
if ($response -eq 'y' -or $response -eq 'Y') {
    $SolutionPath = Join-Path $FullProjectPath "$ProjectName.sln"
    if (Test-Path $SolutionPath) {
        Start-Process $SolutionPath
        Write-Host "Opening Visual Studio..." -ForegroundColor Green
    } else {
        Write-Warning "Solution file not found, please open manually"
    }
}

Write-Host "`nScript execution complete!" -ForegroundColor Green
