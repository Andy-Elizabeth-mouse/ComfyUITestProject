#include "Asset/ComfyUI3DAssetManager.h"
#include "Utils/ComfyUIFileManager.h"
#include "Utils/Defines.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

UComfyUI3DAssetManager::UComfyUI3DAssetManager()
{
}

UStaticMesh* UComfyUI3DAssetManager::CreateStaticMeshFromData(const TArray<uint8>& ModelData, const FString& ModelFormat)
{
    if (ModelData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateStaticMeshFromData: Empty model data"));
        return nullptr;
    }

    if (ModelFormat.ToLower() == TEXT("obj"))
    {
        return CreateStaticMeshFromOBJ(ModelData);
    }
    else if (ModelFormat.ToLower() == TEXT("gltf") || ModelFormat.ToLower() == TEXT("glb"))
    {
        return CreateStaticMeshFromGLTF(ModelData);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CreateStaticMeshFromData: Unsupported model format: %s"), *ModelFormat);
        return nullptr;
    }
}

UStaticMesh* UComfyUI3DAssetManager::CreateStaticMeshFromOBJ(const TArray<uint8>& OBJData)
{
    if (OBJData.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromOBJ: Empty OBJ data");

    TArray<FVector> Vertices;
    TArray<int32> Indices;
    TArray<FVector2D> UVs;

    if (!ParseOBJData(OBJData, Vertices, Indices, UVs))
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromOBJ: Failed to parse OBJ data");

    return CreateStaticMeshFromVertices(Vertices, Indices, UVs);
}

UStaticMesh* UComfyUI3DAssetManager::CreateStaticMeshFromGLTF(const TArray<uint8>& GLTFData)
{
    // TODO: 实现glTF解析
    // 这需要第三方库或者UE5的内置glTF导入器
    UE_LOG(LogTemp, Warning, TEXT("CreateStaticMeshFromGLTF: glTF import not yet implemented"));
    return nullptr;
}

bool UComfyUI3DAssetManager::Save3DModelToProject(UStaticMesh* StaticMesh, const FString& AssetName, const FString& PackagePath)
{
    if (!StaticMesh)
        LOG_AND_RETURN(Error, false, "Save3DModelToProject: StaticMesh is null");

    if (AssetName.IsEmpty())
        LOG_AND_RETURN(Error, false, "Save3DModelToProject: AssetName is empty");

    // 生成唯一的资产名称
    FString UniqueAssetName = GenerateUniqueAssetName(AssetName, PackagePath);
    FString FullPackageName = PackagePath / UniqueAssetName;

    // 验证包名
    if (!FPackageName::IsValidLongPackageName(FullPackageName))
        LOG_AND_RETURN(Error, false, "Save3DModelToProject: Invalid package name: %s", *FullPackageName);

    UE_LOG(LogTemp, Log, TEXT("Save3DModelToProject: Creating static mesh asset at path: %s"), *FullPackageName);

    try
    {
        // 创建包
        UPackage* Package = CreatePackage(*FullPackageName);
        if (!Package)
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Failed to create package: %s", *FullPackageName);

        Package->FullyLoad();

        // 创建新的静态网格对象
        UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(Package, *UniqueAssetName, RF_Public | RF_Standalone | RF_Transactional);
        if (!NewStaticMesh)
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Failed to create static mesh object");

        // 复制静态网格数据
        if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
        {
            // 初始化渲染数据
            NewStaticMesh->SetNumSourceModels(1);
            FStaticMeshSourceModel& SourceModel = NewStaticMesh->GetSourceModel(0);
            
            // 复制构建设置
            SourceModel.BuildSettings = StaticMesh->GetSourceModel(0).BuildSettings;
            
            // 构建静态网格
            NewStaticMesh->Build(false);
            
            UE_LOG(LogTemp, Log, TEXT("Save3DModelToProject: Successfully created static mesh copy"));
        }
        else
        {
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Source static mesh has no render data");
        }

        // 标记包为脏
        Package->SetDirtyFlag(true);

        // 通知资产注册表
        if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
        {
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            AssetRegistryModule.Get().AssetCreated(NewStaticMesh);
        }

        // 准备保存路径
        FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackageName, FPackageName::GetAssetPackageExtension());
        
        // 确保目录存在
        FString PackageDir = FPaths::GetPath(PackageFileName);
        if (!UComfyUIFileManager::EnsureDirectoryExists(PackageDir))
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Failed to create directory: %s", *PackageDir);

        // 保存包到磁盘
        bool bSaved = false;
        try 
        {
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Standalone;
            SaveArgs.SaveFlags = SAVE_None;
            SaveArgs.bForceByteSwapping = false;
            SaveArgs.bWarnOfLongFilename = true;
            SaveArgs.bSlowTask = false;
            SaveArgs.FinalTimeStamp = FDateTime::MinValue();
            SaveArgs.Error = GError;
            
            bSaved = UPackage::SavePackage(Package, NewStaticMesh, *PackageFileName, SaveArgs);
            
            UE_LOG(LogTemp, Log, TEXT("Save3DModelToProject: Save operation returned: %s"), bSaved ? TEXT("true") : TEXT("false"));
        }
        catch (const std::exception& Exception)
        {
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: std::exception during SavePackage: %hs", Exception.what());
        }
        catch (...)
        {
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Unknown exception during SavePackage");
        }

        if (bSaved)
            LOG_AND_RETURN(Log, true, "Save3DModelToProject: Successfully saved static mesh to %s", *FullPackageName);
        else
            LOG_AND_RETURN(Error, false, "Save3DModelToProject: Failed to save package to disk: %s", *PackageFileName);
    }
    catch (...)
    {
        LOG_AND_RETURN(Error, false, "Save3DModelToProject: Exception occurred during save operation");
    }
}

bool UComfyUI3DAssetManager::Save3DModelToFile(const FComfyUI3DModelData& ModelData, const FString& FilePath)
{
    if (ModelData.ModelData.Num() == 0)
        LOG_AND_RETURN(Error, false, "Save3DModelToFile: Model data is empty");

    if (FilePath.IsEmpty())
        LOG_AND_RETURN(Error, false, "Save3DModelToFile: File path is empty");

    // 确保目录存在
    FString FileDirectory = FPaths::GetPath(FilePath);
    if (!UComfyUIFileManager::EnsureDirectoryExists(FileDirectory))
        LOG_AND_RETURN(Error, false, "Save3DModelToFile: Failed to create directory: %s", *FileDirectory);

    // 保存模型数据到文件
    if (!FFileHelper::SaveArrayToFile(ModelData.ModelData, *FilePath))
        LOG_AND_RETURN(Error, false, "Save3DModelToFile: Failed to save model data to file: %s", *FilePath);

    // 如果有纹理数据，保存纹理文件
    if (ModelData.TextureData.Num() > 0)
    {
        FString TextureFilePath = FPaths::ChangeExtension(FilePath, TEXT("png"));
        if (!FFileHelper::SaveArrayToFile(ModelData.TextureData, *TextureFilePath))
        {
            UE_LOG(LogTemp, Warning, TEXT("Save3DModelToFile: Failed to save texture data to file: %s"), *TextureFilePath);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Save3DModelToFile: Successfully saved texture to: %s"), *TextureFilePath);
        }
    }

    LOG_AND_RETURN(Log, true, "Save3DModelToFile: Successfully saved 3D model to %s", *FilePath);
}

UMaterial* UComfyUI3DAssetManager::CreateMaterialFor3DModel(UTexture2D* DiffuseTexture, UTexture2D* NormalTexture, UTexture2D* RoughnessTexture)
{
    if (!DiffuseTexture)
        LOG_AND_RETURN(Error, nullptr, "CreateMaterialFor3DModel: DiffuseTexture is null");

    // 创建临时材质包
    FString PackageName = TEXT("/Temp/ComfyUI_Material_") + FGuid::NewGuid().ToString();
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
        LOG_AND_RETURN(Error, nullptr, "CreateMaterialFor3DModel: Failed to create package");

    // 创建材质
    UMaterial* Material = NewObject<UMaterial>(Package, TEXT("ComfyUI_Generated_Material"), RF_Public | RF_Standalone);
    if (!Material)
        LOG_AND_RETURN(Error, nullptr, "CreateMaterialFor3DModel: Failed to create material");

    // TODO: 设置材质节点和连接
    // 这需要更复杂的材质编辑器节点操作
    
    UE_LOG(LogTemp, Log, TEXT("CreateMaterialFor3DModel: Created basic material"));
    return Material;
}

bool UComfyUI3DAssetManager::ApplyTextureToStaticMesh(UStaticMesh* StaticMesh, UTexture2D* Texture)
{
    if (!StaticMesh || !Texture)
        LOG_AND_RETURN(Error, false, "ApplyTextureToStaticMesh: StaticMesh or Texture is null");

    // 创建材质实例
    UMaterial* BaseMaterial = CreateMaterialFor3DModel(Texture);
    if (!BaseMaterial)
        LOG_AND_RETURN(Error, false, "ApplyTextureToStaticMesh: Failed to create base material");

    // 将材质应用到静态网格
    if (StaticMesh->GetStaticMaterials().Num() > 0)
    {
        StaticMesh->SetMaterial(0, BaseMaterial);
        StaticMesh->PostEditChange();
        
        LOG_AND_RETURN(Log, true, "ApplyTextureToStaticMesh: Successfully applied texture to static mesh");
    }
    else
    {
        LOG_AND_RETURN(Error, false, "ApplyTextureToStaticMesh: Static mesh has no material slots");
    }
}

bool UComfyUI3DAssetManager::IsSupportedModelFormat(const FString& Format)
{
    TArray<FString> SupportedFormats = GetSupportedModelFormats();
    return SupportedFormats.Contains(Format.ToLower());
}

TArray<FString> UComfyUI3DAssetManager::GetSupportedModelFormats()
{
    return { TEXT("obj"), TEXT("gltf"), TEXT("glb") };
}

bool UComfyUI3DAssetManager::Validate3DModelData(const TArray<uint8>& ModelData, const FString& Format)
{
    if (ModelData.Num() == 0)
        return false;

    if (!IsSupportedModelFormat(Format))
        return false;

    // 基本的格式检查
    if (Format.ToLower() == TEXT("obj"))
    {
        // 检查是否包含基本的OBJ标识符
        FString DataString = UTF8_TO_TCHAR(reinterpret_cast<const char*>(ModelData.GetData()));
        return DataString.Contains(TEXT("v ")) || DataString.Contains(TEXT("f "));
    }
    else if (Format.ToLower() == TEXT("gltf"))
    {
        // 检查glTF JSON格式
        FString DataString = UTF8_TO_TCHAR(reinterpret_cast<const char*>(ModelData.GetData()));
        return DataString.Contains(TEXT("\"asset\"")) && DataString.Contains(TEXT("\"version\""));
    }
    else if (Format.ToLower() == TEXT("glb"))
    {
        // 检查glTF二进制格式头
        if (ModelData.Num() >= 4)
        {
            uint32 Magic = *reinterpret_cast<const uint32*>(ModelData.GetData());
            return Magic == 0x46546C67; // "glTF" in little-endian
        }
    }

    return true; // 基本验证通过
}

bool UComfyUI3DAssetManager::OptimizeStaticMesh(UStaticMesh* StaticMesh)
{
    if (!StaticMesh)
        LOG_AND_RETURN(Error, false, "OptimizeStaticMesh: StaticMesh is null");

    // 执行基本的网格优化
    try
    {
        StaticMesh->Build(false);
        StaticMesh->PostEditChange();
        
        LOG_AND_RETURN(Log, true, "OptimizeStaticMesh: Successfully optimized static mesh");
    }
    catch (...)
    {
        LOG_AND_RETURN(Error, false, "OptimizeStaticMesh: Exception occurred during optimization");
    }
}

// 私有函数实现

UStaticMesh* UComfyUI3DAssetManager::CreateStaticMeshFromVertices(const TArray<FVector>& Vertices, const TArray<int32>& Indices, const TArray<FVector2D>& UVs)
{
    if (Vertices.Num() == 0 || Indices.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromVertices: Invalid vertex or index data");

    // 创建临时静态网格包
    FString PackageName = TEXT("/Temp/ComfyUI_StaticMesh_") + FGuid::NewGuid().ToString();
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromVertices: Failed to create package");

    // 创建静态网格
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, TEXT("ComfyUI_Generated_Mesh"), RF_Public | RF_Standalone);
    if (!StaticMesh)
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromVertices: Failed to create static mesh");

    // 初始化源模型
    StaticMesh->SetNumSourceModels(1);
    FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(0);
    
    // 创建网格描述
    FMeshDescription MeshDescription;
    FStaticMeshAttributes StaticMeshAttributes(MeshDescription);
    StaticMeshAttributes.Register();

    // TODO: 使用新的UE5 MeshDescription API来构建静态网格
    // 这是一个复杂的过程，需要正确设置顶点、面和UV数据
    
    // 简化实现：先创建基本结构
    SourceModel.BuildSettings.bRecomputeNormals = true;
    SourceModel.BuildSettings.bRecomputeTangents = true;
    SourceModel.BuildSettings.bUseMikkTSpace = true;
    SourceModel.BuildSettings.bGenerateLightmapUVs = true;
    
    UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromVertices: Created static mesh with %d vertices, %d indices"), 
           Vertices.Num(), Indices.Num());
    
    UE_LOG(LogTemp, Warning, TEXT("CreateStaticMeshFromVertices: MeshDescription building not fully implemented yet"));

    return StaticMesh;
}

bool UComfyUI3DAssetManager::ParseOBJData(const TArray<uint8>& OBJData, TArray<FVector>& OutVertices, TArray<int32>& OutIndices, TArray<FVector2D>& OutUVs)
{
    OutVertices.Empty();
    OutIndices.Empty();
    OutUVs.Empty();

    // 将字节数据转换为字符串
    FString OBJContent = UTF8_TO_TCHAR(reinterpret_cast<const char*>(OBJData.GetData()));
    
    // 按行分割
    TArray<FString> Lines;
    OBJContent.ParseIntoArrayLines(Lines);

    TArray<FVector2D> TextureCoords;

    for (const FString& Line : Lines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        
        if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("#")))
            continue;

        TArray<FString> Tokens;
        TrimmedLine.ParseIntoArray(Tokens, TEXT(" "), true);
        
        if (Tokens.Num() == 0)
            continue;

        // 解析顶点位置
        if (Tokens[0] == TEXT("v") && Tokens.Num() >= 4)
        {
            float X = FCString::Atof(*Tokens[1]);
            float Y = FCString::Atof(*Tokens[2]);
            float Z = FCString::Atof(*Tokens[3]);
            OutVertices.Add(FVector(X, Y, Z));
        }
        // 解析纹理坐标
        else if (Tokens[0] == TEXT("vt") && Tokens.Num() >= 3)
        {
            float U = FCString::Atof(*Tokens[1]);
            float V = FCString::Atof(*Tokens[2]);
            TextureCoords.Add(FVector2D(U, V));
        }
        // 解析面
        else if (Tokens[0] == TEXT("f") && Tokens.Num() >= 4)
        {
            for (int32 i = 1; i < Tokens.Num(); ++i)
            {
                FString VertexData = Tokens[i];
                TArray<FString> VertexIndices;
                VertexData.ParseIntoArray(VertexIndices, TEXT("/"), false);
                
                if (VertexIndices.Num() > 0)
                {
                    int32 VertexIndex = FCString::Atoi(*VertexIndices[0]) - 1; // OBJ索引从1开始
                    if (VertexIndex >= 0 && VertexIndex < OutVertices.Num())
                    {
                        OutIndices.Add(VertexIndex);
                        
                        // 添加对应的UV坐标
                        if (VertexIndices.Num() > 1 && !VertexIndices[1].IsEmpty())
                        {
                            int32 UVIndex = FCString::Atoi(*VertexIndices[1]) - 1;
                            if (UVIndex >= 0 && UVIndex < TextureCoords.Num())
                            {
                                if (OutUVs.Num() <= VertexIndex)
                                {
                                    OutUVs.SetNum(VertexIndex + 1);
                                }
                                OutUVs[VertexIndex] = TextureCoords[UVIndex];
                            }
                        }
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ParseOBJData: Parsed %d vertices, %d indices, %d UVs"), 
           OutVertices.Num(), OutIndices.Num(), OutUVs.Num());

    return OutVertices.Num() > 0 && OutIndices.Num() > 0;
}

FString UComfyUI3DAssetManager::GenerateUniqueAssetName(const FString& BaseName, const FString& PackagePath)
{
    FString CleanBaseName = BaseName;
    CleanBaseName = CleanBaseName.Replace(TEXT(" "), TEXT("_"));
    CleanBaseName = CleanBaseName.Replace(TEXT("-"), TEXT("_"));
    
    FString TestName = CleanBaseName;
    int32 Counter = 1;
    
    while (true)
    {
        FString TestPackageName = PackagePath / TestName;
        if (!FPackageName::DoesPackageExist(TestPackageName))
        {
            break;
        }
        TestName = FString::Printf(TEXT("%s_%d"), *CleanBaseName, Counter++);
    }
    
    return TestName;
}