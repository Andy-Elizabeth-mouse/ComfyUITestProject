#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowConfig.h"
#include "ComfyUIWorkflowService.generated.h"

class UComfyUIWorkflowManager;
class FJsonObject;

/**
 * ComfyUI工作流服务
 * 提供统一的工作流管理接口，协调工作流管理器和其他组件
 */
UCLASS(BlueprintType, Blueprintable)
class COMFYUIINTEGRATION_API UComfyUIWorkflowService : public UObject
{
    GENERATED_BODY()

public:
    UComfyUIWorkflowService();

    // ========== 单例模式 ==========
    
    /** 获取全局工作流服务实例 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    static UComfyUIWorkflowService* Get();
    
    /** 初始化服务 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void Initialize();
    
    /** 关闭服务 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void Shutdown();

    // ========== 工作流管理接口 ==========
    
    /** 获取所有可用工作流名称 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    TArray<FString> GetAvailableWorkflowNames() const;
    
    /** 根据名称获取工作流配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool GetWorkflowConfig(const FString& WorkflowName, FWorkflowConfig& OutConfig) const;
    
    /** 刷新工作流配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void RefreshWorkflows();
    
    /** 验证工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ValidateWorkflow(const FString& FilePath, FString& OutError);
    
    /** 导入工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ImportWorkflow(const FString& FilePath, const FString& WorkflowName, FString& OutError);

    // ========== JSON构建接口 ==========
    
    /** 构建工作流JSON - 蓝图版本 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    FString BuildWorkflowJson(const FString& WorkflowName, 
                             const FString& Prompt, 
                             const FString& NegativePrompt,
                             const TMap<FString, FString>& Parameters);
    
    /** 构建工作流JSON - C++重载版本，支持默认参数 */
    FString BuildWorkflowJson(const FString& WorkflowName, 
                             const FString& Prompt, 
                             const FString& NegativePrompt);
    
    /** 构建工作流JSON - 支持float参数的版本 */
    FString BuildWorkflowJson(const FString& WorkflowName, 
                             const FString& Prompt, 
                             const FString& NegativePrompt,
                             const TMap<FString, float>& Parameters);
    
    /** 构建带图像的工作流JSON - C++专用，不暴露给蓝图 */
    void BuildWorkflowJsonWithImage(const FString& WorkflowName,
                                   const FString& Prompt,
                                   const FString& NegativePrompt,
                                   const TArray<uint8>& ImageData,
                                   const FString& ServerUrl,
                                   TFunction<void(const FString& WorkflowJson, bool bSuccess)> Callback);

    // ========== 参数管理接口 ==========
    
    /** 获取工作流参数列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    TArray<FString> GetWorkflowParameters(const FString& WorkflowName) const;
    
    /** 设置工作流参数 - C++专用 */
    bool SetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName, const FString& Value);
    
    /** 获取工作流参数值 - C++专用 */
    FString GetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName) const;

    // ========== 便捷接口 ==========
    
    /** 检查工作流是否存在 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool IsWorkflowValid(const FString& WorkflowName) const;
    
    /** 获取工作流描述 - C++专用 */
    FString GetWorkflowDescription(const FString& WorkflowName) const;
    
    /** 获取工作流类型 - C++专用 */
    FString GetWorkflowType(const FString& WorkflowName) const;
    
    /** 确定工作流类型 - C++专用 */
    EComfyUIWorkflowType DetectWorkflowType(const FString &WorkflowName);
private:
    /** 工作流管理器实例 */
    TObjectPtr<UComfyUIWorkflowManager> WorkflowManager;

private:
    /** 全局服务实例 */
    static TObjectPtr<UComfyUIWorkflowService> Instance;
    
    /** 是否已初始化 */
    bool bInitialized = false;
};
