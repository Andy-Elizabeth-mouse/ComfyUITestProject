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
    
    /** 析构函数 - 清理资源 */
    virtual ~SComfyUIWidget();

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
    
    /** 输入模型路径（用于纹理生成等工作流） */
    FString InputModelPath;

    /** 当前活动的ComfyUI客户端（用于取消操作） */
    UPROPERTY()
    UComfyUIClient* CurrentClient;

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

    /** 模型输入相关UI组件 */
    TSharedPtr<SEditableTextBox> ModelPathTextBox;
    TSharedPtr<SButton> LoadModelButton;
    TSharedPtr<SButton> ClearModelButton;

    /** 生成的内容预览 */
    TSharedPtr<class SImage> ImagePreview;
    TSharedPtr<class SWidgetSwitcher> ContentSwitcher;
    TSharedPtr<class SComfyUI3DPreviewViewport> ModelViewport;
    TSharedPtr<class STextBlock> ModelPreviewWidget;
    UTexture2D* GeneratedTexture;
    UStaticMesh* GeneratedMesh;
    
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
    TSharedRef<SWidget> CreateModelInputWidget();

    /** 事件处理 */
    FReply OnGenerateClicked();
    FReply OnCancelClicked();
    FReply OnSaveClicked();
    FReply OnSaveAsClicked();
    FReply OnImportWorkflowClicked();
    FReply OnRefreshWorkflowsClicked();
    FReply OnValidateWorkflowClicked();
    FReply OnLoadImageClicked();
    FReply OnClearImageClicked();
    FReply OnLoadModelClicked();
    FReply OnClearModelClicked();
    
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
    
    /** 检查当前工作流是否需要图像输入 */
    bool DoesCurrentWorkflowNeedImage() const;
    
    /** 计算图片在指定容器中的适配大小（保持比例） */
    FVector2D CalculateImageFitSize(const FVector2D& ImageSize, const FVector2D& ContainerSize) const;
    
    /** UI可见性控制 */
    EVisibility GetCustomWorkflowVisibility() const;
    EVisibility GetInputImageVisibility() const;
    EVisibility GetModelInputVisibility() const;
    EVisibility GetPromptInputVisibility() const;
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
    
    /** 内容生成回调 */
    void OnImageGenerationComplete(UTexture2D* GeneratedImage);
    void OnMeshGenerationComplete(UStaticMesh* InGeneratedMesh);
    void OnGenerationProgressUpdate(const FComfyUIProgressInfo& ProgressInfo);
    void OnGenerationStarted(const FString& PromptId);
    void OnGenerationCompleted();
    
    /** 内容预览管理 */
    void SwitchToImagePreview();
    void SwitchToModelPreview();
    void ClearContentPreview();
    
    /** 文件保存通知 */
    void ShowSaveSuccessNotification(const FString& AssetPath);
    void ShowSaveErrorNotification(const FString& ErrorMessage);
};
