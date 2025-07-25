#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowConfig.h"
#include "ComfyUINodeAnalyzer.generated.h"

class FJsonObject;

/**
 * ComfyUI节点分析器
 * 负责分析工作流节点，确定输入输出类型和工作流类型
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUINodeAnalyzer : public UObject
{
    GENERATED_BODY()

public:
    UComfyUINodeAnalyzer();

    /**
     * 分析工作流JSON，提取输入输出信息
     * 注意：由于使用TSharedPtr，此函数不能标记为UFUNCTION
     */
    bool AnalyzeWorkflow(TSharedPtr<FJsonObject> WorkflowJson, TArray<FWorkflowInputInfo>& OutInputs, TArray<FWorkflowOutputInfo>& OutOutputs);

    /**
     * 基于输入输出信息确定工作流类型
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Analysis")
    EComfyUIWorkflowType DetermineWorkflowType(const TArray<FWorkflowInputInfo>& Inputs, const TArray<FWorkflowOutputInfo>& Outputs);

    /**
     * 根据节点类型确定输入参数类型
     */
    EComfyUINodeInputType DetermineInputType(const FString& NodeType, const FString& ParameterName, const FString& Value);

    /**
     * 根据节点类型确定输出类型
     */
    EComfyUINodeOutputType DetermineOutputType(const FString& NodeType);

    /**
     * 生成用户友好的参数显示名称
     */
    FString GenerateDisplayName(const FString& ParameterName, EComfyUINodeInputType InputType);

private:
    /**
     * 分析单个节点
     */
    bool AnalyzeNode(const FString& NodeId, TSharedPtr<FJsonObject> NodeData, TArray<FWorkflowInputInfo>& OutInputs, TArray<FWorkflowOutputInfo>& OutOutputs);

    /**
     * 检查值是否为占位符（如 {POSITIVE_PROMPT}）
     */
    bool IsPlaceholderValue(const FString& Value);

    /**
     * 提取占位符名称
     */
    FString ExtractPlaceholderName(const FString& Value);

    /**
     * 设置参数约束
     */
    void SetParameterConstraints(FWorkflowInputInfo& InputInfo, const FString& NodeType, const FString& ParameterName);

private:
    /**
     * 确保映射已初始化（延迟初始化）
     */
    void EnsureInitialized();
    
    /**
     * 初始化节点类型映射
     */
    void InitializeNodeTypeMappings();

    /** 是否已初始化 */
    bool bIsInitialized = false;

    /** 节点类型到输出类型的映射 */
    TMap<FString, EComfyUINodeOutputType> NodeTypeToOutputType;

    /** 已知的文本输入参数名称 */
    TArray<FString> TextInputParameters;

    /** 已知的图像输入参数名称 */
    TArray<FString> ImageInputParameters;

    /** 已知的数值输入参数名称 */
    TArray<FString> NumberInputParameters;

    /** 已知的网格输入参数名称 */
    TArray<FString> MeshInputParameters;
};
