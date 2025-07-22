# ComfyUI Integration Plugin - 蓝图用户专用最小化打包脚本
param(
    [string]$UEPath = "E:\ue\UE_5.5",
    [string]$ProjectPath = "C:\UnrealProjects\ComfyUITestProject",
    [string]$OutputPath = "C:\UnrealProjects\ComfyUITestProject\Releases",
    [string]$Version = "0.8.0-beta"
)

Write-Host "================================" -ForegroundColor Green
Write-Host "ComfyUI Integration Plugin" -ForegroundColor Green
Write-Host "蓝图用户专用最小化打包脚本" -ForegroundColor Green  
Write-Host "版本: v$Version" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""

# 检查路径
$PluginPath = "$ProjectPath\Plugins\ComfyUIIntegration"
if (!(Test-Path $PluginPath)) {
    Write-Host "错误: 插件路径不存在 $PluginPath" -ForegroundColor Red
    exit 1
}

Write-Host "开始创建蓝图用户专用包..." -ForegroundColor Yellow

# 1. 创建输出目录
$ReleaseDir = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint"
if (Test-Path $ReleaseDir) {
    Remove-Item $ReleaseDir -Recurse -Force
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
$DestPath = "$ReleaseDir\ComfyUIIntegration"
New-Item -ItemType Directory -Path $DestPath -Force | Out-Null

Write-Host "输出目录创建完成" -ForegroundColor Green

# 2. 复制必要文件（排除源码）
Write-Host ""
Write-Host "复制运行时必要文件..." -ForegroundColor Yellow

# 2.1 复制插件配置文件
Write-Host "  复制插件配置..." -ForegroundColor Gray
Copy-Item "$PluginPath\ComfyUIIntegration.uplugin" "$DestPath\" -Force

# 2.2 复制编译后的二进制文件（排除调试文件）
Write-Host "  复制二进制文件（排除调试信息）..." -ForegroundColor Gray
if (Test-Path "$PluginPath\Binaries") {
    # 创建Binaries目录结构
    New-Item -ItemType Directory -Path "$DestPath\Binaries" -Force | Out-Null
    
    # 复制所有Binaries文件，但排除.pdb调试文件
    Get-ChildItem "$PluginPath\Binaries" -Recurse | Where-Object {
        $_.Extension -ne ".pdb" -and $_.Name -notlike "*.pdb"
    } | ForEach-Object {
        $relativePath = $_.FullName.Substring($PluginPath.Length + "\Binaries\".Length)
        $destFile = Join-Path "$DestPath\Binaries" $relativePath
        $destDir = Split-Path $destFile -Parent
        
        if (!(Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        }
        
        Copy-Item $_.FullName $destFile -Force
    }
    
    Write-Host "    已排除调试文件 (.pdb)，减少约60MB" -ForegroundColor Green
} else {
    Write-Host "警告: 未找到编译后的二进制文件!" -ForegroundColor Yellow
}

# 2.3 复制Content目录（如果有）
if (Test-Path "$PluginPath\Content") {
    Write-Host "  复制Content文件..." -ForegroundColor Gray
    Copy-Item "$PluginPath\Content" "$DestPath\Content" -Recurse -Force
}

# 2.4 复制Config目录（如果有）
if (Test-Path "$PluginPath\Config") {
    Write-Host "  复制配置文件..." -ForegroundColor Gray
    Copy-Item "$PluginPath\Config" "$DestPath\Config" -Recurse -Force
}

# 2.5 复制蓝图用户相关文档
Write-Host "  复制用户文档..." -ForegroundColor Gray
$BlueprintDocs = @(
    "BLUEPRINT_USER_GUIDE.md"
)

foreach ($Doc in $BlueprintDocs) {
    $SourceDoc = "$PluginPath\$Doc"
    if (Test-Path $SourceDoc) {
        Copy-Item $SourceDoc "$DestPath\" -Force
    }
}

Write-Host "文件复制完成" -ForegroundColor Green

# 3. 创建蓝图用户专用安装脚本
Write-Host ""
Write-Host "创建安装脚本..." -ForegroundColor Yellow

$InstallScript = @"
@echo off
chcp 65001 >nul
cls
@echo off
echo ========================================
echo ComfyUI Integration Plugin v$Version
echo 自动安装脚本
echo ========================================
echo.

:INPUT_PATH
set /p "ProjectPath=请输入您的UE5项目路径: "

if not exist "%ProjectPath%" (
    echo.
    echo [错误] 项目路径不存在！
    echo 请检查路径是否正确。
    echo.
    goto INPUT_PATH
)

:: 检查是否为UE项目
if not exist "%ProjectPath%\*.uproject" (
    echo.
    echo [警告] 指定目录下没有找到.uproject文件
    set /p "Continue=是否继续安装？^(y/n^): "
    if /i not "%Continue%"=="y" goto INPUT_PATH
)

:: 检查UE版本兼容性
echo.
echo 检查项目兼容性...
for %%f in ("%ProjectPath%\*.uproject") do (
    findstr /C:"5.3" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.4" "%%f" >nul  
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.5" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.5" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
)

if not defined UE_COMPATIBLE (
    echo [警告] 项目可能不兼容，建议使用UE 5.3+版本
    echo 是否继续安装？
    set /p "Continue=继续^(y^)还是退出^(n^): "
    if /i not "%Continue%"=="y" exit /b 0
)

:: 创建Plugins目录
if not exist "%ProjectPath%\Plugins" (
    echo 创建 Plugins 目录...
    mkdir "%ProjectPath%\Plugins"
)

:: 检查是否已安装旧版本
if exist "%ProjectPath%\Plugins\ComfyUIIntegration" (
    echo.
    echo 检测到已安装的插件版本
    set /p "Overwrite=是否覆盖安装？^(y/n^): "
    if /i not "%Overwrite%"=="y" (
        echo 安装已取消。
        pause
        exit /b 0
    )
    echo 正在移除旧版本...
    rmdir /s /q "%ProjectPath%\Plugins\ComfyUIIntegration"
)

echo.
echo ========================================
echo 正在安装插件...
echo ========================================
xcopy /s /y /i "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] 文件复制失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 安装成功！
echo ========================================
echo.
echo ComfyUI Integration Plugin 已安装到您的项目中！
echo.
echo "接下来的步骤："
echo "1. 启动虚幻引擎编辑器"
echo "2. 打开您的项目"
echo "3. 在菜单栏找到：Tools ^> ComfyUI Integration"
echo.
echo 使用提示：
echo "- 确保ComfyUI服务正在运行 ^(默认: http://127.0.0.1:8188^)"
echo "- 查看用户指南: ComfyUIIntegration\BLUEPRINT_USER_GUIDE.md"
echo.
echo 注意：
echo 如果插件菜单没有出现，请到 Edit ^> Plugins 中启用 "ComfyUI Integration"
echo.
pause
"@

$InstallScript | Out-File "$ReleaseDir\一键安装.bat" -Encoding UTF8

# 4. 创建蓝图用户专用README
# Write-Host "创建专用说明文档..." -ForegroundColor Yellow

# $BlueprintReadme = @"
# # ComfyUI Integration Plugin v$Version
# ## 蓝图用户专用版本

# ? **专为蓝图项目优化的轻量版本**

# ### ? 特点

# - ? **轻量化**：仅包含运行必需文件，体积减少70%+
# - ? **即插即用**：无需编译环境，适合所有蓝图开发者  
# - ? **完整功能**：包含所有AI生成功能和新的拖拽特性
# - ? **一键安装**：运行"蓝图项目一键安装.bat"即可

# ### ? 5秒安装

# 1. **双击运行**：`蓝图项目一键安装.bat`
# 2. **输入路径**：您的UE5蓝图项目路径
# 3. **完成**：重启编辑器即可使用

# ### ? 新功能亮点

# **? 拖拽资产输入**
# - 直接从内容浏览器拖拽纹理到输入框
# - 实时视觉反馈和智能验证
# - 大幅提升工作效率

# **? 多种AI工作流**
# - 文生图 (Text to Image)
# - 图生图 (Image to Image) 
# - 文生3D、图生3D
# - 纹理生成

# ### ? 系统要求

# - ? **UE5蓝图项目** (5.3+)
# - ? **Windows 10/11**
# - ? **ComfyUI服务** (本地或远程)
# - ? **无需Visual Studio或编译工具**

# ### ? 使用指南

# - **快速入门**：BLUEPRINT_USER_GUIDE.md
# - **详细安装**：INSTALLATION_GUIDE.md
# - **发布说明**：RELEASE_NOTES_v$Version.md

# ### ? 版本对比

# | 项目 | 完整版 | 蓝图专用版 |
# |------|--------|-----------|
# | 文件大小 | ~15MB | ~4MB |
# | 包含源码 | ? | ? |
# | 蓝图项目支持 | ? | ? |
# | 功能完整性 | 100% | 100% |
# | 安装复杂度 | 中等 | 极简 |

# ### ? 快速开始

# 1. **启动ComfyUI**: `python main.py --listen 127.0.0.1 --port 8188`
# 2. **打开UE5**: Tools > ComfyUI Integration
# 3. **开始创作**: 选择工作流 > 输入提示词 > 生成！

# ### ? 反馈

# 蓝图用户的反馈特别宝贵！请告诉我们：
# - 安装体验如何？
# - 功能是否好用？  
# - 还需要什么改进？

# ---

# **? 让AI为您的蓝图项目注入创意！**

# *版本: v$Version-Blueprint | 构建时间: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")*
# "@

# $BlueprintReadme | Out-File "$ReleaseDir\README.md" -Encoding UTF8

# 5. 验证必要文件
Write-Host ""
Write-Host "验证安装包完整性..." -ForegroundColor Yellow

$RequiredFiles = @(
    "$DestPath\ComfyUIIntegration.uplugin",
    "$DestPath\Binaries\Win64\UnrealEditor-ComfyUIIntegration.dll"
)

$AllFilesExist = $true
foreach ($File in $RequiredFiles) {
    if (Test-Path $File) {
        Write-Host "  存在: $(Split-Path $File -Leaf)" -ForegroundColor Green
    } else {
        Write-Host "  缺失: $(Split-Path $File -Leaf)" -ForegroundColor Red
        $AllFilesExist = $false
    }
}

if (!$AllFilesExist) {
    Write-Host "错误: 关键文件缺失!" -ForegroundColor Red
    exit 1
}

# 6. 创建压缩包
Write-Host ""
Write-Host "创建蓝图专用发布包..." -ForegroundColor Yellow

$ZipPath = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($ReleaseDir, $ZipPath)

$ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)

# 7. 生成校验和
$Hash = Get-FileHash $ZipPath -Algorithm SHA256
$ChecksumFile = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint.sha256"
$Hash.Hash | Out-File $ChecksumFile -Encoding ASCII

# 8. 清理临时文件
Remove-Item $ReleaseDir -Recurse -Force

# 完成报告
Write-Host ""
Write-Host "================================" -ForegroundColor Green
Write-Host "蓝图专用版本打包完成!" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""
Write-Host "? 输出文件:" -ForegroundColor Cyan
Write-Host "蓝图专用包: $ZipPath" -ForegroundColor White
Write-Host "校验文件: $ChecksumFile" -ForegroundColor White
Write-Host ""
Write-Host "? 大小对比:" -ForegroundColor Cyan
$FullPackageSize = if (Test-Path "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip") {
    [math]::Round((Get-Item "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip").Length / 1MB, 2)
} else { "未知" }
Write-Host "完整版: $($FullPackageSize) MB" -ForegroundColor Gray
Write-Host "蓝图版: $($ZipSizeMB) MB" -ForegroundColor White
if ($FullPackageSize -ne "未知") {
    $ReductionPercent = [math]::Round((1 - $ZipSizeMB / $FullPackageSize) * 100, 1)
    Write-Host "减少: $($ReductionPercent)%" -ForegroundColor Green
}
Write-Host ""
Write-Host "? 蓝图用户现在可以享受轻量化安装体验!" -ForegroundColor Green
