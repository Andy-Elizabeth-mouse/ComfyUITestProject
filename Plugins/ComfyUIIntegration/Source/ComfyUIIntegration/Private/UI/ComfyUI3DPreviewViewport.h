#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

/**
 * 自定义视口客户端，用于3D模型预览
 */
class FComfyUI3DPreviewViewportClient : public FEditorViewportClient
{
public:
    FComfyUI3DPreviewViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr);
    virtual ~FComfyUI3DPreviewViewportClient();

    // 设置要预览的静态网格
    void SetPreviewMesh(UStaticMesh* InStaticMesh);
    
    // 清除预览内容
    void ClearPreview();
    
    // 重置相机和光照到默认状态
    void ResetToDefaults();
    
    // 光照控制方法
    void SetupDefaultLighting();
    void SetLightDirection(const FRotator& Direction);

    // FEditorViewportClient 接口
    virtual void Tick(float DeltaSeconds) override;
    virtual bool InputKey(const FInputKeyEventArgs& InEventArgs) override;
    virtual bool InputAxis(FViewport* InViewport, FInputDeviceId ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
    virtual FLinearColor GetBackgroundColor() const override;

protected:
    /** 预览用的静态网格组件 */
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> PreviewMeshComponent;
    
    /** 当前预览的静态网格 */
    UPROPERTY()
    TObjectPtr<UStaticMesh> CurrentPreviewMesh;

private:
    void SetupDefaultCamera();
    void UpdatePreviewMesh();
    void UpdateCameraPosition();
    
    /** 相机控制状态 */
    bool bIsOrbitingCamera;
    bool bIsPanningCamera;
    FVector2D LastMousePosition;
    float CameraDistance;
    FVector CameraTargetLocation;
    
    /** 光源控制 */
    bool bLightFollowsCamera; // 光源是否跟随摄像机
};

/**
 * 3D模型预览视口小部件
 */
class SComfyUI3DPreviewViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SComfyUI3DPreviewViewport) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SComfyUI3DPreviewViewport();

    // 设置预览网格
    void SetPreviewMesh(UStaticMesh* InStaticMesh);
    
    // 清除预览
    void ClearPreview();

protected:
    // SEditorViewport 接口
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual TSharedPtr<SWidget> MakeViewportToolbar() override;

private:
    /** 视口客户端 */
    TSharedPtr<FComfyUI3DPreviewViewportClient> ViewportClient;
    
    /** 预览场景 */
    TUniquePtr<FPreviewScene> PreviewScene;
};
