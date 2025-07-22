# 配置系统和构建系统开发文档

## 概述

本文档详细介绍了ComfyUI Integration插件的配置系统和构建系统，包括构建文件、配置文件、插件描述文件等核心组件的配置和使用方法。

## 目录

1. [构建系统配置](#构建系统配置)
2. [插件描述文件](#插件描述文件)
3. [默认配置系统](#默认配置系统)
4. [项目结构说明](#项目结构说明)
5. [开发环境配置](#开发环境配置)
6. [部署和发布](#部署和发布)

---

## 构建系统配置

### ComfyUIIntegration.Build.cs

```csharp
using UnrealBuildTool;

public class ComfyUIIntegration : ModuleRules
{
    public ComfyUIIntegration(ReadOnlyTargetRules Target) : base(Target)
    {
        // 预编译头文件设置
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        // 公共依赖模块
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorStyle",
            "EditorWidgets",
            "SlateCore",
            "Slate",
            "ToolMenus",
            "HTTP",
            "Json",
            "JsonUtilities"
        });
        
        // 私有依赖模块
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "InputCore",
            "EditorStyle",
            "EditorWidgets",
            "LevelEditor",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "WorkspaceMenuStructure",
            "MainFrame",
            "PluginManager",
            "AssetTools",
            "ContentBrowser",
            "ImageWrapper",
            "RenderCore",
            "RHI",
            "ApplicationCore",
            "DesktopPlatform",
            "PlatformFile",
            "TargetPlatform",
            "LauncherPlatform",
            "GameProjectGeneration",
            "DeveloperSettings"
        });
        
        // 动态加载的模块
        DynamicallyLoadedModuleNames.AddRange(new string[]
        {
            "AssetRegistry",
            "PropertyEditor",
            "GraphEditor",
            "KismetCompiler",
            "BlueprintGraph",
            "AnimGraph"
        });
        
        // 优化设置
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        
        // 包含路径
        PublicIncludePaths.AddRange(new string[]
        {
            "ComfyUIIntegration/Public"
        });
        
        PrivateIncludePaths.AddRange(new string[]
        {
            "ComfyUIIntegration/Private"
        });
        
        // 定义宏
        PublicDefinitions.AddRange(new string[]
        {
            "COMFYUI_INTEGRATION_VERSION=1.0",
            "WITH_COMFYUI_INTEGRATION=1"
        });
        
        // 条件编译
        if (Target.bBuildEditor == true)
        {
            PublicDefinitions.Add("WITH_EDITOR=1");
        }
        
        // 平台特定设置
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("PLATFORM_WINDOWS=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDefinitions.Add("PLATFORM_MAC=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicDefinitions.Add("PLATFORM_LINUX=1");
        }
    }
}
```

### 依赖模块说明

#### 核心模块
- **Core**: UE5核心功能
- **CoreUObject**: 对象系统
- **Engine**: 引擎基础功能
- **UnrealEd**: 编辑器核心功能

#### UI模块
- **SlateCore**: Slate UI核心
- **Slate**: Slate UI组件
- **EditorStyle**: 编辑器样式
- **EditorWidgets**: 编辑器Widget
- **ToolMenus**: 工具菜单系统

#### 网络和数据模块
- **HTTP**: HTTP通信
- **Json**: JSON数据处理
- **JsonUtilities**: JSON工具

#### 编辑器集成模块
- **LevelEditor**: 关卡编辑器
- **PropertyEditor**: 属性编辑器
- **WorkspaceMenuStructure**: 工作区菜单结构
- **MainFrame**: 主框架

#### 资源和文件模块
- **AssetTools**: 资源工具
- **ContentBrowser**: 内容浏览器
- **ImageWrapper**: 图像包装器
- **PlatformFile**: 平台文件系统
- **DesktopPlatform**: 桌面平台

#### 渲染模块
- **RenderCore**: 渲染核心
- **RHI**: 渲染硬件接口

---

## 插件描述文件

### ComfyUIIntegration.uplugin

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "ComfyUI Integration",
    "Description": "Integrates ComfyUI workflow functionality into Unreal Engine 5 for AI-powered content generation",
    "Category": "Content Generation",
    "CreatedBy": "ComfyUI Integration Team",
    "CreatedByURL": "https://github.com/your-repo/ComfyUI-Integration",
    "DocsURL": "https://github.com/your-repo/ComfyUI-Integration/wiki",
    "MarketplaceURL": "",
    "SupportURL": "https://github.com/your-repo/ComfyUI-Integration/issues",
    "CanContainContent": false,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "RequiresBuildPlatform": false,
    "SupportedTargetPlatforms": [
        "Win64",
        "Mac",
        "Linux"
    ],
    "Modules": [
        {
            "Name": "ComfyUIIntegration",
            "Type": "Editor",
            "LoadingPhase": "PostEngineInit",
            "AdditionalDependencies": [
                "Engine",
                "UnrealEd"
            ]
        }
    ],
    "Plugins": [
        {
            "Name": "EditorScriptingUtilities",
            "Enabled": true
        },
        {
            "Name": "PythonScriptPlugin",
            "Enabled": false,
            "Optional": true
        }
    ]
}
```

### 插件描述文件字段说明

#### 基本信息
- **FileVersion**: 插件文件格式版本
- **Version**: 插件版本号
- **VersionName**: 版本名称（语义化版本）
- **FriendlyName**: 友好显示名称
- **Description**: 插件描述
- **Category**: 插件分类

#### 作者和支持信息
- **CreatedBy**: 创建者
- **CreatedByURL**: 创建者网址
- **DocsURL**: 文档链接
- **MarketplaceURL**: 市场链接
- **SupportURL**: 支持链接

#### 平台和兼容性
- **SupportedTargetPlatforms**: 支持的目标平台
- **RequiresBuildPlatform**: 是否需要构建平台
- **CanContainContent**: 是否可以包含内容

#### 模块配置
- **Modules**: 模块列表
  - **Name**: 模块名称
  - **Type**: 模块类型（Editor, Runtime, Developer等）
  - **LoadingPhase**: 加载阶段
  - **AdditionalDependencies**: 额外依赖

#### 插件依赖
- **Plugins**: 依赖的其他插件
  - **Name**: 插件名称
  - **Enabled**: 是否启用
  - **Optional**: 是否可选

---

## 默认配置系统

### Config/default_config.json

```json
{
    "server_settings": {
        "default_url": "http://127.0.0.1:8188",
        "timeout_seconds": 300,
        "max_retries": 3,
        "retry_delay_seconds": 5,
        "connection_check_interval": 30
    },
    "workflow_settings": {
        "default_workflow": "TextToImage",
        "default_steps": 20,
        "default_cfg_scale": 8.0,
        "default_sampler": "euler",
        "default_scheduler": "normal",
        "default_model": "v1-5-pruned-emaonly-fp16.safetensors"
    },
    "output_settings": {
        "output_directory": "Generated",
        "default_filename_prefix": "ComfyUI_Generated",
        "max_image_size": 2048,
        "supported_formats": ["png", "jpg", "jpeg", "webp"],
        "default_format": "png",
        "auto_save": false,
        "save_metadata": true
    },
    "ui_settings": {
        "theme": "Dark",
        "show_tooltips": true,
        "show_progress": true,
        "auto_preview": true,
        "confirm_actions": true,
        "remember_settings": true
    },
    "performance_settings": {
        "max_concurrent_requests": 2,
        "image_cache_size_mb": 256,
        "enable_compression": true,
        "use_threading": true,
        "log_level": "Warning"
    },
    "advanced_settings": {
        "enable_debug_mode": false,
        "enable_profiling": false,
        "custom_headers": {},
        "proxy_settings": {
            "enabled": false,
            "host": "",
            "port": 8080,
            "username": "",
            "password": ""
        }
    }
}
```

### 配置加载系统

```cpp
// 配置管理器
class FComfyUIConfigManager
{
public:
    static FComfyUIConfigManager& Get()
    {
        static FComfyUIConfigManager Instance;
        return Instance;
    }
    
    // 加载配置
    void LoadConfig()
    {
        FString ConfigPath = GetConfigPath();
        FString JsonString;
        
        if (FFileHelper::LoadFileToString(JsonString, *ConfigPath))
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
            
            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                ParseConfig(JsonObject);
            }
        }
        else
        {
            // 如果配置文件不存在，创建默认配置
            CreateDefaultConfig();
        }
    }
    
    // 保存配置
    void SaveConfig()
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        
        // 序列化配置
        SerializeConfig(JsonObject);
        
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        FString ConfigPath = GetConfigPath();
        FFileHelper::SaveStringToFile(JsonString, *ConfigPath);
    }
    
    // 获取配置值
    template<typename T>
    T GetConfigValue(const FString& Section, const FString& Key, const T& DefaultValue)
    {
        if (ConfigData.Contains(Section))
        {
            const TSharedPtr<FJsonObject>& SectionObject = ConfigData[Section]->AsObject();
            if (SectionObject.IsValid() && SectionObject->HasField(Key))
            {
                return GetJsonValue<T>(SectionObject->GetField<T>(Key));
            }
        }
        return DefaultValue;
    }
    
    // 设置配置值
    template<typename T>
    void SetConfigValue(const FString& Section, const FString& Key, const T& Value)
    {
        if (!ConfigData.Contains(Section))
        {
            ConfigData.Add(Section, MakeShareable(new FJsonValueObject(MakeShareable(new FJsonObject))));
        }
        
        const TSharedPtr<FJsonObject>& SectionObject = ConfigData[Section]->AsObject();
        if (SectionObject.IsValid())
        {
            SetJsonValue<T>(SectionObject, Key, Value);
        }
    }
    
private:
    TMap<FString, TSharedPtr<FJsonValue>> ConfigData;
    
    FString GetConfigPath()
    {
        return FPaths::ProjectConfigDir() / TEXT("ComfyUI") / TEXT("config.json");
    }
    
    void ParseConfig(TSharedPtr<FJsonObject> JsonObject)
    {
        // 解析服务器设置
        if (JsonObject->HasField("server_settings"))
        {
            ConfigData.Add("server_settings", JsonObject->GetField("server_settings"));
        }
        
        // 解析工作流设置
        if (JsonObject->HasField("workflow_settings"))
        {
            ConfigData.Add("workflow_settings", JsonObject->GetField("workflow_settings"));
        }
        
        // 解析输出设置
        if (JsonObject->HasField("output_settings"))
        {
            ConfigData.Add("output_settings", JsonObject->GetField("output_settings"));
        }
        
        // 解析UI设置
        if (JsonObject->HasField("ui_settings"))
        {
            ConfigData.Add("ui_settings", JsonObject->GetField("ui_settings"));
        }
        
        // 解析性能设置
        if (JsonObject->HasField("performance_settings"))
        {
            ConfigData.Add("performance_settings", JsonObject->GetField("performance_settings"));
        }
        
        // 解析高级设置
        if (JsonObject->HasField("advanced_settings"))
        {
            ConfigData.Add("advanced_settings", JsonObject->GetField("advanced_settings"));
        }
    }
    
    void SerializeConfig(TSharedPtr<FJsonObject> JsonObject)
    {
        for (const auto& Pair : ConfigData)
        {
            JsonObject->SetField(Pair.Key, Pair.Value);
        }
    }
    
    void CreateDefaultConfig()
    {
        // 创建默认配置
        UE_LOG(LogComfyUIIntegration, Warning, TEXT("Creating default configuration"));
        
        // 设置默认值
        SetConfigValue("server_settings", "default_url", FString("http://127.0.0.1:8188"));
        SetConfigValue("server_settings", "timeout_seconds", 300);
        SetConfigValue("workflow_settings", "default_workflow", FString("TextToImage"));
        SetConfigValue("output_settings", "output_directory", FString("Generated"));
        SetConfigValue("ui_settings", "theme", FString("Dark"));
        SetConfigValue("performance_settings", "max_concurrent_requests", 2);
        
        // 保存默认配置
        SaveConfig();
    }
};
```

### 配置使用示例

```cpp
// 获取配置值
FString ServerUrl = FComfyUIConfigManager::Get().GetConfigValue<FString>("server_settings", "default_url", TEXT("http://127.0.0.1:8188"));
int32 TimeoutSeconds = FComfyUIConfigManager::Get().GetConfigValue<int32>("server_settings", "timeout_seconds", 300);
bool AutoSave = FComfyUIConfigManager::Get().GetConfigValue<bool>("output_settings", "auto_save", false);

// 设置配置值
FComfyUIConfigManager::Get().SetConfigValue("server_settings", "default_url", TEXT("http://192.168.1.100:8188"));
FComfyUIConfigManager::Get().SetConfigValue("ui_settings", "theme", TEXT("Light"));

// 保存配置
FComfyUIConfigManager::Get().SaveConfig();
```

---

## 项目结构说明

### 目录结构

```
ComfyUIIntegration/
├── ComfyUIIntegration.uplugin          # 插件描述文件
├── README.md                           # 项目说明
├── PROJECT_STATUS.md                   # 项目状态
├── Config/                             # 配置文件目录
│   └── default_config.json            # 默认配置
├── Source/                             # 源代码目录
│   └── ComfyUIIntegration/            # 主模块
│       ├── ComfyUIIntegration.Build.cs # 构建配置
│       ├── Public/                     # 公共头文件
│       │   ├── ComfyUIIntegration.h
│       │   ├── ComfyUIClient.h
│       │   ├── ComfyUIWidget.h
│       │   ├── ComfyUIIntegrationCommands.h
│       │   └── ComfyUIIntegrationStyle.h
│       └── Private/                    # 私有实现文件
│           ├── ComfyUIIntegration.cpp
│           ├── ComfyUIClient.cpp
│           ├── ComfyUIWidget.cpp
│           ├── ComfyUIIntegrationCommands.cpp
│           └── ComfyUIIntegrationStyle.cpp
├── Resources/                          # 资源文件
│   ├── Icons/                         # 图标文件
│   ├── Buttons/                       # 按钮纹理
│   ├── Borders/                       # 边框纹理
│   └── Panels/                        # 面板纹理
└── Documentation/                      # 文档目录
    ├── API_Documentation.md           # API文档
    ├── DevelopmentGuide.md            # 开发指南
    ├── UserGuide.md                   # 用户指南
    └── WorkflowIntegration.md         # 工作流集成
```

### 文件说明

#### 根目录文件
- **ComfyUIIntegration.uplugin**: 插件描述文件，定义插件元数据
- **README.md**: 项目说明文档
- **PROJECT_STATUS.md**: 项目状态和开发进度

#### 配置文件
- **Config/default_config.json**: 默认配置文件
- **Config/user_config.json**: 用户自定义配置（可选）

#### 源代码
- **Source/ComfyUIIntegration/**: 主模块源代码
- **Public/**: 公共头文件，供其他模块使用
- **Private/**: 私有实现文件，模块内部使用

#### 资源文件
- **Resources/**: 静态资源文件
- **Icons/**: 图标和图像文件
- **Buttons/**: 按钮样式纹理
- **Borders/**: 边框样式纹理

#### 文档
- **Documentation/**: 项目文档
- **API_Documentation.md**: API参考文档
- **DevelopmentGuide.md**: 开发指南
- **UserGuide.md**: 用户使用指南

---

## 开发环境配置

### 环境要求

#### 软件要求
- **Unreal Engine 5.0+**: 主要开发环境
- **Visual Studio 2019/2022**: C++编译器
- **Git**: 版本控制
- **ComfyUI**: AI生成服务（可选，用于测试）

#### 硬件要求
- **CPU**: Intel i5 或 AMD Ryzen 5 以上
- **内存**: 16GB RAM 以上
- **显卡**: GTX 1060 或 RX 580 以上
- **存储**: 50GB 可用空间

### 开发环境设置

#### 1. 创建测试项目

**推荐方式：创建空白项目**

1. 打开Unreal Engine 5编辑器
2. 选择"Games" -> "Blank"
3. 设置项目参数：
   - Blueprint/C++：选择"C++"
   - 目标平台：Desktop/Console
   - 质量预设：Maximum Quality
   - 包含初学者内容：不勾选（保持项目简洁）
4. 项目名称：建议使用"ComfyUITestProject"
5. 点击"Create"创建项目

#### 2. 集成插件到测试项目

**方法一：复制插件到项目文件夹**

```bash
# 1. 在项目根目录创建Plugins文件夹
mkdir ComfyUITestProject/Plugins

# 2. 复制插件文件
xcopy "d:\forcopilot" "ComfyUITestProject\Plugins\ComfyUIIntegration" /s /e /h
```

**方法二：使用符号链接（推荐开发时使用）**

```bash
# 创建符号链接，方便开发时同步更新
mklink /D "ComfyUITestProject\Plugins\ComfyUIIntegration" "d:\forcopilot"
```

#### 3. 生成项目文件

```bash
# 右键点击.uproject文件，选择"Generate Visual Studio project files"
# 或使用命令行：
"C:\Program Files\Epic Games\UE_5.0\Engine\Binaries\DotNET\UnrealBuildTool.exe" -projectfiles -project="ComfyUITestProject.uproject" -game -rocket -progress
```

#### 4. 编译和调试

**Visual Studio中编译**

1. 打开生成的.sln文件
2. 设置构建配置：
   - Configuration: "Development Editor" 
   - Platform: "Win64"
3. 右键点击项目名称，选择"Set as StartUp Project"
4. 按F5或点击"Local Windows Debugger"开始调试

**常见编译问题解决**

```cpp
// 如果遇到模块找不到的错误，检查.Build.cs文件
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject", 
    "Engine",
    "UnrealEd",
    "HTTP",
    "Json",
    "JsonUtilities",
    // 确保所有需要的模块都已包含
});
```

#### 5. 在编辑器中启用插件

1. 编译成功后，UE5编辑器会自动启动
2. 进入"Edit" -> "Plugins"
3. 在"Project"分类下找到"ComfyUI Integration"
4. 勾选"Enabled"复选框
5. 点击"Restart Now"重启编辑器

#### 6. 验证插件功能

1. 重启后，在编辑器菜单栏应该能看到"ComfyUI"菜单
2. 点击菜单项，应该能打开ComfyUI Widget界面
3. 检查Output Log，确认插件正常加载：
   ```
   LogComfyUIIntegration: ComfyUI Integration Plugin loaded successfully
   ```

#### 7. 调试技巧

**启用详细日志**
```ini
; 在项目的Config/DefaultEngine.ini中添加：
[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
```

**断点调试**
- 在Visual Studio中设置断点
- 使用F5启动调试模式
- 在UE5编辑器中触发插件功能

**性能分析**
```cpp
// 在关键函数中添加性能统计
DECLARE_CYCLE_STAT(TEXT("ComfyUI API Call"), STAT_ComfyUIAPICall, STATGROUP_ComfyUI);

void UComfyUIClient::CallAPI()
{
    SCOPE_CYCLE_COUNTER(STAT_ComfyUIAPICall);
    // API调用逻辑
}
```

### 调试配置

#### 1. 调试器设置

```json
// launch.json (VS Code)
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug ComfyUIIntegration",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/../../Engine/Binaries/Win64/UnrealEditor.exe",
            "args": ["${workspaceFolder}/../../YourProject.uproject"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "externalTerminal"
        }
    ]
}
```

#### 2. 日志配置

```ini
; DefaultEngine.ini
[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Warning
LogHttp=Warning
```

#### 3. 性能分析

```cpp
// 启用性能分析
DECLARE_CYCLE_STAT(TEXT("ComfyUI Image Generation"), STAT_ComfyUIImageGeneration, STATGROUP_ComfyUI);

void UComfyUIClient::GenerateImage(...)
{
    SCOPE_CYCLE_COUNTER(STAT_ComfyUIImageGeneration);
    
    // 生成逻辑
}
```

---

## 部署和发布

### 打包配置

#### 1. 插件打包

```bash
# 使用UnrealPak工具打包
UnrealPak.exe ComfyUIIntegration.pak -Create=ComfyUIIntegration.txt

# ComfyUIIntegration.txt 内容示例
"Source/ComfyUIIntegration/Public/*.h" "../../../Engine/Plugins/ComfyUIIntegration/Source/ComfyUIIntegration/Public/"
"Source/ComfyUIIntegration/Private/*.cpp" "../../../Engine/Plugins/ComfyUIIntegration/Source/ComfyUIIntegration/Private/"
"Resources/*" "../../../Engine/Plugins/ComfyUIIntegration/Resources/"
```

#### 2. 发布构建

```bash
# 创建发布版本
UnrealBuildTool.exe -Project=YourProject.uproject -Target=YourProjectEditor -Platform=Win64 -Configuration=Shipping -Clean
UnrealBuildTool.exe -Project=YourProject.uproject -Target=YourProjectEditor -Platform=Win64 -Configuration=Shipping
```

### 版本管理

#### 1. 语义化版本控制

```json
// 版本格式: MAJOR.MINOR.PATCH
{
    "Version": 1,
    "VersionName": "1.0.0"
}
```

#### 2. 发布流程

```bash
# 1. 更新版本号
git tag v1.0.0

# 2. 创建发布分支
git checkout -b release/1.0.0

# 3. 构建和测试
./build.sh release

# 4. 创建发布包
./package.sh

# 5. 合并到主分支
git checkout main
git merge release/1.0.0

# 6. 推送标签
git push origin v1.0.0
```

### 持续集成

#### GitHub Actions配置

```yaml
# .github/workflows/build.yml
name: Build and Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Unreal Engine
      uses: actions/setup-unreal@v1
      with:
        unreal-version: '5.0'
    
    - name: Build Plugin
      run: |
        UnrealBuildTool.exe ComfyUIIntegration Win64 Development
    
    - name: Run Tests
      run: |
        UnrealEditor.exe -ExecCmds="Automation RunTests ComfyUIIntegration"
    
    - name: Package Plugin
      run: |
        ./package.sh
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: ComfyUIIntegration-Plugin
        path: Packages/
```

### 分发和安装

#### 1. 手动安装

```
1. 下载插件包
2. 解压到项目的 Plugins 目录
3. 重新生成项目文件
4. 编译项目
5. 在编辑器中启用插件
```

#### 2. 自动安装脚本

```bash
#!/bin/bash
# install.sh

PLUGIN_NAME="ComfyUIIntegration"
PROJECT_DIR="$1"

if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: $0 <project_directory>"
    exit 1
fi

PLUGINS_DIR="$PROJECT_DIR/Plugins"
PLUGIN_DIR="$PLUGINS_DIR/$PLUGIN_NAME"

# 创建插件目录
mkdir -p "$PLUGIN_DIR"

# 复制插件文件
cp -r Source "$PLUGIN_DIR/"
cp -r Resources "$PLUGIN_DIR/"
cp -r Config "$PLUGIN_DIR/"
cp ComfyUIIntegration.uplugin "$PLUGIN_DIR/"

echo "Plugin installed successfully to $PLUGIN_DIR"
echo "Please regenerate project files and rebuild the project"
```

#### 3. 卸载脚本

```bash
#!/bin/bash
# uninstall.sh

PLUGIN_NAME="ComfyUIIntegration"
PROJECT_DIR="$1"

if [ -z "$PROJECT_DIR" ]; then
    echo "Usage: $0 <project_directory>"
    exit 1
fi

PLUGIN_DIR="$PROJECT_DIR/Plugins/$PLUGIN_NAME"

if [ -d "$PLUGIN_DIR" ]; then
    rm -rf "$PLUGIN_DIR"
    echo "Plugin uninstalled successfully"
else
    echo "Plugin not found at $PLUGIN_DIR"
fi
```

## 最佳实践

### 1. 代码组织
- 保持模块化设计
- 使用适当的命名约定
- 添加充分的注释和文档

### 2. 性能优化
- 使用预编译头文件
- 优化包含路径
- 避免不必要的依赖

### 3. 平台兼容性
- 测试所有支持的平台
- 使用平台无关的API
- 处理平台特定的差异

### 4. 版本控制
- 使用语义化版本控制
- 维护变更日志
- 标记重要的里程碑

### 5. 文档维护
- 保持文档同步更新
- 提供完整的API文档
- 包含使用示例和教程
