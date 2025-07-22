# ComfyUI Integration Plugin 打包脚本
param(
    [string]$UEPath = "E:\ue\UE_5.5",
    [string]$ProjectPath = "C:\UnrealProjects\ComfyUITestProject",
    [string]$OutputPath = "C:\UnrealProjects\ComfyUITestProject\Releases",
    [string]$Version = "0.8.0-beta"
)

Write-Host "================================" -ForegroundColor Green
Write-Host "ComfyUI Integration Plugin 打包脚本" -ForegroundColor Green
Write-Host "版本: v$Version" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""

# 检查路径是否存在
Write-Host "检查环境..." -ForegroundColor Yellow
if (!(Test-Path $UEPath)) {
    Write-Host "错误：UE5路径不存在: $UEPath" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $ProjectPath)) {
    Write-Host "错误：项目路径不存在: $ProjectPath" -ForegroundColor Red
    exit 1
}

$PluginPath = "$ProjectPath\Plugins\ComfyUIIntegration"
if (!(Test-Path $PluginPath)) {
    Write-Host "错误：插件路径不存在: $PluginPath" -ForegroundColor Red
    exit 1
}

Write-Host "✓ 环境检查通过" -ForegroundColor Green

# 1. 清理编译输出
Write-Host ""
Write-Host "步骤 1: 清理编译输出..." -ForegroundColor Yellow
$BinariesPath = "$PluginPath\Binaries"
$IntermediatePath = "$PluginPath\Intermediate"

if (Test-Path $BinariesPath) {
    Remove-Item $BinariesPath -Recurse -Force
    Write-Host "  清理 Binaries 目录" -ForegroundColor Gray
}
if (Test-Path $IntermediatePath) {
    Remove-Item $IntermediatePath -Recurse -Force
    Write-Host "  清理 Intermediate 目录" -ForegroundColor Gray
}
Write-Host "✓ 清理完成" -ForegroundColor Green

# 2. 编译插件
Write-Host ""
Write-Host "步骤 2: 编译插件..." -ForegroundColor Yellow
Write-Host "  这可能需要几分钟时间..." -ForegroundColor Gray

$BuildCmd = "$UEPath\Engine\Build\BatchFiles\Build.bat"
$BuildArgs = @(
    "ComfyUITestProjectEditor",
    "Win64", 
    "Development",
    "`"$ProjectPath\ComfyUITestProject.uproject`"",
    "-waitmutex"
)

Write-Host "  执行编译命令..." -ForegroundColor Gray
$BuildProcess = Start-Process -FilePath $BuildCmd -ArgumentList $BuildArgs -Wait -PassThru -NoNewWindow

if ($BuildProcess.ExitCode -ne 0) {
    Write-Host "✗ 编译失败！退出代码: $($BuildProcess.ExitCode)" -ForegroundColor Red
    exit 1
}
Write-Host "✓ 编译成功" -ForegroundColor Green

# 3. 创建输出目录
Write-Host ""
Write-Host "步骤 3: 准备输出目录..." -ForegroundColor Yellow
if (!(Test-Path $OutputPath)) {
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
    Write-Host "  创建输出根目录: $OutputPath" -ForegroundColor Gray
}

$ReleaseDir = "$OutputPath\ComfyUIIntegration-v$Version"
if (Test-Path $ReleaseDir) {
    Remove-Item $ReleaseDir -Recurse -Force
    Write-Host "  清理旧的发布目录" -ForegroundColor Gray
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
Write-Host "✓ 输出目录准备完成" -ForegroundColor Green

# 4. 复制插件文件
Write-Host ""
Write-Host "步骤 4: 复制插件文件..." -ForegroundColor Yellow
$DestPath = "$ReleaseDir\ComfyUIIntegration"

# 直接复制整个插件目录，然后删除不需要的文件
Write-Host "  复制插件目录..." -ForegroundColor Gray
Copy-Item -Path $PluginPath -Destination $DestPath -Recurse -Force

# 删除不需要的文件和目录
$ItemsToRemove = @(
    "$DestPath\Intermediate",
    "$DestPath\.vs",
    "$DestPath\*.tmp",
    "$DestPath\*.log",
    "$DestPath\*.suo",
    "$DestPath\*.user"
)

foreach ($Item in $ItemsToRemove) {
    if (Test-Path $Item) {
        Remove-Item $Item -Recurse -Force
        Write-Host "  移除: $(Split-Path $Item -Leaf)" -ForegroundColor Gray
    }
}

Write-Host "✓ 文件复制完成" -ForegroundColor Green

# 5. 验证关键文件
Write-Host ""
Write-Host "步骤 5: 验证文件完整性..." -ForegroundColor Yellow
$RequiredFiles = @(
    "$DestPath\ComfyUIIntegration.uplugin",
    "$DestPath\Source\ComfyUIIntegration\ComfyUIIntegration.Build.cs",
    "$DestPath\Binaries\Win64\UnrealEditor-ComfyUIIntegration.dll"
)

$AllFilesExist = $true
foreach ($File in $RequiredFiles) {
    if (Test-Path $File) {
        Write-Host "  ✓ $(Split-Path $File -Leaf)" -ForegroundColor Green
    } else {
        Write-Host "  ✗ 缺失: $(Split-Path $File -Leaf)" -ForegroundColor Red
        $AllFilesExist = $false
    }
}

if (!$AllFilesExist) {
    Write-Host "错误：关键文件缺失！" -ForegroundColor Red
    exit 1
}

# 6. 创建安装脚本
Write-Host ""
Write-Host "步骤 6: 创建安装脚本..." -ForegroundColor Yellow
$InstallScript = @"
@echo off
chcp 65001 >nul
echo ================================
echo ComfyUI Integration Plugin v$Version
echo 自动安装脚本
echo ================================
echo.

set /p "ProjectPath=请输入您的UE5项目路径: "

if not exist "%ProjectPath%" (
    echo.
    echo [错误] 项目路径不存在！
    echo 请检查路径是否正确。
    echo.
    pause
    exit /b 1
)

if not exist "%ProjectPath%\*.uproject" (
    echo.
    echo [警告] 指定目录下没有找到.uproject文件
    echo 请确认这是正确的UE5项目目录。
    echo.
)

if not exist "%ProjectPath%\Plugins" (
    echo 创建 Plugins 目录...
    mkdir "%ProjectPath%\Plugins"
)

echo.
echo 正在安装插件...
xcopy /s /y /i "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] 文件复制失败！
    pause
    exit /b 1
)

echo.
echo ================================
echo 安装完成！
echo ================================
echo.
echo 接下来的步骤：
echo 1. 右键点击项目的 .uproject 文件
echo 2. 选择 "Generate Visual Studio project files"
echo 3. 重新编译项目（如果需要）
echo 4. 启动虚幻引擎编辑器
echo 5. 在 Edit ^> Plugins 中启用 "ComfyUI Integration"
echo.
echo 使用方法：
echo Tools ^> ComfyUI Integration
echo.
echo 详细文档请查看：
echo ComfyUIIntegration\INSTALLATION_GUIDE.md
echo.
pause
"@

$InstallScript | Out-File "$ReleaseDir\Install.bat" -Encoding UTF8

# 7. 创建发布 README
Write-Host ""
Write-Host "步骤 7: 创建发布文档..." -ForegroundColor Yellow
$BuildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$ReleaseReadme = @"
# ComfyUI Integration Plugin v$Version

**构建时间**: $BuildDate  
**支持版本**: Unreal Engine 5.3+  
**平台支持**: Windows 10/11  

## 🚀 快速安装

### 自动安装（推荐新手）
1. 双击运行 ``Install.bat``
2. 输入您的UE5项目完整路径
3. 按照提示完成安装

### 手动安装（推荐开发者）
1. 将 ``ComfyUIIntegration`` 文件夹复制到项目的 ``Plugins`` 目录
2. 右键项目的 ``.uproject`` 文件，选择 "Generate Visual Studio project files"  
3. 重新编译项目（如需要）
4. 启动虚幻引擎编辑器

## 📚 完整文档

详细的安装、使用和测试指南请查看：

- **安装指南**: ``ComfyUIIntegration/INSTALLATION_GUIDE.md``
- **发布说明**: ``ComfyUIIntegration/RELEASE_NOTES_v$Version.md``
- **测试指南**: ``ComfyUIIntegration/TESTING_GUIDE.md``
- **项目状态**: ``ComfyUIIntegration/PROJECT_STATUS.md``

## ⚙️ 系统要求

### 必需
- Unreal Engine 5.3 或更高版本
- Windows 10/11 (64位)
- Visual Studio 2019/2022 (如需重新编译)
- ComfyUI 服务（本地或远程）

### 推荐
- 16GB+ 内存
- 支持CUDA的NVIDIA显卡
- SSD存储

## 🔗 快速开始

1. **启动ComfyUI服务**
   ``````
   # 默认地址：http://127.0.0.1:8188
   python main.py --listen 127.0.0.1 --port 8188
   ``````

2. **在UE5中使用**
   - 菜单：``Tools > ComfyUI Integration``
   - 配置服务器地址
   - 选择工作流类型
   - 开始创作！

## 🆘 遇到问题？

1. **查看文档**: 首先查阅对应的 .md 文档文件
2. **检查日志**: UE5编辑器的输出日志中查看错误信息  
3. **测试连接**: 确认ComfyUI服务正常运行
4. **重启编辑器**: 尝试重启虚幻引擎编辑器

## 📊 版本信息

- **版本号**: v$Version
- **构建日期**: $BuildDate
- **兼容性**: UE 5.3, 5.4, 5.5
- **架构**: Win64

---

**感谢使用 ComfyUI Integration Plugin！**  
如有问题或建议，欢迎反馈。

*此版本为测试版，仅供评估和测试使用。*
"@

$ReleaseReadme | Out-File "$ReleaseDir\README.md" -Encoding UTF8

# 8. 创建压缩包
Write-Host ""
Write-Host "步骤 8: 创建分发包..." -ForegroundColor Yellow
$ZipPath = "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
    Write-Host "  移除旧的压缩包" -ForegroundColor Gray
}

Write-Host "  正在压缩文件..." -ForegroundColor Gray
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($ReleaseDir, $ZipPath)

$ZipSize = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host "✓ 压缩包创建完成 ($ZipSize MB)" -ForegroundColor Green

# 9. 生成文件校验和
Write-Host ""
Write-Host "步骤 9: 生成文件校验..." -ForegroundColor Yellow
$Hash = Get-FileHash $ZipPath -Algorithm SHA256
$ChecksumFile = "$OutputPath\ComfyUIIntegration-v$Version-Win64.sha256"
$Hash.Hash | Out-File $ChecksumFile -Encoding ASCII
Write-Host "✓ 校验和生成完成" -ForegroundColor Green

# 10. 生成发布信息
Write-Host ""
Write-Host "步骤 10: 生成发布信息..." -ForegroundColor Yellow
$ReleaseInfo = @"
ComfyUI Integration Plugin v$Version - 发布信息

构建时间: $BuildDate
构建环境: Windows PowerShell
UE版本支持: 5.3+

文件信息:
- 发布包: ComfyUIIntegration-v$Version-Win64.zip ($ZipSize MB)
- SHA256校验: $($Hash.Hash)

包含内容:
- 插件源码和编译文件
- 完整文档 (安装、使用、测试指南)
- 自动安装脚本
- 项目模板和示例

安装要求:
- Unreal Engine 5.3+
- Windows 10/11 x64
- ComfyUI服务

新功能亮点:
- 拖拽资产输入支持
- 改进的用户界面
- 增强的稳定性

已知限制:
- 当前仅支持Windows平台
- 需要Visual Studio进行重新编译（如需要）
- Beta版本，可能存在未知问题

发布类型: Beta测试版
质量等级: 适用于测试和评估
"@

$ReleaseInfo | Out-File "$OutputPath\ComfyUIIntegration-v$Version-ReleaseInfo.txt" -Encoding UTF8

# 11. 清理临时文件
Write-Host ""
Write-Host "步骤 11: 清理临时文件..." -ForegroundColor Yellow
Remove-Item $ReleaseDir -Recurse -Force
Write-Host "✓ 临时文件清理完成" -ForegroundColor Green

# 完成报告
Write-Host ""
Write-Host "================================" -ForegroundColor Green
Write-Host "打包完成！" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""
Write-Host "输出文件:" -ForegroundColor Cyan
Write-Host "📦 发布包: $ZipPath" -ForegroundColor White
Write-Host "🔒 校验和: $ChecksumFile" -ForegroundColor White  
Write-Host "📋 发布信息: $OutputPath\ComfyUIIntegration-v$Version-ReleaseInfo.txt" -ForegroundColor White
Write-Host ""
Write-Host "统计信息:" -ForegroundColor Cyan
Write-Host "文件大小: $ZipSize MB" -ForegroundColor White
Write-Host "SHA256: $($Hash.Hash.Substring(0,16))..." -ForegroundColor White
Write-Host ""
Write-Host "下一步:" -ForegroundColor Yellow
Write-Host "1. 在测试环境中验证安装包" -ForegroundColor White
Write-Host "2. 执行完整的测试流程" -ForegroundColor White
Write-Host "3. 准备发布通知和分发" -ForegroundColor White
Write-Host ""
Write-Host "🎉 ComfyUI Integration Plugin v$Version 已准备就绪！" -ForegroundColor Green
