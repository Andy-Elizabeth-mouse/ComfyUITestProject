#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowConfig.h"
#include "ComfyUIExecutionTypes.h"
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
    
    /** 静态方法：关闭和清理全局实例 */
    static void ShutdownGlobal();

    // ========== 工作流管理接口 ==========
    
    /** 获取所有可用工作流名称 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    TArray<FString> GetAvailableWorkflowNames() const;
    
    /** 验证工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ValidateWorkflow(const FString& FilePath, FString& OutError);
    
    /** 导入工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ImportWorkflow(const FString& FilePath, const FString& WorkflowName, FString& OutError);

    // ========== JSON构建接口 ==========
    
    /** 构建工作流JSON - 使用完整的FComfyUIWorkflowInput参数 */
    FString BuildWorkflowJson(const FString& WorkflowName, 
                             const FComfyUIWorkflowInput& Input);

    // ========== 参数管理接口 ==========
    
    /** 设置工作流参数 */
    bool SetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName, const FString& Value);
    
    /** 获取工作流参数值 */
    FString GetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName) const;

    // ========== 便捷接口 ==========
    
    /** 确定工作流类型 */
    EComfyUIWorkflowType DetectWorkflowType(const FString &WorkflowName);
private:
    /** 工作流管理器实例 */
    UPROPERTY()
    TObjectPtr<UComfyUIWorkflowManager> WorkflowManager;

private:
    /** 全局服务实例 */
    static TObjectPtr<UComfyUIWorkflowService> Instance;
    
    /** 是否已初始化 */
    bool bInitialized = false;
};
