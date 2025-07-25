#pragma once

#include "CoreMinimal.h"
#include "ComfyUIExecutionTypes.h"
#include "ComfyUIDelegates.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "ComfyUIUnifiedExecution.generated.h"

USTRUCT(BlueprintType)
struct FComfyUIUnifiedExecutionParams
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    EComfyUIWorkflowType WorkflowType;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    FComfyUIWorkflowInput Input;

    // 回调委托（C++专用，不暴露给蓝图）
    FOnImageGenerated OnImageGenerated;
    FOnGenerationProgress OnProgress;
    FOnGenerationStarted OnStarted;
    FOnGenerationCompleted OnCompleted;
};

USTRUCT(BlueprintType)
struct FComfyUIUnifiedExecutionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    UTexture2D* GeneratedImage = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    UStaticMesh* GeneratedMesh = nullptr;
    // 可扩展更多类型
};

class FComfyUIUnifiedExecution
{
public:
    static FComfyUIUnifiedExecutionResult ExecuteWorkflow(const FComfyUIUnifiedExecutionParams& Params);
};
