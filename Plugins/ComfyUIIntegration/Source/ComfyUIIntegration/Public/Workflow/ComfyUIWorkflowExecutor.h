#pragma once

#include "CoreMinimal.h"
#include "ComfyUIExecutionTypes.h"
#include "ComfyUIDelegates.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "ComfyUIWorkflowExecutor.generated.h"

// 前向声明
class UComfyUIClient;

/**
 * 工作流执行器参数结构体
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIWorkflowExecutorParams
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    EComfyUIWorkflowType WorkflowType;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    FComfyUIWorkflowInput Input;

    // 回调委托（C++专用，不暴露给蓝图）
    FOnImageGenerated OnImageGenerated;
    FOnMeshGenerated OnMeshGenerated;
    FOnGenerationProgress OnProgress;
    FOnGenerationStarted OnStarted;
    FOnGenerationFailed OnFailed;
    FOnGenerationCompleted OnCompleted;
};

USTRUCT(BlueprintType)
struct FComfyUIWorkflowExecutorResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    UTexture2D* GeneratedImage = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    UTexture2D* GeneratedTexture = nullptr;  // 保持向后兼容性

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    UStaticMesh* GeneratedMesh = nullptr;
    // 可扩展更多类型
};

class FComfyUIWorkflowExecutor
{
public:
    static FComfyUIWorkflowExecutorResult RunGeneration(EComfyUIWorkflowType WorkflowType,
                                                        const FString& Prompt,
                                                        const FString& NegativePrompt,
                                                        UTexture2D* InputImage,
                                                        const FString& InputModelPath,
                                                        UComfyUIClient* Client,
                                                        // 回调委托
                                                        FOnImageGenerated OnImageGenerated = FOnImageGenerated(),
                                                        FOnMeshGenerated OnMeshGenerated = FOnMeshGenerated(),
                                                        FOnGenerationProgress OnProgress = FOnGenerationProgress(),
                                                        FOnGenerationStarted OnStarted = FOnGenerationStarted(),
                                                        FOnGenerationFailed OnFailed = FOnGenerationFailed(),
                                                        FOnGenerationCompleted OnCompleted = FOnGenerationCompleted());
    static FComfyUIWorkflowExecutorResult ExecuteWorkflow(const FComfyUIWorkflowExecutorParams& Params, class UComfyUIClient* Client);

private:
    // 辅助函数：检查工作流是否需要图像输入
    static bool WorkflowNeedsImageInput(EComfyUIWorkflowType WorkflowType);
    
    // 辅助函数：检查工作流是否需要网格输入
    static bool WorkflowNeedsMeshInput(EComfyUIWorkflowType WorkflowType);
    
    // 内部实现函数
    static FString GetWorkflowNameFromType(EComfyUIWorkflowType WorkflowType);
};
