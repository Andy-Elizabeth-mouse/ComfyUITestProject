#include "UI/ComfyUI3DPreviewViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/EngineVersionComparison.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

// ========== FComfyUI3DPreviewViewportClient ==========

FComfyUI3DPreviewViewportClient::FComfyUI3DPreviewViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
    : FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
    , PreviewMeshComponent(nullptr)
    , CurrentPreviewMesh(nullptr)
    , bIsOrbitingCamera(false)
    , bIsPanningCamera(false)
    , LastMousePosition(FVector2D::ZeroVector)
    , CameraDistance(200.0f)
    , CameraTargetLocation(FVector::ZeroVector)
    , bLightFollowsCamera(true)
{
    // 设置视口类型为透视视图
    ViewportType = LVT_Perspective;
    
    // 启用实时更新
    SetRealtime(true);
    
    // 只有在PreviewScene有效时才创建组件
    if (InPreviewScene && IsValid(InPreviewScene->GetWorld()))
    {
        PreviewMeshComponent = NewObject<UStaticMeshComponent>(InPreviewScene->GetWorld());
        if (IsValid(PreviewMeshComponent))
        {
            PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PreviewMeshComponent->SetCastShadow(true);
            PreviewMeshComponent->SetFlags(RF_Transient);
            InPreviewScene->AddComponent(PreviewMeshComponent, FTransform::Identity);
        }
    }
    
    SetupDefaultCamera();
    
    // 设置默认光照
    if (GetPreviewScene())
    {
        SetupDefaultLighting();
    }
}

FComfyUI3DPreviewViewportClient::~FComfyUI3DPreviewViewportClient()
{
    // 不主动销毁组件，让PreviewScene处理
    // 强制调用DestroyComponent可能导致UObjectArray越界
    PreviewMeshComponent = nullptr;
    CurrentPreviewMesh = nullptr;
}

void FComfyUI3DPreviewViewportClient::SetPreviewMesh(UStaticMesh* InStaticMesh)
{
    CurrentPreviewMesh = InStaticMesh;
    if (IsValid(CurrentPreviewMesh))
    {
        CurrentPreviewMesh->SetFlags(RF_Transient);
    }
    UpdatePreviewMesh();
}

void FComfyUI3DPreviewViewportClient::ClearPreview()
{
    CurrentPreviewMesh = nullptr;
    UpdatePreviewMesh();
}

// void FComfyUI3DPreviewViewportClient::Tick(float DeltaSeconds)
// {
//     FEditorViewportClient::Tick(DeltaSeconds);
    
//     // 可以在这里添加动画或其他逻辑
// }

bool FComfyUI3DPreviewViewportClient::InputKey(const FInputKeyEventArgs& InEventArgs)
{
    // 处理鼠标按键
    if (InEventArgs.Key == EKeys::LeftMouseButton)
    {
        if (InEventArgs.Event == IE_Pressed)
        {
            bIsOrbitingCamera = true;
            LastMousePosition = FVector2D(InEventArgs.Viewport->GetMouseX(), InEventArgs.Viewport->GetMouseY());
            return true;
        }
        else if (InEventArgs.Event == IE_Released)
        {
            bIsOrbitingCamera = false;
            return true;
        }
    }
    else if (InEventArgs.Key == EKeys::RightMouseButton)
    {
        if (InEventArgs.Event == IE_Pressed)
        {
            bIsPanningCamera = true;
            LastMousePosition = FVector2D(InEventArgs.Viewport->GetMouseX(), InEventArgs.Viewport->GetMouseY());
            return true;
        }
        else if (InEventArgs.Event == IE_Released)
        {
            bIsPanningCamera = false;
            return true;
        }
    }
    else if (InEventArgs.Key == EKeys::MouseScrollUp)
    {
        // 缩放摄像机（拉近）
        CameraDistance = FMath::Max(CameraDistance * 0.9f, 50.0f);
        UpdateCameraPosition();
        return true;
    }
    else if (InEventArgs.Key == EKeys::MouseScrollDown)
    {
        // 缩放摄像机（拉远）
        CameraDistance = FMath::Min(CameraDistance * 1.1f, 2000.0f);
        UpdateCameraPosition();
        return true;
    }
    
    return FEditorViewportClient::InputKey(InEventArgs);
}

bool FComfyUI3DPreviewViewportClient::InputAxis(FViewport* InViewport, FInputDeviceId ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
    // 处理鼠标移动
    if (Key == EKeys::MouseX || Key == EKeys::MouseY)
    {
        if (bIsOrbitingCamera || bIsPanningCamera)
        {
            FVector2D CurrentMousePosition(InViewport->GetMouseX(), InViewport->GetMouseY());
            FVector2D MouseDelta = CurrentMousePosition - LastMousePosition;
            
            if (bIsOrbitingCamera)
            {
                // 环绕摄像机旋转
                float YawDelta = MouseDelta.X * 0.5f;
                float PitchDelta = MouseDelta.Y * 0.5f;
                
                FRotator CurrentRotation = GetViewRotation();
                CurrentRotation.Yaw += YawDelta;
                CurrentRotation.Pitch = FMath::Clamp(CurrentRotation.Pitch - PitchDelta, -80.0f, 80.0f);
                
                SetViewRotation(CurrentRotation);
                UpdateCameraPosition();
            }
            else if (bIsPanningCamera)
            {
                // 平移摄像机
                FVector RightVector = GetViewRotation().RotateVector(FVector::RightVector);
                FVector UpVector = GetViewRotation().RotateVector(FVector::UpVector);
                
                FVector PanDelta = (RightVector * -MouseDelta.X + UpVector * MouseDelta.Y) * 0.5f;
                CameraTargetLocation += PanDelta;
                UpdateCameraPosition();
            }
            
            LastMousePosition = CurrentMousePosition;
            return true;
        }
    }
    
    return FEditorViewportClient::InputAxis(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
}

FLinearColor FComfyUI3DPreviewViewportClient::GetBackgroundColor() const
{
    // 返回深灰色背景
    return FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void FComfyUI3DPreviewViewportClient::SetupDefaultLighting()
{
    FPreviewScene* _PreviewScene = GetPreviewScene();
    if (!_PreviewScene) return;
    
    // 设置默认的适中光照强度
    _PreviewScene->SetLightBrightness(0.05f);  // 降低定向光强度，避免过曝
    // PreviewScene->SetSkyBrightness(1.0f);  // 设置环境光强度
    
    // 设置初始光照方向
    FRotator DefaultLightRotation(-45.0f, 30.0f, 0.0f);
    _PreviewScene->SetLightDirection(DefaultLightRotation);
    
    UE_LOG(LogTemp, Log, TEXT("FComfyUI3DPreviewViewportClient: Setup default lighting"));
}

void FComfyUI3DPreviewViewportClient::SetLightDirection(const FRotator& Direction)
{
    FPreviewScene* _PreviewScene = GetPreviewScene();
    if (_PreviewScene)
    {
        _PreviewScene->SetLightDirection(Direction);
        UE_LOG(LogTemp, VeryVerbose, TEXT("FComfyUI3DPreviewViewportClient: Set light direction to %s"), 
            *Direction.ToString());
    }
}

void FComfyUI3DPreviewViewportClient::SetupDefaultCamera()
{
    // 设置默认的摄像机参数
    CameraDistance = 200.0f;
    CameraTargetLocation = FVector::ZeroVector;
    
    FRotator DefaultRotation(-15, 0, 0);
    SetViewRotation(DefaultRotation);
    
    // 设置视场角
    ViewFOV = 90.0f;
    
    // 更新摄像机位置
    UpdateCameraPosition();
}

void FComfyUI3DPreviewViewportClient::UpdateCameraPosition()
{
    // 根据当前的旋转和距离计算摄像机位置
    FVector ForwardVector = GetViewRotation().RotateVector(-FVector::ForwardVector);
    FVector CameraLocation = CameraTargetLocation + (ForwardVector * CameraDistance);
    
    SetViewLocation(CameraLocation);
    
    // 如果启用了光源跟随摄像机，则更新光源方向
    if (bLightFollowsCamera)
    {
        // 获取摄像机方向
        FRotator CameraRotation = GetViewRotation();
        
        // 将光源设置为从摄像机后方45度角照射
        FRotator LightRotation = CameraRotation;
        LightRotation.Pitch -= 45.0f; // 从上方45度照射
        LightRotation.Yaw += 30.0f;   // 稍微偏移一点角度，避免正面照射
        
        SetLightDirection(LightRotation);
    }
}

void FComfyUI3DPreviewViewportClient::UpdatePreviewMesh()
{
    if (!IsValid(PreviewMeshComponent)) 
        return;
    
    if (IsValid(CurrentPreviewMesh))
    {
        // 设置新的静态网格
        PreviewMeshComponent->SetStaticMesh(CurrentPreviewMesh);
        
        // 获取网格的边界框来调整摄像机位置 - 添加安全检查
        const FStaticMeshRenderData* RenderData = CurrentPreviewMesh->GetRenderData();
        if (RenderData && RenderData->LODResources.Num() > 0)
        {
            FBoxSphereBounds Bounds = CurrentPreviewMesh->GetBounds();
            float BoundsRadius = Bounds.SphereRadius;
            
            // 根据模型大小调整摄像机距离和目标位置
            CameraDistance = FMath::Max(BoundsRadius * 3.0f, 200.0f);
            CameraTargetLocation = Bounds.Origin;
            
            // 更新摄像机位置
            UpdateCameraPosition();
        }
        
        // 确保组件可见
        PreviewMeshComponent->SetVisibility(true);
    }
    else
    {
        // 清除网格 - 即使CurrentPreviewMesh无效也要清理组件
        if (IsValid(PreviewMeshComponent))
        {
            PreviewMeshComponent->SetStaticMesh(nullptr);
            PreviewMeshComponent->SetVisibility(false);
        }
        
        // 重置摄像机
        CameraTargetLocation = FVector::ZeroVector;
        CameraDistance = 200.0f;
        UpdateCameraPosition();
    }
    
    // 刷新视口
    Invalidate();
}

// ========== SComfyUI3DPreviewViewport ==========

void SComfyUI3DPreviewViewport::Construct(const FArguments& InArgs)
{
    // 创建PreviewScene，使用特殊配置避免触发自动保存
    FPreviewScene::ConstructionValues CVS;
    CVS.bDefaultLighting = true;
    CVS.bAllowAudioPlayback = false;
    CVS.bCreatePhysicsScene = false;
    
    PreviewScene = MakeUnique<FPreviewScene>(CVS);
    
    if (PreviewScene && PreviewScene->GetWorld())
    {
        UWorld* World = PreviewScene->GetWorld();
        
        // 关键：设置世界类型为 EditorPreview，这样自动保存系统会忽略它
        World->WorldType = EWorldType::EditorPreview;
        
        // 设置标志阻止序列化和自动保存
        World->SetFlags(RF_Transient | RF_DuplicateTransient | RF_NonPIEDuplicateTransient);
        World->bShouldSimulatePhysics = false;
        
        UE_LOG(LogTemp, Log, TEXT("PreviewScene World type set to EditorPreview"));
    }
    
    // 调用父类构造
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

SComfyUI3DPreviewViewport::~SComfyUI3DPreviewViewport()
{
    // 确保在父类销毁前清理ViewportClient
    ViewportClient.Reset();
    
    // 让PreviewScene自然销毁
    PreviewScene.Reset();
}
// }

void SComfyUI3DPreviewViewport::SetPreviewMesh(UStaticMesh* InStaticMesh)
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->SetPreviewMesh(InStaticMesh);
    }
}

void SComfyUI3DPreviewViewport::ClearPreview()
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->ClearPreview();
    }
}

TSharedRef<FEditorViewportClient> SComfyUI3DPreviewViewport::MakeEditorViewportClient()
{
    ViewportClient = MakeShareable(new FComfyUI3DPreviewViewportClient(PreviewScene.Get(), SharedThis(this)));
    return ViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SComfyUI3DPreviewViewport::MakeViewportToolbar()
{
    // 不需要工具栏
    return SNullWidget::NullWidget;
}
