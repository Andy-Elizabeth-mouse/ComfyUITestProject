#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowConfig.h"
#include "ComfyUIWorkflowManager.generated.h"

class FJsonObject;

/**
 * ComfyUI工作流管理器
 * 负责处理自定义工作流的加载、验证、管理等功能
 */
UCLASS(BlueprintType, Blueprintable)
class COMFYUIINTEGRATION_API UComfyUIWorkflowManager : public UObject
{
    GENERATED_BODY()

public:
    UComfyUIWorkflowManager();

    // ========== 工作流加载和管理 ==========
    
    /** 从配置文件加载所有工作流配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void LoadWorkflowConfigs();
    
    /** 从文件加载自定义工作流 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool LoadCustomWorkflowFromFile(const FString& FilePath);
    
    /** 扫描并加载模板目录中的所有工作流 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void LoadTemplateDirectoryWorkflows();
    
    /** 获取所有可用工作流名称 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    TArray<FString> GetAvailableWorkflowNames() const;
    
    /** 根据名称查找工作流配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool FindWorkflowConfig(const FString& WorkflowName, FWorkflowConfig& OutConfig) const;

    // ========== 工作流验证 ==========
    
    /** 验证工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ValidateWorkflowFile(const FString& FilePath, FString& OutError);
    
    /** 验证工作流JSON内容 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ValidateWorkflowJson(const FString& JsonContent, FWorkflowConfig& OutConfig, FString& OutError);
    
    // ========== 工作流类型检测 ==========
    
    /** 检测工作流类型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    EComfyUIWorkflowType DetectWorkflowType(const FString& WorkflowName);
    
    /** 从配置分析工作流类型 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    EComfyUIWorkflowType AnalyzEComfyUIWorkflowTypeFromConfig(const FWorkflowConfig& Config);
    
    /** 更新工作流的输入输出信息 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool UpdateWorkflowInputOutputInfo(const FString& WorkflowName);

    /** 运行工作流测试 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Test")
    bool RunWorkflowTests();

    // ========== 工作流导入和导出 ==========
    
    /** 导入工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ImportWorkflowFile(const FString& FilePath, const FString& WorkflowName, FString& OutError);
    
    /** 导出工作流配置到文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool ExportWorkflowConfig(const FWorkflowConfig& Config, const FString& FilePath, FString& OutError);

    // ========== 工作流JSON构建 ==========
    
    /** 构建自定义工作流JSON */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    FString BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                                   const FString& CustomWorkflowName);
    
    /** 替换工作流模板中的占位符 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    FString ReplaceWorkflowPlaceholders(const FString& WorkflowTemplate, 
                                       const FString& Prompt, 
                                       const FString& NegativePrompt,
                                       const TMap<FString, FString>& CustomParameters);
    
    /** 替换工作流模板中的占位符 - C++重载版本，支持默认参数 */
    FString ReplaceWorkflowPlaceholders(const FString& WorkflowTemplate, 
                                       const FString& Prompt, 
                                       const FString& NegativePrompt);

    // ========== 工作流参数管理 ==========
    
    /** 获取工作流参数名称列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    TArray<FString> GetWorkflowParameterNames(const FString& WorkflowName) const;
    
    /** 设置工作流参数 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    bool SetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName, const FString& Value);
    
    /** 获取工作流参数 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    FString GetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName) const;

    // ========== 工具函数 ==========
    
    /** 清理工作流配置列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void ClearWorkflowConfigs();
    
    /** 重新加载所有工作流配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Workflow")
    void RefreshWorkflowConfigs();

protected:
    /** 自定义工作流配置列表 */
    UPROPERTY()
    TArray<FWorkflowConfig> CustomWorkflowConfigs;

private:
    // ========== 内部验证函数 ==========
    
    /** 分析工作流节点 */
    bool AnalyzeWorkflowNodes(TSharedPtr<FJsonObject> WorkflowJson, FWorkflowConfig& OutConfig);
    
    /** 查找工作流输入节点 */
    bool FindWorkflowInputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutInputs);
    
    /** 查找工作流输出节点 */
    bool FindWorkflowOutputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutOutputs);
    
    /** 清理工作流名称 */
    FString SanitizeWorkflowName(const FString& Name) const;
    
    /** 根据名称查找工作流配置的内部版本 */
    FWorkflowConfig* FindWorkflowConfigInternal(const FString& WorkflowName);
    const FWorkflowConfig* FindWorkflowConfigInternal(const FString& WorkflowName) const;
    
    /** 从配置JSON加载工作流 */
    void LoadWorkflowsFromConfigJson(TSharedPtr<FJsonObject> ConfigJson);
};
