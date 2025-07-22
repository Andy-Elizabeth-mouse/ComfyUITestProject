# FComfyUIIntegrationModule - 主模块开发文档

## 概述

`FComfyUIIntegrationModule`是ComfyUI Integration插件的核心模块，负责插件的生命周期管理、UI集成和编辑器扩展。该模块继承自`IModuleInterface`，实现了Unreal Engine的标准模块接口。

## 文件位置

- **头文件**: `Source/ComfyUIIntegration/Public/ComfyUIIntegration.h`
- **实现文件**: `Source/ComfyUIIntegration/Private/ComfyUIIntegration.cpp`

## 类结构

```cpp
class FComfyUIIntegrationModule : public IModuleInterface
{
public:
    // IModuleInterface 实现
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 菜单管理
    void RegisterMenus();
    void UnregisterMenus();
    void RegisterMenusInternal();
    void PluginButtonClicked();
    
    // Tab管理
    TSharedRef<SDockTab> OnSpawnComfyUITab(const FSpawnTabArgs& SpawnTabArgs);
    
    // 成员变量
    TSharedPtr<FUICommandList> PluginCommands;
};
```

## 核心功能

### 1. 模块生命周期管理

#### StartupModule()

```cpp
virtual void StartupModule() override;
```

**功能描述**：
- 初始化插件模块
- 注册样式系统
- 注册命令系统
- 注册编辑器菜单
- 注册Tab生成器

**执行流程**：
1. 记录模块加载日志
2. 初始化样式系统 (`FComfyUIIntegrationStyle::Initialize()`)
3. 重新加载纹理资源 (`FComfyUIIntegrationStyle::ReloadTextures()`)
4. 注册命令 (`FComfyUIIntegrationCommands::Register()`)
5. 创建命令列表并映射操作
6. 注册编辑器菜单 (`RegisterMenus()`)
7. 注册Tab生成器到全局Tab管理器

**关键代码**：
```cpp
void FComfyUIIntegrationModule::StartupModule()
{
    UE_LOG(LogComfyUIIntegration, Warning, TEXT("ComfyUI Integration module has been loaded"));
    
    // 初始化样式
    FComfyUIIntegrationStyle::Initialize();
    FComfyUIIntegrationStyle::ReloadTextures();

    // 初始化命令
    FComfyUIIntegrationCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::PluginButtonClicked),
        FCanExecuteAction());

    // 注册菜单和Tab
    RegisterMenus();
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ComfyUIIntegrationTabName,
        FOnSpawnTab::CreateRaw(this, &FComfyUIIntegrationModule::OnSpawnComfyUITab))
        .SetDisplayName(LOCTEXT("FComfyUIIntegrationTabTitle", "ComfyUI Integration"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}
```

#### ShutdownModule()

```cpp
virtual void ShutdownModule() override;
```

**功能描述**：
- 清理插件资源
- 注销Tab生成器
- 注销菜单
- 关闭样式系统
- 注销命令系统

**执行流程**：
1. 记录模块卸载日志
2. 注销Tab生成器
3. 注销菜单 (`UnregisterMenus()`)
4. 关闭样式系统 (`FComfyUIIntegrationStyle::Shutdown()`)
5. 注销命令 (`FComfyUIIntegrationCommands::Unregister()`)

### 2. 菜单系统集成

#### RegisterMenus()

```cpp
void RegisterMenus();
```

**功能描述**：
- 注册插件菜单到UE5编辑器
- 仅在非命令行模式下执行

**实现原理**：
- 使用`UToolMenus::RegisterStartupCallback`注册启动回调
- 回调函数`RegisterMenusInternal()`执行实际的菜单注册

#### RegisterMenusInternal()

```cpp
void RegisterMenusInternal();
```

**功能描述**：
- 将ComfyUI菜单项添加到Level Editor主菜单
- 菜单位置：`LevelEditor.MainMenu.Window`

**关键代码**：
```cpp
void FComfyUIIntegrationModule::RegisterMenusInternal()
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    
    Section.AddMenuEntryWithCommandList(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow, 
        PluginCommands);
}
```

#### UnregisterMenus()

```cpp
void UnregisterMenus();
```

**功能描述**：
- 注销启动回调
- 注销菜单所有者

### 3. Tab管理系统

#### OnSpawnComfyUITab()

```cpp
TSharedRef<SDockTab> OnSpawnComfyUITab(const FSpawnTabArgs& SpawnTabArgs);
```

**功能描述**：
- 创建ComfyUI工具窗口的Tab
- 返回包含主界面Widget的SDockTab

**参数**：
- `SpawnTabArgs`：Tab生成参数

**返回值**：
- `TSharedRef<SDockTab>`：新创建的Tab引用

**关键代码**：
```cpp
TSharedRef<SDockTab> FComfyUIIntegrationModule::OnSpawnComfyUITab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SComfyUIWidget)
        ];
}
```

#### PluginButtonClicked()

```cpp
void PluginButtonClicked();
```

**功能描述**：
- 响应菜单点击事件
- 调用Tab管理器打开ComfyUI工具窗口

**实现**：
```cpp
void FComfyUIIntegrationModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ComfyUIIntegrationTabName);
}
```

## 日志系统

### 日志分类声明

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogComfyUIIntegration, Log, All);
```

**功能描述**：
- 声明插件专用的日志分类
- 日志级别：Log（默认）
- 编译时级别：All（所有级别）

### 日志使用示例

```cpp
// 信息日志
UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI客户端已连接"));

// 警告日志
UE_LOG(LogComfyUIIntegration, Warning, TEXT("ComfyUI服务器响应缓慢"));

// 错误日志
UE_LOG(LogComfyUIIntegration, Error, TEXT("ComfyUI连接失败：%s"), *ErrorMessage);

// 详细日志
UE_LOG(LogComfyUIIntegration, Verbose, TEXT("发送HTTP请求：%s"), *RequestUrl);
```

## 常量定义

### Tab名称常量

```cpp
static const FName ComfyUIIntegrationTabName("ComfyUIIntegration");
```

**功能描述**：
- 定义ComfyUI工具窗口的Tab名称
- 用于Tab注册和调用

### 本地化文本命名空间

```cpp
#define LOCTEXT_NAMESPACE "FComfyUIIntegrationModule"
```

**功能描述**：
- 定义本地化文本的命名空间
- 支持多语言界面

## 依赖关系

### 头文件依赖

```cpp
#include "ComfyUIIntegration.h"
#include "ComfyUIIntegrationStyle.h"
#include "ComfyUIIntegrationCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "ComfyUIWidget.h"
```

### 模块依赖

- **CoreMinimal**：核心UE5功能
- **LevelEditor**：编辑器集成
- **Slate**：UI框架
- **ToolMenus**：菜单系统
- **EditorStyle**：编辑器样式

## 使用示例

### 获取模块实例

```cpp
// 方法1：通过模块管理器
FComfyUIIntegrationModule& Module = FModuleManager::LoadModuleChecked<FComfyUIIntegrationModule>("ComfyUIIntegration");

// 方法2：检查模块是否加载
if (FModuleManager::Get().IsModuleLoaded("ComfyUIIntegration"))
{
    FComfyUIIntegrationModule& Module = FModuleManager::GetModuleChecked<FComfyUIIntegrationModule>("ComfyUIIntegration");
}
```

### 手动打开工具窗口

```cpp
// 直接调用Tab管理器
FGlobalTabmanager::Get()->TryInvokeTab(FName("ComfyUIIntegration"));

// 或者调用模块方法
FComfyUIIntegrationModule& Module = FModuleManager::LoadModuleChecked<FComfyUIIntegrationModule>("ComfyUIIntegration");
Module.PluginButtonClicked();
```

## 扩展指南

### 添加新的菜单项

1. 在`FComfyUIIntegrationCommands`中定义新命令
2. 在`RegisterMenusInternal()`中添加菜单项
3. 在`StartupModule()`中映射命令到处理函数

```cpp
// 在Commands中定义
TSharedPtr<FUICommandInfo> OpenSettingsWindow;

// 在RegisterMenusInternal中添加
Section.AddMenuEntryWithCommandList(
    FComfyUIIntegrationCommands::Get().OpenSettingsWindow, 
    PluginCommands);

// 在StartupModule中映射
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().OpenSettingsWindow,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenSettingsWindow),
    FCanExecuteAction());
```

### 添加新的工具窗口

1. 定义新的Tab名称常量
2. 创建Tab生成器函数
3. 在`StartupModule()`中注册Tab生成器

```cpp
// 定义Tab名称
static const FName ComfyUISettingsTabName("ComfyUISettings");

// 创建生成器函数
TSharedRef<SDockTab> OnSpawnSettingsTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SComfyUISettingsWidget)
        ];
}

// 注册Tab生成器
FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ComfyUISettingsTabName,
    FOnSpawnTab::CreateRaw(this, &FComfyUIIntegrationModule::OnSpawnSettingsTab))
    .SetDisplayName(LOCTEXT("ComfyUISettingsTabTitle", "ComfyUI Settings"));
```

## 性能考虑

### 内存管理

- 使用`TSharedPtr`管理Slate Widget生命周期
- 及时清理`FUICommandList`和事件绑定
- 在`ShutdownModule()`中正确释放资源

### 启动性能

- 避免在`StartupModule()`中执行耗时操作
- 延迟加载非必要资源
- 使用启动回调机制避免阻塞编辑器启动

## 调试技巧

### 日志调试

```cpp
// 开启详细日志
UE_LOG(LogComfyUIIntegration, VeryVerbose, TEXT("模块启动详细信息"));

// 条件日志
if (GEngine)
{
    UE_LOG(LogComfyUIIntegration, Warning, TEXT("引擎已初始化"));
}
```

### 模块状态检查

```cpp
// 检查模块是否已加载
bool bIsLoaded = FModuleManager::Get().IsModuleLoaded("ComfyUIIntegration");

// 检查Tab是否已注册
bool bIsTabRegistered = FGlobalTabmanager::Get()->HasTabSpawner(ComfyUIIntegrationTabName);
```

## 常见问题

### 1. 模块加载失败

**问题**：模块无法正常加载
**解决方案**：
- 检查`.uplugin`文件中的模块配置
- 确认模块名称拼写正确
- 检查构建文件依赖关系

### 2. 菜单项不显示

**问题**：ComfyUI菜单项没有出现在编辑器菜单中
**解决方案**：
- 确认`RegisterMenus()`正确调用
- 检查菜单路径是否正确
- 验证命令是否正确注册

### 3. Tab无法打开

**问题**：点击菜单项后工具窗口没有打开
**解决方案**：
- 确认Tab生成器已注册
- 检查Tab名称是否匹配
- 验证Widget构造函数是否正确

## 版本兼容性

### Unreal Engine版本支持

- **UE 5.0+**：完全支持
- **UE 4.27**：需要修改部分API调用
- **UE 4.26及以下**：不支持

### 平台兼容性

- **Windows**：完全支持
- **Mac**：完全支持
- **Linux**：完全支持
- **Mobile**：编辑器插件，不适用

## 最佳实践

1. **错误处理**：所有公共函数都应包含适当的错误处理
2. **日志记录**：记录关键操作和错误信息
3. **资源管理**：正确使用智能指针管理资源生命周期
4. **本地化**：所有用户可见文本都应支持本地化
5. **性能优化**：避免在主线程执行耗时操作
