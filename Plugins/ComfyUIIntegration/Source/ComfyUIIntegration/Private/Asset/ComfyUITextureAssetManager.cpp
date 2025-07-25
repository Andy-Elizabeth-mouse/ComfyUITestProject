#include "Asset/ComfyUITextureAssetManager.h"
#include "Utils/ComfyUIFileManager.h"
#include "Utils/Defines.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "TextureResource.h"
#include "RHI.h"
#include "ImageUtils.h"

UComfyUITextureAssetManager::UComfyUITextureAssetManager()
{
}

FComfyUIPBRTextureSet UComfyUITextureAssetManager::CreatePBRTextureSet(const TMap<FString, TSharedPtr<TArray<uint8>>>& TextureDataMap)
{
    FComfyUIPBRTextureSet TextureSet;

    for (const auto& TexturePair : TextureDataMap)
    {
        const FString& TextureType = TexturePair.Key.ToLower();
        const TSharedPtr<TArray<uint8>>& TextureDataPtr = TexturePair.Value;

        if (!TextureDataPtr.IsValid() || TextureDataPtr->Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreatePBRTextureSet: Empty texture data for type: %s"), *TextureType);
            continue;
        }

        UTexture2D* Texture = UComfyUIFileManager::CreateTextureFromImageData(*TextureDataPtr);
        if (!Texture)
        {
            UE_LOG(LogTemp, Error, TEXT("CreatePBRTextureSet: Failed to create texture for type: %s"), *TextureType);
            continue;
        }

        // 根据纹理类型设置适当的压缩和设置
        SetTextureCompressionSettings(Texture, TextureType);

        // 分配到相应的纹理槽
        if (TextureType.Contains(TEXT("diffuse")) || TextureType.Contains(TEXT("albedo")) || TextureType.Contains(TEXT("base")))
        {
            TextureSet.DiffuseTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("normal")))
        {
            TextureSet.NormalTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("roughness")))
        {
            TextureSet.RoughnessTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("metallic")) || TextureType.Contains(TEXT("metalness")))
        {
            TextureSet.MetallicTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("height")) || TextureType.Contains(TEXT("displacement")))
        {
            TextureSet.HeightTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("ao")) || TextureType.Contains(TEXT("ambient")))
        {
            TextureSet.AOTexture = Texture;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CreatePBRTextureSet: Unknown texture type: %s, treating as diffuse"), *TextureType);
            if (!TextureSet.DiffuseTexture)
            {
                TextureSet.DiffuseTexture = Texture;
            }
        }

        UE_LOG(LogTemp, Log, TEXT("CreatePBRTextureSet: Successfully created texture for type: %s"), *TextureType);
    }

    return TextureSet;
}

FComfyUIPBRTextureSet UComfyUITextureAssetManager::CreatePBRTextureSetFromFiles(const TMap<FString, FString>& TextureFilePathMap)
{
    FComfyUIPBRTextureSet TextureSet;

    for (const auto& TexturePair : TextureFilePathMap)
    {
        const FString& TextureType = TexturePair.Key.ToLower();
        const FString& FilePath = TexturePair.Value;

        if (FilePath.IsEmpty() || !FPaths::FileExists(FilePath))
        {
            UE_LOG(LogTemp, Warning, TEXT("CreatePBRTextureSetFromFiles: File does not exist: %s"), *FilePath);
            continue;
        }

        // 加载纹理文件
        TArray<uint8> ImageData;
        if (!FFileHelper::LoadFileToArray(ImageData, *FilePath))
        {
            UE_LOG(LogTemp, Error, TEXT("CreatePBRTextureSetFromFiles: Failed to load file: %s"), *FilePath);
            continue;
        }

        UTexture2D* Texture = UComfyUIFileManager::CreateTextureFromImageData(ImageData);
        if (!Texture)
        {
            UE_LOG(LogTemp, Error, TEXT("CreatePBRTextureSetFromFiles: Failed to create texture from file: %s"), *FilePath);
            continue;
        }

        // 根据纹理类型设置适当的压缩和设置
        SetTextureCompressionSettings(Texture, TextureType);

        // 分配到相应的纹理槽
        if (TextureType.Contains(TEXT("diffuse")) || TextureType.Contains(TEXT("albedo")) || TextureType.Contains(TEXT("base")))
        {
            TextureSet.DiffuseTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("normal")))
        {
            TextureSet.NormalTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("roughness")))
        {
            TextureSet.RoughnessTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("metallic")) || TextureType.Contains(TEXT("metalness")))
        {
            TextureSet.MetallicTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("height")) || TextureType.Contains(TEXT("displacement")))
        {
            TextureSet.HeightTexture = Texture;
        }
        else if (TextureType.Contains(TEXT("ao")) || TextureType.Contains(TEXT("ambient")))
        {
            TextureSet.AOTexture = Texture;
        }

        UE_LOG(LogTemp, Log, TEXT("CreatePBRTextureSetFromFiles: Successfully loaded texture from file: %s"), *FilePath);
    }

    return TextureSet;
}

UMaterial* UComfyUITextureAssetManager::CreatePBRMaterial(const FComfyUIPBRTextureSet& TextureSet, const FString& MaterialName)
{
    if (!TextureSet.DiffuseTexture)
        LOG_AND_RETURN(Error, nullptr, "CreatePBRMaterial: No diffuse texture provided");

    // 创建材质包
    FString PackageName = TEXT("/Temp/ComfyUI_Material_") + FGuid::NewGuid().ToString();
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
        LOG_AND_RETURN(Error, nullptr, "CreatePBRMaterial: Failed to create package");

    // 创建材质
    UMaterial* Material = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
    if (!Material)
        LOG_AND_RETURN(Error, nullptr, "CreatePBRMaterial: Failed to create material");

    // 设置材质为PBR模式
    Material->SetShadingModel(MSM_DefaultLit);

    // TODO: 这里需要使用材质编辑器节点来构建材质图
    // 由于UE5的材质节点创建比较复杂，这里简化实现
    
    UE_LOG(LogTemp, Log, TEXT("CreatePBRMaterial: Created PBR material: %s"), *MaterialName);
    return Material;
}

UMaterialInstanceDynamic* UComfyUITextureAssetManager::CreatePBRMaterialInstance(const FComfyUIPBRTextureSet& TextureSet, UMaterial* BaseMaterial)
{
    if (!BaseMaterial)
    {
        BaseMaterial = LoadBasePBRMaterial();
        if (!BaseMaterial)
            LOG_AND_RETURN(Error, nullptr, "CreatePBRMaterialInstance: No base material available");
    }

    // 创建动态材质实例
    UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
    if (!MaterialInstance)
        LOG_AND_RETURN(Error, nullptr, "CreatePBRMaterialInstance: Failed to create material instance");

    // 设置纹理参数
    if (TextureSet.DiffuseTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("DiffuseTexture"), TextureSet.DiffuseTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("BaseColor"), TextureSet.DiffuseTexture);
    }

    if (TextureSet.NormalTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("NormalTexture"), TextureSet.NormalTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("Normal"), TextureSet.NormalTexture);
    }

    if (TextureSet.RoughnessTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("RoughnessTexture"), TextureSet.RoughnessTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("Roughness"), TextureSet.RoughnessTexture);
    }

    if (TextureSet.MetallicTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("MetallicTexture"), TextureSet.MetallicTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("Metallic"), TextureSet.MetallicTexture);
    }

    if (TextureSet.HeightTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("HeightTexture"), TextureSet.HeightTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("Height"), TextureSet.HeightTexture);
    }

    if (TextureSet.AOTexture)
    {
        MaterialInstance->SetTextureParameterValue(TEXT("AOTexture"), TextureSet.AOTexture);
        MaterialInstance->SetTextureParameterValue(TEXT("AO"), TextureSet.AOTexture);
    }

    LOG_AND_RETURN(Log, MaterialInstance, "CreatePBRMaterialInstance: Successfully created PBR material instance");
}

UTexture2D* UComfyUITextureAssetManager::MakeTextureSeamless(UTexture2D* Texture)
{
    if (!Texture)
        LOG_AND_RETURN(Error, nullptr, "MakeTextureSeamless: Texture is null");

    // 提取纹理颜色数据
    TArray<FColor> ColorData = ExtractColorDataFromTexture(Texture);
    if (ColorData.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "MakeTextureSeamless: Failed to extract color data");

    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();

    // 应用无缝滤镜
    if (!ApplySeamlessFilter(ColorData, Width, Height))
        LOG_AND_RETURN(Error, nullptr, "MakeTextureSeamless: Failed to apply seamless filter");

    // 创建新的无缝纹理
    FString TextureName = Texture->GetName() + TEXT("_Seamless");
    return CreateTextureFromColorData(ColorData, Width, Height, TextureName);
}

UTexture2D* UComfyUITextureAssetManager::ResizeTexture(UTexture2D* Texture, int32 NewWidth, int32 NewHeight)
{
    if (!Texture)
        LOG_AND_RETURN(Error, nullptr, "ResizeTexture: Texture is null");

    if (NewWidth <= 0 || NewHeight <= 0)
        LOG_AND_RETURN(Error, nullptr, "ResizeTexture: Invalid dimensions: %dx%d", NewWidth, NewHeight);

    // TODO: 实现纹理缩放算法
    // 这需要从原纹理提取数据，进行重采样，然后创建新纹理
    
    UE_LOG(LogTemp, Warning, TEXT("ResizeTexture: Texture resizing not yet implemented"));
    return Texture; // 临时返回原纹理
}

UTexture2D* UComfyUITextureAssetManager::GenerateNormalFromHeight(UTexture2D* HeightTexture, float Strength)
{
    if (!HeightTexture)
        LOG_AND_RETURN(Error, nullptr, "GenerateNormalFromHeight: HeightTexture is null");

    // TODO: 实现从高度图生成法线贴图的算法
    // 这需要计算高度梯度并转换为法线向量
    
    UE_LOG(LogTemp, Warning, TEXT("GenerateNormalFromHeight: Normal generation not yet implemented"));
    return nullptr;
}

bool UComfyUITextureAssetManager::SavePBRTextureSetToProject(const FComfyUIPBRTextureSet& TextureSet, const FString& BaseName, const FString& PackagePath)
{
    bool bAllSaved = true;
    int32 SavedCount = 0;

    // 保存各种纹理类型
    if (TextureSet.DiffuseTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("Diffuse"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.DiffuseTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save diffuse texture"));
        }
    }

    if (TextureSet.NormalTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("Normal"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.NormalTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save normal texture"));
        }
    }

    if (TextureSet.RoughnessTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("Roughness"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.RoughnessTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save roughness texture"));
        }
    }

    if (TextureSet.MetallicTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("Metallic"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.MetallicTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save metallic texture"));
        }
    }

    if (TextureSet.HeightTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("Height"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.HeightTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save height texture"));
        }
    }

    if (TextureSet.AOTexture)
    {
        FString TextureName = GenerateUniqueTextureName(BaseName, TEXT("AO"), PackagePath);
        if (UComfyUIFileManager::SaveTextureToProject(TextureSet.AOTexture, TextureName, PackagePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
            UE_LOG(LogTemp, Error, TEXT("SavePBRTextureSetToProject: Failed to save AO texture"));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SavePBRTextureSetToProject: Saved %d textures, all successful: %s"), 
           SavedCount, bAllSaved ? TEXT("true") : TEXT("false"));

    return SavedCount > 0;
}

bool UComfyUITextureAssetManager::SavePBRTextureSetToFiles(const FComfyUIPBRTextureSet& TextureSet, const FString& BaseFilePath)
{
    bool bAllSaved = true;
    int32 SavedCount = 0;

    FString BaseName = FPaths::GetBaseFilename(BaseFilePath);
    FString Directory = FPaths::GetPath(BaseFilePath);

    // 保存各种纹理类型到文件
    if (TextureSet.DiffuseTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_Diffuse.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.DiffuseTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    if (TextureSet.NormalTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_Normal.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.NormalTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    if (TextureSet.RoughnessTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_Roughness.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.RoughnessTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    if (TextureSet.MetallicTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_Metallic.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.MetallicTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    if (TextureSet.HeightTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_Height.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.HeightTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    if (TextureSet.AOTexture)
    {
        FString FilePath = Directory / (BaseName + TEXT("_AO.png"));
        if (UComfyUIFileManager::SaveTextureToFile(TextureSet.AOTexture, FilePath))
        {
            SavedCount++;
        }
        else
        {
            bAllSaved = false;
        }
    }

    LOG_AND_RETURN(Log, SavedCount > 0, "SavePBRTextureSetToFiles: Saved %d texture files", SavedCount);
}

bool UComfyUITextureAssetManager::ValidatePBRTexture(UTexture2D* Texture, const FString& TextureType)
{
    if (!Texture)
        return false;

    // 检查纹理尺寸
    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();

    if (Width <= 0 || Height <= 0)
        return false;

    // 检查纹理是否为2的幂次
    bool bIsPowerOfTwo = FMath::IsPowerOfTwo(Width) && FMath::IsPowerOfTwo(Height);
    if (!bIsPowerOfTwo)
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidatePBRTexture: Texture %s is not power of two (%dx%d)"), 
               *TextureType, Width, Height);
    }

    // 检查纹理格式是否合适
    EPixelFormat PixelFormat = Texture->GetPixelFormat();
    
    if (TextureType.ToLower().Contains(TEXT("normal")))
    {
        // 法线贴图应该使用适合的压缩格式
        if (PixelFormat != PF_BC5 && PixelFormat != PF_B8G8R8A8 && PixelFormat != PF_R8G8B8A8)
        {
            UE_LOG(LogTemp, Warning, TEXT("ValidatePBRTexture: Normal texture has non-optimal format"));
        }
    }

    return true;
}

FString UComfyUITextureAssetManager::AnalyzeTextureProperties(UTexture2D* Texture)
{
    if (!Texture)
        return TEXT("Invalid texture");

    FString Analysis;
    Analysis += FString::Printf(TEXT("Name: %s\n"), *Texture->GetName());
    Analysis += FString::Printf(TEXT("Size: %dx%d\n"), Texture->GetSizeX(), Texture->GetSizeY());
    Analysis += FString::Printf(TEXT("Format: %s\n"), GetPixelFormatString(Texture->GetPixelFormat()));
    Analysis += FString::Printf(TEXT("Compression: %s\n"), *UEnum::GetValueAsString(Texture->CompressionSettings));
    Analysis += FString::Printf(TEXT("SRGB: %s\n"), Texture->SRGB ? TEXT("Yes") : TEXT("No"));
    Analysis += FString::Printf(TEXT("Mip Count: %d\n"), Texture->GetNumMips());

    return Analysis;
}

FString UComfyUITextureAssetManager::DetectTextureType(const TArray<uint8>& TextureData)
{
    if (TextureData.Num() < 4)
        return TEXT("Unknown");

    // 检查文件头来判断格式
    if (TextureData[0] == 0x89 && TextureData[1] == 0x50 && TextureData[2] == 0x4E && TextureData[3] == 0x47)
    {
        return TEXT("PNG");
    }
    else if (TextureData[0] == 0xFF && TextureData[1] == 0xD8)
    {
        return TEXT("JPEG");
    }
    else if (TextureData[0] == 0x42 && TextureData[1] == 0x4D)
    {
        return TEXT("BMP");
    }

    return TEXT("Unknown");
}

UTexture2D* UComfyUITextureAssetManager::ConvertTextureFormat(UTexture2D* Texture, EPixelFormat NewFormat)
{
    if (!Texture)
        LOG_AND_RETURN(Error, nullptr, "ConvertTextureFormat: Texture is null");

    // TODO: 实现纹理格式转换
    UE_LOG(LogTemp, Warning, TEXT("ConvertTextureFormat: Format conversion not yet implemented"));
    return Texture;
}

bool UComfyUITextureAssetManager::SetTextureCompressionSettings(UTexture2D* Texture, const FString& TextureType)
{
    if (!Texture)
        return false;

    FString LowerType = TextureType.ToLower();

    if (LowerType.Contains(TEXT("normal")))
    {
        Texture->CompressionSettings = TC_Normalmap;
        Texture->SRGB = false;
    }
    else if (LowerType.Contains(TEXT("roughness")) || LowerType.Contains(TEXT("metallic")) || LowerType.Contains(TEXT("height")))
    {
        Texture->CompressionSettings = TC_Masks;
        Texture->SRGB = false;
    }
    else if (LowerType.Contains(TEXT("diffuse")) || LowerType.Contains(TEXT("albedo")) || LowerType.Contains(TEXT("base")))
    {
        Texture->CompressionSettings = TC_Default;
        Texture->SRGB = true;
    }
    else
    {
        Texture->CompressionSettings = TC_Default;
        Texture->SRGB = true;
    }

    Texture->UpdateResource();
    return true;
}

TArray<FString> UComfyUITextureAssetManager::GetSupportedPBRTextureTypes()
{
    return {
        TEXT("Diffuse"),
        TEXT("Albedo"),
        TEXT("BaseColor"),
        TEXT("Normal"),
        TEXT("Roughness"),
        TEXT("Metallic"),
        TEXT("Metalness"),
        TEXT("Height"),
        TEXT("Displacement"),
        TEXT("AO"),
        TEXT("Ambient")
    };
}

UTexture2D* UComfyUITextureAssetManager::CreateTexturePlaceholder(int32 Width, int32 Height, FColor Color)
{
    if (Width <= 0 || Height <= 0)
        LOG_AND_RETURN(Error, nullptr, "CreateTexturePlaceholder: Invalid dimensions: %dx%d", Width, Height);

    TArray<FColor> ColorData;
    ColorData.SetNumUninitialized(Width * Height);
    
    for (int32 i = 0; i < ColorData.Num(); ++i)
    {
        ColorData[i] = Color;
    }

    FString TextureName = TEXT("ComfyUI_Placeholder_") + FGuid::NewGuid().ToString();
    return CreateTextureFromColorData(ColorData, Width, Height, TextureName);
}

// 私有函数实现

UTexture2D* UComfyUITextureAssetManager::CreateTextureFromColorData(const TArray<FColor>& ColorData, int32 Width, int32 Height, const FString& TextureName)
{
    if (ColorData.Num() != Width * Height)
        LOG_AND_RETURN(Error, nullptr, "CreateTextureFromColorData: Color data size mismatch");

    // 创建临时纹理包
    FString PackageName = TEXT("/Temp/ComfyUI_Texture_") + FGuid::NewGuid().ToString();
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
        LOG_AND_RETURN(Error, nullptr, "CreateTextureFromColorData: Failed to create package");

    // 创建纹理
    UTexture2D* Texture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone);
    if (!Texture)
        LOG_AND_RETURN(Error, nullptr, "CreateTextureFromColorData: Failed to create texture");

    // TODO: 使用新的UE5 API来设置纹理数据
    // 这里需要使用FTexture2DMipMap和纹理平台数据
    
    UE_LOG(LogTemp, Log, TEXT("CreateTextureFromColorData: Created texture %s (%dx%d)"), *TextureName, Width, Height);
    return Texture;
}

bool UComfyUITextureAssetManager::ApplySeamlessFilter(TArray<FColor>& ColorData, int32 Width, int32 Height)
{
    if (ColorData.Num() != Width * Height)
        return false;

    // 简单的无缝滤镜：混合边缘像素
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            int32 Index = Y * Width + X;
            
            // 处理边缘像素
            if (X == 0 || X == Width - 1 || Y == 0 || Y == Height - 1)
            {
                // 与对面边缘的像素进行混合
                int32 OppositeX = (X == 0) ? Width - 1 : (X == Width - 1) ? 0 : X;
                int32 OppositeY = (Y == 0) ? Height - 1 : (Y == Height - 1) ? 0 : Y;
                int32 OppositeIndex = OppositeY * Width + OppositeX;
                
                if (OppositeIndex < ColorData.Num())
                {
                    FColor& CurrentColor = ColorData[Index];
                    const FColor& OppositeColor = ColorData[OppositeIndex];
                    
                    CurrentColor.R = (CurrentColor.R + OppositeColor.R) / 2;
                    CurrentColor.G = (CurrentColor.G + OppositeColor.G) / 2;
                    CurrentColor.B = (CurrentColor.B + OppositeColor.B) / 2;
                    CurrentColor.A = (CurrentColor.A + OppositeColor.A) / 2;
                }
            }
        }
    }

    return true;
}

TArray<FColor> UComfyUITextureAssetManager::ExtractColorDataFromTexture(UTexture2D* Texture)
{
    TArray<FColor> ColorData;
    
    if (!Texture)
        return ColorData;

    // TODO: 实现从UTexture2D提取颜色数据
    // 这需要访问纹理的平台数据和Mip级别
    
    UE_LOG(LogTemp, Warning, TEXT("ExtractColorDataFromTexture: Color data extraction not yet implemented"));
    return ColorData;
}

FString UComfyUITextureAssetManager::GenerateUniqueTextureName(const FString& BaseName, const FString& TextureType, const FString& PackagePath)
{
    FString CleanBaseName = BaseName;
    CleanBaseName = CleanBaseName.Replace(TEXT(" "), TEXT("_"));
    CleanBaseName = CleanBaseName.Replace(TEXT("-"), TEXT("_"));
    
    FString TextureName = CleanBaseName + TEXT("_") + TextureType;
    FString TestName = TextureName;
    int32 Counter = 1;
    
    while (true)
    {
        FString TestPackageName = PackagePath / TestName;
        if (!FPackageName::DoesPackageExist(TestPackageName))
        {
            break;
        }
        TestName = FString::Printf(TEXT("%s_%d"), *TextureName, Counter++);
    }
    
    return TestName;
}

UMaterial* UComfyUITextureAssetManager::LoadBasePBRMaterial()
{
    // TODO: 加载或创建基础PBR材质
    // 这里应该返回一个预设的PBR材质模板
    
    UE_LOG(LogTemp, Warning, TEXT("LoadBasePBRMaterial: Base PBR material loading not yet implemented"));
    return nullptr;
}