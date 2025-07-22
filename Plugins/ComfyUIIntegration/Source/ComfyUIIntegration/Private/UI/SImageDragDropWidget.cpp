#include "UI/SImageDragDropWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SImageDragDropWidget"

void SImageDragDropWidget::Construct(const FArguments& InArgs)
{
    PreviewSize = InArgs._PreviewSize;
    AllowedClasses = InArgs._AllowedClasses;
    OnImageDropped = InArgs._OnImageDropped;
    
    // 确保至少有一个允许的类型
    if (AllowedClasses.Num() == 0)
    {
        AllowedClasses.Add(UTexture2D::StaticClass());
    }
    
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(this, &SImageDragDropWidget::GetBorderBrush)
        .BorderBackgroundColor(this, &SImageDragDropWidget::GetBackgroundColor)
        .Padding(5.0f)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(PreviewSize.X)
            .HeightOverride(PreviewSize.Y)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(SOverlay)
                
                // 图像预览层
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SAssignNew(ImagePreview, SImage)
                    .Image(FAppStyle::GetBrush("WhiteBrush"))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
                ]
                
                // 提示文本层
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(this, &SImageDragDropWidget::GetHintText)
                    .Visibility(this, &SImageDragDropWidget::GetHintTextVisibility)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                    .ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
                    .Justification(ETextJustify::Center)
                    .AutoWrapText(true)
                ]
            ]
        ]
    ];
}

void SImageDragDropWidget::SetImage(UTexture2D* InTexture)
{
    CurrentTexture = InTexture;
    UpdateImagePreview();
}

void SImageDragDropWidget::ClearImage()
{
    CurrentTexture = nullptr;
    CurrentImageBrush = nullptr;
    UpdateImagePreview();
}

void SImageDragDropWidget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    if (IsValidDragDrop(DragDropEvent))
    {
        bIsDragHovering = true;
    }
    
    SCompoundWidget::OnDragEnter(MyGeometry, DragDropEvent);
}

void SImageDragDropWidget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
    bIsDragHovering = false;
    SCompoundWidget::OnDragLeave(DragDropEvent);
}

FReply SImageDragDropWidget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    if (IsValidDragDrop(DragDropEvent))
    {
        return FReply::Handled();
    }
    
    return FReply::Unhandled();
}

FReply SImageDragDropWidget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    bIsDragHovering = false;
    
    if (IsValidDragDrop(DragDropEvent))
    {
        UTexture2D* DroppedTexture = ExtractTextureFromDragDrop(DragDropEvent);
        if (DroppedTexture)
        {
            SetImage(DroppedTexture);
            
            // 触发回调
            if (OnImageDropped.IsBound())
            {
                OnImageDropped.ExecuteIfBound(DroppedTexture);
            }
            
            // 显示成功通知
            FNotificationInfo Info(FText::Format(
                LOCTEXT("ImageDroppedNotification", "已加载图像: {0}"),
                FText::FromString(DroppedTexture->GetName())
            ));
            Info.ExpireDuration = 3.0f;
            Info.bFireAndForget = true;
            Info.Image = FAppStyle::GetBrush("Icons.SuccessWithColor");
            
            FSlateNotificationManager::Get().AddNotification(Info);
            
            return FReply::Handled();
        }
    }
    
    return FReply::Unhandled();
}

bool SImageDragDropWidget::IsValidDragDrop(const FDragDropEvent& DragDropEvent) const
{
    // 检查是否是资产拖拽操作
    TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
    if (!AssetDragDropOp.IsValid())
    {
        return false;
    }
    
    // 检查是否有资产
    const TArray<FAssetData>& Assets = AssetDragDropOp->GetAssets();
    if (Assets.Num() == 0)
    {
        return false;
    }
    
    // 检查是否是允许的资产类型
    for (const FAssetData& Asset : Assets)
    {
        UClass* AssetClass = Asset.GetClass();
        if (AssetClass)
        {
            for (UClass* AllowedClass : AllowedClasses)
            {
                if (AssetClass->IsChildOf(AllowedClass))
                {
                    return true;
                }
            }
        }
    }
    
    return false;
}

UTexture2D* SImageDragDropWidget::ExtractTextureFromDragDrop(const FDragDropEvent& DragDropEvent) const
{
    TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
    if (!AssetDragDropOp.IsValid())
    {
        return nullptr;
    }
    
    const TArray<FAssetData>& Assets = AssetDragDropOp->GetAssets();
    if (Assets.Num() == 0)
    {
        return nullptr;
    }
    
    // 获取第一个有效的纹理资产
    for (const FAssetData& Asset : Assets)
    {
        UClass* AssetClass = Asset.GetClass();
        if (AssetClass && AssetClass->IsChildOf(UTexture2D::StaticClass()))
        {
            UObject* AssetObject = Asset.GetAsset();
            if (UTexture2D* Texture = Cast<UTexture2D>(AssetObject))
            {
                return Texture;
            }
        }
    }
    
    return nullptr;
}

void SImageDragDropWidget::UpdateImagePreview()
{
    if (CurrentTexture)
    {
        // 创建或更新画刷
        if (!CurrentImageBrush.IsValid())
        {
            CurrentImageBrush = MakeShareable(new FSlateBrush());
        }
        
        CurrentImageBrush->SetResourceObject(CurrentTexture);
        CurrentImageBrush->DrawAs = ESlateBrushDrawType::Image;
        CurrentImageBrush->Tiling = ESlateBrushTileType::NoTile;
        
        // 计算适合的预览尺寸
        FVector2D OriginalSize = FVector2D(CurrentTexture->GetSizeX(), CurrentTexture->GetSizeY());
        FVector2D FitSize = CalculateImageFitSize(OriginalSize, PreviewSize);
        CurrentImageBrush->ImageSize = FitSize;
        
        // 更新图像预览
        if (ImagePreview.IsValid())
        {
            ImagePreview->SetImage(CurrentImageBrush.Get());
            ImagePreview->SetColorAndOpacity(FLinearColor::White);
        }
    }
    else
    {
        // 清除图像预览
        if (ImagePreview.IsValid())
        {
            ImagePreview->SetImage(FAppStyle::GetBrush("WhiteBrush"));
            ImagePreview->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f));
        }
        CurrentImageBrush = nullptr;
    }
}

FVector2D SImageDragDropWidget::CalculateImageFitSize(const FVector2D& ImageSize, const FVector2D& ContainerSize) const
{
    if (ImageSize.X <= 0 || ImageSize.Y <= 0)
    {
        return ContainerSize;
    }
    
    float ScaleX = ContainerSize.X / ImageSize.X;
    float ScaleY = ContainerSize.Y / ImageSize.Y;
    float Scale = FMath::Min(ScaleX, ScaleY);
    
    return FVector2D(ImageSize.X * Scale, ImageSize.Y * Scale);
}

const FSlateBrush* SImageDragDropWidget::GetBorderBrush() const
{
    if (bIsDragHovering)
    {
        return FAppStyle::GetBrush("ToolPanel.DarkGroupBorder");
    }
    else
    {
        return FAppStyle::GetBrush("ToolPanel.GroupBorder");
    }
}

FSlateColor SImageDragDropWidget::GetBackgroundColor() const
{
    if (bIsDragHovering)
    {
        return FLinearColor(0.2f, 0.6f, 1.0f, 0.3f); // 蓝色高亮
    }
    else
    {
        return FLinearColor::White;
    }
}

FText SImageDragDropWidget::GetHintText() const
{
    if (CurrentTexture)
    {
        return FText::FromString(CurrentTexture->GetName());
    }
    else if (bIsDragHovering)
    {
        return LOCTEXT("DropHint", "释放以设置图像");
    }
    else
    {
        return LOCTEXT("DragDropHint", "拖拽纹理资产到此处\n或点击按钮选择");
    }
}

EVisibility SImageDragDropWidget::GetHintTextVisibility() const
{
    // 有图像时显示图像名称，无图像时显示提示文本
    return EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
