# ComfyUI Integration Plugin - 发布打包脚本

## 📦 打包发布版本

这个文档描述了如何准备和打包 ComfyUI Integration Plugin 的发布版本。

### 🔄 预打包检查清单

#### 代码质量检查
- [ ] 所有代码编译通过，无警告
- [ ] 单元测试通过（如果有）
- [ ] 内存泄漏检查
- [ ] 线程安全检查

#### 版本信息更新
- [ ] 更新 `ComfyUIIntegration.uplugin` 中的版本号
- [ ] 更新发布说明文档
- [ ] 确认Beta标识正确设置

#### 文档完整性
- [ ] README.md 已更新
- [ ] INSTALLATION_GUIDE.md 完整
- [ ] RELEASE_NOTES 包含所有新功能
- [ ] API文档（如有）已更新

### 🛠️ 自动化打包脚本 (Windows)

创建 `PackagePlugin.ps1`:

```powershell
# ComfyUI Integration Plugin 打包脚本
param(
    [string]$UEPath = "E:\ue\UE_5.5",
    [string]$ProjectPath = "C:\UnrealProjects\ComfyUITestProject",
    [string]$OutputPath = "C:\UnrealProjects\ComfyUITestProject\Releases",
    [string]$Version = "0.8.0-beta"
)

Write-Host "ComfyUI Integration Plugin 打包脚本 v$Version" -ForegroundColor Green

# 1. 清理编译输出
Write-Host "清理编译输出..." -ForegroundColor Yellow
$BinariesPath = "$ProjectPath\Plugins\ComfyUIIntegration\Binaries"
$IntermediatePath = "$ProjectPath\Plugins\ComfyUIIntegration\Intermediate"

if (Test-Path $BinariesPath) {
    Remove-Item $BinariesPath -Recurse -Force
}
if (Test-Path $IntermediatePath) {
    Remove-Item $IntermediatePath -Recurse -Force
}

# 2. 编译插件
Write-Host "编译插件..." -ForegroundColor Yellow
$BuildCmd = "$UEPath\Engine\Build\BatchFiles\Build.bat"
$BuildArgs = @(
    "ComfyUITestProjectEditor",
    "Win64", 
    "Development",
    "`"$ProjectPath\ComfyUITestProject.uproject`"",
    "-waitmutex"
)

& $BuildCmd @BuildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败！" -ForegroundColor Red
    exit 1
}

# 3. 创建输出目录
Write-Host "创建输出目录..." -ForegroundColor Yellow
$ReleaseDir = "$OutputPath\ComfyUIIntegration-v$Version"
if (Test-Path $ReleaseDir) {
    Remove-Item $ReleaseDir -Recurse -Force
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null

# 4. 复制插件文件
Write-Host "复制插件文件..." -ForegroundColor Yellow
$SourcePath = "$ProjectPath\Plugins\ComfyUIIntegration"
$DestPath = "$ReleaseDir\ComfyUIIntegration"

# 复制必要文件
$FilesToCopy = @(
    "*.uplugin",
    "Source\**\*",
    "Binaries\**\*",
    "Config\**\*",
    "Content\**\*", 
    "*.md",
    "Scripts\**\*"
)

New-Item -ItemType Directory -Path $DestPath -Force | Out-Null

foreach ($Pattern in $FilesToCopy) {
    $Files = Get-ChildItem -Path $SourcePath -Include ($Pattern -split '\\')[-1] -Recurse
    foreach ($File in $Files) {
        $RelativePath = $File.FullName.Substring($SourcePath.Length + 1)
        $DestFile = Join-Path $DestPath $RelativePath
        $DestFileDir = Split-Path $DestFile -Parent
        
        if (!(Test-Path $DestFileDir)) {
            New-Item -ItemType Directory -Path $DestFileDir -Force | Out-Null
        }
        
        Copy-Item $File.FullName $DestFile -Force
    }
}

# 5. 创建安装脚本
Write-Host "创建安装脚本..." -ForegroundColor Yellow
$InstallScript = @"
@echo off
echo ComfyUI Integration Plugin v$Version 安装脚本
echo.

set /p "ProjectPath=请输入您的UE5项目路径: "
if not exist "%ProjectPath%" (
    echo 项目路径不存在！
    pause
    exit /b 1
)

if not exist "%ProjectPath%\Plugins" (
    mkdir "%ProjectPath%\Plugins"
)

echo 正在复制插件文件...
xcopy /s /y "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

echo.
echo 安装完成！
echo 请重新生成项目文件并重启虚幻引擎编辑器。
echo.
pause
"@

$InstallScript | Out-File "$ReleaseDir\Install.bat" -Encoding ASCII

# 6. 创建README
Write-Host "创建发布README..." -ForegroundColor Yellow
$ReleaseReadme = @"
# ComfyUI Integration Plugin v$Version

## 快速安装

### 自动安装（推荐）
1. 运行 `Install.bat`
2. 输入您的UE5项目路径
3. 重启虚幻引擎编辑器

### 手动安装  
1. 将 `ComfyUIIntegration` 文件夹复制到您项目的 `Plugins` 目录
2. 右键点击项目的 `.uproject` 文件
3. 选择 "Generate Visual Studio project files"
4. 重新编译项目
5. 重启虚幻引擎编辑器

## 文档
- **安装指南**: ComfyUIIntegration/INSTALLATION_GUIDE.md
- **发布说明**: ComfyUIIntegration/RELEASE_NOTES_v$Version.md  
- **测试指南**: ComfyUIIntegration/TESTING_GUIDE.md

## 系统要求
- Unreal Engine 5.3+
- Windows 10/11
- ComfyUI (运行在本地或远程服务器)

## 快速开始
1. 确保 ComfyUI 服务正在运行（默认端口8188）
2. 在虚幻编辑器中：Tools > ComfyUI Integration
3. 配置服务器地址并开始使用！

## 新功能亮点 ⭐
- **拖拽支持**: 直接从内容浏览器拖拽纹理资产到输入框
- **更好的用户体验**: 直观的视觉反馈和错误处理
- **稳定性提升**: 改进的内存管理和连接处理

---
版本: v$Version | 构建日期: $(Get-Date -Format 'yyyy-MM-dd')
"@

$ReleaseReadme | Out-File "$ReleaseDir\README.md" -Encoding UTF8

# 7. 创建压缩包
Write-Host "创建压缩包..." -ForegroundColor Yellow
$ZipPath = "$OutputPath\ComfyUIIntegration-v$Version.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($ReleaseDir, $ZipPath)

# 8. 生成校验和
Write-Host "生成校验和..." -ForegroundColor Yellow
$Hash = Get-FileHash $ZipPath -Algorithm SHA256
$Hash.Hash | Out-File "$OutputPath\ComfyUIIntegration-v$Version.sha256" -Encoding ASCII

# 9. 清理临时文件
Write-Host "清理临时文件..." -ForegroundColor Yellow
Remove-Item $ReleaseDir -Recurse -Force

# 完成
Write-Host "" 
Write-Host "打包完成！" -ForegroundColor Green
Write-Host "输出文件:" -ForegroundColor Cyan
Write-Host "  - $ZipPath" -ForegroundColor White
Write-Host "  - $OutputPath\ComfyUIIntegration-v$Version.sha256" -ForegroundColor White
Write-Host ""
Write-Host "文件大小: $((Get-Item $ZipPath).Length / 1MB) MB" -ForegroundColor Cyan
Write-Host "SHA256: $($Hash.Hash)" -ForegroundColor Cyan
```

### 📋 手动打包步骤

如果不使用自动化脚本，可以按以下步骤手动打包：

#### 1. 准备工作
```bash
# 创建发布目录
mkdir C:\Releases\ComfyUIIntegration-v0.8.0-beta
```

#### 2. 编译插件
```bash
# 清理输出
del /s /q "Plugins\ComfyUIIntegration\Binaries"
del /s /q "Plugins\ComfyUIIntegration\Intermediate"

# 重新编译
"E:\ue\UE_5.5\Engine\Build\BatchFiles\Build.bat" ComfyUITestProjectEditor Win64 Development "C:\UnrealProjects\ComfyUITestProject\ComfyUITestProject.uproject" -waitmutex
```

#### 3. 复制文件
复制以下文件到发布目录：
- `ComfyUIIntegration.uplugin`
- `Source/` 目录（完整）
- `Binaries/` 目录（编译后）
- `Config/` 目录（如果有）
- `Content/` 目录（如果有）
- 所有 `.md` 文档文件

#### 4. 排除文件
确保不包含以下文件：
- `.vs/`
- `Intermediate/` 
- `*.tmp`, `*.log`
- 源码管理文件 (`.git/`, `.svn/`)

#### 5. 验证包完整性
- [ ] 检查 `.uplugin` 文件存在
- [ ] 编译的二进制文件存在 `Binaries/Win64/`
- [ ] 源码文件完整
- [ ] 文档文件齐全

### 🚀 发布流程

#### 准备发布
1. **测试验证**
   - 在干净的UE5项目中安装测试
   - 运行完整测试套件
   - 验证所有新功能

2. **文档最终检查**
   - 确保版本号一致
   - 更新发布日期
   - 检查安装说明准确性

3. **创建发布包**
   - 运行打包脚本
   - 验证压缩包内容
   - 测试安装脚本

#### 发布分发
1. **内部发布**
   - 团队成员测试
   - 核心用户预览
   - 收集初步反馈

2. **公开测试版发布**
   - 上传到共享平台
   - 发布测试通知
   - 收集用户反馈

3. **正式发布准备**
   - 根据反馈修复问题
   - 准备正式版文档
   - 制定发布计划

### 📊 发布检查清单

**技术检查**:
- [ ] 编译无错误无警告
- [ ] 在多个UE版本测试
- [ ] 内存泄漏检查
- [ ] 性能测试通过

**文档检查**:
- [ ] 安装指南完整准确
- [ ] 发布说明详细
- [ ] 测试指南可操作
- [ ] API文档最新

**用户体验检查**:
- [ ] 安装过程简单
- [ ] 首次使用体验良好
- [ ] 错误信息友好
- [ ] 功能符合预期

**发布准备**:
- [ ] 版本号正确
- [ ] 文件完整性验证
- [ ] 安装脚本测试
- [ ] 分发渠道准备

---

*打包指南版本: v1.0 | 最后更新: 2025年7月21日*
