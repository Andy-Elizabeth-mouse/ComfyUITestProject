#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Components/StaticMeshComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ComfyUI3DAssetManager.generated.h"

/**
 * 3D模型数据结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUI3DModelData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "3D Model")
    FString ModelName;

    UPROPERTY(BlueprintReadWrite, Category = "3D Model")
    FString ModelFormat; // obj, fbx, gltf, etc.

    UPROPERTY(BlueprintReadWrite, Category = "3D Model")
    TArray<uint8> ModelData;

    UPROPERTY(BlueprintReadWrite, Category = "3D Model")
    TArray<uint8> TextureData;

    UPROPERTY(BlueprintReadWrite, Category = "3D Model")
    FString MaterialName;

    FComfyUI3DModelData()
        : ModelFormat(TEXT("obj"))
    {
    }
};

/**
 * 3D资产管理器，负责处理ComfyUI生成的3D模型
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUI3DAssetManager : public UObject
{
    GENERATED_BODY()

public:
    UComfyUI3DAssetManager();

    // === 3D模型数据处理 ===
    
    /** 从字节数据创建3D模型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static UStaticMesh* CreateStaticMeshFromData(const TArray<uint8>& ModelData, const FString& ModelFormat = TEXT("obj"));

    /** 从OBJ数据创建静态网格 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static UStaticMesh* CreateStaticMeshFromOBJ(const TArray<uint8>& OBJData);

    /** 从glTF数据创建静态网格 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static UStaticMesh* CreateStaticMeshFromGLTF(const TArray<uint8>& GLTFData);

    /** 保存3D模型到项目资产 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool Save3DModelToProject(UStaticMesh* StaticMesh, const FString& AssetName, const FString& PackagePath = TEXT("/Game/ComfyUI/Generated/Models"));

    /** 保存3D模型到文件系统 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool Save3DModelToFile(const FComfyUI3DModelData& ModelData, const FString& FilePath);

    // === 3D模型导出功能 ===
    
    /** 导出静态网格为OBJ格式 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool ExportStaticMeshToOBJ(UStaticMesh* StaticMesh, const FString& FilePath);
    
    /** 导出静态网格为FBX格式 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool ExportStaticMeshToFBX(UStaticMesh* StaticMesh, const FString& FilePath);
    
    /** 保存原始glTF/GLB数据到文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool SaveOriginalModelData(const TArray<uint8>& OriginalData, const FString& FilePath);

    // === 材质和纹理处理 ===
    
    /** 为3D模型创建材质 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static UMaterial* CreateMaterialFor3DModel(UTexture2D* DiffuseTexture, UTexture2D* NormalTexture = nullptr, UTexture2D* RoughnessTexture = nullptr);

    /** 应用纹理到3D模型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool ApplyTextureToStaticMesh(UStaticMesh* StaticMesh, UTexture2D* Texture);

    // === 文件格式支持 ===
    
    /** 检查支持的3D模型格式 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool IsSupportedModelFormat(const FString& Format);

    /** 获取支持的3D模型格式列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static TArray<FString> GetSupportedModelFormats();

    // === 网格验证和优化 ===
    
    /** 验证3D模型数据 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool Validate3DModelData(const TArray<uint8>& ModelData, const FString& Format);

    /** 优化静态网格 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|3D")
    static bool OptimizeStaticMesh(UStaticMesh* StaticMesh);

private:
    // 内部工具函数
    static UStaticMesh* CreateStaticMeshFromVertices(const TArray<FVector>& Vertices, const TArray<int32>& Indices, const TArray<FVector2D>& UVs);
    static bool ParseOBJData(const TArray<uint8>& OBJData, TArray<FVector>& OutVertices, TArray<int32>& OutIndices, TArray<FVector2D>& OutUVs);
    static FString GenerateUniqueAssetName(const FString& BaseName, const FString& PackagePath);
    static UStaticMesh* CreateFallbackMeshFromGLTF(const TArray<uint8>& GLTFData);
    static UStaticMesh* ParseGLBData(const TArray<uint8>& GLBData);
    static UStaticMesh* ParseGLTFJson(const FString& JsonContent);
};