# FComfyUIIntegrationStyle - 样式系统模块开发文档

## 概述

`FComfyUIIntegrationStyle`是ComfyUI Integration插件的样式系统，负责管理插件的视觉外观、图标资源和UI样式。该系统基于Unreal Engine的Slate样式框架，提供了统一的视觉设计语言。

## 文件位置

- **头文件**: `Source/ComfyUIIntegration/Public/ComfyUIIntegrationStyle.h`
- **实现文件**: `Source/ComfyUIIntegration/Private/ComfyUIIntegrationStyle.cpp`

## 类结构

```cpp
class FComfyUIIntegrationStyle
{
public:
    // 生命周期管理
    static void Initialize();
    static void Shutdown();
    
    // 资源管理
    static void ReloadTextures();
    static const ISlateStyle& Get();
    static FName GetStyleSetName();

private:
    // 样式创建
    static TSharedRef<FSlateStyleSet> Create();
    
    // 成员变量
    static TSharedPtr<FSlateStyleSet> StyleInstance;
    
    // 样式名称常量
    static const FName StyleSetName;
};
```

## 核心功能

### 1. 样式系统初始化

#### Initialize()

```cpp
void FComfyUIIntegrationStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
        
        UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Integration style initialized"));
    }
}
```

**功能描述**：
- 初始化样式系统
- 创建样式集实例
- 注册到Slate样式注册表
- 确保只初始化一次

**调用时机**：
- 在模块启动时调用
- 确保在UI创建之前完成

#### Shutdown()

```cpp
void FComfyUIIntegrationStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        ensure(StyleInstance.IsUnique());
        StyleInstance.Reset();
        
        UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Integration style shutdown"));
    }
}
```

**功能描述**：
- 清理样式系统
- 从注册表中注销样式
- 释放样式实例

**调用时机**：
- 在模块关闭时调用
- 确保资源正确释放

### 2. 样式集创建

#### Create()

```cpp
TSharedRef<FSlateStyleSet> FComfyUIIntegrationStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(StyleSetName));
    
    // 设置内容根目录
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("ComfyUIIntegration")->GetBaseDir() / TEXT("Resources"));
    
    // 定义图标样式
    DefineIconStyles(Style);
    
    // 定义按钮样式
    DefineButtonStyles(Style);
    
    // 定义文本样式
    DefineTextStyles(Style);
    
    // 定义颜色样式
    DefineColorStyles(Style);
    
    // 定义布局样式
    DefineLayoutStyles(Style);
    
    return Style;
}
```

**功能描述**：
- 创建新的样式集实例
- 设置资源根目录
- 定义各种UI元素的样式
- 返回完整的样式集

### 3. 图标样式定义

#### DefineIconStyles()

```cpp
void FComfyUIIntegrationStyle::DefineIconStyles(TSharedRef<FSlateStyleSet> Style)
{
    // 主图标
    Style->Set("ComfyUI.Icon.Main", new IMAGE_BRUSH(TEXT("Icons/ComfyUI_40x"), Icon40x40));
    Style->Set("ComfyUI.Icon.Small", new IMAGE_BRUSH(TEXT("Icons/ComfyUI_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Large", new IMAGE_BRUSH(TEXT("Icons/ComfyUI_64x"), Icon64x64));
    
    // 工作流图标
    Style->Set("ComfyUI.Icon.TextToImage", new IMAGE_BRUSH(TEXT("Icons/TextToImage_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.ImageToImage", new IMAGE_BRUSH(TEXT("Icons/ImageToImage_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.TextTo3D", new IMAGE_BRUSH(TEXT("Icons/TextTo3D_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.ImageTo3D", new IMAGE_BRUSH(TEXT("Icons/ImageTo3D_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.TextureGeneration", new IMAGE_BRUSH(TEXT("Icons/TextureGeneration_16x"), Icon16x16));
    
    // 操作图标
    Style->Set("ComfyUI.Icon.Generate", new IMAGE_BRUSH(TEXT("Icons/Generate_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Save", new IMAGE_BRUSH(TEXT("Icons/Save_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Preview", new IMAGE_BRUSH(TEXT("Icons/Preview_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Settings", new IMAGE_BRUSH(TEXT("Icons/Settings_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Refresh", new IMAGE_BRUSH(TEXT("Icons/Refresh_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Help", new IMAGE_BRUSH(TEXT("Icons/Help_16x"), Icon16x16));
    
    // 状态图标
    Style->Set("ComfyUI.Icon.Connected", new IMAGE_BRUSH(TEXT("Icons/Connected_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Disconnected", new IMAGE_BRUSH(TEXT("Icons/Disconnected_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Loading", new IMAGE_BRUSH(TEXT("Icons/Loading_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Error", new IMAGE_BRUSH(TEXT("Icons/Error_16x"), Icon16x16));
    Style->Set("ComfyUI.Icon.Success", new IMAGE_BRUSH(TEXT("Icons/Success_16x"), Icon16x16));
}
```

**图标规范**：
- **16x16**: 小图标，用于菜单项和按钮
- **40x40**: 中等图标，用于工具栏
- **64x64**: 大图标，用于对话框和设置

**支持的图标格式**：
- PNG（推荐）
- SVG（矢量图标）
- BMP
- TGA

### 4. 按钮样式定义

#### DefineButtonStyles()

```cpp
void FComfyUIIntegrationStyle::DefineButtonStyles(TSharedRef<FSlateStyleSet> Style)
{
    // 主要按钮样式
    FButtonStyle PrimaryButtonStyle = FButtonStyle()
        .SetNormal(BOX_BRUSH("Buttons/PrimaryButton_Normal", FMargin(8.0f / 32.0f)))
        .SetHovered(BOX_BRUSH("Buttons/PrimaryButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/PrimaryButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(BOX_BRUSH("Buttons/PrimaryButton_Disabled", FMargin(8.0f / 32.0f)))
        .SetNormalPadding(FMargin(10.0f, 5.0f))
        .SetPressedPadding(FMargin(10.0f, 6.0f));
    
    Style->Set("ComfyUI.Button.Primary", PrimaryButtonStyle);
    
    // 成功按钮样式
    FButtonStyle SuccessButtonStyle = FButtonStyle()
        .SetNormal(BOX_BRUSH("Buttons/SuccessButton_Normal", FMargin(8.0f / 32.0f)))
        .SetHovered(BOX_BRUSH("Buttons/SuccessButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/SuccessButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(BOX_BRUSH("Buttons/SuccessButton_Disabled", FMargin(8.0f / 32.0f)))
        .SetNormalPadding(FMargin(10.0f, 5.0f))
        .SetPressedPadding(FMargin(10.0f, 6.0f));
    
    Style->Set("ComfyUI.Button.Success", SuccessButtonStyle);
    
    // 警告按钮样式
    FButtonStyle WarningButtonStyle = FButtonStyle()
        .SetNormal(BOX_BRUSH("Buttons/WarningButton_Normal", FMargin(8.0f / 32.0f)))
        .SetHovered(BOX_BRUSH("Buttons/WarningButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/WarningButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(BOX_BRUSH("Buttons/WarningButton_Disabled", FMargin(8.0f / 32.0f)))
        .SetNormalPadding(FMargin(10.0f, 5.0f))
        .SetPressedPadding(FMargin(10.0f, 6.0f));
    
    Style->Set("ComfyUI.Button.Warning", WarningButtonStyle);
    
    // 危险按钮样式
    FButtonStyle DangerButtonStyle = FButtonStyle()
        .SetNormal(BOX_BRUSH("Buttons/DangerButton_Normal", FMargin(8.0f / 32.0f)))
        .SetHovered(BOX_BRUSH("Buttons/DangerButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/DangerButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(BOX_BRUSH("Buttons/DangerButton_Disabled", FMargin(8.0f / 32.0f)))
        .SetNormalPadding(FMargin(10.0f, 5.0f))
        .SetPressedPadding(FMargin(10.0f, 6.0f));
    
    Style->Set("ComfyUI.Button.Danger", DangerButtonStyle);
    
    // 平面按钮样式
    FButtonStyle FlatButtonStyle = FButtonStyle()
        .SetNormal(FSlateNoResource())
        .SetHovered(BOX_BRUSH("Buttons/FlatButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/FlatButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(FSlateNoResource())
        .SetNormalPadding(FMargin(8.0f, 4.0f))
        .SetPressedPadding(FMargin(8.0f, 5.0f));
    
    Style->Set("ComfyUI.Button.Flat", FlatButtonStyle);
}
```

**按钮状态**：
- **Normal**: 正常状态
- **Hovered**: 鼠标悬停状态
- **Pressed**: 按下状态
- **Disabled**: 禁用状态

### 5. 文本样式定义

#### DefineTextStyles()

```cpp
void FComfyUIIntegrationStyle::DefineTextStyles(TSharedRef<FSlateStyleSet> Style)
{
    // 标题文本样式
    FTextBlockStyle TitleTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Bold", 16))
        .SetColorAndOpacity(FLinearColor::White)
        .SetShadowOffset(FVector2D(1.0f, 1.0f))
        .SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
    
    Style->Set("ComfyUI.Text.Title", TitleTextStyle);
    
    // 副标题文本样式
    FTextBlockStyle SubtitleTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Bold", 12))
        .SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
    
    Style->Set("ComfyUI.Text.Subtitle", SubtitleTextStyle);
    
    // 正文文本样式
    FTextBlockStyle BodyTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Regular", 10))
        .SetColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
    
    Style->Set("ComfyUI.Text.Body", BodyTextStyle);
    
    // 提示文本样式
    FTextBlockStyle HintTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Italic", 9))
        .SetColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f));
    
    Style->Set("ComfyUI.Text.Hint", HintTextStyle);
    
    // 错误文本样式
    FTextBlockStyle ErrorTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Regular", 10))
        .SetColorAndOpacity(FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
    
    Style->Set("ComfyUI.Text.Error", ErrorTextStyle);
    
    // 成功文本样式
    FTextBlockStyle SuccessTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Regular", 10))
        .SetColorAndOpacity(FLinearColor(0.3f, 1.0f, 0.3f, 1.0f));
    
    Style->Set("ComfyUI.Text.Success", SuccessTextStyle);
    
    // 警告文本样式
    FTextBlockStyle WarningTextStyle = FTextBlockStyle()
        .SetFont(DEFAULT_FONT("Regular", 10))
        .SetColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.3f, 1.0f));
    
    Style->Set("ComfyUI.Text.Warning", WarningTextStyle);
}
```

**文本样式层级**：
- **Title**: 16px, Bold - 主标题
- **Subtitle**: 12px, Bold - 副标题
- **Body**: 10px, Regular - 正文
- **Hint**: 9px, Italic - 提示文本

### 6. 颜色样式定义

#### DefineColorStyles()

```cpp
void FComfyUIIntegrationStyle::DefineColorStyles(TSharedRef<FSlateStyleSet> Style)
{
    // 主色调
    Style->Set("ComfyUI.Color.Primary", FLinearColor(0.2f, 0.6f, 1.0f, 1.0f));
    Style->Set("ComfyUI.Color.PrimaryDark", FLinearColor(0.1f, 0.4f, 0.8f, 1.0f));
    Style->Set("ComfyUI.Color.PrimaryLight", FLinearColor(0.4f, 0.8f, 1.0f, 1.0f));
    
    // 辅助色调
    Style->Set("ComfyUI.Color.Secondary", FLinearColor(0.6f, 0.2f, 1.0f, 1.0f));
    Style->Set("ComfyUI.Color.Accent", FLinearColor(1.0f, 0.6f, 0.2f, 1.0f));
    
    // 语义色调
    Style->Set("ComfyUI.Color.Success", FLinearColor(0.2f, 0.8f, 0.2f, 1.0f));
    Style->Set("ComfyUI.Color.Warning", FLinearColor(1.0f, 0.8f, 0.2f, 1.0f));
    Style->Set("ComfyUI.Color.Error", FLinearColor(0.8f, 0.2f, 0.2f, 1.0f));
    Style->Set("ComfyUI.Color.Info", FLinearColor(0.2f, 0.8f, 0.8f, 1.0f));
    
    // 中性色调
    Style->Set("ComfyUI.Color.Background", FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
    Style->Set("ComfyUI.Color.Surface", FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
    Style->Set("ComfyUI.Color.Panel", FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
    Style->Set("ComfyUI.Color.Border", FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));
    
    // 文本色调
    Style->Set("ComfyUI.Color.TextPrimary", FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
    Style->Set("ComfyUI.Color.TextSecondary", FLinearColor(0.6f, 0.6f, 0.6f, 1.0f));
    Style->Set("ComfyUI.Color.TextDisabled", FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));
}
```

**颜色设计原则**：
- 主色调用于重要操作和强调
- 语义色调用于状态表示
- 中性色调用于背景和分割
- 确保足够的对比度

### 7. 布局样式定义

#### DefineLayoutStyles()

```cpp
void FComfyUIIntegrationStyle::DefineLayoutStyles(TSharedRef<FSlateStyleSet> Style)
{
    // 面板样式
    FTableRowStyle PanelRowStyle = FTableRowStyle()
        .SetEvenRowBackgroundBrush(BOX_BRUSH("Panels/Panel_Even", FMargin(8.0f / 32.0f)))
        .SetOddRowBackgroundBrush(BOX_BRUSH("Panels/Panel_Odd", FMargin(8.0f / 32.0f)))
        .SetSelectorFocusedBrush(BOX_BRUSH("Panels/Panel_Selected", FMargin(8.0f / 32.0f)))
        .SetActiveBrush(BOX_BRUSH("Panels/Panel_Active", FMargin(8.0f / 32.0f)))
        .SetActiveHoveredBrush(BOX_BRUSH("Panels/Panel_ActiveHovered", FMargin(8.0f / 32.0f)))
        .SetInactiveBrush(BOX_BRUSH("Panels/Panel_Inactive", FMargin(8.0f / 32.0f)))
        .SetInactiveHoveredBrush(BOX_BRUSH("Panels/Panel_InactiveHovered", FMargin(8.0f / 32.0f)));
    
    Style->Set("ComfyUI.Panel.Row", PanelRowStyle);
    
    // 边框样式
    Style->Set("ComfyUI.Border.Default", new BOX_BRUSH("Borders/Border_Default", FMargin(8.0f / 32.0f)));
    Style->Set("ComfyUI.Border.Rounded", new BOX_BRUSH("Borders/Border_Rounded", FMargin(8.0f / 32.0f)));
    Style->Set("ComfyUI.Border.Highlighted", new BOX_BRUSH("Borders/Border_Highlighted", FMargin(8.0f / 32.0f)));
    
    // 分隔符样式
    Style->Set("ComfyUI.Separator.Horizontal", new IMAGE_BRUSH("Separators/Separator_Horizontal", FVector2D(32.0f, 1.0f)));
    Style->Set("ComfyUI.Separator.Vertical", new IMAGE_BRUSH("Separators/Separator_Vertical", FVector2D(1.0f, 32.0f)));
    
    // 滚动条样式
    FScrollBarStyle ScrollBarStyle = FScrollBarStyle()
        .SetVerticalTopSlotImage(IMAGE_BRUSH("ScrollBar/ScrollBar_VerticalTop", FVector2D(8.0f, 8.0f)))
        .SetVerticalBottomSlotImage(IMAGE_BRUSH("ScrollBar/ScrollBar_VerticalBottom", FVector2D(8.0f, 8.0f)))
        .SetHorizontalTopSlotImage(IMAGE_BRUSH("ScrollBar/ScrollBar_HorizontalLeft", FVector2D(8.0f, 8.0f)))
        .SetHorizontalBottomSlotImage(IMAGE_BRUSH("ScrollBar/ScrollBar_HorizontalRight", FVector2D(8.0f, 8.0f)))
        .SetNormalThumbImage(BOX_BRUSH("ScrollBar/ScrollBar_Thumb", FMargin(4.0f / 16.0f)))
        .SetDraggedThumbImage(BOX_BRUSH("ScrollBar/ScrollBar_ThumbDragged", FMargin(4.0f / 16.0f)))
        .SetHoveredThumbImage(BOX_BRUSH("ScrollBar/ScrollBar_ThumbHovered", FMargin(4.0f / 16.0f)));
    
    Style->Set("ComfyUI.ScrollBar", ScrollBarStyle);
}
```

### 8. 资源管理

#### ReloadTextures()

```cpp
void FComfyUIIntegrationStyle::ReloadTextures()
{
    if (StyleInstance.IsValid())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
        UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Integration textures reloaded"));
    }
}
```

**功能描述**：
- 重新加载纹理资源
- 用于热重载和资源更新
- 调试时非常有用

#### GetStyleSetName()

```cpp
FName FComfyUIIntegrationStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("ComfyUIIntegrationStyle"));
    return StyleSetName;
}
```

**功能描述**：
- 返回样式集名称
- 用于样式注册和引用
- 确保名称唯一性

## 样式使用示例

### 1. 在Widget中使用样式

```cpp
// 使用按钮样式
SNew(SButton)
    .ButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Primary"))
    .Text(LOCTEXT("GenerateButton", "生成"))

// 使用文本样式
SNew(STextBlock)
    .TextStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FTextBlockStyle>("ComfyUI.Text.Title"))
    .Text(LOCTEXT("TitleText", "ComfyUI Integration"))

// 使用颜色
SNew(SBorder)
    .BorderBackgroundColor(FComfyUIIntegrationStyle::Get().GetColor("ComfyUI.Color.Primary"))
    .BorderImage(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FBorderStyle>("ComfyUI.Border.Default"))

// 使用图标
SNew(SImage)
    .Image(&FComfyUIIntegrationStyle::Get().GetBrush("ComfyUI.Icon.Generate"))
```

### 2. 动态样式应用

```cpp
// 根据状态动态应用样式
void SComfyUIWidget::UpdateButtonStyle(EButtonState State)
{
    FString StyleName;
    switch (State)
    {
        case EButtonState::Normal:
            StyleName = TEXT("ComfyUI.Button.Primary");
            break;
        case EButtonState::Success:
            StyleName = TEXT("ComfyUI.Button.Success");
            break;
        case EButtonState::Warning:
            StyleName = TEXT("ComfyUI.Button.Warning");
            break;
        case EButtonState::Error:
            StyleName = TEXT("ComfyUI.Button.Danger");
            break;
    }
    
    if (GenerateButton.IsValid())
    {
        GenerateButton->SetButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>(*StyleName));
    }
}
```

## 主题系统

### 1. 主题定义

```cpp
// 主题枚举
enum class EComfyUITheme
{
    Dark,    // 暗色主题
    Light,   // 亮色主题
    Auto     // 自动主题
};

// 主题管理器
class FComfyUIThemeManager
{
public:
    static void SetTheme(EComfyUITheme Theme)
    {
        CurrentTheme = Theme;
        ApplyTheme();
    }
    
    static EComfyUITheme GetCurrentTheme()
    {
        return CurrentTheme;
    }
    
    static void ApplyTheme()
    {
        switch (CurrentTheme)
        {
            case EComfyUITheme::Dark:
                ApplyDarkTheme();
                break;
            case EComfyUITheme::Light:
                ApplyLightTheme();
                break;
            case EComfyUITheme::Auto:
                ApplyAutoTheme();
                break;
        }
    }
    
private:
    static EComfyUITheme CurrentTheme;
    
    static void ApplyDarkTheme()
    {
        // 应用暗色主题
        TSharedPtr<FSlateStyleSet> Style = StaticCastSharedPtr<FSlateStyleSet>(FComfyUIIntegrationStyle::StyleInstance);
        if (Style.IsValid())
        {
            Style->Set("ComfyUI.Color.Background", FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
            Style->Set("ComfyUI.Color.Surface", FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
            Style->Set("ComfyUI.Color.TextPrimary", FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
        }
    }
    
    static void ApplyLightTheme()
    {
        // 应用亮色主题
        TSharedPtr<FSlateStyleSet> Style = StaticCastSharedPtr<FSlateStyleSet>(FComfyUIIntegrationStyle::StyleInstance);
        if (Style.IsValid())
        {
            Style->Set("ComfyUI.Color.Background", FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
            Style->Set("ComfyUI.Color.Surface", FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
            Style->Set("ComfyUI.Color.TextPrimary", FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
        }
    }
    
    static void ApplyAutoTheme()
    {
        // 根据系统设置自动选择主题
        bool bDarkMode = FPlatformMisc::GetSystemDarkMode();
        if (bDarkMode)
        {
            ApplyDarkTheme();
        }
        else
        {
            ApplyLightTheme();
        }
    }
};
```

### 2. 响应式设计

```cpp
// DPI感知样式
class FComfyUIDPIAwareStyle
{
public:
    static void UpdateStyleForDPI(float DPIScale)
    {
        TSharedPtr<FSlateStyleSet> Style = StaticCastSharedPtr<FSlateStyleSet>(FComfyUIIntegrationStyle::StyleInstance);
        if (Style.IsValid())
        {
            // 更新字体大小
            UpdateFontSizes(Style, DPIScale);
            
            // 更新图标大小
            UpdateIconSizes(Style, DPIScale);
            
            // 更新边距和间距
            UpdateMargins(Style, DPIScale);
        }
    }
    
private:
    static void UpdateFontSizes(TSharedPtr<FSlateStyleSet> Style, float DPIScale)
    {
        // 更新标题字体
        FTextBlockStyle TitleStyle = Style->GetWidgetStyle<FTextBlockStyle>("ComfyUI.Text.Title");
        TitleStyle.SetFont(DEFAULT_FONT("Bold", FMath::RoundToInt(16.0f * DPIScale)));
        Style->Set("ComfyUI.Text.Title", TitleStyle);
        
        // 更新其他字体...
    }
    
    static void UpdateIconSizes(TSharedPtr<FSlateStyleSet> Style, float DPIScale)
    {
        // 根据DPI调整图标大小
        FVector2D SmallIconSize = FVector2D(16.0f * DPIScale, 16.0f * DPIScale);
        FVector2D LargeIconSize = FVector2D(32.0f * DPIScale, 32.0f * DPIScale);
        
        // 更新图标资源...
    }
    
    static void UpdateMargins(TSharedPtr<FSlateStyleSet> Style, float DPIScale)
    {
        // 调整边距和间距
        FButtonStyle ButtonStyle = Style->GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Primary");
        ButtonStyle.SetNormalPadding(FMargin(10.0f * DPIScale, 5.0f * DPIScale));
        Style->Set("ComfyUI.Button.Primary", ButtonStyle);
    }
};
```

## 资源管理

### 1. 资源组织结构

```
Resources/
├── Icons/
│   ├── ComfyUI_16x.png
│   ├── ComfyUI_40x.png
│   ├── ComfyUI_64x.png
│   ├── TextToImage_16x.png
│   ├── ImageToImage_16x.png
│   ├── Generate_16x.png
│   └── ...
├── Buttons/
│   ├── PrimaryButton_Normal.png
│   ├── PrimaryButton_Hovered.png
│   ├── PrimaryButton_Pressed.png
│   ├── PrimaryButton_Disabled.png
│   └── ...
├── Borders/
│   ├── Border_Default.png
│   ├── Border_Rounded.png
│   └── ...
├── Panels/
│   ├── Panel_Even.png
│   ├── Panel_Odd.png
│   └── ...
└── ScrollBar/
    ├── ScrollBar_Thumb.png
    ├── ScrollBar_ThumbHovered.png
    └── ...
```

### 2. 资源加载优化

```cpp
// 延迟加载资源
class FComfyUIResourceLoader
{
public:
    static void LoadResourcesAsync()
    {
        // 异步加载非关键资源
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, []()
        {
            LoadIcons();
            LoadTextures();
            LoadFonts();
        });
    }
    
private:
    static void LoadIcons()
    {
        // 加载图标资源
        TArray<FString> IconPaths;
        IconPaths.Add(TEXT("Icons/ComfyUI_16x.png"));
        IconPaths.Add(TEXT("Icons/ComfyUI_40x.png"));
        // ... 更多图标
        
        for (const FString& IconPath : IconPaths)
        {
            LoadIconResource(IconPath);
        }
    }
    
    static void LoadIconResource(const FString& Path)
    {
        // 加载单个图标资源
        FString FullPath = FPaths::Combine(GetResourcePath(), Path);
        
        if (FPaths::FileExists(FullPath))
        {
            // 加载并缓存资源
            CacheResource(Path, FullPath);
        }
    }
    
    static void CacheResource(const FString& Key, const FString& Path)
    {
        // 缓存资源以供后续使用
        static TMap<FString, TSharedPtr<FSlateDynamicImageBrush>> ResourceCache;
        
        if (!ResourceCache.Contains(Key))
        {
            TSharedPtr<FSlateDynamicImageBrush> Brush = MakeShareable(new FSlateDynamicImageBrush(
                FName(*Path),
                FVector2D::ZeroVector,
                FLinearColor::White,
                ESlateBrushTileType::NoTile
            ));
            
            ResourceCache.Add(Key, Brush);
        }
    }
};
```

## 调试和工具

### 1. 样式调试器

```cpp
// 样式调试工具
class FComfyUIStyleDebugger
{
public:
    static void EnableStyleDebugging(bool bEnable)
    {
        if (bEnable)
        {
            // 创建调试窗口
            CreateDebugWindow();
        }
        else
        {
            // 关闭调试窗口
            CloseDebugWindow();
        }
    }
    
private:
    static void CreateDebugWindow()
    {
        TSharedRef<SWindow> DebugWindow = SNew(SWindow)
            .Title(LOCTEXT("StyleDebuggerTitle", "ComfyUI Style Debugger"))
            .ClientSize(FVector2D(800, 600))
            .SupportsMaximize(true)
            .SupportsMinimize(true);
        
        TSharedRef<SWidget> DebugContent = CreateDebugContent();
        DebugWindow->SetContent(DebugContent);
        
        FSlateApplication::Get().AddWindow(DebugWindow);
    }
    
    static TSharedRef<SWidget> CreateDebugContent()
    {
        return SNew(SVerticalBox)
        
        // 样式预览
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            CreateStylePreview()
        ]
        
        // 控制面板
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f)
        [
            CreateControlPanel()
        ];
    }
    
    static TSharedRef<SWidget> CreateStylePreview()
    {
        // 创建样式预览界面
        return SNew(SScrollBox)
        
        // 按钮样式预览
        + SScrollBox::Slot()
        .Padding(10.0f)
        [
            CreateButtonPreview()
        ]
        
        // 文本样式预览
        + SScrollBox::Slot()
        .Padding(10.0f)
        [
            CreateTextPreview()
        ]
        
        // 颜色样式预览
        + SScrollBox::Slot()
        .Padding(10.0f)
        [
            CreateColorPreview()
        ];
    }
    
    static TSharedRef<SWidget> CreateButtonPreview()
    {
        return SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Primary"))
            .Text(LOCTEXT("PrimaryButton", "Primary"))
        ]
        
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Success"))
            .Text(LOCTEXT("SuccessButton", "Success"))
        ]
        
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Warning"))
            .Text(LOCTEXT("WarningButton", "Warning"))
        ]
        
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FComfyUIIntegrationStyle::Get().GetWidgetStyle<FButtonStyle>("ComfyUI.Button.Danger"))
            .Text(LOCTEXT("DangerButton", "Danger"))
        ];
    }
};
```

### 2. 性能监控

```cpp
// 样式性能监控
class FComfyUIStyleProfiler
{
public:
    static void StartProfiling()
    {
        // 开始性能监控
        ProfilingStartTime = FPlatformTime::Seconds();
        ResourceLoadTimes.Reset();
        StyleApplicationTimes.Reset();
    }
    
    static void EndProfiling()
    {
        // 结束性能监控
        double TotalTime = FPlatformTime::Seconds() - ProfilingStartTime;
        
        UE_LOG(LogComfyUIIntegration, Log, TEXT("Style profiling results:"));
        UE_LOG(LogComfyUIIntegration, Log, TEXT("  Total time: %.3f seconds"), TotalTime);
        UE_LOG(LogComfyUIIntegration, Log, TEXT("  Resource loads: %d"), ResourceLoadTimes.Num());
        UE_LOG(LogComfyUIIntegration, Log, TEXT("  Style applications: %d"), StyleApplicationTimes.Num());
        
        // 分析性能瓶颈
        AnalyzePerformanceBottlenecks();
    }
    
    static void RecordResourceLoad(const FString& ResourceName, double LoadTime)
    {
        ResourceLoadTimes.Add(ResourceName, LoadTime);
    }
    
    static void RecordStyleApplication(const FString& StyleName, double ApplicationTime)
    {
        StyleApplicationTimes.Add(StyleName, ApplicationTime);
    }
    
private:
    static double ProfilingStartTime;
    static TMap<FString, double> ResourceLoadTimes;
    static TMap<FString, double> StyleApplicationTimes;
    
    static void AnalyzePerformanceBottlenecks()
    {
        // 分析资源加载瓶颈
        double MaxResourceLoadTime = 0.0;
        FString SlowestResource;
        
        for (const auto& Pair : ResourceLoadTimes)
        {
            if (Pair.Value > MaxResourceLoadTime)
            {
                MaxResourceLoadTime = Pair.Value;
                SlowestResource = Pair.Key;
            }
        }
        
        if (MaxResourceLoadTime > 0.1) // 100ms阈值
        {
            UE_LOG(LogComfyUIIntegration, Warning, TEXT("Slow resource load detected: %s (%.3f seconds)"), 
                *SlowestResource, MaxResourceLoadTime);
        }
        
        // 分析样式应用瓶颈
        double MaxStyleApplicationTime = 0.0;
        FString SlowestStyle;
        
        for (const auto& Pair : StyleApplicationTimes)
        {
            if (Pair.Value > MaxStyleApplicationTime)
            {
                MaxStyleApplicationTime = Pair.Value;
                SlowestStyle = Pair.Key;
            }
        }
        
        if (MaxStyleApplicationTime > 0.01) // 10ms阈值
        {
            UE_LOG(LogComfyUIIntegration, Warning, TEXT("Slow style application detected: %s (%.3f seconds)"), 
                *SlowestStyle, MaxStyleApplicationTime);
        }
    }
};
```

## 扩展指南

### 1. 添加新的样式

```cpp
// 添加新的按钮样式
void FComfyUIIntegrationStyle::DefineCustomButtonStyle(TSharedRef<FSlateStyleSet> Style)
{
    FButtonStyle CustomButtonStyle = FButtonStyle()
        .SetNormal(BOX_BRUSH("Buttons/CustomButton_Normal", FMargin(8.0f / 32.0f)))
        .SetHovered(BOX_BRUSH("Buttons/CustomButton_Hovered", FMargin(8.0f / 32.0f)))
        .SetPressed(BOX_BRUSH("Buttons/CustomButton_Pressed", FMargin(8.0f / 32.0f)))
        .SetDisabled(BOX_BRUSH("Buttons/CustomButton_Disabled", FMargin(8.0f / 32.0f)))
        .SetNormalPadding(FMargin(12.0f, 6.0f))
        .SetPressedPadding(FMargin(12.0f, 7.0f));
    
    Style->Set("ComfyUI.Button.Custom", CustomButtonStyle);
}

// 在Create()函数中调用
DefineCustomButtonStyle(Style);
```

### 2. 创建自定义Widget样式

```cpp
// 为自定义Widget创建样式
USTRUCT()
struct FComfyUICustomWidgetStyle : public FSlateWidgetStyle
{
    GENERATED_BODY()

    FComfyUICustomWidgetStyle();
    virtual ~FComfyUICustomWidgetStyle() {}

    // FSlateWidgetStyle interface
    virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
    virtual const FName GetTypeName() const override { return TypeName; }
    static const FComfyUICustomWidgetStyle& GetDefault();

    // 样式属性
    UPROPERTY(EditAnywhere, Category = Appearance)
    FSlateBrush BackgroundBrush;

    UPROPERTY(EditAnywhere, Category = Appearance)
    FSlateColor TextColor;

    UPROPERTY(EditAnywhere, Category = Appearance)
    FSlateFontInfo Font;

    static const FName TypeName;
};

// 在样式系统中注册
void FComfyUIIntegrationStyle::DefineCustomWidgetStyle(TSharedRef<FSlateStyleSet> Style)
{
    const FComfyUICustomWidgetStyle CustomWidgetStyle = FComfyUICustomWidgetStyle()
        .SetBackgroundBrush(BOX_BRUSH("Widgets/CustomWidget_Background", FMargin(8.0f / 32.0f)))
        .SetTextColor(FSlateColor(FLinearColor::White))
        .SetFont(DEFAULT_FONT("Regular", 12));
    
    Style->Set("ComfyUI.Widget.Custom", CustomWidgetStyle);
}
```

## 最佳实践

1. **资源优化**：使用适当的图像格式和尺寸
2. **主题一致性**：保持颜色和样式的一致性
3. **可访问性**：确保足够的对比度和字体大小
4. **性能考虑**：避免过度复杂的样式和大量资源
5. **扩展性**：设计易于扩展的样式系统
6. **调试友好**：提供样式调试工具和性能监控
7. **文档化**：为自定义样式提供详细文档
8. **版本控制**：合理管理样式资源的版本
