#include "Utils/ComfyUIFileManager.h"
#include "Utils/ComfyUIConfigManager.h"
#include "Utils/Defines.h"
#include "Workflow/ComfyUIWorkflowService.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "TextureResource.h"
#include "RHI.h"
#include "Modules/ModuleManager.h"
#include "DesktopPlatformModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/TextureMipDataProviderFactory.h"

bool UComfyUIFileManager::LoadImageFromFile(const FString& FilePath, TArray<uint8>& OutImageData)
{
    return FFileHelper::LoadFileToArray(OutImageData, *FilePath);
}

bool UComfyUIFileManager::SaveArrayToFile(const TArray<uint8>& Data, const FString& FilePath)
{
    return FFileHelper::SaveArrayToFile(Data, *FilePath);
}

bool UComfyUIFileManager::LoadJsonFromFile(const FString& FilePath, FString& OutJsonContent)
{
    return FFileHelper::LoadFileToString(OutJsonContent, *FilePath);
}

bool UComfyUIFileManager::SaveJsonToFile(const FString& JsonContent, const FString& FilePath)
{
    return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}

TArray<FString> UComfyUIFileManager::ScanFilesInDirectory(const FString& DirectoryPath, const FString& FileExtension)
{
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *(DirectoryPath / (TEXT("*") + FileExtension)), true, false);
    return FoundFiles;
}

bool UComfyUIFileManager::EnsureDirectoryExists(const FString& DirectoryPath)
{
    if (!FPaths::DirectoryExists(DirectoryPath))
    {
        return IFileManager::Get().MakeDirectory(*DirectoryPath, true);
    }
    return true;
}

FString UComfyUIFileManager::GetPluginDirectory()
{
    return FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration");
}

FString UComfyUIFileManager::GetTemplatesDirectory()
{
    return GetPluginDirectory() / TEXT("Config/Templates");
}

FString UComfyUIFileManager::GetConfigDirectory()
{
    return GetPluginDirectory() / TEXT("Config");
}

UTexture2D* UComfyUIFileManager::CreateTextureFromImageData(const TArray<uint8>& ImageData)
{
    if (ImageData.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "CreateTextureFromImageData: Empty image data");

    // 使用ImageWrapper模块来解码图像
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

    // 尝试不同的图像格式
    static const TArray<EImageFormat> SupportedFormats = {
        EImageFormat::PNG,
        EImageFormat::JPEG,
        EImageFormat::BMP,
        EImageFormat::EXR
    };

    for (EImageFormat Format : SupportedFormats)
    {
        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
        if (!ImageWrapper.IsValid()) continue;

        if (ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
        {
            TArray<uint8> UncompressedBGRA;
            if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
            {
                // 创建纹理
                int32 Width = ImageWrapper->GetWidth();
                int32 Height = ImageWrapper->GetHeight();
                
                UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
                if (NewTexture)
                {
                    // 使用更现代的方式填充纹理数据
                    FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
                    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
                    FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
                    Mip.BulkData.Unlock();
                    
                    // 更新纹理
                    NewTexture->UpdateResource();
                    
                    UE_LOG(LogTemp, Log, TEXT("CreateTextureFromImageData: Successfully created %dx%d texture"), Width, Height);
                    return NewTexture;
                }
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("CreateTextureFromImageData: Failed to decode image data with any supported format"));
    return nullptr;
}

bool UComfyUIFileManager::SaveTextureToProject(UTexture2D* Texture, const FString& AssetName, const FString& PackagePath)
{
    if (!Texture)
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Texture is null");

    // 验证源纹理是否有效
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Source texture has no platform data or mips");
    if (Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Invalid texture dimensions: %dx%d", Texture->GetSizeX(), Texture->GetSizeY());
    if (AssetName.IsEmpty())
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: AssetName is empty");

    // 确保包路径有效
    FString FinalPackagePath = PackagePath;
    if (FinalPackagePath.IsEmpty() || !FinalPackagePath.StartsWith(TEXT("/Game/")))
        FinalPackagePath = TEXT("/Game/ComfyUI/Generated");

    UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Starting save process - AssetName: %s, PackagePath: %s"), *AssetName, *FinalPackagePath);

    // 生成唯一的资产名称
    FString UniqueAssetName = GenerateUniqueAssetName(AssetName, FinalPackagePath);
    if (UniqueAssetName.IsEmpty())
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to generate unique asset name");

    FString FullPackageName = FinalPackagePath + TEXT("/") + UniqueAssetName;

    // 验证包名称
    if (!FPackageName::IsValidLongPackageName(FullPackageName))
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Invalid package name: %s", *FullPackageName);

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
                        UE_LOG(LogTemp, Warning, TEXT("SaveTextureToProject: Failed to relocate texture, falling back to copy"));
                }
            }
        }
        
        // 如果无法重用纹理，则创建新的纹理对象
        if (!bCanReuseTexture)
        {
            // 创建包
            Package = CreatePackage(*FullPackageName);
            if (!Package)
                LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to create package: %s", *FullPackageName);
            Package->FullyLoad();

            // 创建新纹理对象
            UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_Transactional);
            if (!NewTexture)
                LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to create texture object");

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
                    LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to get source mip data");
            }
            else
                LOG_AND_RETURN(Error, false, "SaveTextureToProject: Source texture has no valid source data");
            
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
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: Texture to save has no platform data or mips");

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
        if (!EnsureDirectoryExists(PackageDir))
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to create directory: %s", *PackageDir);

        UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: About to save package to: %s"), *PackageFileName);

        // 最终验证纹理状态
        if (!IsValid(TextureToSave))
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: TextureToSave is not valid before save");

        // 确保纹理平台数据已准备就绪
        if (TextureToSave->GetPlatformData())
        {
            // 强制等待纹理编译完成
            TextureToSave->FinishCachePlatformData();
            
            // 再次验证平台数据
            if (!TextureToSave->GetPlatformData() || TextureToSave->GetPlatformData()->Mips.Num() == 0)
                LOG_AND_RETURN(Error, false, "SaveTextureToProject: Platform data not ready after cache");
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
            SaveArgs.FinalTimeStamp = FDateTime::MinValue();
            SaveArgs.Error = GError;
            
            bSaved = UPackage::SavePackage(Package, TextureToSave, *PackageFileName, SaveArgs);
            
            UE_LOG(LogTemp, Log, TEXT("SaveTextureToProject: Save operation returned: %s"), bSaved ? TEXT("true") : TEXT("false"));
        }
        catch (const std::exception& Exception)
        {
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: std::exception during SavePackage: %hs", Exception.what());
        }
        catch (...)
        {
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: Unknown exception during SavePackage");
        }

        if (bSaved)
            LOG_AND_RETURN(Log, true, "SaveTextureToProject: Successfully saved texture to %s", *FullPackageName);
        else
            LOG_AND_RETURN(Error, false, "SaveTextureToProject: Failed to save package to disk: %s", *PackageFileName);
    }
    catch (...)
    {
        LOG_AND_RETURN(Error, false, "SaveTextureToProject: Exception occurred during save operation");
    }
}

bool UComfyUIFileManager::SaveTextureToFile(UTexture2D* Texture, const FString& FilePath, EComfyUIImageFormat ImageFormat)
{
    if (!Texture)
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Texture is null");

    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Texture has no platform data or mips");

    // 创建图像包装器
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ConvertToImageWrapperFormat(ImageFormat));

    if (!ImageWrapper.IsValid())
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to create image wrapper");

    // 获取纹理尺寸
    const int32 Width = Texture->GetSizeX();
    const int32 Height = Texture->GetSizeY();
    
    // 获取纹理数据 - 使用网上推荐的方法
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    const FColor* TextureData = (const FColor*)Mip.BulkData.Lock(LOCK_READ_ONLY);
    
    if (!TextureData)
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to lock texture data");

    // 设置原始数据 - 使用BGRA格式和正确的数据大小
    bool bSetResult = ImageWrapper->SetRaw(
        TextureData,
        Mip.BulkData.GetBulkDataSize(),
        Width,
        Height,
        ERGBFormat::BGRA,
        8
    );

    // 解锁纹理数据
    Mip.BulkData.Unlock();

    if (!bSetResult)
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to set image data");

    // 获取压缩后的数据
    TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(
        ImageFormat == EComfyUIImageFormat::JPEG ? 85 : 100
    );

    if (CompressedData.Num() == 0)
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to compress image data");

    // 确保保存目录存在
    FString SaveDirectory = FPaths::GetPath(FilePath);
    if (!EnsureDirectoryExists(SaveDirectory))
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to create directory: %s", *SaveDirectory);

    // 保存到文件
    if (FFileHelper::SaveArrayToFile(CompressedData, *FilePath))
        LOG_AND_RETURN(Log, true, "SaveTextureToFile: Successfully saved to: %s", *FilePath);
    else
        LOG_AND_RETURN(Error, false, "SaveTextureToFile: Failed to write file: %s", *FilePath);
}

bool UComfyUIFileManager::ParseJsonString(const FString& JsonString, TSharedPtr<FJsonObject>& OutJsonObject)
{
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    return FJsonSerializer::Deserialize(Reader, OutJsonObject);
}

bool UComfyUIFileManager::ShowOpenFileDialog(const FString& DialogTitle, const FString& FileTypes, const FString& DefaultPath, TArray<FString>& OutFileNames)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        return DesktopPlatform->OpenFileDialog(
            nullptr,
            DialogTitle,
            DefaultPath,
            TEXT(""),
            FileTypes,
            EFileDialogFlags::None,
            OutFileNames
        );
    }
    return false;
}

bool UComfyUIFileManager::ShowSaveFileDialog(const FString& DialogTitle, const FString& FileTypes, const FString& DefaultPath, const FString& DefaultFileName, FString& OutFileName)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> OutFileNames;
        bool bResult = DesktopPlatform->SaveFileDialog(
            nullptr,
            DialogTitle,
            DefaultPath,
            DefaultFileName,
            FileTypes,
            EFileDialogFlags::None,
            OutFileNames
        );
        
        if (bResult && OutFileNames.Num() > 0)
        {
            OutFileName = OutFileNames[0];
            return true;
        }
    }
    return false;
}

bool UComfyUIFileManager::ImportWorkflowTemplate(const FString& SourceFilePath, const FString& WorkflowName, FString& OutError)
{
    // 已重构：使用工作流服务
    UE_LOG(LogTemp, Warning, TEXT("UComfyUIFileManager::ImportWorkflowTemplate: Function moved to workflow service"));
    
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (WorkflowService)
    {
        return WorkflowService->ImportWorkflow(SourceFilePath, WorkflowName, OutError);
    }
    
    OutError = TEXT("Workflow service is not available");
    return false;
}

TArray<FString> UComfyUIFileManager::ScanWorkflowTemplates()
{
    // 已重构：使用工作流服务
    UE_LOG(LogTemp, Warning, TEXT("UComfyUIFileManager::ScanWorkflowTemplates: Function moved to workflow service"));
    
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (WorkflowService)
    {
        return WorkflowService->GetAvailableWorkflowNames();
    }
    
    // 如果服务不可用，回退到文件扫描
    FString TemplatesDir = GetTemplatesDirectory();
    TArray<FString> TemplateFiles = ScanFilesInDirectory(TemplatesDir, TEXT(".json"));
    
    TArray<FString> WorkflowNames;
    for (const FString& TemplateFile : TemplateFiles)
    {
        FString WorkflowName = FPaths::GetBaseFilename(TemplateFile);
        WorkflowNames.Add(WorkflowName);
    }
    
    return WorkflowNames;
}

bool UComfyUIFileManager::LoadWorkflowTemplate(const FString& TemplateName, FString& OutJsonContent)
{
    // 已重构：保持文件操作功能
    FString TemplatesDir = GetTemplatesDirectory();
    FString TemplateFile = TemplatesDir / (TemplateName + TEXT(".json"));
    
    return LoadJsonFromFile(TemplateFile, OutJsonContent);
}

EImageFormat UComfyUIFileManager::ConvertToImageWrapperFormat(EComfyUIImageFormat Format)
{
    switch (Format)
    {
        case EComfyUIImageFormat::PNG:
            return EImageFormat::PNG;
        case EComfyUIImageFormat::JPEG:
            return EImageFormat::JPEG;
        case EComfyUIImageFormat::BMP:
            return EImageFormat::BMP;
        default:
            return EImageFormat::PNG;
    }
}

FString UComfyUIFileManager::GenerateUniqueAssetName(const FString& BaseName, const FString& PackagePath)
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
        CleanBaseName = TEXT("GeneratedTexture");
    }

    // 检查是否已经存在同名资产
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FString UniqueAssetName = CleanBaseName;

    int32 Counter = 1;
    while (true)
    {
        FString TestPackagePath = PackagePath + TEXT("/") + UniqueAssetName;
        
        // 检查资产注册表中是否存在
        FAssetData ExistingAsset = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(*TestPackagePath));
        
        // 如果不存在，使用这个名称
        if (!ExistingAsset.IsValid()) break;
        
        // 如果存在，尝试下一个编号
        UniqueAssetName = FString::Printf(TEXT("%s_%d"), *CleanBaseName, Counter);
        Counter++;
        
        // 防止无限循环
        if (Counter > 9999)
        {
            UniqueAssetName = FString::Printf(TEXT("%s_%s"), *CleanBaseName, *FGuid::NewGuid().ToString());
            break;
        }
    }

    return UniqueAssetName;
}

// ========== 3D文件处理实现 ==========

bool UComfyUIFileManager::Load3DModelFromFile(const FString& FilePath, TArray<uint8>& OutModelData)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Load3DModelFromFile: File does not exist: %s"), *FilePath);
        return false;
    }

    if (!FFileHelper::LoadFileToArray(OutModelData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Load3DModelFromFile: Failed to load file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Load3DModelFromFile: Successfully loaded %d bytes from %s"), OutModelData.Num(), *FilePath);
    return true;
}

bool UComfyUIFileManager::Save3DModelToFile(const TArray<uint8>& ModelData, const FString& FilePath, EComfyUI3DFormat Format)
{
    if (ModelData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToFile: Model data is empty"));
        return false;
    }

    // 确保目录存在
    FString DirectoryPath = FPaths::GetPath(FilePath);
    if (!EnsureDirectoryExists(DirectoryPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToFile: Failed to create directory: %s"), *DirectoryPath);
        return false;
    }

    // 验证格式（基础验证）
    if (!ValidateF3DFormat(ModelData, Format))
    {
        UE_LOG(LogTemp, Warning, TEXT("Save3DModelToFile: Format validation failed, proceeding anyway"));
    }

    // 保存文件
    if (!FFileHelper::SaveArrayToFile(ModelData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToFile: Failed to save file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Save3DModelToFile: Successfully saved %d bytes to %s"), ModelData.Num(), *FilePath);
    return true;
}

bool UComfyUIFileManager::Save3DModelToProject(const TArray<uint8>& ModelData, const FString& AssetName, const FString& PackagePath)
{
    if (ModelData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToProject: Model data is empty"));
        return false;
    }

    if (AssetName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToProject: AssetName is empty"));
        return false;
    }

    // 清理资产名称
    FString CleanAssetName = AssetName;
    CleanAssetName = CleanAssetName.Replace(TEXT(" "), TEXT("_"));
    CleanAssetName = CleanAssetName.Replace(TEXT("-"), TEXT("_"));

    // 生成唯一名称
    FString FinalPackagePath = PackagePath.StartsWith(TEXT("/")) ? PackagePath : (TEXT("/Game/") + PackagePath);
    FString UniqueAssetName = GenerateUniqueAssetName(CleanAssetName, FinalPackagePath);

    // 创建临时文件路径
    FString TempDir = FPaths::ProjectSavedDir() / TEXT("ComfyUI") / TEXT("Temp");
    EnsureDirectoryExists(TempDir);
    FString TempFilePath = TempDir / (UniqueAssetName + TEXT(".glb"));

    // 保存到临时文件
    if (!Save3DModelToFile(ModelData, TempFilePath, EComfyUI3DFormat::GLB))
    {
        UE_LOG(LogTemp, Error, TEXT("Save3DModelToProject: Failed to save temporary file"));
        return false;
    }

    // TODO: 使用UE5的FBX或glTF导入器导入到项目中
    // 这需要更复杂的实现，涉及到FBXImporter或第三方glTF导入器
    
    UE_LOG(LogTemp, Log, TEXT("Save3DModelToProject: Saved temporary 3D model file at %s"), *TempFilePath);
    UE_LOG(LogTemp, Warning, TEXT("Save3DModelToProject: Automatic import to project not yet implemented. Manual import required."));
    
    return true;
}

bool UComfyUIFileManager::ValidateF3DFormat(const TArray<uint8>& ModelData, EComfyUI3DFormat Format)
{
    if (ModelData.Num() < 4)
    {
        return false;
    }

    // 基础文件格式验证
    switch (Format)
    {
    case EComfyUI3DFormat::GLB:
        // GLB文件以"glTF"魔数开头（在偏移量4处）
        return ModelData.Num() >= 12 && 
               ModelData[4] == 'g' && ModelData[5] == 'l' && 
               ModelData[6] == 'T' && ModelData[7] == 'F';
        
    case EComfyUI3DFormat::GLTF:
        // GLTF是JSON格式，检查开头是否为'{'
        return ModelData[0] == '{';
        
    case EComfyUI3DFormat::OBJ:
        // OBJ文件通常以'#'或'v'开头
        return ModelData[0] == '#' || ModelData[0] == 'v';
        
    case EComfyUI3DFormat::FBX:
        // FBX二进制文件以特定魔数开头
        return ModelData.Num() >= 23 && FMemory::Memcmp(ModelData.GetData(), "Kaydara FBX Binary", 18) == 0;
        
    case EComfyUI3DFormat::PLY:
        // PLY文件以"ply"开头
        return ModelData.Num() >= 3 && 
               ModelData[0] == 'p' && ModelData[1] == 'l' && ModelData[2] == 'y';
        
    case EComfyUI3DFormat::STL:
        // STL二进制文件头部检查（非ASCII格式）
        if (ModelData.Num() >= 80)
        {
            // 检查是否为ASCII格式（以"solid"开头）
            bool bIsASCII = ModelData.Num() >= 5 && 
                           ModelData[0] == 's' && ModelData[1] == 'o' && 
                           ModelData[2] == 'l' && ModelData[3] == 'i' && ModelData[4] == 'd';
            return bIsASCII || ModelData.Num() >= 84; // 二进制STL至少84字节
        }
        return false;
        
    default:
        return true; // 未知格式，假设有效
    }
}

// ========== 简化的纹理处理实现 ==========

bool UComfyUIFileManager::SaveGeneratedTexture(UTexture2D* Texture, const FString& BaseName, const FString& PackagePath)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveGeneratedTexture: Texture is null"));
        return false;
    }

    if (BaseName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveGeneratedTexture: BaseName is empty"));
        return false;
    }

    FString FinalPackagePath = PackagePath.StartsWith(TEXT("/")) ? PackagePath : (TEXT("/Game/") + PackagePath);
    
    // 直接保存纹理
    return SaveTextureToProject(Texture, BaseName, FinalPackagePath);
}

UMaterialInstanceDynamic* UComfyUIFileManager::CreateMaterialFromTexture(UTexture2D* DiffuseTexture, const FString& MaterialName)
{
    // 这个函数需要一个基础材质模板来创建材质实例
    // 目前返回nullptr，需要在项目中添加合适的基础材质
    UE_LOG(LogTemp, Warning, TEXT("CreateMaterialFromTexture: Function not yet implemented - requires base material template"));
    
    // TODO: 实现材质实例创建逻辑
    // 1. 加载基础材质模板
    // 2. 创建动态材质实例
    // 3. 设置纹理参数
    
    return nullptr;
}
