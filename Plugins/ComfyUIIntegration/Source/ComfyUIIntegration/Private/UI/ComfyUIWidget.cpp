#include "UI/ComfyUIWidget.h"
#include "UI/SImageDragDropWidget.h"
#include "UI/ComfyUI3DPreviewViewport.h"
#include "Client/ComfyUIClient.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "Workflow/ComfyUIWorkflowService.h"
#include "Workflow/ComfyUIWorkflowManager.h"
#include "Workflow/ComfyUIWorkflowExecutor.h"
#include "Utils/ComfyUIFileManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UnrealEngine.h"
#include "Misc/EngineVersionComparison.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateImageBrush.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
// 图像处理相关
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
// 资产管理相关
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/TextureFactory.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"

#define LOCTEXT_NAMESPACE "ComfyUIWidget"

SComfyUIWidget::~SComfyUIWidget()
{
    // 取消正在进行的生成
    if (CurrentClient)
    {
        CurrentClient->CancelCurrentGeneration();
    }
    
    // 清理3D预览视口
    if (ModelViewport.IsValid())
    {
        ModelViewport->ClearPreview();
    }
    
    // 清理生成的内容引用
    GeneratedTexture = nullptr;
    GeneratedMesh = nullptr;
    
    // 清理当前图像画刷
    CurrentImageBrush.Reset();
    
    UE_LOG(LogTemp, Log, TEXT("SComfyUIWidget destroyed"));
}

void SComfyUIWidget::Construct(const FArguments& InArgs)
{
    // 初始化成员变量
    GeneratedTexture = nullptr;
    CurrentImageBrush = nullptr;
    bIsGenerating = false;
    
    // 初始化工作流选项
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::TextToImage)));
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::ImageToImage)));
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::TextTo3D)));
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::ImageTo3D)));
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::MeshTexturing)));
    WorkflowOptions.Add(MakeShareable(new EComfyUIWorkflowType(EComfyUIWorkflowType::Custom)));
    
    // 设置默认选择
    CurrentWorkflowType = WorkflowOptions[0];
    
    // 刷新自定义工作流列表
    RefreshCustomWorkflowList();

    ChildSlot
    [
        SNew(SVerticalBox)
        
        // 标题
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ComfyUIIntegrationTitle", "ComfyUI 集成"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
            .Justification(ETextJustify::Center)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(SSeparator)
        ]

        // 服务器配置区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateServerConfigWidget()
        ]

        // 工作流选择区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateWorkflowSelectionWidget()
        ]

        // 输入图像区域（仅在需要时显示）
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateInputImageWidget()
        ]

        // 模型输入区域（仅在纹理生成时显示）
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateModelInputWidget()
        ]

        // 提示词输入区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreatePromptInputWidget()
        ]

        // 控制按钮区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateControlButtonsWidget()
        ]

        // 进度显示区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            CreateProgressWidget()
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(SSeparator)
        ]

        // 图像预览区域
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(5.0f)
        [
            CreateImagePreviewWidget()
        ]
    ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateServerConfigWidget()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ServerConfigLabel", "ComfyUI 服务器配置"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, 5.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ServerUrlLabel", "服务器URL："))
                .MinDesiredWidth(80.0f)
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(ComfyUIServerUrlTextBox, SEditableTextBox)
                .Text(FText::FromString(TEXT("http://192.168.2.169:8188")))
                .HintText(LOCTEXT("ServerUrlHint", "请输入ComfyUI服务器URL"))
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ConnectionStatusInfo", "服务器连接将在生成图像时自动检测"))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateWorkflowSelectionWidget()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WorkflowSelectionLabel", "工作流类型"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SAssignNew(WorkflowTypeComboBox, SComboBox<TSharedPtr<EComfyUIWorkflowType>>)
            .OptionsSource(&WorkflowOptions)
            .OnGenerateWidget(this, &SComfyUIWidget::OnGeneratEComfyUIWorkflowTypeWidget)
            .OnSelectionChanged(this, &SComfyUIWidget::OnWorkflowTypeChanged)
            .InitiallySelectedItem(CurrentWorkflowType)
            [
                SNew(STextBlock)
                .Text(this, &SComfyUIWidget::GetCurrentWorkflowTypeText)
            ]
        ]
        
        // 自定义工作流选择 (仅在选择Custom时显示)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            .Visibility(this, &SComfyUIWidget::GetCustomWorkflowVisibility)
            
            + SHorizontalBox::Slot()
            .FillWidth(0.7f)
            .Padding(0.0f, 0.0f, 5.0f, 0.0f)
            [
                SAssignNew(CustomWorkflowComboBox, SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&CustomWorkflowNames)
                .OnGenerateWidget(this, &SComfyUIWidget::OnGenerateCustomWorkflowWidget)
                .OnSelectionChanged(this, &SComfyUIWidget::OnCustomWorkflowChanged)
                .InitiallySelectedItem(CurrentCustomWorkflow)
                [
                    SNew(STextBlock)
                    .Text(this, &SComfyUIWidget::GetCustomWorkflowText, CurrentCustomWorkflow)
                ]
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f)
            [
                SAssignNew(RefreshWorkflowsButton, SButton)
                .Text(LOCTEXT("RefreshButton", "刷新"))
                .OnClicked(this, &SComfyUIWidget::OnRefreshWorkflowsClicked)
                .ToolTipText(LOCTEXT("RefreshWorkflowsTooltip", "刷新自定义工作流列表"))
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f)
            [
                SAssignNew(ImportWorkflowButton, SButton)
                .Text(LOCTEXT("ImportButton", "导入"))
                .OnClicked(this, &SComfyUIWidget::OnImportWorkflowClicked)
                .ToolTipText(LOCTEXT("ImportWorkflowTooltip", "导入ComfyUI工作流JSON文件"))
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("ValidateButton", "验证"))
                .OnClicked(this, &SComfyUIWidget::OnValidateWorkflowClicked)
                .ToolTipText(LOCTEXT("ValidateWorkflowTooltip", "验证当前工作流的有效性"))
            ]
        ]
        
        // 工作流类型检测标注（仅在自定义工作流时显示）
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
        [
            SNew(SHorizontalBox)
            .Visibility(this, &SComfyUIWidget::GetCustomWorkflowVisibility)
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, 5.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("DetectedTypeLabel", "检测到的工作流类型："))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(WorkflowTypeLabel, STextBlock)
                .Text(this, &SComfyUIWidget::GetDetectedWorkflowTypeText)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.7f, 0.2f))) // 绿色表示检测结果
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreatePromptInputWidget()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PromptInputLabel", "提示词输入"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            .Visibility(this, &SComfyUIWidget::GetPromptInputVisibility)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PositivePromptLabel", "正面提示词："))
            .Visibility(this, &SComfyUIWidget::GetPromptInputVisibility)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.0f)
            .Visibility(this, &SComfyUIWidget::GetPromptInputVisibility)
            [
                SNew(SBox)
                .MinDesiredHeight(80.0f)
                [
                    SAssignNew(PromptTextBox, SMultiLineEditableTextBox)
                    .HintText(LOCTEXT("PromptHint", "请在这里输入提示词..."))
                ]
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 5.0f, 0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NegativePromptLabel", "负面提示词："))
            .Visibility(this, &SComfyUIWidget::GetPromptInputVisibility)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.0f)
            .Visibility(this, &SComfyUIWidget::GetPromptInputVisibility)
            [
                SNew(SBox)
                .MinDesiredHeight(60.0f)
                [
                    SAssignNew(NegativePromptTextBox, SMultiLineEditableTextBox)
                    .HintText(LOCTEXT("NegativePromptHint", "请在这里输入负面提示词..."))
                ]
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateControlButtonsWidget()
{
    return SNew(SVerticalBox)

        // 生成和取消按钮区域
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(2.0f)
            [
                SAssignNew(GenerateButton, SButton)
                .Text(this, &SComfyUIWidget::GetGenerateButtonText)
                .OnClicked(this, &SComfyUIWidget::OnGenerateClicked)
                .IsEnabled(this, &SComfyUIWidget::IsGenerateButtonEnabled)
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT("GenerateTooltip", "使用当前工作流生成图像"))
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(2.0f)
            [
                SAssignNew(CancelButton, SButton)
                .Text(LOCTEXT("CancelButton", "取消"))
                .OnClicked(this, &SComfyUIWidget::OnCancelClicked)
                .Visibility(this, &SComfyUIWidget::GetCancelButtonVisibility)
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT("CancelTooltip", "取消当前生成任务"))
            ]
        ]

        // 保存和预览按钮区域  
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(2.0f)
            [
                SAssignNew(SaveButton, SButton)
                .Text(LOCTEXT("SaveButton", "保存"))
                .OnClicked(this, &SComfyUIWidget::OnSaveClicked)
                .HAlign(HAlign_Center)
                .IsEnabled(false) // 初始状态禁用
                .ToolTipText(LOCTEXT("SaveTooltip", "保存生成的图片"))
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(2.0f)
            [
                SAssignNew(SaveAsButton, SButton)
                .Text(LOCTEXT("SaveAsButton", "另存为"))
                .OnClicked(this, &SComfyUIWidget::OnSaveAsClicked)
                .HAlign(HAlign_Center)
                .IsEnabled(false) // 初始状态禁用
                .ToolTipText(LOCTEXT("SaveAsTooltip", "将图片另存为指定位置"))
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateInputImageWidget()
{
    return SNew(SVerticalBox)
        .Visibility(this, &SComfyUIWidget::GetInputImageVisibility)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("InputImageLabel", "输入图像"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(0.6f)
            .Padding(0.0f, 0.0f, 5.0f, 0.0f)
            [
                // 使用支持拖拽的新Widget
                SAssignNew(InputImageDragDropWidget, SImageDragDropWidget)
                .PreviewSize(FVector2D(150.0f, 150.0f))
                .OnImageDropped(this, &SComfyUIWidget::OnImageDropped)
            ]
            
            + SHorizontalBox::Slot()
            .FillWidth(0.4f)
            .VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SAssignNew(LoadImageButton, SButton)
                    .Text(LOCTEXT("LoadImageButton", "加载图像"))
                    .OnClicked(this, &SComfyUIWidget::OnLoadImageClicked)
                    .HAlign(HAlign_Center)
                    .ToolTipText(LOCTEXT("LoadImageTooltip", "选择要处理的输入图像，也可以直接从内容浏览器拖拽纹理资产"))
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SAssignNew(ClearImageButton, SButton)
                    .Text(LOCTEXT("ClearImageButton", "清除图像"))
                    .OnClicked(this, &SComfyUIWidget::OnClearImageClicked)
                    .HAlign(HAlign_Center)
                    .IsEnabled(false) // 初始状态禁用
                    .ToolTipText(LOCTEXT("ClearImageTooltip", "清除当前输入图像"))
                ]
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateModelInputWidget()
{
    return SNew(SVerticalBox)
        .Visibility(this, &SComfyUIWidget::GetModelInputVisibility)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ModelInputLabel", "模型输入"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 5.0f, 0.0f)
            [
                SAssignNew(ModelPathTextBox, SEditableTextBox)
                .HintText(LOCTEXT("ModelPathHint", "3D模型文件路径 (支持拖拽)"))
                .Text_Lambda([this]() { return FText::FromString(InputModelPath); })
                .OnTextChanged_Lambda([this](const FText& NewText) {
                    InputModelPath = NewText.ToString();
                })
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f)
            [
                SAssignNew(LoadModelButton, SButton)
                .Text(LOCTEXT("LoadModelButton", "浏览"))
                .OnClicked(this, &SComfyUIWidget::OnLoadModelClicked)
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT("LoadModelTooltip", "选择3D模型文件(.obj/.fbx/.glb/.ply)"))
            ]
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f)
            [
                SAssignNew(ClearModelButton, SButton)
                .Text(LOCTEXT("ClearModelButton", "清除"))
                .OnClicked(this, &SComfyUIWidget::OnClearModelClicked)
                .HAlign(HAlign_Center)
                .IsEnabled(false) // 初始状态禁用
                .ToolTipText(LOCTEXT("ClearModelTooltip", "清除当前3D模型路径"))
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::CreateImagePreviewWidget()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ContentPreviewLabel", "生成内容预览"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(0.0f, 2.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(5.0f)
            [
                // 使用固定高度的SBox来限制预览区域的大小
                SNew(SBox)
                .HeightOverride(300.0f) // 设置预览区域的固定高度
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    // 使用SWidgetSwitcher来在图像和3D模型预览之间切换
                    SAssignNew(ContentSwitcher, SWidgetSwitcher)
                    .WidgetIndex(0) // 默认显示图像预览
                    
                    // 索引0：图像预览
                    + SWidgetSwitcher::Slot()
                    [
                        SAssignNew(ImagePreview, SImage)
                        .Image(FAppStyle::GetBrush("WhiteBrush"))
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                    
                    // 索引1：3D模型预览（使用自定义3D视口）
                    + SWidgetSwitcher::Slot()
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                        .Padding(2.0f)
                        [
                            SNew(SBox)
                            .WidthOverride(280.0f)
                            .HeightOverride(280.0f)
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            [
                                // 创建3D模型预览视口
                                SAssignNew(ModelViewport, SComfyUI3DPreviewViewport)
                            ]
                        ]
                    ]
                    
                    // 索引2：空状态
                    + SWidgetSwitcher::Slot()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("NoContentGenerated", "暂无生成内容"))
                        .Justification(ETextJustify::Center)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                        .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::OnGeneratEComfyUIWorkflowTypeWidget(TSharedPtr<EComfyUIWorkflowType> InOption)
{
    return SNew(STextBlock)
        .Text(WorkflowTypeToText(*InOption.Get()));
}

void SComfyUIWidget::OnWorkflowTypeChanged(TSharedPtr<EComfyUIWorkflowType> NewSelection, ESelectInfo::Type SelectInfo)
{
    CurrentWorkflowType = NewSelection;
    UpdateWorkflowVisibility();
}

FText SComfyUIWidget::GetCurrentWorkflowTypeText() const
{
    if (CurrentWorkflowType.IsValid())
        return WorkflowTypeToText(*CurrentWorkflowType.Get());
    return LOCTEXT("NoWorkflowSelected", "选择工作流");
}

FText SComfyUIWidget::GetGenerateButtonText() const
{
    if (bIsGenerating)
        return LOCTEXT("GeneratingButton", "生成中...");
    return LOCTEXT("GenerateButton", "开始生成");
}

FText SComfyUIWidget::WorkflowTypeToText(EComfyUIWorkflowType Type) const
{
    switch (Type)
    {
    case EComfyUIWorkflowType::TextToImage:
        return LOCTEXT("TextToImage", "文生图");
    case EComfyUIWorkflowType::ImageToImage:
        return LOCTEXT("ImageToImage", "图生图");
    case EComfyUIWorkflowType::TextTo3D:
        return LOCTEXT("TextTo3D", "文生3D模型");
    case EComfyUIWorkflowType::ImageTo3D:
        return LOCTEXT("ImageTo3D", "图生3D模型");
    case EComfyUIWorkflowType::MeshTexturing:
        return LOCTEXT("MeshTexturing", "纹理生成");
    case EComfyUIWorkflowType::Custom:
        return LOCTEXT("CustomWorkflow", "自定义工作流");
    default:
        return LOCTEXT("Unknown", "未知");
    }
}
#pragma optimize("", off)
FReply SComfyUIWidget::OnGenerateClicked()
{
    FString ServerUrl = ComfyUIServerUrlTextBox->GetText().ToString();
    FString Prompt = PromptTextBox->GetText().ToString();
    FString NegativePrompt = NegativePromptTextBox->GetText().ToString();

    // 检查服务器URL是必须的
    if (ServerUrl.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("InvalidServerUrl", "请输入服务器URL"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 检查当前工作流类型是否需要prompt输入
    EComfyUIWorkflowType TypeToCheck = EComfyUIWorkflowType::TextToImage;
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
        TypeToCheck = DetectedCustomWorkflowType;
    else if (CurrentWorkflowType.IsValid())
        TypeToCheck = *CurrentWorkflowType;
    
    // 只有需要prompt的工作流才检查prompt是否为空
    bool bNeedsPrompt = (TypeToCheck == EComfyUIWorkflowType::TextToImage ||
                        TypeToCheck == EComfyUIWorkflowType::ImageToImage ||
                        TypeToCheck == EComfyUIWorkflowType::TextTo3D);
    
    if (bNeedsPrompt && Prompt.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("InvalidPrompt", "请输入提示词"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 检查是否需要图像输入
    bool bNeedsImage = (TypeToCheck == EComfyUIWorkflowType::ImageToImage ||
                       TypeToCheck == EComfyUIWorkflowType::ImageTo3D ||
                       TypeToCheck == EComfyUIWorkflowType::MeshTexturing);
    
    if (bNeedsImage && !InputImage)
    {
        FNotificationInfo Info(LOCTEXT("InvalidImage", "请先加载输入图像"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 检查是否需要3D模型输入
    bool bNeedsModel = (TypeToCheck == EComfyUIWorkflowType::MeshTexturing);
    
    if (bNeedsModel && InputModelPath.IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("InvalidModel", "请先选择3D模型文件"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 检查自定义工作流选择
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
    {
        if (!CurrentCustomWorkflow.IsValid() || CurrentCustomWorkflow->IsEmpty())
        {
            FNotificationInfo Info(LOCTEXT("NoCustomWorkflowSelected", "请选择一个自定义工作流"));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
            return FReply::Handled();
        }
    }

    // 禁用生成按钮防止重复点击，并设置生成状态
    GenerateButton->SetEnabled(false);
    bIsGenerating = true;

    // 获取ComfyUI客户端单例实例
    CurrentClient = UComfyUIClient::GetInstance();
    if (CurrentClient)
    {
        CurrentClient->SetServerUrl(ServerUrl);
        
        // 先测试连接
        CurrentClient->TestServerConnection(FOnConnectionTested::CreateLambda([this, TypeToCheck, Prompt, NegativePrompt](bool bSuccess, FString ErrorMessage)
        {
            if (bSuccess)
            {
                FComfyUIWorkflowExecutor::RunGeneration(
                    TypeToCheck,
                    Prompt,
                    NegativePrompt,
                    InputImage,
                    InputModelPath,
                    CurrentClient,
                    // 回调委托，按正确顺序
                    FOnImageGenerated::CreateSP(this, &SComfyUIWidget::OnImageGenerationComplete),
                    FOnMeshGenerated::CreateSP(this, &SComfyUIWidget::OnMeshGenerationComplete),
                    FOnGenerationProgress::CreateSP(this, &SComfyUIWidget::OnGenerationProgressUpdate),
                    FOnGenerationStarted::CreateSP(this, &SComfyUIWidget::OnGenerationStarted),
                    FOnGenerationFailed(),  // 暂时为空，稍后添加失败处理
                    FOnGenerationCompleted::CreateSP(this, &SComfyUIWidget::OnGenerationCompleted)
                );
            }
            else
            {
                // 连接失败，显示错误并恢复按钮状态
                GenerateButton->SetEnabled(true);
                bIsGenerating = false;
                // 注意：CurrentClient 是单例，不需要设置为 nullptr
                
                FNotificationInfo Info(FText::Format(
                    LOCTEXT("ConnectionFailed", "无法连接到ComfyUI服务器：{0}\n\n请检查：\n• 服务器是否正在运行\n• 网络连接是否正常\n• 服务器地址是否正确\n• 防火墙设置"),
                    FText::FromString(ErrorMessage)
                ));
                Info.ExpireDuration = 8.0f;
                Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
                FSlateNotificationManager::Get().AddNotification(Info);
            }
        }));
    }

    return FReply::Handled();
}
#pragma optimize("", on)
FReply SComfyUIWidget::OnSaveClicked()
{
    // 生成默认文件名（基于时间戳）
    FString DefaultName = FString::Printf(TEXT("ComfyUI_Generated_%s"),
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));

    if (GeneratedTexture)
    {
        // 保存纹理到项目的默认路径
        if (UComfyUIFileManager::SaveTextureToProject(GeneratedTexture, DefaultName))
            ShowSaveSuccessNotification(FString::Printf(TEXT("/Game/ComfyUI/Generated/%s"), *DefaultName));
        else
            ShowSaveErrorNotification(TEXT("保存图像时发生错误"));
    }
    else if (GeneratedMesh)
    {
        // 保存3D模型到项目的默认路径
        // TODO: 实现3D模型保存功能
        FNotificationInfo Info(LOCTEXT("MeshSaveNotImplemented", "3D模型保存功能正在开发中"));
        Info.ExpireDuration = 3.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoContentError", "没有可保存的内容。请先生成图像或3D模型。"));
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnSaveAsClicked()
{
    UE_LOG(LogTemp, Log, TEXT("OnSaveAsClicked: Starting save dialog"));

    // 检查是否有生成的内容
    bool bHasImage = GeneratedTexture != nullptr;
    bool bHasMesh = GeneratedMesh != nullptr;
    
    if (!bHasImage && !bHasMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnSaveAsClicked: No generated content to save"));
        ShowSaveErrorNotification(TEXT("没有生成的内容可以保存"));
        return FReply::Handled();
    }

    FString SavePath;
    FString DefaultFilename;
    FString FileExtensions;
    FString DialogTitle;

    // 根据内容类型设置对话框参数
    if (bHasImage)
    {
        DefaultFilename = FString::Printf(TEXT("ComfyUI_Generated_%s.png"),
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
        FileExtensions = TEXT("PNG Files (*.png)|*.png|JPEG Files (*.jpg)|*.jpg|BMP Files (*.bmp)|*.bmp");
        DialogTitle = TEXT("保存生成的图像");
    }
    else if (bHasMesh)
    {
        DefaultFilename = FString::Printf(TEXT("ComfyUI_Generated_%s.obj"),
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
        FileExtensions = TEXT("OBJ Files (*.obj)|*.obj|GLB Files (*.glb)|*.glb|GLTF Files (*.gltf)|*.gltf");
        DialogTitle = TEXT("保存生成的3D模型");
    }

    // 显示保存对话框
    if (UComfyUIFileManager::ShowSaveFileDialog(
        DialogTitle,
        FileExtensions,
        UComfyUIFileManager::GetPluginDirectory(),
        DefaultFilename,
        SavePath))
    {
        UE_LOG(LogTemp, Log, TEXT("OnSaveAsClicked: User selected save path: %s"), *SavePath);

        if (bHasImage)
        {
            // 保存图像文件
            if (UComfyUIFileManager::SaveTextureToFile(GeneratedTexture, SavePath))
                ShowSaveSuccessNotification(FString::Printf(TEXT("文件已保存到: %s"), *SavePath));
            else
                ShowSaveErrorNotification(TEXT("保存图像文件时发生错误"));
        }
        else if (bHasMesh)
        {
            // TODO: 实现3D模型保存功能
            ShowSaveErrorNotification(TEXT("3D模型保存功能暂未实现"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("OnSaveAsClicked: User cancelled save dialog"));
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnLoadImageClicked()
{
    TArray<FString> FileNames;
    const FString FileTypes = TEXT("Image Files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp");
    
    if (UComfyUIFileManager::ShowOpenFileDialog(
        TEXT("选择输入图像"),
        FileTypes,
        TEXT(""),
        FileNames))
    {
        if (FileNames.Num() > 0)
        {
            FString ImagePath = FileNames[0];
            
            // 加载图像文件
            TArray<uint8> ImageData;
            if (UComfyUIFileManager::LoadImageFromFile(ImagePath, ImageData))
            {
                // 使用ComfyUIFileManager创建纹理
                UTexture2D* LoadedTexture = UComfyUIFileManager::CreateTextureFromImageData(ImageData);
                if (LoadedTexture)
                {
                    InputImage = LoadedTexture;
                    
                    // 同步到拖拽Widget
                    if (InputImageDragDropWidget.IsValid())
                        InputImageDragDropWidget->SetImage(InputImage);
                    
                    // 创建Slate画刷（为了兼容性保留）
                    InputImageBrush = MakeShareable(new FSlateBrush());
                    InputImageBrush->SetResourceObject(InputImage);
                    InputImageBrush->DrawAs = ESlateBrushDrawType::Image;
                    
                    // 计算合适的显示尺寸
                    FVector2D OriginalSize = FVector2D(InputImage->GetSizeX(), InputImage->GetSizeY());
                    FVector2D PreviewSize = CalculateImageFitSize(OriginalSize, FVector2D(140.0f, 140.0f));
                    InputImageBrush->ImageSize = PreviewSize;
                    
                    // 更新UI
                    if (InputImagePreview.IsValid())
                        InputImagePreview->SetImage(InputImageBrush.Get());
                    
                    // 启用清除按钮
                    if (ClearImageButton.IsValid())
                        ClearImageButton->SetEnabled(true);
                    
                    // 显示成功通知
                    FNotificationInfo Info(FText::Format(
                        LOCTEXT("ImageLoaded", "成功加载图像：{0}"),
                        FText::FromString(FPaths::GetBaseFilename(ImagePath))
                    ));
                    Info.ExpireDuration = 3.0f;
                    FSlateNotificationManager::Get().AddNotification(Info);
                }
                else
                {
                    FNotificationInfo Info(LOCTEXT("ImageLoadFailed", "无法创建纹理，请检查图像格式"));
                    Info.ExpireDuration = 3.0f;
                    FSlateNotificationManager::Get().AddNotification(Info);
                }
            }
            else
            {
                FNotificationInfo Info(LOCTEXT("FileLoadFailed", "无法读取图像文件"));
                Info.ExpireDuration = 3.0f;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
        }
    }
    else
        ShowSaveErrorNotification(TEXT("无法打开文件对话框"));
    return FReply::Handled();
}

FReply SComfyUIWidget::OnClearImageClicked()
{
    // 清除输入图像
    InputImage = nullptr;
    InputImageBrush = nullptr;
    
    // 同步到拖拽Widget
    if (InputImageDragDropWidget.IsValid())
        InputImageDragDropWidget->ClearImage();
    
    // 恢复默认图像显示（为了兼容性保留）
    if (InputImagePreview.IsValid())
    {
        InputImagePreview->SetImage(FAppStyle::GetBrush("WhiteBrush"));
        InputImagePreview->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f));
    }
    
    // 禁用清除按钮
    if (ClearImageButton.IsValid())
        ClearImageButton->SetEnabled(false);
    
    // 显示通知
    FNotificationInfo Info(LOCTEXT("ImageCleared", "已清除输入图像"));
    Info.ExpireDuration = 2.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnLoadModelClicked()
{
    TArray<FString> FileNames;
    const FString FileTypes = TEXT("3D Model Files (*.obj;*.fbx;*.glb;*.gltf;*.ply;*.stl)|*.obj;*.fbx;*.glb;*.gltf;*.ply;*.stl");
    
    if (UComfyUIFileManager::ShowOpenFileDialog(
        TEXT("选择3D模型文件"),
        FileTypes,
        TEXT(""),
        FileNames))
    {
        if (FileNames.Num() > 0)
        {
            InputModelPath = FileNames[0];
            
            if (ModelPathTextBox.IsValid())
                ModelPathTextBox->SetText(FText::FromString(InputModelPath));
            
            // 启用清除按钮
            if (ClearModelButton.IsValid())
                ClearModelButton->SetEnabled(true);
            
            // 显示成功通知
            FNotificationInfo Info(FText::Format(
                LOCTEXT("ModelLoaded", "成功选择3D模型：{0}"),
                FText::FromString(FPaths::GetBaseFilename(InputModelPath))
            ));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnClearModelClicked()
{
    // 清除3D模型路径
    InputModelPath.Empty();
    
    if (ModelPathTextBox.IsValid())
        ModelPathTextBox->SetText(FText::GetEmpty());
    
    // 禁用清除按钮
    if (ClearModelButton.IsValid())
        ClearModelButton->SetEnabled(false);
    
    // 显示通知
    FNotificationInfo Info(LOCTEXT("ModelCleared", "已清除3D模型路径"));
    Info.ExpireDuration = 2.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}

void SComfyUIWidget::OnImageDropped(UTexture2D* DroppedTexture)
{
    if (!DroppedTexture)
        return;
    
    // 设置输入图像
    InputImage = DroppedTexture;
    
    // 同步到拖拽Widget（虽然它应该已经自己处理了）
    if (InputImageDragDropWidget.IsValid())
        InputImageDragDropWidget->SetImage(InputImage);
    
    // 启用清除按钮
    if (ClearImageButton.IsValid())
        ClearImageButton->SetEnabled(true);
}

void SComfyUIWidget::OnImageGenerationComplete(UTexture2D* GeneratedImage)
{
    // 重新启用生成按钮并恢复生成状态
    GenerateButton->SetEnabled(true);
    bIsGenerating = false;
    CurrentClient = nullptr;  // 清除客户端引用

    if (GeneratedImage)
    {
        GeneratedTexture = GeneratedImage;
        
        // 创建一个持久的Slate画刷并存储在成员变量中
        CurrentImageBrush = MakeShareable(new FSlateBrush());
        CurrentImageBrush->SetResourceObject(GeneratedTexture);
        
        // 获取原始图片大小
        FVector2D OriginalImageSize = FVector2D(GeneratedTexture->GetSizeX(), GeneratedTexture->GetSizeY());
        
        // 计算适合预览区域的大小（保持比例）
        FVector2D PreviewAreaSize(290.0f, 290.0f); // 预览区域大小（考虑到Padding）
        FVector2D FittedSize = CalculateImageFitSize(OriginalImageSize, PreviewAreaSize);
        
        // 设置画刷的显示大小为适配后的大小
        CurrentImageBrush->ImageSize = FittedSize;
        CurrentImageBrush->DrawAs = ESlateBrushDrawType::Image;
        
        // 更新图像预览
        ImagePreview->SetImage(CurrentImageBrush.Get());
        
        // 切换到图像预览模式
        SwitchToImagePreview();

        // 启用保存按钮（移除预览按钮，因为内容直接显示在预览区域）
        SaveButton->SetEnabled(true);
        SaveAsButton->SetEnabled(true);

        // 显示成功通知
        FNotificationInfo Info(LOCTEXT("GenerationComplete", "图像生成完成！"));
        Info.ExpireDuration = 3.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        // 显示图像生成错误通知（连接已经成功，所以这是生成过程的错误）
        FNotificationInfo Info(LOCTEXT("GenerationFailed", "图像生成失败，请检查工作流配置或服务器资源"));
        Info.ExpireDuration = 5.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

void SComfyUIWidget::OnMeshGenerationComplete(UStaticMesh* InGeneratedMesh)
{
    // 重新启用生成按钮并恢复生成状态
    GenerateButton->SetEnabled(true);
    bIsGenerating = false;
    CurrentClient = nullptr;  // 清除客户端引用

    if (InGeneratedMesh)
    {
        this->GeneratedMesh = InGeneratedMesh;
        
        // 在3D视口中显示生成的模型
        if (ModelViewport.IsValid())
        {
            ModelViewport->SetPreviewMesh(InGeneratedMesh);
            UE_LOG(LogTemp, Log, TEXT("OnMeshGenerationComplete: Set mesh in viewport"));
        }
        
        // 切换到3D模型预览模式
        SwitchToModelPreview();

        // 启用保存按钮
        SaveButton->SetEnabled(true);
        SaveAsButton->SetEnabled(true);

        // 显示成功通知
        FNotificationInfo Info(LOCTEXT("MeshGenerationComplete", "3D模型生成完成！"));
        Info.ExpireDuration = 3.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        // 显示3D模型生成错误通知
        FNotificationInfo Info(LOCTEXT("MeshGenerationFailed", "3D模型生成失败，请检查工作流配置或服务器资源"));
        Info.ExpireDuration = 5.0f;
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

void SComfyUIWidget::SwitchToImagePreview()
{
    if (ContentSwitcher.IsValid())
    {
        ContentSwitcher->SetActiveWidgetIndex(0); // 切换到图像预览
    }
}

void SComfyUIWidget::SwitchToModelPreview()
{
    if (ContentSwitcher.IsValid())
    {
        ContentSwitcher->SetActiveWidgetIndex(1); // 切换到3D模型预览
    }
}

void SComfyUIWidget::ClearContentPreview()
{
    // 清除生成的内容
    GeneratedTexture = nullptr;
    GeneratedMesh = nullptr;
    
    // 清除3D视口中的预览
    if (ModelViewport.IsValid())
    {
        ModelViewport->ClearPreview();
    }
    
    // 切换到空状态
    if (ContentSwitcher.IsValid())
    {
        ContentSwitcher->SetActiveWidgetIndex(2); // 切换到空状态
    }
}

FReply SComfyUIWidget::OnImportWorkflowClicked()
{
    TArray<FString> FileNames;
    const FString FileTypes = TEXT("ComfyUI Workflow Files (*.json)|*.json");
    
    if (UComfyUIFileManager::ShowOpenFileDialog(
        TEXT("Import ComfyUI Workflow"),
        FileTypes,
        TEXT(""),
        FileNames))
    {
        if (FileNames.Num() > 0)
        {
            FString SourceFile = FileNames[0];
            FString WorkflowName = FPaths::GetBaseFilename(SourceFile);
            
            // 使用工作流服务进行导入
            UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
            FString ImportError;
            bool bImportSuccess = false;
            
            if (WorkflowService)
            {
                bImportSuccess = WorkflowService->ImportWorkflow(SourceFile, WorkflowName, ImportError);
            }
            else
            {
                // 如果服务不可用，回退到文件管理器
                bImportSuccess = UComfyUIFileManager::ImportWorkflowTemplate(SourceFile, WorkflowName, ImportError);
            }
            
            if (bImportSuccess)
            {
                RefreshCustomWorkflowList();
                        
                FNotificationInfo Info(FText::Format(
                    LOCTEXT("WorkflowImported", "成功导入并验证工作流：{0}"),
                    FText::FromString(WorkflowName)
                ));
                Info.ExpireDuration = 3.0f;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
            else
            {
                FNotificationInfo Info(FText::Format(
                    LOCTEXT("ImportFailed", "导入工作流失败：{0}"),
                    FText::FromString(ImportError)
                ));
                Info.ExpireDuration = 5.0f;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
        }
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnRefreshWorkflowsClicked()
{
    RefreshCustomWorkflowList();
    
    FNotificationInfo Info(LOCTEXT("WorkflowsRefreshed", "自定义工作流列表已刷新"));
    Info.ExpireDuration = 2.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}

TSharedRef<SWidget> SComfyUIWidget::OnGenerateCustomWorkflowWidget(TSharedPtr<FString> InOption)
{
    return SNew(STextBlock)
        .Text(FText::FromString(InOption.IsValid() ? *InOption : TEXT("None")));
}

#pragma optimize( "", off )
FText SComfyUIWidget::GetCustomWorkflowText(TSharedPtr<FString> InOption) const
{
    if (CurrentCustomWorkflow.IsValid())
        return FText::FromString(*CurrentCustomWorkflow);
    return LOCTEXT("NoCustomWorkflowSelected", "选择自定义工作流...");
}
#pragma optimize( "", on )

void SComfyUIWidget::OnCustomWorkflowChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    CurrentCustomWorkflow = NewSelection;
    
    // 检测新选择的工作流类型
    if (CurrentCustomWorkflow.IsValid())
    {
        DetectWorkflowType(*CurrentCustomWorkflow);
        
        // 刷新UI以更新可见性
        UpdateWorkflowVisibility();
    }
}

FText SComfyUIWidget::GetWorkflowTypeText(TSharedPtr<EComfyUIWorkflowType> InOption) const
{
    if (!InOption.IsValid())
        return LOCTEXT("InvalidWorkflowType", "无效");
    
    return WorkflowTypeToText(*InOption);
}

void SComfyUIWidget::RefreshCustomWorkflowList()
{
    CustomWorkflowNames.Empty();
    
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    TArray<FString> WorkflowNames;
    
    if (WorkflowService && IsValid(WorkflowService))
    {
        WorkflowNames = WorkflowService->GetAvailableWorkflowNames();
        UE_LOG(LogTemp, Log, TEXT("RefreshCustomWorkflowList: Got %d workflows from service"), WorkflowNames.Num());
    }
    
    for (const FString& WorkflowName : WorkflowNames)
        CustomWorkflowNames.Add(MakeShareable(new FString(WorkflowName)));
    
    // 如果有自定义工作流ComboBox，刷新它
    if (CustomWorkflowComboBox.IsValid())
    {
        CustomWorkflowComboBox->RefreshOptions();
        if (CustomWorkflowNames.Num() > 0 && !CurrentCustomWorkflow.IsValid())
        {
            CurrentCustomWorkflow = CustomWorkflowNames[0];
            CustomWorkflowComboBox->SetSelectedItem(CurrentCustomWorkflow);
        }
    }
}

void SComfyUIWidget::UpdateWorkflowVisibility()
{
    // 当工作流类型改变时，刷新UI布局以更新可见性
    // 这会触发所有绑定了可见性回调的Widget重新计算可见性状态
    
    // 强制整个Widget重新布局，确保可见性变化立即生效
    Invalidate(EInvalidateWidget::Layout);
    
    // 如果切换到自定义工作流且当前没有选择工作流，尝试选择第一个可用的
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
    {
        // 确保自定义工作流列表是最新的
        if (CustomWorkflowNames.Num() == 0)
            RefreshCustomWorkflowList();
        
        // 如果没有选择自定义工作流但有可用选项，选择第一个
        if (!CurrentCustomWorkflow.IsValid() && CustomWorkflowNames.Num() > 0)
            CurrentCustomWorkflow = CustomWorkflowNames[0];

        // 只有CurrentCustomWorkflow有效时才设置ComboBox选中项，否则输出警告
        if (CurrentCustomWorkflow.IsValid())
        {
            if (CustomWorkflowComboBox.IsValid())
                CustomWorkflowComboBox->SetSelectedItem(CurrentCustomWorkflow);
            DetectWorkflowType(*CurrentCustomWorkflow);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateWorkflowVisibility: CurrentCustomWorkflow无效，无法设置ComboBox选中项。Widget=%p, WorkflowType=%d, CustomWorkflowNames.Num=%d"),
                this,
                CurrentWorkflowType.IsValid() ? (int32)*CurrentWorkflowType : -1,
                CustomWorkflowNames.Num());
        }
    }
    else
    {
        // 切换到非自定义工作流时，清除自定义工作流选择（可选）
        // 这样可以避免UI状态混乱
        // if (CurrentCustomWorkflow.IsValid())
        // {
        //     // 注意：这里不清除选择，保持用户的选择状态
        //     // CurrentCustomWorkflow = nullptr;
        // }
    }
}

EVisibility SComfyUIWidget::GetCustomWorkflowVisibility() const
{
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
        return EVisibility::Visible;
    return EVisibility::Collapsed;
}

EVisibility SComfyUIWidget::GetInputImageVisibility() const
{
    // 检查当前工作流类型是否需要输入图像
    EComfyUIWorkflowType TypeToCheck = EComfyUIWorkflowType::TextToImage;
    
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
        // 自定义工作流：使用检测到的类型
        TypeToCheck = DetectedCustomWorkflowType;
    else if (CurrentWorkflowType.IsValid())
        // 预定义工作流：使用选择的类型
        TypeToCheck = *CurrentWorkflowType;
    
    // 这些工作流类型需要输入图像
    if (TypeToCheck == EComfyUIWorkflowType::ImageToImage ||
        TypeToCheck == EComfyUIWorkflowType::ImageTo3D ||
        TypeToCheck == EComfyUIWorkflowType::MeshTexturing)
        return EVisibility::Visible;
    
    return EVisibility::Collapsed;
}

EVisibility SComfyUIWidget::GetModelInputVisibility() const
{
    // 检查当前工作流类型是否需要模型输入
    EComfyUIWorkflowType TypeToCheck = EComfyUIWorkflowType::TextToImage;
    
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
        // 自定义工作流：使用检测到的类型
        TypeToCheck = DetectedCustomWorkflowType;
    else if (CurrentWorkflowType.IsValid())
        // 预定义工作流：使用选择的类型
        TypeToCheck = *CurrentWorkflowType;
    
    // 网格纹理化工作流需要3D模型输入
    if (TypeToCheck == EComfyUIWorkflowType::MeshTexturing)
        return EVisibility::Visible;
    
    return EVisibility::Collapsed;
}

EVisibility SComfyUIWidget::GetPromptInputVisibility() const
{
    // 检查当前工作流类型是否需要提示词输入
    EComfyUIWorkflowType TypeToCheck = EComfyUIWorkflowType::TextToImage;
    
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
        // 自定义工作流：使用检测到的类型
        TypeToCheck = DetectedCustomWorkflowType;
    else if (CurrentWorkflowType.IsValid())
        // 预定义工作流：使用选择的类型
        TypeToCheck = *CurrentWorkflowType;
    
    // 这些工作流类型需要提示词输入
    if (TypeToCheck == EComfyUIWorkflowType::TextToImage ||
        TypeToCheck == EComfyUIWorkflowType::ImageToImage ||
        TypeToCheck == EComfyUIWorkflowType::TextTo3D)
        return EVisibility::Visible;
    
    // 图生3D和网格纹理化不需要提示词输入
    return EVisibility::Collapsed;
}

FText SComfyUIWidget::GetDetectedWorkflowTypeText() const
{
    return WorkflowTypeToText(DetectedCustomWorkflowType);
}

FReply SComfyUIWidget::OnValidateWorkflowClicked()
{
    if (!CurrentCustomWorkflow.IsValid() || CurrentCustomWorkflow->IsEmpty())
    {
        FNotificationInfo Info(LOCTEXT("NoWorkflowSelected", "Please select a workflow to validate"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 使用工作流服务进行验证
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (WorkflowService)
    {
        // 查找工作流文件
        FString TemplatesDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration/Config/Templates");
        FString WorkflowFile = TemplatesDir / (*CurrentCustomWorkflow + TEXT(".json"));
        
        FString ValidationError;
        bool bIsValid = WorkflowService->ValidateWorkflow(WorkflowFile, ValidationError);
        
        if (bIsValid)
        {
            FNotificationInfo Info(FText::Format(
                LOCTEXT("WorkflowValid", "Workflow '{0}' is valid!"),
                FText::FromString(*CurrentCustomWorkflow)
            ));
            Info.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
        else
        {
            FNotificationInfo Info(FText::Format(
                LOCTEXT("WorkflowInvalid", "Workflow '{0}' validation failed: {1}"),
                FText::FromString(*CurrentCustomWorkflow),
                FText::FromString(ValidationError)
            ));
            Info.ExpireDuration = 5.0f;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
    }
    
    return FReply::Handled();
}

FVector2D SComfyUIWidget::CalculateImageFitSize(const FVector2D& ImageSize, const FVector2D& ContainerSize) const
{
    if (ImageSize.X <= 0 || ImageSize.Y <= 0 || ContainerSize.X <= 0 || ContainerSize.Y <= 0)
    {
        return FVector2D::ZeroVector;
    }
    
    // 计算宽高比
    float ImageAspectRatio = ImageSize.X / ImageSize.Y;
    float ContainerAspectRatio = ContainerSize.X / ContainerSize.Y;
    
    FVector2D FitSize;
    
    if (ImageAspectRatio > ContainerAspectRatio)
    {
        // 图片比容器更宽，以宽度为准
        FitSize.X = ContainerSize.X;
        FitSize.Y = ContainerSize.X / ImageAspectRatio;
    }
    else
    {
        // 图片比容器更高，以高度为准
        FitSize.Y = ContainerSize.Y;
        FitSize.X = ContainerSize.Y * ImageAspectRatio;
    }
    
    return FitSize;
}

void SComfyUIWidget::ShowSaveSuccessNotification(const FString& AssetPath)
{
    FNotificationInfo Info(FText::Format(
        LOCTEXT("SaveSuccessText", "图像已成功保存到: {0}"),
        FText::FromString(AssetPath)
    ));
    Info.ExpireDuration = 5.0f;
    Info.bUseLargeFont = false;
    Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
    
    FSlateNotificationManager::Get().AddNotification(Info);
}

void SComfyUIWidget::ShowSaveErrorNotification(const FString& ErrorMessage)
{
    FNotificationInfo Info(FText::Format(
        LOCTEXT("SaveErrorText", "保存失败: {0}"),
        FText::FromString(ErrorMessage)
    ));
    Info.ExpireDuration = 5.0f;
    Info.bUseLargeFont = false;
    Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.DefaultMessage"));
    
    FSlateNotificationManager::Get().AddNotification(Info);
}

void SComfyUIWidget::DetectWorkflowType(const FString& WorkflowName)
{
    // 默认类型
    DetectedCustomWorkflowType = EComfyUIWorkflowType::Unknown;
    
    if (WorkflowName.IsEmpty()) return;
    
    // 使用工作流管理器检测工作流类型
    DetectedCustomWorkflowType = UComfyUIWorkflowService::Get()->DetectWorkflowType(WorkflowName);
}

bool SComfyUIWidget::DoesCurrentWorkflowNeedImage() const
{
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EComfyUIWorkflowType::Custom)
    {
        // 对于自定义工作流，检查工作流名称是否包含图像到3D的关键词
        if (CurrentCustomWorkflow.IsValid())
        {
            FString WorkflowName = CurrentCustomWorkflow->ToLower();
            return WorkflowName.Contains(TEXT("image_to_3d")) || 
                   WorkflowName.Contains(TEXT("img2mesh")) ||
                   WorkflowName.Contains(TEXT("mesh_texture")) ||
                   WorkflowName.Contains(TEXT("texture_mesh"));
        }
    }
    else if (CurrentWorkflowType.IsValid())
    {
        // 对于预定义工作流类型，检查是否是图像到图像的类型
        EComfyUIWorkflowType Type = *CurrentWorkflowType;
        return Type == EComfyUIWorkflowType::ImageToImage ||
               Type == EComfyUIWorkflowType::ImageTo3D ||
               Type == EComfyUIWorkflowType::MeshTexturing;
    }
    
    return false;
}

TSharedRef<SWidget> SComfyUIWidget::CreateProgressWidget()
{
    return SNew(SVerticalBox)
        .Visibility(this, &SComfyUIWidget::GetProgressVisibility)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ProgressLabel", "生成进度"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 5.0f)
        [
            SAssignNew(GenerationProgressBar, SProgressBar)
            .Percent(this, &SComfyUIWidget::GetProgressPercent)
            .BackgroundImage(FAppStyle::GetBrush("ProgressBar.Background"))
            .FillImage(FAppStyle::GetBrush("ProgressBar.Fill"))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, 10.0f, 0.0f)
            [
                SAssignNew(ProgressStatusText, STextBlock)
                .Text(this, &SComfyUIWidget::GetProgressStatusText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(QueuePositionText, STextBlock)
                .Text(this, &SComfyUIWidget::GetQueuePositionText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SAssignNew(CurrentNodeText, STextBlock)
            .Text(this, &SComfyUIWidget::GetCurrentNodeText)
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
            .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
        ];
}

EVisibility SComfyUIWidget::GetProgressVisibility() const
{
    return bIsGenerating ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SComfyUIWidget::GetCancelButtonVisibility() const
{
    return bIsGenerating ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SComfyUIWidget::IsGenerateButtonEnabled() const
{
    return !bIsGenerating;
}

TOptional<float> SComfyUIWidget::GetProgressPercent() const
{
    return CurrentProgressInfo.ProgressPercentage;
}

FText SComfyUIWidget::GetProgressStatusText() const
{
    return FText::FromString(CurrentProgressInfo.StatusMessage);
}

FText SComfyUIWidget::GetQueuePositionText() const
{
    if (CurrentProgressInfo.QueuePosition > 0)
    {
        return FText::Format(LOCTEXT("QueuePosition", "队列位置: {0}"), 
                           FText::AsNumber(CurrentProgressInfo.QueuePosition));
    }
    return FText::GetEmpty();
}

FText SComfyUIWidget::GetCurrentNodeText() const
{
    if (!CurrentProgressInfo.CurrentNodeName.IsEmpty())
    {
        return FText::Format(LOCTEXT("CurrentNode", "当前节点: {0}"), 
                           FText::FromString(CurrentProgressInfo.CurrentNodeName));
    }
    return FText::GetEmpty();
}

FReply SComfyUIWidget::OnCancelClicked()
{
    if (bIsGenerating && CurrentClient && IsValid(CurrentClient))
    {
        // 取消当前生成任务
        CurrentClient->CancelCurrentGeneration();
        
        // 重置UI状态
        bIsGenerating = false;
        CurrentProgressInfo = FComfyUIProgressInfo();
        
        // 恢复生成按钮状态
        GenerateButton->SetEnabled(true);
        
        // 清除客户端引用
        CurrentClient = nullptr;
        
        // 显示取消通知
        FNotificationInfo Info(LOCTEXT("GenerationCancelled", "生成任务已取消"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        
        UE_LOG(LogTemp, Log, TEXT("Generation cancelled by user"));
    }
    
    return FReply::Handled();
}

void SComfyUIWidget::OnGenerationProgressUpdate(const FComfyUIProgressInfo& ProgressInfo)
{
    CurrentProgressInfo = ProgressInfo;
    UE_LOG(LogTemp, VeryVerbose, TEXT("Progress Update: %s - %.1f%% - Queue: %d"), 
           *ProgressInfo.StatusMessage, ProgressInfo.ProgressPercentage * 100.0f, ProgressInfo.QueuePosition);
}

void SComfyUIWidget::OnGenerationStarted(const FString& PromptId)
{
    bIsGenerating = true;
    CurrentProgressInfo = FComfyUIProgressInfo(0, 0.0f, TEXT(""), TEXT("开始生成..."), false);
    UE_LOG(LogTemp, Log, TEXT("Generation started with Prompt ID: %s"), *PromptId);
}

void SComfyUIWidget::OnGenerationCompleted()
{
    bIsGenerating = false;
    CurrentProgressInfo = FComfyUIProgressInfo();
    CurrentClient = nullptr;  // 清除客户端引用，生成完成后不再需要
    UE_LOG(LogTemp, Log, TEXT("Generation completed"));
}

#undef LOCTEXT_NAMESPACE
