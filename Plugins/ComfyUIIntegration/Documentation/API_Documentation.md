# ComfyUI Integration Plugin API 文档

## 概述

本文档详细介绍了ComfyUI Integration插件的所有模块、主要函数和暴露的API。该插件为Unreal Engine 5编辑器提供了与ComfyUI服务器的集成功能，支持AI驱动的内容生成。

## 目录

1. [FComfyUIIntegrationModule - 主模块](#fcomfyuiintegrationmodule---主模块)
2. [UComfyUIClient - HTTP客户端](#ucomfyuiclient---http客户端)
3. [SComfyUIWidget - 主界面Widget](#scomfyuiwidget---主界面widget)
4. [FComfyUIIntegrationCommands - 命令系统](#fcomfyuiintegrationcommands---命令系统)
5. [FComfyUIIntegrationStyle - 样式系统](#fcomfyuiintegrationstyle---样式系统)
6. [配置和数据结构](#配置和数据结构)
7. [使用示例](#使用示例)

---

## FComfyUIIntegrationModule - 主模块

### 概述
主模块负责插件的生命周期管理、菜单注册和UI界面的创建。

### 头文件位置
`Source/ComfyUIIntegration/Public/ComfyUIIntegration.h`

### 主要功能

#### 模块生命周期管理
- **插件初始化**：注册样式、命令和菜单
- **UI集成**：将ComfyUI工具窗口集成到UE5编辑器中
- **资源管理**：管理插件资源和清理

### 公共API

#### 核心函数

```cpp
// 模块启动
virtual void StartupModule() override;
```
**功能**：初始化插件模块，注册样式、命令和菜单
**调用时机**：插件加载时自动调用
**无返回值**

```cpp
// 模块关闭  
virtual void ShutdownModule() override;
```
**功能**：清理插件资源，注销菜单和命令
**调用时机**：插件卸载时自动调用
**无返回值**

#### 私有函数

```cpp
void RegisterMenus();
```
**功能**：注册插件菜单到UE5编辑器菜单栏
**用途**：在Level Editor的Window菜单中添加ComfyUI选项

```cpp
void UnregisterMenus();
```
**功能**：注销插件菜单
**用途**：插件卸载时清理菜单

```cpp
TSharedRef<SDockTab> OnSpawnComfyUITab(const FSpawnTabArgs& SpawnTabArgs);
```
**功能**：创建ComfyUI工具窗口的Tab
**返回值**：包含ComfyUI主界面Widget的SDockTab
**用途**：响应用户打开ComfyUI工具窗口的操作

### 日志系统

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogComfyUIIntegration, Log, All);
```
**功能**：声明插件专用的日志分类
**用途**：统一管理插件的调试和错误信息

### 使用示例

```cpp
// 在其他模块中使用日志
UE_LOG(LogComfyUIIntegration, Warning, TEXT("ComfyUI连接失败"));

// 获取模块实例
FComfyUIIntegrationModule& ComfyUIModule = FModuleManager::LoadModuleChecked<FComfyUIIntegrationModule>("ComfyUIIntegration");
```

---

## UComfyUIClient - HTTP客户端

### 概述
负责与ComfyUI服务器的HTTP通信，包括提交工作流、查询状态和下载生成的图像。

### 头文件位置
`Source/ComfyUIIntegration/Public/ComfyUIClient.h`

### 主要功能

#### 网络通信
- **HTTP请求管理**：处理与ComfyUI服务器的所有HTTP通信
- **工作流执行**：支持多种AI生成工作流
- **状态轮询**：监控生成任务的进度
- **图像下载**：获取生成结果并转换为UE5纹理

#### 工作流支持
- **文生图**：基于文本提示生成图像
- **图生图**：基于输入图像和提示生成新图像
- **文生3D**：生成3D模型（实验性）
- **图生3D**：从图像生成3D模型（实验性）
- **纹理生成**：生成游戏纹理

### 公共API

#### 配置函数

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void SetServerUrl(const FString& Url);
```
**功能**：设置ComfyUI服务器URL
**参数**：
- `Url`：服务器地址（例如："http://127.0.0.1:8188"）
**Blueprint支持**：是
**用途**：允许用户配置自定义的ComfyUI服务器地址

#### 核心生成函数

```cpp
void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                   SComfyUIWidget::EWorkflowType WorkflowType, 
                   const FOnImageGenerated& OnComplete);
```
**功能**：执行图像生成任务
**参数**：
- `Prompt`：正面提示词
- `NegativePrompt`：负面提示词
- `WorkflowType`：工作流类型枚举
- `OnComplete`：完成回调函数
**返回值**：无（异步操作）
**用途**：核心的AI生成功能

#### 状态查询函数

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void CheckServerStatus(const FOnImageGeneratedDynamic& OnComplete);
```
**功能**：检查ComfyUI服务器状态
**参数**：
- `OnComplete`：状态查询完成回调
**Blueprint支持**：是
**用途**：验证服务器连接和可用性

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void GetAvailableWorkflows();
```
**功能**：获取服务器可用的工作流列表
**Blueprint支持**：是
**用途**：动态发现可用的AI工作流

### 回调委托

```cpp
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnImageGeneratedDynamic, UTexture2D*, GeneratedImage);
```
**功能**：动态委托，用于Blueprint回调
**参数**：
- `GeneratedImage`：生成的纹理对象

### 私有API

#### 工作流管理

```cpp
struct FWorkflowConfig
{
    FString Name;           // 工作流名称
    FString JsonTemplate;   // JSON模板
    TMap<FString, FString> Parameters;  // 参数映射
};
```
**功能**：工作流配置结构
**用途**：存储预定义的工作流模板

```cpp
void InitializeWorkflowConfigs();
```
**功能**：初始化预定义工作流配置
**用途**：设置支持的AI生成工作流

```cpp
FString BuildWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                         SComfyUIWidget::EWorkflowType WorkflowType);
```
**功能**：构建工作流JSON数据
**参数**：提示词和工作流类型
**返回值**：ComfyUI API格式的JSON字符串
**用途**：将用户输入转换为ComfyUI可执行的工作流

#### HTTP响应处理

```cpp
void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```
**功能**：处理工作流提交响应
**用途**：解析服务器返回的任务ID

```cpp
void OnImageDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```
**功能**：处理图像下载响应
**用途**：将下载的图像数据转换为UE5纹理

```cpp
void OnQueueStatusChecked(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```
**功能**：处理队列状态查询响应
**用途**：监控生成任务进度

#### 工具函数

```cpp
UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);
```
**功能**：从图像数据创建UE5纹理
**参数**：
- `ImageData`：原始图像数据
**返回值**：UTexture2D对象
**用途**：将下载的图像转换为可在UE5中使用的纹理

```cpp
void PollGenerationStatus(const FString& PromptId);
```
**功能**：轮询生成任务状态
**参数**：
- `PromptId`：任务ID
**用途**：定期检查生成任务进度

### 使用示例

```cpp
// 创建客户端实例
UComfyUIClient* Client = NewObject<UComfyUIClient>();

// 设置服务器
Client->SetServerUrl(TEXT("http://127.0.0.1:8188"));

// 生成图像
FOnImageGenerated OnComplete;
OnComplete.BindUObject(this, &AMyActor::OnImageGenerated);

Client->GenerateImage(
    TEXT("a beautiful landscape, highly detailed"),
    TEXT("blurry, low quality"),
    SComfyUIWidget::EWorkflowType::TextToImage,
    OnComplete
);
```

---

## SComfyUIWidget - 主界面Widget

### 概述
使用Slate框架构建的主用户界面，提供直观的AI内容生成交互体验。

### 头文件位置
`Source/ComfyUIIntegration/Public/ComfyUIWidget.h`

### 主要功能

#### 用户界面组件
- **工作流选择**：下拉菜单选择AI生成类型
- **提示词输入**：文本框输入正面和负面提示
- **服务器配置**：设置ComfyUI服务器地址
- **控制按钮**：生成、保存、预览等操作按钮
- **图像预览**：显示生成结果的预览窗口

#### 交互功能
- **实时预览**：即时显示生成的图像
- **资产保存**：将生成的纹理保存到项目中
- **参数调整**：可视化调整生成参数

### 公共API

#### 工作流类型枚举

```cpp
enum class EWorkflowType
{
    TextToImage,      // 文生图
    ImageToImage,     // 图生图
    TextTo3D,         // 文生3D
    ImageTo3D,        // 图生3D
    TextureGeneration // 纹理生成
};
```
**功能**：定义支持的AI生成工作流类型
**用途**：界面工作流选择和API调用

#### 构造函数

```cpp
void Construct(const FArguments& InArgs);
```
**功能**：构造Slate Widget界面
**参数**：
- `InArgs`：Slate参数结构
**用途**：创建和布局UI组件

### 私有API

#### UI创建函数

```cpp
TSharedRef<SWidget> CreateWorkflowSelectionWidget();
```
**功能**：创建工作流选择下拉菜单
**返回值**：工作流选择Widget
**用途**：让用户选择AI生成类型

```cpp
TSharedRef<SWidget> CreatePromptInputWidget();
```
**功能**：创建提示词输入界面
**返回值**：提示词输入Widget
**用途**：允许用户输入正面和负面提示

```cpp
TSharedRef<SWidget> CreateServerConfigWidget();
```
**功能**：创建服务器配置界面
**返回值**：服务器配置Widget
**用途**：设置ComfyUI服务器地址

```cpp
TSharedRef<SWidget> CreateControlButtonsWidget();
```
**功能**：创建控制按钮栏
**返回值**：按钮栏Widget
**用途**：生成、保存、预览等操作按钮

```cpp
TSharedRef<SWidget> CreateImagePreviewWidget();
```
**功能**：创建图像预览区域
**返回值**：图像预览Widget
**用途**：显示生成的图像结果

#### 事件处理函数

```cpp
FReply OnGenerateClicked();
```
**功能**：处理生成按钮点击事件
**返回值**：事件处理结果
**用途**：启动AI图像生成流程

```cpp
FReply OnSaveClicked();
```
**功能**：处理保存按钮点击事件
**返回值**：事件处理结果
**用途**：保存生成的纹理到项目中

```cpp
FReply OnSaveAsClicked();
```
**功能**：处理另存为按钮点击事件
**返回值**：事件处理结果
**用途**：以指定名称保存纹理

```cpp
FReply OnPreviewClicked();
```
**功能**：处理预览按钮点击事件
**返回值**：事件处理结果
**用途**：在更大窗口中预览生成结果

#### ComboBox事件处理

```cpp
TSharedRef<SWidget> OnGenerateWorkflowTypeWidget(TSharedPtr<EWorkflowType> InOption);
```
**功能**：生成工作流类型选项的Widget
**参数**：
- `InOption`：工作流类型选项
**返回值**：选项Widget
**用途**：自定义下拉菜单选项显示

```cpp
void OnWorkflowTypeChanged(TSharedPtr<EWorkflowType> NewSelection, ESelectInfo::Type SelectInfo);
```
**功能**：处理工作流类型变更事件
**参数**：
- `NewSelection`：新选择的工作流类型
- `SelectInfo`：选择信息
**用途**：响应用户工作流类型变更

```cpp
FText GetCurrentWorkflowTypeText() const;
```
**功能**：获取当前选中工作流类型的文本
**返回值**：工作流类型显示文本
**用途**：更新UI显示

#### 工具函数

```cpp
FText WorkflowTypeToText(EWorkflowType Type) const;
```
**功能**：将工作流类型枚举转换为显示文本
**参数**：
- `Type`：工作流类型枚举
**返回值**：本地化的显示文本
**用途**：界面文本显示

```cpp
void OnImageGenerationComplete(UTexture2D* GeneratedImage);
```
**功能**：处理图像生成完成回调
**参数**：
- `GeneratedImage`：生成的纹理对象
**用途**：更新预览界面和启用保存功能

### 委托定义

```cpp
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*);
```
**功能**：图像生成完成委托
**参数**：
- `UTexture2D*`：生成的纹理对象
**用途**：回调通知图像生成完成

### 使用示例

```cpp
// 创建Widget实例
TSharedRef<SComfyUIWidget> ComfyUIWidget = SNew(SComfyUIWidget);

// 添加到Tab中
return SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
        ComfyUIWidget
    ];
```

---

## FComfyUIIntegrationCommands - 命令系统

### 概述
管理插件的UI命令，包括菜单项和快捷键绑定。

### 头文件位置
`Source/ComfyUIIntegration/Public/ComfyUIIntegrationCommands.h`

### 主要功能

#### 命令管理
- **菜单命令**：定义插件菜单项
- **快捷键**：支持键盘快捷键
- **本地化**：支持多语言命令文本

### 公共API

#### 构造函数

```cpp
FComfyUIIntegrationCommands()
    : TCommands<FComfyUIIntegrationCommands>(TEXT("ComfyUIIntegration"), 
        NSLOCTEXT("Contexts", "ComfyUIIntegration", "ComfyUI Integration Plugin"), 
        NAME_None, FComfyUIIntegrationStyle::GetStyleSetName())
```
**功能**：初始化命令系统
**参数**：
- 命令上下文名称
- 本地化显示名称
- 父命令集
- 关联的样式集

#### 命令注册

```cpp
virtual void RegisterCommands() override;
```
**功能**：注册所有插件命令
**用途**：定义可用的UI命令和快捷键

#### 命令定义

```cpp
TSharedPtr<FUICommandInfo> OpenPluginWindow;
```
**功能**：打开插件窗口命令
**用途**：从菜单或快捷键打开ComfyUI工具窗口

### 使用示例

```cpp
// 注册命令
FComfyUIIntegrationCommands::Register();

// 映射命令到执行函数
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().OpenPluginWindow,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::PluginButtonClicked),
    FCanExecuteAction());

// 添加到菜单
Section.AddMenuEntryWithCommandList(
    FComfyUIIntegrationCommands::Get().OpenPluginWindow, 
    PluginCommands);
```

---

## FComfyUIIntegrationStyle - 样式系统

### 概述
管理插件的视觉样式，包括图标、颜色和布局样式。

### 头文件位置
`Source/ComfyUIIntegration/Public/ComfyUIIntegrationStyle.h`

### 主要功能

#### 样式管理
- **图标资源**：管理插件图标和图形资源
- **颜色主题**：定义插件UI的颜色方案
- **布局样式**：设置Widget的默认样式

### 公共API

#### 生命周期管理

```cpp
static void Initialize();
```
**功能**：初始化样式系统
**用途**：加载插件样式资源

```cpp
static void Shutdown();
```
**功能**：关闭样式系统
**用途**：释放样式资源

#### 资源管理

```cpp
static void ReloadTextures();
```
**功能**：重新加载纹理资源
**用途**：动态更新样式资源

```cpp
static const ISlateStyle& Get();
```
**功能**：获取样式集实例
**返回值**：样式集引用
**用途**：访问定义的样式

```cpp
static FName GetStyleSetName();
```
**功能**：获取样式集名称
**返回值**：样式集名称
**用途**：样式集标识

### 私有API

```cpp
static TSharedRef<FSlateStyleSet> Create();
```
**功能**：创建样式集实例
**返回值**：新的样式集
**用途**：初始化插件样式定义

### 使用示例

```cpp
// 初始化样式
FComfyUIIntegrationStyle::Initialize();

// 获取样式
const ISlateStyle& Style = FComfyUIIntegrationStyle::Get();

// 使用样式
SNew(SButton)
    .ButtonStyle(&Style.GetWidgetStyle<FButtonStyle>("ComfyUI.Button"))
```

---

## 配置和数据结构

### 工作流配置

```cpp
struct FWorkflowConfig
{
    FString Name;                           // 工作流名称
    FString JsonTemplate;                   // JSON模板
    TMap<FString, FString> Parameters;      // 参数映射
};
```

### 默认配置文件

位置：`Config/default_config.json`

```json
{
    "server_url": "http://127.0.0.1:8188",
    "default_workflow": "TextToImage",
    "output_directory": "Generated",
    "max_image_size": 2048,
    "timeout_seconds": 300
}
```

---

## 使用示例

### 基本使用流程

```cpp
// 1. 获取模块实例
FComfyUIIntegrationModule& Module = FModuleManager::LoadModuleChecked<FComfyUIIntegrationModule>("ComfyUIIntegration");

// 2. 创建客户端
UComfyUIClient* Client = NewObject<UComfyUIClient>();
Client->SetServerUrl(TEXT("http://127.0.0.1:8188"));

// 3. 设置回调
FOnImageGenerated OnComplete;
OnComplete.BindUObject(this, &AMyActor::OnImageGenerated);

// 4. 生成图像
Client->GenerateImage(
    TEXT("a beautiful sunset over mountains"),
    TEXT("blurry, low quality"),
    SComfyUIWidget::EWorkflowType::TextToImage,
    OnComplete
);

// 5. 处理结果
void AMyActor::OnImageGenerated(UTexture2D* GeneratedTexture)
{
    if (GeneratedTexture)
    {
        // 使用生成的纹理
        MyMaterial->SetTextureParameterValue(TEXT("BaseColor"), GeneratedTexture);
    }
}
```

### Blueprint使用

```cpp
// 在Blueprint中调用
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void GenerateImageFromBlueprint(const FString& Prompt)
{
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    
    FOnImageGeneratedDynamic OnComplete;
    OnComplete.BindUFunction(this, TEXT("OnBlueprintImageGenerated"));
    
    Client->CheckServerStatus(OnComplete);
}
```

---

## 错误处理和调试

### 日志记录

```cpp
// 使用插件日志分类
UE_LOG(LogComfyUIIntegration, Warning, TEXT("服务器连接失败: %s"), *ErrorMessage);
UE_LOG(LogComfyUIIntegration, Error, TEXT("图像生成失败: %s"), *ErrorDetails);
```

### 常见问题

1. **服务器连接失败**：检查ComfyUI服务器是否运行
2. **工作流执行失败**：验证工作流JSON格式
3. **图像下载失败**：检查网络连接和文件权限
4. **内存泄漏**：确保正确释放HTTP请求和纹理资源

---

## 扩展开发

### 添加新工作流类型

1. 在`EWorkflowType`枚举中添加新类型
2. 在`InitializeWorkflowConfigs()`中添加配置
3. 更新UI文本映射
4. 测试新工作流

### 自定义UI组件

```cpp
// 创建自定义Widget
class SCustomComfyUIWidget : public SComfyUIWidget
{
    // 重写UI创建函数
    virtual TSharedRef<SWidget> CreatePromptInputWidget() override;
};
```

### 性能优化建议

1. **HTTP连接池**：重用HTTP连接
2. **纹理缓存**：避免重复创建纹理
3. **异步处理**：使用后台线程处理图像数据
4. **内存管理**：及时释放大型纹理资源

---

## 版本兼容性

- **Unreal Engine**: 5.0+
- **ComfyUI**: 0.1.0+
- **平台支持**: Windows, Mac, Linux

---

## 许可证

本插件采用MIT许可证，详见LICENSE文件。
