#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Engine/Texture2D.h"
#include "ComfyUIDelegates.h"
#include "Styling/SlateBrush.h"
#include "Workflow/ComfyUIWorkflowConfig.h"

class UComfyUIClient;
class SImageDragDropWidget;

/**
 * ComfyUI集成的主界面Widget
 */
class COMFYUIINTEGRATION_API SComfyUIWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SComfyUIWidget)
    {}
    SLATE_END_ARGS()

    /** 构造Widget */
    void Construct(const FArguments& InArgs);

private:
    /** 当前选择的工作流类型 */
    TSharedPtr<EComfyUIWorkflowType> CurrentWorkflowType;
    
    /** 工作流类型选项 */
    TArray<TSharedPtr<EComfyUIWorkflowType>> WorkflowOptions;
    
    /** 自定义工作流名称列表 */
    TArray<TSharedPtr<FString>> CustomWorkflowNames;
    
    /** 当前选择的自定义工作流 */
    TSharedPtr<FString> CurrentCustomWorkflow;
    
    /** 检测到的自定义工作流类型 */
    EComfyUIWorkflowType DetectedCustomWorkflowType = EComfyUIWorkflowType::TextToImage;

    /** 生成按钮状态 */
    bool bIsGenerating = false;
    
    /** 当前生成进度信息 */
    FComfyUIProgressInfo CurrentProgressInfo;
    
    /** 输入图像（用于图生图等工作流） */
    UTexture2D* InputImage = nullptr;
    TSharedPtr<FSlateBrush> InputImageBrush;

    /** UI组件 */
    TSharedPtr<SMultiLineEditableTextBox> PromptTextBox;
    TSharedPtr<SMultiLineEditableTextBox> NegativePromptTextBox;
    TSharedPtr<SComboBox<TSharedPtr<EComfyUIWorkflowType>>> WorkflowTypeComboBox;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> CustomWorkflowComboBox;
    TSharedPtr<SEditableTextBox> ComfyUIServerUrlTextBox;
    TSharedPtr<SButton> GenerateButton;
    TSharedPtr<SButton> SaveButton;
    TSharedPtr<SButton> SaveAsButton;
    TSharedPtr<SButton> ImportWorkflowButton;
    TSharedPtr<SButton> RefreshWorkflowsButton;
    TSharedPtr<SButton> PreviewButton;
    TSharedPtr<SButton> CancelButton;
    
    /** 进度相关UI组件 */
    TSharedPtr<SProgressBar> GenerationProgressBar;
    TSharedPtr<class STextBlock> ProgressStatusText;
    TSharedPtr<class STextBlock> QueuePositionText;
    TSharedPtr<class STextBlock> CurrentNodeText;
    
    /** 工作流类型标注显示 */
    TSharedPtr<class STextBlock> WorkflowTypeLabel;
    
    /** 输入图像相关UI组件 */
    TSharedPtr<class SImage> InputImagePreview;
    TSharedPtr<SImageDragDropWidget> InputImageDragDropWidget;
    TSharedPtr<SButton> LoadImageButton;
    TSharedPtr<SButton> ClearImageButton;

    /** 生成的图像预览 */
    TSharedPtr<class SImage> ImagePreview;
    UTexture2D* GeneratedTexture;
    
    /** 当前图像的Slate画刷，保持引用避免被释放 */
    TSharedPtr<FSlateBrush> CurrentImageBrush;

    /** 创建UI布局 */
    TSharedRef<SWidget> CreateWorkflowSelectionWidget();
    TSharedRef<SWidget> CreatePromptInputWidget();
    TSharedRef<SWidget> CreateServerConfigWidget();
    TSharedRef<SWidget> CreateControlButtonsWidget();
    TSharedRef<SWidget> CreateProgressWidget();
    TSharedRef<SWidget> CreateImagePreviewWidget();
    TSharedRef<SWidget> CreateInputImageWidget();

    /** 事件处理 */
    FReply OnGenerateClicked();
    FReply OnCancelClicked();
    FReply OnSaveClicked();
    FReply OnSaveAsClicked();
    FReply OnPreviewClicked();
    FReply OnImportWorkflowClicked();
    FReply OnRefreshWorkflowsClicked();
    FReply OnValidateWorkflowClicked();
    FReply OnLoadImageClicked();
    FReply OnClearImageClicked();
    
    /** 拖拽图像处理 */
    void OnImageDropped(UTexture2D* DroppedTexture);

    /** ComboBox事件 */
    TSharedRef<SWidget> OnGeneratEComfyUIWorkflowTypeWidget(TSharedPtr<EComfyUIWorkflowType> InOption);
    FText GetWorkflowTypeText(TSharedPtr<EComfyUIWorkflowType> InOption) const;
    void OnWorkflowTypeChanged(TSharedPtr<EComfyUIWorkflowType> NewSelection, ESelectInfo::Type SelectInfo);
    
    /** 自定义工作流ComboBox事件 */
    TSharedRef<SWidget> OnGenerateCustomWorkflowWidget(TSharedPtr<FString> InOption);
    FText GetCustomWorkflowText(TSharedPtr<FString> InOption) const;
    void OnCustomWorkflowChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    
    /** 工具函数 */
    void RefreshCustomWorkflowList();
    void UpdateWorkflowVisibility();
    void DetectWorkflowType(const FString& WorkflowName);
    EComfyUIWorkflowType AnalyzEComfyUIWorkflowTypeFromConfig(const FWorkflowConfig& Config);
    
    /** 计算图片在指定容器中的适配大小（保持比例） */
    FVector2D CalculateImageFitSize(const FVector2D& ImageSize, const FVector2D& ContainerSize) const;
    
    /** UI可见性控制 */
    EVisibility GetCustomWorkflowVisibility() const;
    EVisibility GetInputImageVisibility() const;
    EVisibility GetProgressVisibility() const;
    EVisibility GetCancelButtonVisibility() const;
    FText GetCurrentWorkflowTypeText() const;
    FText GetDetectedWorkflowTypeText() const;
    
    /** 生成按钮文本控制 */
    FText GetGenerateButtonText() const;
    bool IsGenerateButtonEnabled() const;

    /** 进度信息获取 */
    TOptional<float> GetProgressPercent() const;
    FText GetProgressStatusText() const;
    FText GetQueuePositionText() const;
    FText GetCurrentNodeText() const;

    /** 工作流类型转换 */
    FText WorkflowTypeToText(EComfyUIWorkflowType Type) const;
    
    /** 图像生成回调 */
    void OnImageGenerationComplete(UTexture2D* GeneratedImage);
    void OnGenerationProgressUpdate(const FComfyUIProgressInfo& ProgressInfo);
    void OnGenerationStarted(const FString& PromptId);
    void OnGenerationCompleted();
    
    /** 3D模型生成回调 */
    void On3DModelGenerationComplete(const TArray<uint8>& ModelData);
    
    /** PBR纹理集生成回调 */
    void OnTextureGenerationComplete(UTexture2D* NewGeneratedTexture);
    
    /** 获取输入图像路径（用于图生3D） */
    FString GetSelectedInputImagePath() const;
    
    /** 文件保存通知 */
    void ShowSaveSuccessNotification(const FString& AssetPath);
    void ShowSaveErrorNotification(const FString& ErrorMessage);
};
