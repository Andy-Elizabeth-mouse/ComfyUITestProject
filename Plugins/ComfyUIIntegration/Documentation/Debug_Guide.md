# ComfyUI Integration 插件调试指南

## 概述

本指南详细介绍如何在Unreal Engine 5中调试ComfyUI Integration插件，包括测试项目创建、插件集成、调试配置和常见问题解决方案。

## 快速开始

### 1. 创建测试项目

**推荐使用空白项目进行插件调试**

1. **启动UE5编辑器**
2. **选择项目类型**：
   - 点击"Games"
   - 选择"Blank"模板
3. **项目设置**：
   - 项目类型：C++
   - 目标平台：Desktop/Console
   - 质量预设：Maximum Quality
   - 初学者内容：不勾选
4. **项目命名**：建议使用"ComfyUITestProject"
5. **创建项目**

### 2. 集成插件

#### 方法一：复制插件文件（推荐）

```powershell
# 1. 在项目根目录创建Plugins文件夹
New-Item -ItemType Directory -Path "ComfyUITestProject\Plugins" -Force

# 2. 复制插件文件
Copy-Item -Path "d:\forcopilot\*" -Destination "ComfyUITestProject\Plugins\ComfyUIIntegration\" -Recurse -Force
```

#### 方法二：符号链接（开发时推荐）

```powershell
# 创建符号链接，便于开发时同步更新
New-Item -ItemType SymbolicLink -Path "ComfyUITestProject\Plugins\ComfyUIIntegration" -Target "d:\forcopilot"
```

### 3. 生成项目文件

```powershell
# 右键点击.uproject文件，选择"Generate Visual Studio project files"
# 或使用命令行
& "C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\DotNET\UnrealBuildTool.exe" -projectfiles -project="ComfyUITestProject.uproject" -game -rocket -progress
```

## 调试配置

### Visual Studio调试设置

#### 1. 打开项目

1. 双击生成的`ComfyUITestProject.sln`文件
2. 等待Visual Studio加载项目

#### 2. 配置构建设置

```
Configuration: Development Editor
Platform: Win64
StartUp Project: ComfyUITestProject
```

#### 3. 调试配置

```cpp
// 在插件代码中添加调试日志
UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Plugin: %s"), *Message);

// 添加断点调试
void UComfyUIClient::Initialize()
{
    // 在这里设置断点
    UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Client Initialized"));
}
```

### 日志配置

#### 1. 启用详细日志

在项目的`Config/DefaultEngine.ini`中添加：

```ini
[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
LogJson=Log
```

#### 2. 运行时日志查看

```cpp
// 在代码中添加日志输出
DEFINE_LOG_CATEGORY(LogComfyUIIntegration);

// 使用方法
UE_LOG(LogComfyUIIntegration, Warning, TEXT("API调用失败: %s"), *ErrorMessage);
UE_LOG(LogComfyUIIntegration, Log, TEXT("图像生成完成: %s"), *ImagePath);
```

## 编译和运行

### 1. 编译插件

```powershell
# 使用Visual Studio
# 1. 右键点击解决方案
# 2. 选择"Build Solution"
# 3. 或者按Ctrl+Shift+B

# 使用命令行
& "C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\DotNET\UnrealBuildTool.exe" -Target="ComfyUITestProjectEditor Win64 Development" -Project="ComfyUITestProject.uproject" -WaitMutex -FromMsBuild
```

### 2. 启动调试

1. **Visual Studio中**：
   - 按F5启动调试
   - 或点击"Local Windows Debugger"

2. **UE5编辑器启动后**：
   - 检查插件是否正常加载
   - 查看Output Log中的日志信息

### 3. 启用插件

1. 在UE5编辑器中，进入"Edit" -> "Plugins"
2. 搜索"ComfyUI Integration"
3. 勾选"Enabled"
4. 点击"Restart Now"

## 功能验证

### 1. 界面验证

```cpp
// 检查菜单是否正确添加
void FComfyUIIntegrationModule::StartupModule()
{
    // 验证菜单命令是否注册
    if (FComfyUIIntegrationCommands::IsRegistered())
    {
        UE_LOG(LogComfyUIIntegration, Log, TEXT("Commands registered successfully"));
    }
}
```

### 2. API连接测试

```cpp
// 在ComfyUIClient中添加连接测试
void UComfyUIClient::TestConnection()
{
    // 发送测试请求
    FString TestURL = BaseURL + TEXT("/system_stats");
    
    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(TestURL);
    HttpRequest->SetVerb("GET");
    HttpRequest->SetHeader("Content-Type", "application/json");
    
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UComfyUIClient::OnTestConnectionComplete);
    HttpRequest->ProcessRequest();
}
```

## 常见问题解决

### 1. 编译错误

**错误：模块找不到**
```cpp
// 检查.Build.cs文件是否包含所有必要模块
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "UnrealEd",
    "SlateCore",
    "Slate",
    "EditorStyle",
    "EditorWidgets",
    "ToolMenus",
    "HTTP",
    "Json",
    "JsonUtilities"
});
```

**错误：PCH相关问题**
```cpp
// 在.Build.cs文件中设置
PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
```

### 2. 插件加载失败

**检查插件描述文件**
```json
// ComfyUIIntegration.uplugin
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0",
    "FriendlyName": "ComfyUI Integration",
    "Description": "Integration plugin for ComfyUI workflow in Unreal Engine",
    "Category": "Editor",
    "CreatedBy": "Your Name",
    "CreatedByURL": "",
    "DocsURL": "",
    "MarketplaceURL": "",
    "SupportURL": "",
    "CanContainContent": true,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "Modules": [
        {
            "Name": "ComfyUIIntegration",
            "Type": "Editor",
            "LoadingPhase": "Default"
        }
    ]
}
```

### 3. HTTP请求问题

**CORS问题解决**
```cpp
// 添加必要的HTTP头
HttpRequest->SetHeader("Access-Control-Allow-Origin", "*");
HttpRequest->SetHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
HttpRequest->SetHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
```

### 4. Slate UI问题

**Widget不显示**
```cpp
// 检查Widget的构造和注册
void FComfyUIIntegrationModule::StartupModule()
{
    // 确保Style被正确初始化
    FComfyUIIntegrationStyle::Initialize();
    FComfyUIIntegrationStyle::ReloadTextures();
    
    // 注册命令
    FComfyUIIntegrationCommands::Register();
}
```

## 性能优化

### 1. 异步处理

```cpp
// 使用异步任务处理HTTP请求
AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
{
    // 后台处理
    ProcessImageGeneration();
    
    // 回到主线程更新UI
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        UpdateUI();
    });
});
```

### 2. 内存管理

```cpp
// 使用智能指针管理资源
TSharedPtr<SComfyUIWidget> ComfyUIWidget;
TUniquePtr<FComfyUIImageProcessor> ImageProcessor;

// 及时释放大型资源
void CleanupResources()
{
    GeneratedImages.Empty();
    ImageProcessor.Reset();
}
```

## 发布前检查

### 1. 代码审查清单

- [ ] 所有日志级别适当设置
- [ ] 移除调试用的硬编码路径
- [ ] 错误处理完善
- [ ] 内存泄漏检查
- [ ] 多线程安全性检查

### 2. 功能测试

- [ ] 插件正常加载
- [ ] UI界面显示正常
- [ ] API连接功能正常
- [ ] 图像生成功能正常
- [ ] 错误处理正常

### 3. 性能测试

- [ ] 大文件处理性能
- [ ] 内存使用情况
- [ ] 长时间运行稳定性

## 总结

通过创建空白测试项目，您可以：

1. **隔离环境**：避免现有项目的干扰
2. **快速迭代**：专注于插件功能开发
3. **简化调试**：减少不必要的复杂性
4. **性能测试**：在干净环境中测试性能

记住，插件开发是一个迭代过程，建议：
- 频繁编译和测试
- 使用版本控制跟踪变更
- 保持代码简洁和文档更新
- 充分利用UE5的调试工具

遇到问题时，请查看Output Log、确保所有依赖项正确配置，并参考Unreal Engine官方文档。
