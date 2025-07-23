#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "ComfyUIWorkflowTester.generated.h"

/**
 * ComfyUI工作流测试器
 * 用于测试和验证工作流分析功能
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUIWorkflowTester : public UObject
{
    GENERATED_BODY()

public:
    UComfyUIWorkflowTester();

    /**
     * 测试工作流类型检测
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|Test")
    bool TestWorkflowTypeDetection();

    /**
     * 测试基本文生图工作流
     */
    bool TestBasicTextToImageWorkflow();

    /**
     * 测试图生图工作流
     */
    bool TestImageToImageWorkflow();

    /**
     * 测试UI生成功能
     */
    bool TestUIGeneration();

    /**
     * 打印工作流分析结果
     */
    void PrintWorkflowAnalysis(const FString& WorkflowName);

private:
    /**
     * 测试单个工作流文件
     */
    bool TestSingleWorkflow(const FString& WorkflowFilePath, EComfyUIWorkflowType ExpectedType);

    /**
     * 创建测试工作流JSON
     */
    FString CreateTestTextToImageWorkflow();
    FString CreateTestImageToImageWorkflow();
    FString CreateTestNumberedNodeWorkflow();
};
