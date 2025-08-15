#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "AssetRegistry/AssetData.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Input/Reply.h"
#include "Input/DragAndDrop.h"

DECLARE_DELEGATE_OneParam(FOnImageDropped, UTexture2D*);

/**
 * 支持拖拽的图像输入Widget
 * 支持从内容浏览器拖拽纹理资产
 */
class COMFYUIINTEGRATION_API SImageDragDropWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SImageDragDropWidget)
        : _PreviewSize(FVector2D(150.0f, 150.0f))
        , _AllowedClasses()
    {
        _AllowedClasses.Add(UTexture2D::StaticClass());
    }
    
    /** 预览图像的大小 */
    SLATE_ARGUMENT(FVector2D, PreviewSize)
    
    /** 允许的资产类型 */
    SLATE_ARGUMENT(TArray<UClass*>, AllowedClasses)
    
    /** 图像拖拽完成的回调 */
    SLATE_EVENT(FOnImageDropped, OnImageDropped)
    
    SLATE_END_ARGS()

    /** 构造Widget */
    void Construct(const FArguments& InArgs);

    /** 设置当前显示的图像 */
    void SetImage(UTexture2D* InTexture);
    
    /** 获取当前图像 */
    UTexture2D* GetImage() const { return CurrentTexture.Get(); }
    
    /** 清除当前图像 */
    void ClearImage();

protected:
    /** 处理拖拽进入 */
    virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
    
    /** 处理拖拽离开 */
    virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
    
    /** 处理拖拽悬停 */
    virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
    
    /** 处理拖拽放下 */
    virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
    /** 当前显示的纹理 */
    UPROPERTY()
    TWeakObjectPtr<UTexture2D> CurrentTexture;
    
    /** 当前图像的Slate画刷 */
    UPROPERTY()
    TSharedPtr<FSlateBrush> CurrentImageBrush;
    
    /** 预览图像的大小 */
    UPROPERTY()
    FVector2D PreviewSize;
    
    /** 允许的资产类型 */
    UPROPERTY()
    TArray<UClass*> AllowedClasses;
    
    /** 图像拖拽完成的回调 */
    FOnImageDropped OnImageDropped;
    
    /** 是否正在拖拽悬停 */
    bool bIsDragHovering = false;
    
    /** 图像预览Widget */
    UPROPERTY()
    TSharedPtr<class SImage> ImagePreview;
    
    /** 检查拖拽操作是否有效 */
    bool IsValidDragDrop(const FDragDropEvent& DragDropEvent) const;
    
    /** 从拖拽操作中提取纹理 */
    UTexture2D* ExtractTextureFromDragDrop(const FDragDropEvent& DragDropEvent) const;
    
    /** 更新图像预览 */
    void UpdateImagePreview();
    
    /** 计算图片在指定容器中的适配大小（保持比例） */
    FVector2D CalculateImageFitSize(const FVector2D& ImageSize, const FVector2D& ContainerSize) const;
    
    /** 获取拖拽时的边框样式 */
    const FSlateBrush* GetBorderBrush() const;
    
    /** 获取拖拽时的背景颜色 */
    FSlateColor GetBackgroundColor() const;
    
    /** 获取提示文本 */
    FText GetHintText() const;
    
    /** 获取提示文本可见性 */
    EVisibility GetHintTextVisibility() const;
};
