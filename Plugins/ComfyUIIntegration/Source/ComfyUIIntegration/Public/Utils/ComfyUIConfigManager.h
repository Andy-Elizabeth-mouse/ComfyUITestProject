#pragma once
#include "CoreMinimal.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"
#include "ComfyUIConfigManager.generated.h"

struct FWorkflowConfig;
class FJsonObject;

UCLASS()
class COMFYUIINTEGRATION_API UComfyUIConfigManager : public UObject
{
    GENERATED_BODY()

public:
    // 加载默认配置
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Config")
    static bool LoadDefaultConfig();

    // 保存默认配置
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Config")
    static bool SaveDefaultConfig();

    // 加载工作流配置
    static TArray<FWorkflowConfig> LoadWorkflowConfigs();

    // 保存工作流配置
    static bool SaveWorkflowConfig(const FWorkflowConfig& Config);

    // 获取服务器设置
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Config")
    static bool GetServerSettings(FString& OutServerAddress, int32& OutServerPort);

    // 设置服务器设置
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Config")
    static bool SetServerSettings(const FString& ServerAddress, int32 ServerPort);

private:
    static FString ConfigFilePath;
    static TSharedPtr<FJsonObject> DefaultConfigJson;
    
    // 确保配置文件存在
    static bool EnsureConfigFileExists();
    
    // 创建默认配置内容
    static TSharedPtr<FJsonObject> CreateDefaultConfigJson();
};
