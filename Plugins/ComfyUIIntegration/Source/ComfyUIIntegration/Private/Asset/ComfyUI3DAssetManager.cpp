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
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

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
    if (GLTFData.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromGLTF: Empty glTF data");

    // 创建临时文件来保存glTF数据
    FString TempDir = FPaths::ProjectIntermediateDir() / TEXT("ComfyUI") / TEXT("TempGLTF");
    FString TempFileName = FGuid::NewGuid().ToString();
    
    // 根据数据内容确定文件扩展名
    FString FileExtension = TEXT(".gltf");
    if (GLTFData.Num() >= 4)
    {
        uint32 Magic = *reinterpret_cast<const uint32*>(GLTFData.GetData());
        if (Magic == 0x46546C67) // "glTF" in little-endian
        {
            FileExtension = TEXT(".glb");
        }
    }
    
    FString TempFilePath = TempDir / (TempFileName + FileExtension);
    
    // 确保临时目录存在
    if (!UComfyUIFileManager::EnsureDirectoryExists(TempDir))
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromGLTF: Failed to create temp directory: %s", *TempDir);

    // 保存数据到临时文件
    if (!FFileHelper::SaveArrayToFile(GLTFData, *TempFilePath))
        LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromGLTF: Failed to save temp glTF file: %s", *TempFilePath);

    UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromGLTF: Created temp file: %s"), *TempFilePath);

    UStaticMesh* ImportedMesh = nullptr;

    try
    {
        // 使用 AssetImportTask 进行导入
        UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
        if (!ImportTask)
            LOG_AND_RETURN(Error, nullptr, "CreateStaticMeshFromGLTF: Failed to create AssetImportTask");

        // 设置导入任务参数
        ImportTask->Filename = TempFilePath;
        ImportTask->DestinationPath = TEXT("/Temp/ComfyUI_GLTFImport");
        ImportTask->DestinationName = TempFileName;
        ImportTask->bReplaceExisting = true;
        ImportTask->bReplaceExistingSettings = true;
        ImportTask->bAutomated = true;
        ImportTask->bSave = false; // 不保存到磁盘，只是临时导入

        UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromGLTF: Starting import task for file: %s"), *TempFilePath);

        // 获取 AssetTools 模块并执行导入
        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
            IAssetTools& AssetTools = AssetToolsModule.Get();

            // 执行导入任务
            TArray<UAssetImportTask*> ImportTasks;
            ImportTasks.Add(ImportTask);

            UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromGLTF: Executing import tasks"));

            // 使用 IAssetTools::ImportAssetTasks 执行导入
            AssetTools.ImportAssetTasks(ImportTasks);

            // 检查导入结果
            if (ImportTask->GetObjects().Num() > 0)
            {
                for (UObject* ImportedObject : ImportTask->GetObjects())
                {
                    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(ImportedObject))
                    {
                        ImportedMesh = StaticMesh;
                        UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromGLTF: Successfully imported static mesh: %s"), 
                               *StaticMesh->GetName());
                        break;
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("CreateStaticMeshFromGLTF: No objects were imported"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CreateStaticMeshFromGLTF: AssetTools module not available"));
        }
    }
    catch (const std::exception& Exception)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateStaticMeshFromGLTF: std::exception during import: %hs"), Exception.what());
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateStaticMeshFromGLTF: Unknown exception during import"));
    }

    // 清理临时文件
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (PlatformFile.FileExists(*TempFilePath))
    {
        if (PlatformFile.DeleteFile(*TempFilePath))
        {
            UE_LOG(LogTemp, Log, TEXT("CreateStaticMeshFromGLTF: Cleaned up temp file: %s"), *TempFilePath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateStaticMeshFromGLTF: Failed to clean up temp file: %s"), *TempFilePath);
        }
    }

    if (ImportedMesh)
    {
        LOG_AND_RETURN(Log, ImportedMesh, "CreateStaticMeshFromGLTF: Successfully imported glTF as StaticMesh");
    }
    else
    {
        // 作为后备方案，我们可以尝试将glTF解析为简单的几何体
        UE_LOG(LogTemp, Warning, TEXT("CreateStaticMeshFromGLTF: Import failed, attempting fallback parsing"));
        return CreateFallbackMeshFromGLTF(GLTFData);
    }
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

UStaticMesh* UComfyUI3DAssetManager::CreateFallbackMeshFromGLTF(const TArray<uint8>& GLTFData)
{
    UE_LOG(LogTemp, Log, TEXT("CreateFallbackMeshFromGLTF: Attempting basic glTF parsing"));

    if (GLTFData.Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "CreateFallbackMeshFromGLTF: Empty glTF data");

    // 检查是否为glTF二进制格式 (.glb)
    if (GLTFData.Num() >= 4)
    {
        uint32 Magic = *reinterpret_cast<const uint32*>(GLTFData.GetData());
        if (Magic == 0x46546C67) // "glTF" in little-endian
        {
            UE_LOG(LogTemp, Log, TEXT("CreateFallbackMeshFromGLTF: Detected GLB format"));
            return ParseGLBData(GLTFData);
        }
    }

    // 尝试解析为glTF JSON格式
    FString GLTFContent = UTF8_TO_TCHAR(reinterpret_cast<const char*>(GLTFData.GetData()));
    if (GLTFContent.Contains(TEXT("\"asset\"")) && GLTFContent.Contains(TEXT("\"version\"")))
    {
        UE_LOG(LogTemp, Log, TEXT("CreateFallbackMeshFromGLTF: Detected glTF JSON format"));
        return ParseGLTFJson(GLTFContent);
    }

    UE_LOG(LogTemp, Error, TEXT("CreateFallbackMeshFromGLTF: Unrecognized glTF format"));
    return nullptr;
}

UStaticMesh* UComfyUI3DAssetManager::ParseGLBData(const TArray<uint8>& GLBData)
{
    // GLB格式的基本结构：
    // 12字节头部 + JSON块 + (可选)二进制块
    if (GLBData.Num() < 12)
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: GLB data too small");

    const uint8* Data = GLBData.GetData();
    
    // 读取头部
    uint32 Magic = *reinterpret_cast<const uint32*>(Data);
    uint32 Version = *reinterpret_cast<const uint32*>(Data + 4);
    uint32 Length = *reinterpret_cast<const uint32*>(Data + 8);

    UE_LOG(LogTemp, Log, TEXT("ParseGLBData: Magic=0x%08X, Version=%d, Length=%d"), Magic, Version, Length);

    if (Magic != 0x46546C67) // "glTF"
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: Invalid GLB magic");

    if (Version != 2)
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: Unsupported GLB version: %d", Version);

    if (Length > *reinterpret_cast<const uint32*>(GLBData.Num()))
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: GLB length exceeds data size");

    // 读取JSON块
    if (GLBData.Num() < 20) // 12字节头部 + 8字节块头部
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: Not enough data for JSON chunk");

    uint32 JsonChunkLength = *reinterpret_cast<const uint32*>(Data + 12);
    uint32 JsonChunkType = *reinterpret_cast<const uint32*>(Data + 16);

    if (JsonChunkType != 0x4E4F534A) // "JSON"
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: Expected JSON chunk");

    if (12 + 8 + JsonChunkLength > *reinterpret_cast<const uint32*>(GLBData.Num()))
        LOG_AND_RETURN(Error, nullptr, "ParseGLBData: JSON chunk exceeds data size");

    // 提取JSON内容
    FString JsonContent = UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data + 20));
    JsonContent = JsonContent.Left(JsonChunkLength);

    UE_LOG(LogTemp, Log, TEXT("ParseGLBData: Extracted JSON chunk, size=%d"), JsonChunkLength);

    // 解析JSON内容
    return ParseGLTFJson(JsonContent);
}

UStaticMesh* UComfyUI3DAssetManager::ParseGLTFJson(const FString& JsonContent)
{
    UE_LOG(LogTemp, Log, TEXT("ParseGLTFJson: Parsing glTF JSON content"));

    // 这是一个简化的glTF解析器，只处理基本的网格数据
    // 实际的glTF格式非常复杂，包含场景图、动画、材质等
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        LOG_AND_RETURN(Error, nullptr, "ParseGLTFJson: Failed to parse JSON");

    // 检查是否有网格数据
    const TArray<TSharedPtr<FJsonValue>>* Meshes;
    if (!JsonObject->TryGetArrayField(TEXT("meshes"), Meshes) || Meshes->Num() == 0)
        LOG_AND_RETURN(Error, nullptr, "ParseGLTFJson: No meshes found in glTF");

    // 获取第一个网格
    TSharedPtr<FJsonObject> FirstMesh = (*Meshes)[0]->AsObject();
    if (!FirstMesh.IsValid())
        LOG_AND_RETURN(Error, nullptr, "ParseGLTFJson: Invalid mesh object");

    // 简化处理：创建一个基本的立方体网格作为占位符
    // 在实际实现中，这里应该解析顶点、索引和其他几何数据
    TArray<FVector> Vertices = {
        FVector(-50, -50, -50), FVector(50, -50, -50), FVector(50, 50, -50), FVector(-50, 50, -50),  // 底面
        FVector(-50, -50, 50),  FVector(50, -50, 50),  FVector(50, 50, 50),  FVector(-50, 50, 50)    // 顶面
    };

    TArray<int32> Indices = {
        0, 1, 2,  0, 2, 3,  // 底面
        4, 7, 6,  4, 6, 5,  // 顶面
        0, 4, 5,  0, 5, 1,  // 前面
        2, 6, 7,  2, 7, 3,  // 后面
        0, 3, 7,  0, 7, 4,  // 左面
        1, 5, 6,  1, 6, 2   // 右面
    };

    TArray<FVector2D> UVs;
    UVs.SetNum(Vertices.Num());
    for (int32 i = 0; i < UVs.Num(); ++i)
    {
        UVs[i] = FVector2D(0.0f, 0.0f); // 简化的UV坐标
    }

    UE_LOG(LogTemp, Warning, TEXT("ParseGLTFJson: Using placeholder cube mesh (full glTF parsing not implemented)"));
    
    return CreateStaticMeshFromVertices(Vertices, Indices, UVs);
}