# SComfyUIWidget - 主界面Widget模块开发文档

## 概述

`SComfyUIWidget`是ComfyUI Integration插件的主用户界面组件，基于Unreal Engine的Slate UI框架构建。该Widget提供了直观的AI内容生成交互界面，支持多种工作流类型和实时预览功能。

## 文件位置

- **头文件**: `Source/ComfyUIIntegration/Public/ComfyUIWidget.h`
- **实现文件**: `Source/ComfyUIIntegration/Private/ComfyUIWidget.cpp`

## 类结构

```cpp
class COMFYUIINTEGRATION_API SComfyUIWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SComfyUIWidget)
    {}
    SLATE_END_ARGS()

    /** 构造Widget */
    void Construct(const FArguments& InArgs);

    /** 工作流类型枚举 */
    enum class EWorkflowType
    {
        TextToImage,      // 文生图
        ImageToImage,     // 图生图
        TextTo3D,         // 文生3D
        ImageTo3D,        // 图生3D
        TextureGeneration // 纹理生成
    };

private:
    // UI组件
    TSharedPtr<SEditableTextBox> PromptTextBox;
    TSharedPtr<SEditableTextBox> NegativePromptTextBox;
    TSharedPtr<SComboBox<TSharedPtr<EWorkflowType>>> WorkflowTypeComboBox;
    TSharedPtr<SEditableTextBox> ComfyUIServerUrlTextBox;
    TSharedPtr<SButton> GenerateButton;
    TSharedPtr<SButton> SaveButton;
    TSharedPtr<SButton> SaveAsButton;
    TSharedPtr<SButton> PreviewButton;
    TSharedPtr<SImage> ImagePreview;
    
    // 数据成员
    TSharedPtr<EWorkflowType> CurrentWorkflowType;
    TArray<TSharedPtr<EWorkflowType>> WorkflowOptions;
    UTexture2D* GeneratedTexture;
    
    // UI创建函数
    TSharedRef<SWidget> CreateWorkflowSelectionWidget();
    TSharedRef<SWidget> CreatePromptInputWidget();
    TSharedRef<SWidget> CreateServerConfigWidget();
    TSharedRef<SWidget> CreateControlButtonsWidget();
    TSharedRef<SWidget> CreateImagePreviewWidget();
    
    // 事件处理
    FReply OnGenerateClicked();
    FReply OnSaveClicked();
    FReply OnSaveAsClicked();
    FReply OnPreviewClicked();
    
    // ComboBox事件
    TSharedRef<SWidget> OnGenerateWorkflowTypeWidget(TSharedPtr<EWorkflowType> InOption);
    void OnWorkflowTypeChanged(TSharedPtr<EWorkflowType> NewSelection, ESelectInfo::Type SelectInfo);
    FText GetCurrentWorkflowTypeText() const;
    
    // 工具函数
    FText WorkflowTypeToText(EWorkflowType Type) const;
    void OnImageGenerationComplete(UTexture2D* GeneratedImage);
};
```

## 核心功能

### 1. Widget构造和初始化

#### Construct()

```cpp
void SComfyUIWidget::Construct(const FArguments& InArgs)
{
    // 初始化工作流选项
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextToImage)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::ImageToImage)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextTo3D)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::ImageTo3D)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextureGeneration)));
    
    // 设置默认选择
    CurrentWorkflowType = WorkflowOptions[0];
    
    // 构建主界面
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // 工作流选择区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            CreateWorkflowSelectionWidget()
        ]
        
        // 提示词输入区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            CreatePromptInputWidget()
        ]
        
        // 服务器配置区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            CreateServerConfigWidget()
        ]
        
        // 控制按钮区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            CreateControlButtonsWidget()
        ]
        
        // 图像预览区域
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10.0f)
        [
            CreateImagePreviewWidget()
        ]
    ];
}
```

**功能描述**：
- 初始化工作流选项列表
- 设置默认工作流类型
- 构建主界面布局
- 创建各个功能区域的Widget

### 2. 界面布局组件

#### CreateWorkflowSelectionWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::CreateWorkflowSelectionWidget()
{
    return SNew(SVerticalBox)
    
    // 标题
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 5.0f)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("WorkflowTypeLabel", "工作流类型"))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
    ]
    
    // 下拉选择框
    + SVerticalBox::Slot()
    .AutoHeight()
    [
        SAssignNew(WorkflowTypeComboBox, SComboBox<TSharedPtr<EWorkflowType>>)
        .OptionsSource(&WorkflowOptions)
        .OnGenerateWidget(this, &SComfyUIWidget::OnGenerateWorkflowTypeWidget)
        .OnSelectionChanged(this, &SComfyUIWidget::OnWorkflowTypeChanged)
        .InitiallySelectedItem(CurrentWorkflowType)
        [
            SNew(STextBlock)
            .Text(this, &SComfyUIWidget::GetCurrentWorkflowTypeText)
        ]
    ];
}
```

**功能描述**：
- 创建工作流类型选择界面
- 提供下拉菜单选择不同的AI生成类型
- 支持动态更新选择项

**支持的工作流类型**：
- **TextToImage**：文本生成图像
- **ImageToImage**：图像到图像转换
- **TextTo3D**：文本生成3D模型
- **ImageTo3D**：图像生成3D模型
- **TextureGeneration**：纹理生成

#### CreatePromptInputWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::CreatePromptInputWidget()
{
    return SNew(SVerticalBox)
    
    // 正面提示词
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 5.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PromptLabel", "提示词"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SAssignNew(PromptTextBox, SEditableTextBox)
            .HintText(LOCTEXT("PromptHint", "描述您想要生成的内容..."))
            .MinDesiredWidth(400.0f)
            .Padding(5.0f)
        ]
    ]
    
    // 负面提示词
    + SVerticalBox::Slot()
    .AutoHeight()
    [
        SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 5.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NegativePromptLabel", "负面提示词"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SAssignNew(NegativePromptTextBox, SEditableTextBox)
            .HintText(LOCTEXT("NegativePromptHint", "描述您不想要的内容..."))
            .MinDesiredWidth(400.0f)
            .Padding(5.0f)
        ]
    ];
}
```

**功能描述**：
- 创建提示词输入界面
- 包含正面和负面提示词输入框
- 提供输入提示文本

**输入验证**：
- 检查提示词长度限制
- 过滤特殊字符
- 提供实时输入反馈

#### CreateServerConfigWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::CreateServerConfigWidget()
{
    return SNew(SHorizontalBox)
    
    // 服务器URL标签
    + SHorizontalBox::Slot()
    .AutoWidth()
    .VAlign(VAlign_Center)
    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("ServerUrlLabel", "服务器地址:"))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
    ]
    
    // 服务器URL输入框
    + SHorizontalBox::Slot()
    .FillWidth(1.0f)
    .VAlign(VAlign_Center)
    [
        SAssignNew(ComfyUIServerUrlTextBox, SEditableTextBox)
        .Text(FText::FromString(TEXT("http://127.0.0.1:8188")))
        .HintText(LOCTEXT("ServerUrlHint", "ComfyUI服务器地址"))
        .MinDesiredWidth(300.0f)
        .Padding(5.0f)
    ]
    
    // 连接测试按钮
    + SHorizontalBox::Slot()
    .AutoWidth()
    .VAlign(VAlign_Center)
    .Padding(10.0f, 0.0f, 0.0f, 0.0f)
    [
        SNew(SButton)
        .Text(LOCTEXT("TestConnectionButton", "测试连接"))
        .OnClicked(this, &SComfyUIWidget::OnTestConnectionClicked)
        .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
    ];
}
```

**功能描述**：
- 创建服务器配置界面
- 允许用户设置ComfyUI服务器地址
- 提供连接测试功能

**配置验证**：
- URL格式验证
- 连接可用性测试
- 保存用户配置

#### CreateControlButtonsWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::CreateControlButtonsWidget()
{
    return SNew(SHorizontalBox)
    
    // 生成按钮
    + SHorizontalBox::Slot()
    .AutoWidth()
    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
    [
        SAssignNew(GenerateButton, SButton)
        .Text(LOCTEXT("GenerateButton", "生成"))
        .OnClicked(this, &SComfyUIWidget::OnGenerateClicked)
        .ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
        .ContentPadding(FMargin(20.0f, 5.0f))
    ]
    
    // 保存按钮
    + SHorizontalBox::Slot()
    .AutoWidth()
    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
    [
        SAssignNew(SaveButton, SButton)
        .Text(LOCTEXT("SaveButton", "保存"))
        .OnClicked(this, &SComfyUIWidget::OnSaveClicked)
        .ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
        .ContentPadding(FMargin(20.0f, 5.0f))
        .IsEnabled(false) // 初始禁用
    ]
    
    // 另存为按钮
    + SHorizontalBox::Slot()
    .AutoWidth()
    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
    [
        SAssignNew(SaveAsButton, SButton)
        .Text(LOCTEXT("SaveAsButton", "另存为"))
        .OnClicked(this, &SComfyUIWidget::OnSaveAsClicked)
        .ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
        .ContentPadding(FMargin(20.0f, 5.0f))
        .IsEnabled(false) // 初始禁用
    ]
    
    // 预览按钮
    + SHorizontalBox::Slot()
    .AutoWidth()
    [
        SAssignNew(PreviewButton, SButton)
        .Text(LOCTEXT("PreviewButton", "预览"))
        .OnClicked(this, &SComfyUIWidget::OnPreviewClicked)
        .ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
        .ContentPadding(FMargin(20.0f, 5.0f))
        .IsEnabled(false) // 初始禁用
    ];
}
```

**功能描述**：
- 创建控制按钮界面
- 提供生成、保存、预览等操作
- 根据状态动态启用/禁用按钮

**按钮状态管理**：
- **生成按钮**：始终可用
- **保存按钮**：生成完成后启用
- **另存为按钮**：生成完成后启用
- **预览按钮**：生成完成后启用

#### CreateImagePreviewWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::CreateImagePreviewWidget()
{
    return SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
    .Padding(5.0f)
    [
        SNew(SVerticalBox)
        
        // 预览标题
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 5.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PreviewLabel", "预览"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            .Justification(ETextJustify::Center)
        ]
        
        // 图像预览区域
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SAssignNew(ImagePreview, SImage)
            .Image(FAppStyle::GetBrush("Icons.Help"))
            .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f))
        ]
    ];
}
```

**功能描述**：
- 创建图像预览界面
- 显示生成的图像结果
- 提供占位符图像

**预览功能**：
- 实时显示生成结果
- 支持缩放和平移
- 显示图像信息

### 3. 事件处理系统

#### OnGenerateClicked()

```cpp
FReply SComfyUIWidget::OnGenerateClicked()
{
    // 获取用户输入
    FString Prompt = PromptTextBox->GetText().ToString();
    FString NegativePrompt = NegativePromptTextBox->GetText().ToString();
    FString ServerUrl = ComfyUIServerUrlTextBox->GetText().ToString();
    
    // 验证输入
    if (Prompt.IsEmpty())
    {
        // 显示错误提示
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("EmptyPromptError", "请输入提示词"));
        return FReply::Handled();
    }
    
    if (ServerUrl.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("EmptyServerUrlError", "请输入服务器地址"));
        return FReply::Handled();
    }
    
    // 禁用生成按钮防止重复点击
    GenerateButton->SetEnabled(false);
    GenerateButton->SetText(LOCTEXT("GeneratingText", "生成中..."));
    
    // 创建ComfyUI客户端
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    Client->SetServerUrl(ServerUrl);
    
    // 设置完成回调
    FOnImageGenerated OnComplete;
    OnComplete.BindSP(this, &SComfyUIWidget::OnImageGenerationComplete);
    
    // 开始生成
    Client->GenerateImage(Prompt, NegativePrompt, *CurrentWorkflowType, OnComplete);
    
    return FReply::Handled();
}
```

**功能描述**：
- 处理生成按钮点击事件
- 验证用户输入
- 创建ComfyUI客户端并开始生成
- 更新UI状态

**输入验证**：
- 检查提示词是否为空
- 验证服务器地址格式
- 检查工作流类型有效性

#### OnSaveClicked()

```cpp
FReply SComfyUIWidget::OnSaveClicked()
{
    if (!GeneratedTexture)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoTextureError", "没有可保存的图像"));
        return FReply::Handled();
    }
    
    // 生成默认文件名
    FString DefaultName = FString::Printf(TEXT("ComfyUI_Generated_%s"), 
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    
    // 保存纹理到项目
    SaveTextureToProject(GeneratedTexture, DefaultName);
    
    // 显示成功提示
    FNotificationInfo Info(LOCTEXT("SaveSuccessText", "图像已保存"));
    Info.ExpireDuration = 3.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}
```

**功能描述**：
- 处理保存按钮点击事件
- 验证是否有可保存的纹理
- 生成默认文件名并保存到项目
- 显示保存结果通知

#### OnSaveAsClicked()

```cpp
FReply SComfyUIWidget::OnSaveAsClicked()
{
    if (!GeneratedTexture)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoTextureError", "没有可保存的图像"));
        return FReply::Handled();
    }
    
    // 打开文件保存对话框
    TArray<FString> OutFileNames;
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    
    if (DesktopPlatform)
    {
        const FString FileTypes = TEXT("PNG文件|*.png|JPEG文件|*.jpg|所有文件|*.*");
        
        bool bSaved = DesktopPlatform->SaveFileDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
            LOCTEXT("SaveAsDialogTitle", "另存为").ToString(),
            TEXT(""),
            TEXT("ComfyUI_Generated.png"),
            FileTypes,
            EFileDialogFlags::None,
            OutFileNames
        );
        
        if (bSaved && OutFileNames.Num() > 0)
        {
            SaveTextureToFile(GeneratedTexture, OutFileNames[0]);
        }
    }
    
    return FReply::Handled();
}
```

**功能描述**：
- 处理另存为按钮点击事件
- 打开文件保存对话框
- 支持多种图像格式
- 保存到用户指定位置

#### OnPreviewClicked()

```cpp
FReply SComfyUIWidget::OnPreviewClicked()
{
    if (!GeneratedTexture)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoTextureError", "没有可预览的图像"));
        return FReply::Handled();
    }
    
    // 创建预览窗口
    TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
        .Title(LOCTEXT("PreviewWindowTitle", "图像预览"))
        .ClientSize(FVector2D(800, 600))
        .IsInitiallyMaximized(false)
        .SupportsMaximize(true)
        .SupportsMinimize(true);
    
    // 创建预览内容
    TSharedRef<SWidget> PreviewContent = CreatePreviewContent(GeneratedTexture);
    
    PreviewWindow->SetContent(PreviewContent);
    
    // 显示窗口
    FSlateApplication::Get().AddWindow(PreviewWindow);
    
    return FReply::Handled();
}
```

**功能描述**：
- 处理预览按钮点击事件
- 创建独立的预览窗口
- 显示高质量图像预览
- 支持缩放和平移操作

### 4. ComboBox事件处理

#### OnGenerateWorkflowTypeWidget()

```cpp
TSharedRef<SWidget> SComfyUIWidget::OnGenerateWorkflowTypeWidget(TSharedPtr<EWorkflowType> InOption)
{
    return SNew(STextBlock)
        .Text(WorkflowTypeToText(*InOption))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10));
}
```

**功能描述**：
- 生成工作流类型选项的Widget
- 自定义下拉菜单项的显示
- 支持本地化文本

#### OnWorkflowTypeChanged()

```cpp
void SComfyUIWidget::OnWorkflowTypeChanged(TSharedPtr<EWorkflowType> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        CurrentWorkflowType = NewSelection;
        
        // 根据工作流类型更新UI
        UpdateUIForWorkflowType(*NewSelection);
        
        // 记录选择变更
        UE_LOG(LogComfyUIIntegration, Log, TEXT("工作流类型已更改为: %s"), 
            *WorkflowTypeToText(*NewSelection).ToString());
    }
}
```

**功能描述**：
- 处理工作流类型变更事件
- 更新相关UI元素
- 记录用户选择

#### GetCurrentWorkflowTypeText()

```cpp
FText SComfyUIWidget::GetCurrentWorkflowTypeText() const
{
    if (CurrentWorkflowType.IsValid())
    {
        return WorkflowTypeToText(*CurrentWorkflowType);
    }
    return LOCTEXT("NoWorkflowSelected", "未选择工作流");
}
```

**功能描述**：
- 获取当前工作流类型的显示文本
- 用于ComboBox的当前选择显示
- 支持本地化

### 5. 工具函数

#### WorkflowTypeToText()

```cpp
FText SComfyUIWidget::WorkflowTypeToText(EWorkflowType Type) const
{
    switch (Type)
    {
        case EWorkflowType::TextToImage:
            return LOCTEXT("TextToImageWorkflow", "文生图");
        case EWorkflowType::ImageToImage:
            return LOCTEXT("ImageToImageWorkflow", "图生图");
        case EWorkflowType::TextTo3D:
            return LOCTEXT("TextTo3DWorkflow", "文生3D");
        case EWorkflowType::ImageTo3D:
            return LOCTEXT("ImageTo3DWorkflow", "图生3D");
        case EWorkflowType::TextureGeneration:
            return LOCTEXT("TextureGenerationWorkflow", "纹理生成");
        default:
            return LOCTEXT("UnknownWorkflow", "未知工作流");
    }
}
```

**功能描述**：
- 将工作流类型枚举转换为本地化文本
- 支持多语言界面
- 提供未知类型的默认处理

#### OnImageGenerationComplete()

```cpp
void SComfyUIWidget::OnImageGenerationComplete(UTexture2D* GeneratedImage)
{
    // 恢复生成按钮状态
    GenerateButton->SetEnabled(true);
    GenerateButton->SetText(LOCTEXT("GenerateButton", "生成"));
    
    if (GeneratedImage)
    {
        // 保存生成的纹理
        GeneratedTexture = GeneratedImage;
        
        // 更新预览图像
        UpdatePreviewImage(GeneratedImage);
        
        // 启用相关按钮
        SaveButton->SetEnabled(true);
        SaveAsButton->SetEnabled(true);
        PreviewButton->SetEnabled(true);
        
        // 显示成功通知
        FNotificationInfo Info(LOCTEXT("GenerationSuccessText", "图像生成成功"));
        Info.ExpireDuration = 3.0f;
        Info.bFireAndForget = true;
        FSlateNotificationManager::Get().AddNotification(Info);
        
        UE_LOG(LogComfyUIIntegration, Log, TEXT("图像生成成功"));
    }
    else
    {
        // 显示错误通知
        FNotificationInfo Info(LOCTEXT("GenerationFailedText", "图像生成失败"));
        Info.ExpireDuration = 5.0f;
        Info.bFireAndForget = true;
        FSlateNotificationManager::Get().AddNotification(Info);
        
        UE_LOG(LogComfyUIIntegration, Error, TEXT("图像生成失败"));
    }
}
```

**功能描述**：
- 处理图像生成完成回调
- 更新UI状态和按钮可用性
- 显示成功或失败通知
- 更新预览图像

## 委托定义

### FOnImageGenerated

```cpp
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*);
```

**功能描述**：
- 图像生成完成委托
- 用于回调通知图像生成结果

**使用示例**：
```cpp
// 绑定成员函数
FOnImageGenerated OnComplete;
OnComplete.BindSP(this, &SComfyUIWidget::OnImageGenerationComplete);

// 绑定Lambda表达式
OnComplete.BindLambda([this](UTexture2D* GeneratedTexture)
{
    // 处理生成结果
    HandleGeneratedTexture(GeneratedTexture);
});
```

## 样式和主题

### 样式定义

```cpp
// 按钮样式
const FSlateBrush* GetButtonBrush(const FString& ButtonType) const
{
    if (ButtonType == TEXT("Primary"))
    {
        return FAppStyle::GetBrush("FlatButton.Primary");
    }
    else if (ButtonType == TEXT("Success"))
    {
        return FAppStyle::GetBrush("FlatButton.Success");
    }
    else if (ButtonType == TEXT("Warning"))
    {
        return FAppStyle::GetBrush("FlatButton.Warning");
    }
    else if (ButtonType == TEXT("Danger"))
    {
        return FAppStyle::GetBrush("FlatButton.Danger");
    }
    else
    {
        return FAppStyle::GetBrush("FlatButton.Default");
    }
}

// 文本样式
const FSlateFontInfo& GetTextFont(const FString& FontType) const
{
    if (FontType == TEXT("Title"))
    {
        return FCoreStyle::GetDefaultFontStyle("Bold", 16);
    }
    else if (FontType == TEXT("Subtitle"))
    {
        return FCoreStyle::GetDefaultFontStyle("Bold", 12);
    }
    else if (FontType == TEXT("Body"))
    {
        return FCoreStyle::GetDefaultFontStyle("Regular", 10);
    }
    else
    {
        return FCoreStyle::GetDefaultFontStyle("Regular", 9);
    }
}
```

### 颜色主题

```cpp
// 颜色定义
const FLinearColor PrimaryColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
const FLinearColor SuccessColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
const FLinearColor WarningColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
const FLinearColor DangerColor = FLinearColor(0.8f, 0.2f, 0.2f, 1.0f);
const FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
```

## 本地化支持

### 本地化宏定义

```cpp
#define LOCTEXT_NAMESPACE "SComfyUIWidget"

// 使用示例
LOCTEXT("GenerateButton", "生成")
LOCTEXT("SaveButton", "保存")
LOCTEXT("PreviewButton", "预览")
LOCTEXT("WorkflowTypeLabel", "工作流类型")
LOCTEXT("PromptLabel", "提示词")
LOCTEXT("NegativePromptLabel", "负面提示词")
LOCTEXT("ServerUrlLabel", "服务器地址")

#undef LOCTEXT_NAMESPACE
```

### 多语言支持

```cpp
// 根据当前语言设置返回相应文本
FText GetLocalizedText(const FString& Key) const
{
    const FString CurrentCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
    
    if (CurrentCulture.StartsWith(TEXT("zh")))
    {
        // 中文文本
        return GetChineseText(Key);
    }
    else if (CurrentCulture.StartsWith(TEXT("ja")))
    {
        // 日文文本
        return GetJapaneseText(Key);
    }
    else
    {
        // 默认英文文本
        return GetEnglishText(Key);
    }
}
```

## 响应式设计

### 动态布局

```cpp
// 根据窗口大小调整布局
void SComfyUIWidget::OnWindowSizeChanged(const FVector2D& NewSize)
{
    if (NewSize.X < 600.0f)
    {
        // 紧凑布局
        SetCompactLayout();
    }
    else if (NewSize.X < 1000.0f)
    {
        // 标准布局
        SetStandardLayout();
    }
    else
    {
        // 宽屏布局
        SetWideLayout();
    }
}

// 紧凑布局
void SComfyUIWidget::SetCompactLayout()
{
    // 垂直排列所有元素
    // 减少边距和间距
    // 使用较小的字体
}

// 标准布局
void SComfyUIWidget::SetStandardLayout()
{
    // 默认的布局配置
    // 平衡的间距和大小
}

// 宽屏布局
void SComfyUIWidget::SetWideLayout()
{
    // 水平排列某些元素
    // 增加边距和间距
    // 使用较大的预览区域
}
```

### 自适应字体

```cpp
// 根据DPI设置字体大小
float GetScaledFontSize(float BaseSize) const
{
    float DPIScale = FSlateApplication::Get().GetApplicationScale();
    return BaseSize * DPIScale;
}

// 自适应字体样式
FSlateFontInfo GetAdaptiveFontStyle(const FString& FontType) const
{
    FSlateFontInfo FontInfo = GetTextFont(FontType);
    FontInfo.Size = GetScaledFontSize(FontInfo.Size);
    return FontInfo;
}
```

## 状态管理

### UI状态枚举

```cpp
enum class EUIState
{
    Idle,           // 空闲状态
    Generating,     // 生成中
    Processing,     // 处理中
    Completed,      // 完成
    Error           // 错误状态
};
```

### 状态更新函数

```cpp
void SComfyUIWidget::UpdateUIState(EUIState NewState)
{
    CurrentUIState = NewState;
    
    switch (NewState)
    {
        case EUIState::Idle:
            GenerateButton->SetEnabled(true);
            GenerateButton->SetText(LOCTEXT("GenerateButton", "生成"));
            SaveButton->SetEnabled(false);
            SaveAsButton->SetEnabled(false);
            PreviewButton->SetEnabled(false);
            break;
            
        case EUIState::Generating:
            GenerateButton->SetEnabled(false);
            GenerateButton->SetText(LOCTEXT("GeneratingText", "生成中..."));
            SaveButton->SetEnabled(false);
            SaveAsButton->SetEnabled(false);
            PreviewButton->SetEnabled(false);
            break;
            
        case EUIState::Completed:
            GenerateButton->SetEnabled(true);
            GenerateButton->SetText(LOCTEXT("GenerateButton", "生成"));
            SaveButton->SetEnabled(true);
            SaveAsButton->SetEnabled(true);
            PreviewButton->SetEnabled(true);
            break;
            
        case EUIState::Error:
            GenerateButton->SetEnabled(true);
            GenerateButton->SetText(LOCTEXT("GenerateButton", "生成"));
            SaveButton->SetEnabled(false);
            SaveAsButton->SetEnabled(false);
            PreviewButton->SetEnabled(false);
            break;
    }
}
```

## 性能优化

### 图像预览优化

```cpp
// 使用缓存减少重复渲染
TMap<UTexture2D*, TSharedPtr<FSlateBrush>> PreviewBrushCache;

TSharedPtr<FSlateBrush> GetPreviewBrush(UTexture2D* Texture)
{
    if (PreviewBrushCache.Contains(Texture))
    {
        return PreviewBrushCache[Texture];
    }
    
    // 创建新的画刷
    TSharedPtr<FSlateBrush> NewBrush = MakeShareable(new FSlateBrush());
    NewBrush->SetResourceObject(Texture);
    
    // 缓存画刷
    PreviewBrushCache.Add(Texture, NewBrush);
    
    return NewBrush;
}
```

### 内存管理

```cpp
// 清理不再使用的纹理
void SComfyUIWidget::CleanupUnusedTextures()
{
    // 移除缓存中的无效纹理
    for (auto It = PreviewBrushCache.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()))
        {
            It.RemoveCurrent();
        }
    }
    
    // 清理过期的通知
    FSlateNotificationManager::Get().ClearNotifications();
}
```

## 扩展指南

### 添加新的UI组件

```cpp
// 1. 在类中声明新的Widget指针
TSharedPtr<SMyCustomWidget> CustomWidget;

// 2. 在CreateCustomSectionWidget()中创建Widget
TSharedRef<SWidget> SComfyUIWidget::CreateCustomSectionWidget()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .AutoHeight()
    [
        SAssignNew(CustomWidget, SMyCustomWidget)
        .OnValueChanged(this, &SComfyUIWidget::OnCustomValueChanged)
    ];
}

// 3. 在Construct()中添加到主布局
+ SVerticalBox::Slot()
.AutoHeight()
.Padding(10.0f)
[
    CreateCustomSectionWidget()
]
```

### 添加新的工作流类型

```cpp
// 1. 在枚举中添加新类型
enum class EWorkflowType
{
    // 现有类型...
    VideoGeneration,    // 新增：视频生成
    AudioGeneration     // 新增：音频生成
};

// 2. 在WorkflowTypeToText()中添加文本映射
case EWorkflowType::VideoGeneration:
    return LOCTEXT("VideoGenerationWorkflow", "视频生成");
case EWorkflowType::AudioGeneration:
    return LOCTEXT("AudioGenerationWorkflow", "音频生成");

// 3. 在InitializeWorkflowConfigs()中添加到选项列表
WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::VideoGeneration)));
WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::AudioGeneration)));
```

### 自定义主题

```cpp
// 创建自定义主题类
class FComfyUITheme
{
public:
    static void ApplyDarkTheme(TSharedRef<SComfyUIWidget> Widget)
    {
        // 应用暗色主题
        Widget->SetBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
        Widget->SetTextColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
    }
    
    static void ApplyLightTheme(TSharedRef<SComfyUIWidget> Widget)
    {
        // 应用亮色主题
        Widget->SetBackgroundColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
        Widget->SetTextColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
    }
};
```

## 调试和测试

### 调试UI布局

```cpp
// 启用Slate调试工具
void SComfyUIWidget::EnableDebugMode(bool bEnable)
{
    if (bEnable)
    {
        // 显示Widget边界
        FSlateApplication::Get().EnableVisualDebugger();
        
        // 打印Widget层次结构
        PrintWidgetHierarchy();
    }
}

// 打印Widget层次结构
void SComfyUIWidget::PrintWidgetHierarchy() const
{
    UE_LOG(LogComfyUIIntegration, Log, TEXT("Widget层次结构:"));
    PrintWidgetHierarchyRecursive(AsShared(), 0);
}

void SComfyUIWidget::PrintWidgetHierarchyRecursive(TSharedRef<SWidget> Widget, int32 Depth) const
{
    FString Indent = FString::ChrN(Depth * 2, TEXT(' '));
    UE_LOG(LogComfyUIIntegration, Log, TEXT("%s%s"), *Indent, *Widget->GetTypeAsString());
    
    // 递归打印子Widget
    FChildren* Children = Widget->GetChildren();
    if (Children)
    {
        for (int32 i = 0; i < Children->Num(); i++)
        {
            PrintWidgetHierarchyRecursive(Children->GetChildAt(i), Depth + 1);
        }
    }
}
```

### 单元测试

```cpp
// 创建测试用例
class FComfyUIWidgetTest
{
public:
    static bool TestWorkflowTypeConversion()
    {
        SComfyUIWidget TestWidget;
        
        // 测试所有工作流类型的文本转换
        FText TextToImageText = TestWidget.WorkflowTypeToText(SComfyUIWidget::EWorkflowType::TextToImage);
        if (TextToImageText.IsEmpty())
        {
            return false;
        }
        
        // 更多测试...
        return true;
    }
    
    static bool TestUIStateTransitions()
    {
        SComfyUIWidget TestWidget;
        
        // 测试UI状态转换
        TestWidget.UpdateUIState(SComfyUIWidget::EUIState::Generating);
        if (TestWidget.GenerateButton->IsEnabled())
        {
            return false; // 生成中应该禁用按钮
        }
        
        // 更多测试...
        return true;
    }
};
```

## 最佳实践

1. **响应式设计**：支持不同分辨率和DPI设置
2. **性能优化**：使用缓存和延迟加载减少资源消耗
3. **错误处理**：提供友好的错误信息和恢复机制
4. **可访问性**：支持键盘导航和屏幕阅读器
5. **本地化**：支持多语言界面
6. **用户体验**：提供进度反馈和状态提示
7. **内存管理**：及时清理不再使用的资源
8. **代码组织**：将UI逻辑分离到独立的函数中
