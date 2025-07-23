#pragma once
#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "DesktopPlatformModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Templates/SharedPointer.h"
#include "ComfyUIFileManager.generated.h"

class FJsonObject;

// 图像格式枚举
UENUM(BlueprintType)
enum class EComfyUIImageFormat : uint8
{
    PNG,
    JPEG,
    BMP
};

// 3D文件格式枚举
UENUM(BlueprintType)
enum class EComfyUI3DFormat : uint8
{
    GLB,        // glTF Binary
    GLTF,       // glTF Text
    OBJ,        // Wavefront OBJ
    FBX,        // Autodesk FBX
    PLY,        // Stanford PLY
    STL         // Stereolithography
};

// 纹理类型枚举（PBR材质相关）
UENUM(BlueprintType)
enum class EComfyUITextureType : uint8
{
    Diffuse,    // 漫反射贴图
    Normal,     // 法线贴图
    Roughness,  // 粗糙度贴图
    Metallic,   // 金属度贴图
    AO,         // 环境遮蔽贴图
    Emission,   // 自发光贴图
    Height,     // 高度贴图
    Opacity     // 透明度贴图
};

UCLASS()
class COMFYUIINTEGRATION_API UComfyUIFileManager : public UObject
{
    GENERATED_BODY()
public:
    // === 图像文件操作 ===
    // 从文件加载图像数据
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool LoadImageFromFile(const FString& FilePath, TArray<uint8>& OutImageData);

    // 创建纹理从图像数据
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);

    // 保存纹理到项目资产
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool SaveTextureToProject(UTexture2D* Texture, const FString& AssetName, const FString& PackagePath = TEXT("/Game/ComfyUI/Generated"));

    // 保存纹理到文件系统
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool SaveTextureToFile(UTexture2D* Texture, const FString& FilePath, EComfyUIImageFormat ImageFormat = EComfyUIImageFormat::PNG);

    // === 3D文件操作 ===
    // 从文件加载3D模型数据
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool Load3DModelFromFile(const FString& FilePath, TArray<uint8>& OutModelData);

    // 保存3D模型数据到文件
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool Save3DModelToFile(const TArray<uint8>& ModelData, const FString& FilePath, EComfyUI3DFormat Format = EComfyUI3DFormat::GLB);

    // 保存3D模型到项目资产（通过Datasmith或其他导入系统）
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool Save3DModelToProject(const TArray<uint8>& ModelData, const FString& AssetName, const FString& PackagePath = TEXT("/Game/ComfyUI/Generated/Models"));

    // 验证3D文件格式
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool ValidateF3DFormat(const TArray<uint8>& ModelData, EComfyUI3DFormat Format);

    // === 纹理集操作 ===
    // 保存单个纹理（简化版本，避免TMap模板问题）
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool SaveGeneratedTexture(UTexture2D* Texture, const FString& BaseName, const FString& PackagePath = TEXT("/Game/ComfyUI/Generated/Textures"));

    // 创建材质实例从纹理
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static class UMaterialInstanceDynamic* CreateMaterialFromTexture(UTexture2D* DiffuseTexture, const FString& MaterialName = TEXT("ComfyUI_Material"));

    // === JSON文件操作 ===
    // 加载JSON文件内容
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool LoadJsonFromFile(const FString& FilePath, FString& OutJsonContent);

    // 保存JSON到文件
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool SaveJsonToFile(const FString& JsonContent, const FString& FilePath);

    // 解析JSON字符串
    static bool ParseJsonString(const FString& JsonString, TSharedPtr<FJsonObject>& OutJsonObject);

    // === 文件操作 ===
    // 保存字节数组到文件
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool SaveArrayToFile(const TArray<uint8>& Data, const FString& FilePath);

    // === 文件对话框操作 ===
    // 打开文件选择对话框
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool ShowOpenFileDialog(const FString& DialogTitle, const FString& FileTypes, const FString& DefaultPath, TArray<FString>& OutFileNames);

    // 保存文件对话框
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool ShowSaveFileDialog(const FString& DialogTitle, const FString& FileTypes, const FString& DefaultPath, const FString& DefaultFileName, FString& OutFileName);

    // === 目录操作 ===
    // 扫描目录下的特定类型文件
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static TArray<FString> ScanFilesInDirectory(const FString& DirectoryPath, const FString& FileExtension = TEXT("*"));

    // 确保目录存在
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool EnsureDirectoryExists(const FString& DirectoryPath);

    // 获取插件相关路径
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static FString GetPluginDirectory();
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static FString GetTemplatesDirectory();
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static FString GetConfigDirectory();

    // === 工作流模板操作 ===
    // 导入工作流模板
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool ImportWorkflowTemplate(const FString& SourceFilePath, const FString& WorkflowName, FString& OutError);

    // 扫描可用的工作流模板
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static TArray<FString> ScanWorkflowTemplates();

    // 加载工作流模板内容
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|File")
    static bool LoadWorkflowTemplate(const FString& TemplateName, FString& OutJsonContent);

private:
    // 图像格式转换辅助函数
    static EImageFormat ConvertToImageWrapperFormat(EComfyUIImageFormat Format);
    
    // 生成唯一的资产名称
    static FString GenerateUniqueAssetName(const FString& BaseName, const FString& PackagePath);
};
