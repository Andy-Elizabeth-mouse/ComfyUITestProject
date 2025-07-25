#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ComfyUITextureAssetManager.generated.h"

/**
 * PBR纹理数据结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIPBRTextureSet
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* DiffuseTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* NormalTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* RoughnessTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* MetallicTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* HeightTexture = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "PBR Textures")
    UTexture2D* AOTexture = nullptr;

    FComfyUIPBRTextureSet()
    {
    }
};

/**
 * 纹理资产管理器，负责处理ComfyUI生成的纹理和PBR材质
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUITextureAssetManager : public UObject
{
    GENERATED_BODY()

public:
    UComfyUITextureAssetManager();

    // === PBR纹理处理 ===
    
    /** 从多个纹理数据创建PBR纹理集 (C++版本，支持复杂参数类型) */
    static FComfyUIPBRTextureSet CreatePBRTextureSet(const TMap<FString, TSharedPtr<TArray<uint8>>>& TextureDataMap);

    /** 从纹理文件路径创建PBR纹理集 (蓝图友好版本) */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static FComfyUIPBRTextureSet CreatePBRTextureSetFromFiles(const TMap<FString, FString>& TextureFilePathMap);

    /** 创建PBR材质从纹理集 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UMaterial* CreatePBRMaterial(const FComfyUIPBRTextureSet& TextureSet, const FString& MaterialName = TEXT("ComfyUI_PBR_Material"));

    /** 创建材质实例从PBR纹理集 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UMaterialInstanceDynamic* CreatePBRMaterialInstance(const FComfyUIPBRTextureSet& TextureSet, UMaterial* BaseMaterial = nullptr);

    // === 纹理优化和处理 ===
    
    /** 创建无缝纹理 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UTexture2D* MakeTextureSeamless(UTexture2D* Texture);

    /** 调整纹理分辨率 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UTexture2D* ResizeTexture(UTexture2D* Texture, int32 NewWidth, int32 NewHeight);

    /** 生成法线贴图从高度图 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UTexture2D* GenerateNormalFromHeight(UTexture2D* HeightTexture, float Strength = 1.0f);

    // === 纹理集保存 ===
    
    /** 保存PBR纹理集到项目 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static bool SavePBRTextureSetToProject(const FComfyUIPBRTextureSet& TextureSet, const FString& BaseName, const FString& PackagePath = TEXT("/Game/ComfyUI/Generated/Textures"));

    /** 保存PBR纹理集到文件系统 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static bool SavePBRTextureSetToFiles(const FComfyUIPBRTextureSet& TextureSet, const FString& BaseFilePath);

    // === 纹理验证和分析 ===
    
    /** 验证纹理是否为有效的PBR纹理 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static bool ValidatePBRTexture(UTexture2D* Texture, const FString& TextureType);

    /** 分析纹理属性 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static FString AnalyzeTextureProperties(UTexture2D* Texture);

    /** 检测纹理类型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static FString DetectTextureType(const TArray<uint8>& TextureData);

    // === 纹理格式转换 ===
    
    /** 转换纹理格式 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UTexture2D* ConvertTextureFormat(UTexture2D* Texture, EPixelFormat NewFormat);

    /** 设置纹理压缩设置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static bool SetTextureCompressionSettings(UTexture2D* Texture, const FString& TextureType);

    // === 工具函数 ===
    
    /** 获取支持的PBR纹理类型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static TArray<FString> GetSupportedPBRTextureTypes();

    /** 创建纹理占位符 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture")
    static UTexture2D* CreateTexturePlaceholder(int32 Width = 512, int32 Height = 512, FColor Color = FColor(128, 128, 128, 255));

private:
    // 内部工具函数
    static UTexture2D* CreateTextureFromColorData(const TArray<FColor>& ColorData, int32 Width, int32 Height, const FString& TextureName);
    static bool ApplySeamlessFilter(TArray<FColor>& ColorData, int32 Width, int32 Height);
    static TArray<FColor> ExtractColorDataFromTexture(UTexture2D* Texture);
    static FString GenerateUniqueTextureName(const FString& BaseName, const FString& TextureType, const FString& PackagePath);
    static UMaterial* LoadBasePBRMaterial();
};