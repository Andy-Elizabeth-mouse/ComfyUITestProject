#include "UI/ComfyUIWidget.h"
#include "UI/SImageDragDropWidget.h"
#include "Client/ComfyUIClient.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
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

// FSavePackageArgs结构前向声明
// struct FSavePackageArgs
// {
//     EObjectFlags TopLevelFlags = RF_NoFlags;
//     FOutputDevice* Error = nullptr;
//     FLinkerNull* Conform = nullptr;
//     bool bForceByteSwapping = false;
//     bool bWarnOfLongFilename = false;
//     uint32 SaveFlags = SAVE_None;
//     // const class ITargetPlatform* TargetPlatform = nullptr;
//     FDateTime FinalTimeStamp = FDateTime::MinValue();
//     bool bSlowTask = true;
// };

#define LOCTEXT_NAMESPACE "ComfyUIWidget"

void SComfyUIWidget::Construct(const FArguments& InArgs)
{
    // 初始化成员变量
    GeneratedTexture = nullptr;
    CurrentImageBrush = nullptr;
    bIsGenerating = false;
    
    // 初始化工作流选项
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextToImage)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::ImageToImage)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextTo3D)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::ImageTo3D)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::TextureGeneration)));
    WorkflowOptions.Add(MakeShareable(new EWorkflowType(EWorkflowType::Custom)));
    
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
                .Text(FText::FromString(TEXT("http://127.0.0.1:8188")))
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
            SAssignNew(WorkflowTypeComboBox, SComboBox<TSharedPtr<EWorkflowType>>)
            .OptionsSource(&WorkflowOptions)
            .OnGenerateWidget(this, &SComfyUIWidget::OnGenerateWorkflowTypeWidget)
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
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PositivePromptLabel", "正面提示词："))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.0f)
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
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.0f)
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
        
        // 工作流管理按钮区域
        // + SVerticalBox::Slot()
        // .AutoHeight()
        // .Padding(2.0f)
        // [
        //     SNew(SHorizontalBox)
            
        //     + SHorizontalBox::Slot()
        //     .FillWidth(1.0f)
        //     .Padding(2.0f)
        //     [
        //         SNew(SButton)
        //         .Text(LOCTEXT("ImportWorkflowButton", "导入工作流"))
        //         .OnClicked(this, &SComfyUIWidget::OnImportWorkflowClicked)
        //         .HAlign(HAlign_Center)
        //         .ToolTipText(LOCTEXT("ImportWorkflowTooltip", "导入ComfyUI工作流JSON文件"))
        //     ]

        //     + SHorizontalBox::Slot()
        //     .FillWidth(1.0f)
        //     .Padding(2.0f)
        //     [
        //         SNew(SButton)
        //         .Text(LOCTEXT("ValidateWorkflowButton", "验证工作流"))
        //         .OnClicked(this, &SComfyUIWidget::OnValidateWorkflowClicked)
        //         .HAlign(HAlign_Center)
        //         .ToolTipText(LOCTEXT("ValidateWorkflowTooltip", "验证当前工作流的有效性"))
        //     ]
        // ]

        // 生成和保存按钮区域
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
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT("GenerateTooltip", "使用当前工作流生成图像"))
            ]

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

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(2.0f)
            [
                SAssignNew(PreviewButton, SButton)
                .Text(LOCTEXT("PreviewButton", "预览"))
                .OnClicked(this, &SComfyUIWidget::OnPreviewClicked)
                .HAlign(HAlign_Center)
                .IsEnabled(false) // 初始状态禁用
                .ToolTipText(LOCTEXT("PreviewTooltip", "预览生成的图片"))
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

TSharedRef<SWidget> SComfyUIWidget::CreateImagePreviewWidget()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 2.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ImagePreviewLabel", "生成图像预览"))
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
                // 使用固定高度的SBox来限制图片预览区域的大小
                SNew(SBox)
                .HeightOverride(300.0f) // 设置预览区域的固定高度
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SAssignNew(ImagePreview, SImage)
                    .Image(FAppStyle::GetBrush("WhiteBrush"))
                    .ColorAndOpacity(FLinearColor::White)
                ]
            ]
        ];
}

TSharedRef<SWidget> SComfyUIWidget::OnGenerateWorkflowTypeWidget(TSharedPtr<EWorkflowType> InOption)
{
    return SNew(STextBlock)
        .Text(WorkflowTypeToText(*InOption.Get()));
}

void SComfyUIWidget::OnWorkflowTypeChanged(TSharedPtr<EWorkflowType> NewSelection, ESelectInfo::Type SelectInfo)
{
    CurrentWorkflowType = NewSelection;
    UpdateWorkflowVisibility();
}

FText SComfyUIWidget::GetCurrentWorkflowTypeText() const
{
    if (CurrentWorkflowType.IsValid())
    {
        return WorkflowTypeToText(*CurrentWorkflowType.Get());
    }
    return LOCTEXT("NoWorkflowSelected", "选择工作流");
}

FText SComfyUIWidget::GetGenerateButtonText() const
{
    if (bIsGenerating)
    {
        return LOCTEXT("GeneratingButton", "生成中...");
    }
    return LOCTEXT("GenerateButton", "生成图像");
}

FText SComfyUIWidget::WorkflowTypeToText(EWorkflowType Type) const
{
    switch (Type)
    {
    case EWorkflowType::TextToImage:
        return LOCTEXT("TextToImage", "文生图");
    case EWorkflowType::ImageToImage:
        return LOCTEXT("ImageToImage", "图生图");
    case EWorkflowType::TextTo3D:
        return LOCTEXT("TextTo3D", "文生3D模型");
    case EWorkflowType::ImageTo3D:
        return LOCTEXT("ImageTo3D", "图生3D模型");
    case EWorkflowType::TextureGeneration:
        return LOCTEXT("TextureGeneration", "纹理生成");
    case EWorkflowType::Custom:
        return LOCTEXT("CustomWorkflow", "自定义工作流");
    default:
        return LOCTEXT("Unknown", "未知");
    }
}

FReply SComfyUIWidget::OnGenerateClicked()
{
    FString ServerUrl = ComfyUIServerUrlTextBox->GetText().ToString();
    FString Prompt = PromptTextBox->GetText().ToString();
    FString NegativePrompt = NegativePromptTextBox->GetText().ToString();

    if (ServerUrl.IsEmpty() || Prompt.IsEmpty())
    {
        // 显示错误通知
        FNotificationInfo Info(LOCTEXT("InvalidInput", "请输入服务器URL和提示词"));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return FReply::Handled();
    }
    
    // 检查自定义工作流选择
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EWorkflowType::Custom)
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

    // 创建ComfyUI客户端进行连接测试
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    if (Client)
    {
        Client->SetServerUrl(ServerUrl);
        
        // 设置世界上下文
        if (GEngine && GEngine->GetCurrentPlayWorld())
        {
            Client->SetWorldContext(GEngine->GetCurrentPlayWorld());
        }
        else if (GWorld)
        {
            Client->SetWorldContext(GWorld);
        }
        
        // 先测试连接
        Client->TestServerConnection(FOnConnectionTested::CreateLambda([this, Client, Prompt, NegativePrompt](bool bSuccess, FString ErrorMessage)
        {
            if (bSuccess)
            {
                // 连接成功，开始图像生成
                if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EWorkflowType::Custom)
                {
                    Client->GenerateImageWithCustomWorkflow(Prompt, NegativePrompt, *CurrentCustomWorkflow,
                        FOnImageGenerated::CreateSP(this, &SComfyUIWidget::OnImageGenerationComplete));
                }
                else
                {
                    // Convert from SComfyUIWidget::EWorkflowType to EComfyUIWorkflowType
                    EComfyUIWorkflowType ClientWorkflowType = static_cast<EComfyUIWorkflowType>(*CurrentWorkflowType.Get());
                    Client->GenerateImage(Prompt, NegativePrompt, ClientWorkflowType, 
                        FOnImageGenerated::CreateSP(this, &SComfyUIWidget::OnImageGenerationComplete));
                }
            }
            else
            {
                // 连接失败，显示错误并恢复按钮状态
                GenerateButton->SetEnabled(true);
                bIsGenerating = false;
                
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

FReply SComfyUIWidget::OnSaveClicked()
{
    if (!GeneratedTexture)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoTextureError", "没有可保存的图像。请先生成图像。"));
        return FReply::Handled();
    }

    // 生成默认文件名（基于时间戳）
    FString DefaultName = FString::Printf(TEXT("ComfyUI_Generated_%s"),
        *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));

    // 保存纹理到项目的默认路径
    if (SaveTextureToProject(GeneratedTexture, DefaultName))
    {
        ShowSaveSuccessNotification(FString::Printf(TEXT("/Game/ComfyUI/Generated/%s"), *DefaultName));
    }
    else
    {
        ShowSaveErrorNotification(TEXT("保存图像时发生错误"));
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnSaveAsClicked()
{
    if (!GeneratedTexture)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoTextureError", "没有可保存的图像。请先生成图像。"));
        return FReply::Handled();
    }

    // 打开文件对话框让用户选择保存路径和名称
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> OutFileNames;
        FString DefaultFilename = FString::Printf(TEXT("ComfyUI_Generated_%s.png"),
            *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
        
        // 显示保存对话框 - 保存到任意位置
        bool bDialogResult = DesktopPlatform->SaveFileDialog(
            nullptr,
            TEXT("保存生成的图像"),
            FPaths::GetPath(FPaths::GetProjectFilePath()), // 默认到项目目录
            DefaultFilename,
            TEXT("PNG Files (*.png)|*.png|JPEG Files (*.jpg)|*.jpg|BMP Files (*.bmp)|*.bmp"),
            EFileDialogFlags::None,
            OutFileNames
        );

        if (bDialogResult && OutFileNames.Num() > 0)
        {
            FString SavePath = OutFileNames[0];
            UE_LOG(LogTemp, Log, TEXT("OnSaveAsClicked: User selected save path: %s"), *SavePath);

            // 直接保存为图像文件，而不是UE资产
            if (SaveTextureToFile(GeneratedTexture, SavePath))
            {
                ShowSaveSuccessNotification(FString::Printf(TEXT("文件已保存到: %s"), *SavePath));
            }
            else
            {
                ShowSaveErrorNotification(TEXT("保存图像文件时发生错误"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("OnSaveAsClicked: User cancelled save dialog"));
        }
    }
    else
    {
        ShowSaveErrorNotification(TEXT("无法打开文件对话框"));
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnPreviewClicked()
{
    // TODO: 实现预览功能
    FNotificationInfo Info(LOCTEXT("PreviewNotImplemented", "预览功能暂未实现"));
    Info.ExpireDuration = 3.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnLoadImageClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> FileNames;
        const FString FileTypes = TEXT("Image Files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp");
        
        if (DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("选择输入图像"),
            TEXT(""),
            TEXT(""),
            FileTypes,
            EFileDialogFlags::None,
            FileNames))
        {
            if (FileNames.Num() > 0)
            {
                FString ImagePath = FileNames[0];
                
                // 加载图像文件
                TArray<uint8> ImageData;
                if (FFileHelper::LoadFileToArray(ImageData, *ImagePath))
                {
                    // 使用ComfyUIClient创建纹理
                    UComfyUIClient* TempClient = NewObject<UComfyUIClient>();
                    UTexture2D* LoadedTexture = TempClient->CreateTextureFromImageData(ImageData);
                    if (LoadedTexture)
                    {
                        InputImage = LoadedTexture;
                        
                        // 同步到拖拽Widget
                        if (InputImageDragDropWidget.IsValid())
                        {
                            InputImageDragDropWidget->SetImage(InputImage);
                        }
                        
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
                        {
                            InputImagePreview->SetImage(InputImageBrush.Get());
                        }
                        
                        // 启用清除按钮
                        if (ClearImageButton.IsValid())
                        {
                            ClearImageButton->SetEnabled(true);
                        }
                        
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
    }
    
    return FReply::Handled();
}

FReply SComfyUIWidget::OnClearImageClicked()
{
    // 清除输入图像
    InputImage = nullptr;
    InputImageBrush = nullptr;
    
    // 同步到拖拽Widget
    if (InputImageDragDropWidget.IsValid())
    {
        InputImageDragDropWidget->ClearImage();
    }
    
    // 恢复默认图像显示（为了兼容性保留）
    if (InputImagePreview.IsValid())
    {
        InputImagePreview->SetImage(FAppStyle::GetBrush("WhiteBrush"));
        InputImagePreview->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f));
    }
    
    // 禁用清除按钮
    if (ClearImageButton.IsValid())
    {
        ClearImageButton->SetEnabled(false);
    }
    
    // 显示通知
    FNotificationInfo Info(LOCTEXT("ImageCleared", "已清除输入图像"));
    Info.ExpireDuration = 2.0f;
    FSlateNotificationManager::Get().AddNotification(Info);
    
    return FReply::Handled();
}

void SComfyUIWidget::OnImageDropped(UTexture2D* DroppedTexture)
{
    if (!DroppedTexture)
    {
        return;
    }
    
    // 设置输入图像
    InputImage = DroppedTexture;
    
    // 同步到拖拽Widget（虽然它应该已经自己处理了）
    if (InputImageDragDropWidget.IsValid())
    {
        InputImageDragDropWidget->SetImage(InputImage);
    }
    
    // 启用清除按钮
    if (ClearImageButton.IsValid())
    {
        ClearImageButton->SetEnabled(true);
    }
}

void SComfyUIWidget::OnImageGenerationComplete(UTexture2D* GeneratedImage)
{
    // 重新启用生成按钮并恢复生成状态
    GenerateButton->SetEnabled(true);
    bIsGenerating = false;

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

        // 启用保存和预览按钮
        SaveButton->SetEnabled(true);
        SaveAsButton->SetEnabled(true);
        PreviewButton->SetEnabled(true);

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

FReply SComfyUIWidget::OnImportWorkflowClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> FileNames;
        const FString FileTypes = TEXT("ComfyUI Workflow Files (*.json)|*.json");
        
        if (DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("Import ComfyUI Workflow"),
            TEXT(""),
            TEXT(""),
            FileTypes,
            EFileDialogFlags::None,
            FileNames))
        {
            if (FileNames.Num() > 0)
            {
                FString SourceFile = FileNames[0];
                FString WorkflowName = FPaths::GetBaseFilename(SourceFile);
                
                // 创建ComfyUI客户端实例进行导入
                UComfyUIClient* Client = NewObject<UComfyUIClient>();
                if (Client)
                {
                    FString ImportError;
                    bool bImportSuccess = Client->ImportWorkflowFile(SourceFile, WorkflowName, ImportError);
                    
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

FText SComfyUIWidget::GetCustomWorkflowText(TSharedPtr<FString> InOption) const
{
    if (InOption.IsValid())
    {
        return FText::FromString(*InOption);
    }
    return LOCTEXT("NoCustomWorkflowSelected", "选择自定义工作流...");
}

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

FText SComfyUIWidget::GetWorkflowTypeText(TSharedPtr<EWorkflowType> InOption) const
{
    if (!InOption.IsValid())
    {
        return LOCTEXT("InvalidWorkflowType", "无效");
    }
    
    return WorkflowTypeToText(*InOption);
}

void SComfyUIWidget::RefreshCustomWorkflowList()
{
    CustomWorkflowNames.Empty();
    
    // 获取Templates目录中的所有JSON文件
    FString TemplatesDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration/Config/Templates");
    
    if (FPaths::DirectoryExists(TemplatesDir))
    {
        TArray<FString> TemplateFiles;
        IFileManager::Get().FindFiles(TemplateFiles, *(TemplatesDir / TEXT("*.json")), true, false);
        
        for (const FString& TemplateFile : TemplateFiles)
        {
            FString WorkflowName = FPaths::GetBaseFilename(TemplateFile);
            CustomWorkflowNames.Add(MakeShareable(new FString(WorkflowName)));
        }
    }
    
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
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EWorkflowType::Custom)
    {
        // 确保自定义工作流列表是最新的
        if (CustomWorkflowNames.Num() == 0)
        {
            RefreshCustomWorkflowList();
        }
        
    // 如果没有选择自定义工作流但有可用选项，选择第一个
    if (!CurrentCustomWorkflow.IsValid() && CustomWorkflowNames.Num() > 0)
    {
        CurrentCustomWorkflow = CustomWorkflowNames[0];
    }

    // 只有CurrentCustomWorkflow有效时才设置ComboBox选中项，否则输出警告
    if (CurrentCustomWorkflow.IsValid())
    {
        if (CustomWorkflowComboBox.IsValid())
        {
            CustomWorkflowComboBox->SetSelectedItem(CurrentCustomWorkflow);
        }
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
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EWorkflowType::Custom)
    {
        return EVisibility::Visible;
    }
    return EVisibility::Collapsed;
}

EVisibility SComfyUIWidget::GetInputImageVisibility() const
{
    // 检查当前工作流类型是否需要输入图像
    EWorkflowType TypeToCheck = EWorkflowType::TextToImage;
    
    if (CurrentWorkflowType.IsValid() && *CurrentWorkflowType == EWorkflowType::Custom)
    {
        // 自定义工作流：使用检测到的类型
        TypeToCheck = DetectedCustomWorkflowType;
    }
    else if (CurrentWorkflowType.IsValid())
    {
        // 预定义工作流：使用选择的类型
        TypeToCheck = *CurrentWorkflowType;
    }
    
    // 这些工作流类型需要输入图像
    if (TypeToCheck == EWorkflowType::ImageToImage ||
        TypeToCheck == EWorkflowType::ImageTo3D)
    {
        return EVisibility::Visible;
    }
    
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
    
    // 创建ComfyUI客户端实例
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    if (Client)
    {
        // 查找工作流文件
        FString TemplatesDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration/Config/Templates");
        FString WorkflowFile = TemplatesDir / (*CurrentCustomWorkflow + TEXT(".json"));
        
        FString ValidationError;
        bool bIsValid = Client->ValidateWorkflowFile(WorkflowFile, ValidationError);
        
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

bool SComfyUIWidget::SaveTextureToProject(UTexture2D* Texture, const FString& AssetName, const FString& PackagePath)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Texture is null"));
        return false;
    }

    // 验证源纹理是否有效
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Source texture has no platform data or mips"));
        return false;
    }

    if (Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Invalid texture dimensions: %dx%d"), Texture->GetSizeX(), Texture->GetSizeY());
        return false;
    }

    if (AssetName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: AssetName is empty"));
        return false;
    }

    // 确保包路径有效
    FString FinalPackagePath = PackagePath;
    if (FinalPackagePath.IsEmpty() || !FinalPackagePath.StartsWith(TEXT("/Game/")))
    {
        FinalPackagePath = TEXT("/Game/ComfyUI/Generated");
    }

    UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Starting save process - AssetName: %s, PackagePath: %s"), *AssetName, *FinalPackagePath);

    // 生成唯一的资产名称
    FString UniqueAssetName;
    try
    {
        UniqueAssetName = GenerateUniqueAssetName(AssetName, FinalPackagePath);
        if (UniqueAssetName.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to generate unique asset name"));
            return false;
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Exception in GenerateUniqueAssetName"));
        return false;
    }

    FString FullPackageName = FinalPackagePath + TEXT("/") + UniqueAssetName;

    // 验证包名称
    if (!FPackageName::IsValidLongPackageName(FullPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Invalid package name: %s"), *FullPackageName);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Creating asset at path: %s"), *FullPackageName);

    try
    {
        UTexture2D* TextureToSave = nullptr;
        UPackage* Package = nullptr;
        
        // 检查是否可以直接使用传入的纹理
        UPackage* SourcePackage = Texture->GetPackage();
        bool bCanReuseTexture = false;
        
        // 如果传入纹理是临时的或者在临时包中，我们可以尝试直接移动它
        if (SourcePackage && (SourcePackage->HasAnyFlags(RF_Transient) || 
            SourcePackage->GetName().StartsWith(TEXT("/Engine/Transient")) ||
            SourcePackage->GetName().StartsWith(TEXT("/Temp/"))))
        {
            // 检查纹理是否有适当的标志位用于重新定位
            if (Texture->HasAnyFlags(RF_Transient) || !Texture->HasAnyFlags(RF_Public | RF_Standalone))
            {
                UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Attempting to relocate existing transient texture"));
                
                // 创建新包
                Package = CreatePackage(*FullPackageName);
                if (Package)
                {
                    Package->FullyLoad();
                    
                    // 尝试重命名和移动纹理到新包
                    bool bRenamed = Texture->Rename(*UniqueAssetName, Package, REN_None);
                    if (bRenamed)
                    {
                        // 设置正确的对象标志
                        Texture->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
                        Texture->ClearFlags(RF_Transient);
                        
                        // 确保纹理状态正确
                        Texture->PostEditChange();
                        Texture->UpdateResource();
                        
                        TextureToSave = Texture;
                        bCanReuseTexture = true;
                        
                        UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully relocated existing texture"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("SaveTextureToProject: Failed to relocate texture, falling back to copy"));
                    }
                }
            }
        }
        
        // 如果无法重用纹理，则创建新的纹理对象
        if (!bCanReuseTexture)
        {
            // 创建包
            Package = CreatePackage(*FullPackageName);
            if (!Package)
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to create package: %s"), *FullPackageName);
                return false;
            }
            Package->FullyLoad();

            // 创建新纹理对象
            UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_Transactional);
            if (!NewTexture)
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to create texture object"));
                return false;
            }

            // 复制源纹理的源数据
            if (Texture->Source.IsValid())
            {
                // 获取原始纹理的尺寸和格式
                int32 SizeX = Texture->Source.GetSizeX();
                int32 SizeY = Texture->Source.GetSizeY();
                int32 NumSlices = Texture->Source.GetNumSlices();
                int32 NumMips = Texture->Source.GetNumMips();
                ETextureSourceFormat Format = Texture->Source.GetFormat();
                
                // 获取原始纹理的源数据 - 使用正确的API
                TArray64<uint8> SourceData;
                if (Texture->Source.GetMipData(SourceData, 0))
                {
                    // 初始化新纹理的源数据
                    NewTexture->Source.Init(SizeX, SizeY, NumSlices, NumMips, Format, SourceData.GetData());
                    
                    UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully copied source data (%dx%d, %lld bytes)"), 
                           SizeX, SizeY, SourceData.Num());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to get source mip data"));
                    return false;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Source texture has no valid source data"));
                return false;
            }
            
            // 复制纹理属性
            NewTexture->CompressionSettings = Texture->CompressionSettings;
            NewTexture->Filter = Texture->Filter;
            NewTexture->AddressX = Texture->AddressX;
            NewTexture->AddressY = Texture->AddressY;
            NewTexture->LODGroup = Texture->LODGroup;
            NewTexture->SRGB = Texture->SRGB;
            NewTexture->MipGenSettings = Texture->MipGenSettings;
            
            // 触发纹理重建
            NewTexture->PostEditChange();
            NewTexture->UpdateResource();
            
            TextureToSave = NewTexture;
            
            UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully created texture copy"));
        }
        
        // 验证要保存的纹理是否有效
        if (!TextureToSave || !TextureToSave->GetPlatformData() || TextureToSave->GetPlatformData()->Mips.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Texture to save has no platform data or mips"));
            return false;
        }

        UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully prepared texture with %d mips"), 
               TextureToSave->GetPlatformData()->Mips.Num());

        // 标记包为脏
        Package->SetDirtyFlag(true);

        // 通知资产注册表
        if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            AssetRegistryModule.Get().AssetCreated(TextureToSave);
        }

        // 准备保存路径
        FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackageName, FPackageName::GetAssetPackageExtension());
        
        // 确保目录存在
        FString PackageDir = FPaths::GetPath(PackageFileName);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*PackageDir))
        {
            PlatformFile.CreateDirectoryTree(*PackageDir);
        }

        UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: About to save package to: %s"), *PackageFileName);

        // 最终验证纹理状态
        if (!IsValid(TextureToSave))
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: TextureToSave is not valid before save"));
            return false;
        }

        // 确保纹理平台数据已准备就绪
        if (TextureToSave->GetPlatformData())
        {
            // 强制等待纹理编译完成
            TextureToSave->FinishCachePlatformData();
            
            // 再次验证平台数据
            if (!TextureToSave->GetPlatformData() || TextureToSave->GetPlatformData()->Mips.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Platform data not ready after cache"));
                return false;
            }
        }

        // 确保包状态正确
        Package->SetDirtyFlag(true);
        Package->FullyLoad();

        // 保存包到磁盘 - 使用新的FSavePackageArgs API
        bool bSaved = false;
        try 
        {
            UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Calling SavePackage..."));
            
            // 使用新的FSavePackageArgs API
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Standalone;
            SaveArgs.SaveFlags = SAVE_None;
            SaveArgs.bForceByteSwapping = false;
            SaveArgs.bWarnOfLongFilename = true;
            SaveArgs.bSlowTask = false; // 避免UI阻塞
            // SaveArgs.TargetPlatform = nullptr;
            SaveArgs.FinalTimeStamp = FDateTime::MinValue();
            SaveArgs.Error = GError;
            
            bSaved = UPackage::SavePackage(Package, TextureToSave, *PackageFileName, SaveArgs);
            
            UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Save operation returned: %s"), bSaved ? TEXT("true") : TEXT("false"));
        }
        catch (const std::exception& Exception)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: std::exception during SavePackage: %hs"), Exception.what());
            return false;
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Unknown exception during SavePackage"));
            return false;
        }

        if (bSaved)
        {
            if (!FullPackageName.IsEmpty())
            {
                UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully saved texture to %s"), *FullPackageName);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Successfully saved texture (FullPackageName is empty)"));
            }
            return true;
        }
        else
        {
            if (!PackageFileName.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to save package to disk: %s"), *PackageFileName);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Failed to save package to disk (PackageFileName is empty)"));
            }
            return false;
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToProject: Exception occurred during save operation"));
        return false;
    }
}

FString SComfyUIWidget::GenerateUniqueAssetName(const FString& BaseName, const FString& PackagePath) const
{
    FString CleanBaseName = BaseName;
    
    // 移除非法字符
    CleanBaseName = CleanBaseName.Replace(TEXT(" "), TEXT("_"));
    CleanBaseName = CleanBaseName.Replace(TEXT("-"), TEXT("_"));
    CleanBaseName = CleanBaseName.Replace(TEXT("."), TEXT(""));
    CleanBaseName = CleanBaseName.Replace(TEXT(":"), TEXT(""));
    
    // 确保名称不为空
    if (CleanBaseName.IsEmpty())
    {
        CleanBaseName = TEXT("ComfyUI_Generated");
    }

    // 检查是否已经存在同名资产
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FString UniqueAssetName = CleanBaseName;
    int32 Counter = 1;

    while (true)
    {
        FString TestPackageName = PackagePath + TEXT("/") + UniqueAssetName;
        
        // 检查资产注册表中是否存在 - 使用新的API
        FSoftObjectPath SoftObjectPath(TestPackageName);
        FAssetData ExistingAsset = AssetRegistry.GetAssetByObjectPath(SoftObjectPath);
        if (!ExistingAsset.IsValid())
        {
            // 再检查磁盘上是否存在
            FString PackageFileName = FPackageName::LongPackageNameToFilename(TestPackageName, FPackageName::GetAssetPackageExtension());
            if (!FPaths::FileExists(PackageFileName))
            {
                break; // 找到唯一名称
            }
        }

        // 生成新的名称
        UniqueAssetName = FString::Printf(TEXT("%s_%d"), *CleanBaseName, Counter);
        Counter++;
    }

    return UniqueAssetName;
}

bool SComfyUIWidget::SaveTextureToFile(UTexture2D* Texture, const FString& FilePath)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Texture is null"));
        return false;
    }

    // 确定文件格式
    FString FileExtension = FPaths::GetExtension(FilePath).ToLower();
    EImageFormat ImageFormat = EImageFormat::PNG; // 默认PNG
    
    if (FileExtension == TEXT("jpg") || FileExtension == TEXT("jpeg"))
    {
        ImageFormat = EImageFormat::JPEG;
    }
    else if (FileExtension == TEXT("bmp"))
    {
        ImageFormat = EImageFormat::BMP;
    }
    else if (FileExtension == TEXT("png"))
    {
        ImageFormat = EImageFormat::PNG;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveTextureToFile: Unsupported format '%s', defaulting to PNG"), *FileExtension);
        ImageFormat = EImageFormat::PNG;
    }

    UE_LOG(LogTemp, Log, TEXT("SaveTextureToFile: Saving texture as %s to: %s"), 
        ImageFormat == EImageFormat::PNG ? TEXT("PNG") : 
        ImageFormat == EImageFormat::JPEG ? TEXT("JPEG") : TEXT("BMP"), 
        *FilePath);

    try
    {
        // 获取纹理的平台数据
        FTexturePlatformData* PlatformData = Texture->GetPlatformData();
        if (!PlatformData || PlatformData->Mips.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Texture has no platform data"));
            return false;
        }

        // 检查纹理像素格式
        EPixelFormat PixelFormat = PlatformData->PixelFormat;
        UE_LOG(LogTemp, Log, TEXT("SaveTextureToFile: Texture pixel format: %d"), (int32)PixelFormat);

        // 获取第一个Mip级别的数据
        const FTexture2DMipMap& Mip = PlatformData->Mips[0];
        int32 BulkDataSize = Mip.BulkData.GetBulkDataSize();
        
        if (BulkDataSize <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Invalid mip data size: %d"), BulkDataSize);
            return false;
        }

        int32 Width = Texture->GetSizeX();
        int32 Height = Texture->GetSizeY();
        
        UE_LOG(LogTemp, Log, TEXT("SaveTextureToFile: Texture dimensions: %dx%d, BulkDataSize: %d"), Width, Height, BulkDataSize);

        // 锁定纹理数据
        const void* TextureData = Mip.BulkData.LockReadOnly();
        if (!TextureData)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Could not lock texture data"));
            return false;
        }

        // 获取图像包装器模块
        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

        if (!ImageWrapper.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to create image wrapper"));
            Mip.BulkData.Unlock();
            return false;
        }

        bool bSetResult = false;

        // 根据纹理的像素格式选择合适的处理方式
        if (PixelFormat == PF_B8G8R8A8)
        {
            // 标准的BGRA8格式，可以直接使用
            int32 ExpectedDataSize = Width * Height * 4; // 4 bytes per pixel for BGRA8
            
            if (BulkDataSize >= ExpectedDataSize)
            {
                bSetResult = ImageWrapper->SetRaw(
                    TextureData,
                    ExpectedDataSize,  // 使用计算出的期望大小，而不是BulkData的实际大小
                    Width,
                    Height,
                    ERGBFormat::BGRA,
                    8
                );
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: BulkData size (%d) smaller than expected BGRA8 size (%d)"), BulkDataSize, ExpectedDataSize);
                Mip.BulkData.Unlock();
                return false;
            }
        }
        else if (PixelFormat == PF_R8G8B8A8)
        {
            // RGBA8格式
            int32 ExpectedDataSize = Width * Height * 4;
            
            if (BulkDataSize >= ExpectedDataSize)
            {
                bSetResult = ImageWrapper->SetRaw(
                    TextureData,
                    ExpectedDataSize,
                    Width,
                    Height,
                    ERGBFormat::RGBA,
                    8
                );
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: BulkData size (%d) smaller than expected RGBA8 size (%d)"), BulkDataSize, ExpectedDataSize);
                Mip.BulkData.Unlock();
                return false;
            }
        }
        else if (PixelFormat == PF_FloatRGBA)
        {
            // Float RGBA格式，需要转换为8位
            int32 ExpectedDataSize = Width * Height * 4 * sizeof(float);
            
            if (BulkDataSize >= ExpectedDataSize)
            {
                // 创建临时的8位数据缓冲区
                TArray<uint8> ConvertedData;
                ConvertedData.SetNum(Width * Height * 4);
                
                const float* FloatData = static_cast<const float*>(TextureData);
                for (int32 i = 0; i < Width * Height * 4; ++i)
                {
                    ConvertedData[i] = FMath::Clamp(FMath::RoundToInt(FloatData[i] * 255.0f), 0, 255);
                }
                
                bSetResult = ImageWrapper->SetRaw(
                    ConvertedData.GetData(),
                    ConvertedData.Num(),
                    Width,
                    Height,
                    ERGBFormat::RGBA,
                    8
                );
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: BulkData size (%d) smaller than expected FloatRGBA size (%d)"), BulkDataSize, ExpectedDataSize);
                Mip.BulkData.Unlock();
                return false;
            }
        }
        else
        {
            // 对于其他格式，尝试使用纹理的源数据
            UE_LOG(LogTemp, Warning, TEXT("SaveTextureToFile: Unsupported pixel format %d, trying to use source data"), (int32)PixelFormat);
            
            if (Texture->Source.IsValid())
            {
                TArray64<uint8> SourceData;
                if (Texture->Source.GetMipData(SourceData, 0))
                {
                    ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();
                    int32 SourceWidth = Texture->Source.GetSizeX();
                    int32 SourceHeight = Texture->Source.GetSizeY();
                    
                    // 根据源格式设置相应的参数
                    ERGBFormat RGBFormat = ERGBFormat::BGRA;
                    int32 BitDepth = 8;
                    
                    if (SourceFormat == TSF_BGRA8)
                    {
                        RGBFormat = ERGBFormat::BGRA;
                        BitDepth = 8;
                    }
                    // RBGA8格式已弃用
                    // else if (SourceFormat == TSF_RGBA8)
                    // {
                    //     RGBFormat = ERGBFormat::RGBA;
                    //     BitDepth = 8;
                    // }
                    else if (SourceFormat == TSF_RGBA16)
                    {
                        RGBFormat = ERGBFormat::RGBA;
                        BitDepth = 16;
                    }
                    else if (SourceFormat == TSF_RGBA16F)
                    {
                        // 16位浮点需要转换为8位
                        TArray<uint8> ConvertedData;
                        ConvertedData.SetNum(SourceWidth * SourceHeight * 4);
                        
                        const FFloat16* Float16Data = reinterpret_cast<const FFloat16*>(SourceData.GetData());
                        for (int32 i = 0; i < SourceWidth * SourceHeight * 4; ++i)
                        {
                            ConvertedData[i] = FMath::Clamp(FMath::RoundToInt(Float16Data[i].GetFloat() * 255.0f), 0, 255);
                        }
                        
                        bSetResult = ImageWrapper->SetRaw(
                            ConvertedData.GetData(),
                            ConvertedData.Num(),
                            SourceWidth,
                            SourceHeight,
                            ERGBFormat::RGBA,
                            8
                        );
                    }
                    
                    if (!bSetResult && SourceFormat != TSF_RGBA16F)
                    {
                        bSetResult = ImageWrapper->SetRaw(
                            SourceData.GetData(),
                            SourceData.Num(),
                            SourceWidth,
                            SourceHeight,
                            RGBFormat,
                            BitDepth
                        );
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to get source data from texture"));
                    Mip.BulkData.Unlock();
                    return false;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Texture has no source data and unsupported pixel format"));
                Mip.BulkData.Unlock();
                return false;
            }
        }

        // 解锁纹理数据
        Mip.BulkData.Unlock();

        if (!bSetResult)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to set image data in wrapper"));
            return false;
        }

        // 获取压缩后的图像数据
        TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(
            ImageFormat == EImageFormat::JPEG ? 85 : 100  // JPEG质量85，其他格式100
        );

        if (CompressedData.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to compress image data"));
            return false;
        }

        // 确保保存目录存在
        FString SaveDirectory = FPaths::GetPath(FilePath);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*SaveDirectory))
        {
            if (!PlatformFile.CreateDirectoryTree(*SaveDirectory))
            {
                UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to create directory: %s"), *SaveDirectory);
                return false;
            }
        }

        // 保存到文件
        if (FFileHelper::SaveArrayToFile(CompressedData, *FilePath))
        {
            UE_LOG(LogTemp, Log, TEXT("SaveTextureToFile: Successfully saved %d bytes to: %s"), 
                CompressedData.Num(), *FilePath);
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to write file: %s"), *FilePath);
            return false;
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Exception occurred during file save"));
        return false;
    }
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
    DetectedCustomWorkflowType = EWorkflowType::TextToImage;
    
    if (WorkflowName.IsEmpty())
    {
        return;
    }
    
    // 创建ComfyUI客户端来分析工作流
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    if (!Client)
    {
        return;
    }
    
    // 构建工作流文件路径
    FString TemplatesDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration/Config/Templates");
    FString WorkflowFile = TemplatesDir / (WorkflowName + TEXT(".json"));
    
    // 检查文件是否存在
    if (!FPaths::FileExists(WorkflowFile))
    {
        UE_LOG(LogTemp, Warning, TEXT("DetectWorkflowType: Workflow file not found: %s"), *WorkflowFile);
        return;
    }
    
    // 读取并验证工作流文件
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *WorkflowFile))
    {
        UE_LOG(LogTemp, Error, TEXT("DetectWorkflowType: Failed to load workflow file: %s"), *WorkflowFile);
        return;
    }
    
    // 使用ComfyUIClient分析工作流
    FWorkflowConfig Config;
    FString ValidationError;
    
    if (Client->ValidateWorkflowJson(JsonContent, Config, ValidationError))
    {
        // 分析工作流类型
        DetectedCustomWorkflowType = AnalyzeWorkflowTypeFromConfig(Config);
        
        UE_LOG(LogTemp, Log, TEXT("DetectWorkflowType: Detected workflow type for '%s': %d"), 
               *WorkflowName, (int32)DetectedCustomWorkflowType);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DetectWorkflowType: Failed to validate workflow '%s': %s"), 
               *WorkflowName, *ValidationError);
    }
}

SComfyUIWidget::EWorkflowType SComfyUIWidget::AnalyzeWorkflowTypeFromConfig(const FWorkflowConfig& Config)
{
    // 基于输入输出节点分析工作流类型
    // 这是一个简化的检测逻辑，实际应用中可能需要更复杂的分析
    
    bool bHasTextInput = false;
    bool bHasImageInput = false;
    bool bHas3DOutput = false;
    bool bHasImageOutput = false;
    bool bHasTextureOutput = false;
    
    // 分析必需的输入参数
    for (const FString& Input : Config.RequiredInputs)
    {
        FString LowerInput = Input.ToLower();
        
        if (LowerInput.Contains(TEXT("prompt")) || 
            LowerInput.Contains(TEXT("text")) ||
            LowerInput.Contains(TEXT("positive")))
        {
            bHasTextInput = true;
        }
        else if (LowerInput.Contains(TEXT("image")) ||
                 LowerInput.Contains(TEXT("input_image")) ||
                 LowerInput.Contains(TEXT("img")))
        {
            bHasImageInput = true;
        }
    }
    
    // 分析输出节点
    for (const FString& Output : Config.OutputNodes)
    {
        FString LowerOutput = Output.ToLower();
        
        if (LowerOutput.Contains(TEXT("3d")) ||
            LowerOutput.Contains(TEXT("mesh")) ||
            LowerOutput.Contains(TEXT("model")) ||
            LowerOutput.Contains(TEXT("obj")) ||
            LowerOutput.Contains(TEXT("ply")))
        {
            bHas3DOutput = true;
        }
        else if (LowerOutput.Contains(TEXT("texture")) ||
                 LowerOutput.Contains(TEXT("material")) ||
                 LowerOutput.Contains(TEXT("diffuse")) ||
                 LowerOutput.Contains(TEXT("normal")))
        {
            bHasTextureOutput = true;
        }
        else if (LowerOutput.Contains(TEXT("image")) ||
                 LowerOutput.Contains(TEXT("output")) ||
                 LowerOutput.Contains(TEXT("save")))
        {
            bHasImageOutput = true;
        }
    }
    
    // 根据工作流名称进行额外的检测
    FString LowerWorkflowName = Config.Name.ToLower();
    if (LowerWorkflowName.Contains(TEXT("img2img")) || LowerWorkflowName.Contains(TEXT("image_to_image")))
    {
        return EWorkflowType::ImageToImage;
    }
    else if (LowerWorkflowName.Contains(TEXT("txt2img")) || LowerWorkflowName.Contains(TEXT("text_to_image")))
    {
        return EWorkflowType::TextToImage;
    }
    else if (LowerWorkflowName.Contains(TEXT("3d")))
    {
        if (bHasImageInput)
        {
            return EWorkflowType::ImageTo3D;
        }
        else
        {
            return EWorkflowType::TextTo3D;
        }
    }
    else if (LowerWorkflowName.Contains(TEXT("texture")) || LowerWorkflowName.Contains(TEXT("material")))
    {
        return EWorkflowType::TextureGeneration;
    }
    
    // 基于输入输出组合进行推断
    if (bHas3DOutput)
    {
        if (bHasImageInput)
        {
            return EWorkflowType::ImageTo3D;
        }
        else if (bHasTextInput)
        {
            return EWorkflowType::TextTo3D;
        }
    }
    else if (bHasTextureOutput)
    {
        return EWorkflowType::TextureGeneration;
    }
    else if (bHasImageInput && bHasImageOutput)
    {
        return EWorkflowType::ImageToImage;
    }
    else if (bHasTextInput && bHasImageOutput)
    {
        return EWorkflowType::TextToImage;
    }
    
    // 默认返回未知类型
    UE_LOG(LogTemp, Warning, TEXT("AnalyzeWorkflowTypeFromConfig: Unable to determine workflow type for config: %s"), *Config.Name);
    return EWorkflowType::Unknown;
}

#undef LOCTEXT_NAMESPACE
