#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "ComfyUIExecutionTypes.generated.h"

class UStaticMesh;
class UMaterialInterface;

/**
 * 工作流执行状态
 */
UENUM(BlueprintType)
enum class EComfyUIExecutionStatus : uint8
{
    Pending            UMETA(DisplayName = "待执行"),
    Validating         UMETA(DisplayName = "验证中"),
    Uploading          UMETA(DisplayName = "上传中"),
    Executing          UMETA(DisplayName = "执行中"),
    Downloading        UMETA(DisplayName = "下载中"),
    Completed          UMETA(DisplayName = "完成"),
    Failed             UMETA(DisplayName = "失败"),
    Cancelled          UMETA(DisplayName = "已取消")
};

/**
 * 工作流输入参数
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIWorkflowInput
{
    GENERATED_BODY()

    // 文本参数（如提示词、负面提示词等）
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, FString> TextParameters;
    
    // 图像参数（参数名 -> 图像名称或路径）
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, FString> ImageParameters;
    
    // 3D模型参数（参数名 -> 文件路径或资产引用）
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, FString> MeshParameters;
    
    // 数值参数
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, float> NumericParameters;
    
    // 布尔参数
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, bool> BooleanParameters;
    
    // 选择参数
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    TMap<FString, FString> ChoiceParameters;

    FComfyUIWorkflowInput() { }

    // 便捷方法：添加提示词
    void SetPrompt(const FString& Prompt)
    {
        TextParameters.Add(TEXT("POSITIVE_PROMPT"), Prompt);
    }
    
    // 便捷方法：添加负面提示词
    void SetNegativePrompt(const FString& NegativePrompt)
    {
        TextParameters.Add(TEXT("NEGATIVE_PROMPT"), NegativePrompt);
    }
    
    // 便捷方法：添加输入图像
    void SetInputImage(const FString& ImageName)
    {
        ImageParameters.Add(TEXT("INPUT_IMAGE"), ImageName);
    }
};

/**
 * 工作流输出结果项
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIOutputItem
{
    GENERATED_BODY()

    // 输出类型
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    EComfyUINodeOutputType Type = EComfyUINodeOutputType::Unknown;
    
    // 输出节点ID
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString NodeId;
    
    // 输出索引（同一节点可能有多个输出）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    int32 OutputIndex = 0;
    
    // 文件名（如果适用）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString FileName;
    
    // 原始数据（图像、3D模型等的二进制数据）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    TArray<uint8> RawData;
    
    // 处理后的资产（可能是Texture2D、StaticMesh等）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    UObject* ProcessedAsset = nullptr;

    FComfyUIOutputItem() { }
};

/**
 * 工作流执行结果
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIWorkflowResult
{
    GENERATED_BODY()

    // 执行是否成功
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    bool bSuccess = false;
    
    // 执行状态
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    EComfyUIExecutionStatus Status = EComfyUIExecutionStatus::Pending;
    
    // 错误信息（如果失败）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString ErrorMessage;
    
    // 输出结果列表
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    TArray<FComfyUIOutputItem> Outputs;
    
    // 执行时间（秒）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    float ExecutionTime = 0.0f;
    
    // Prompt ID（用于追踪）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString PromptId;

    FComfyUIWorkflowResult() { }

    // 便捷方法：获取第一个图像输出
    UTexture2D* GetFirstImageOutput() const
    {
        for (const auto& Output : Outputs)
        {
            if (Output.Type == EComfyUINodeOutputType::Image && Output.ProcessedAsset)
            {
                return Cast<UTexture2D>(Output.ProcessedAsset);
            }
        }
        return nullptr;
    }
    
    // 便捷方法：获取第一个3D模型输出
    UStaticMesh* GetFirstMeshOutput() const
    {
        for (const auto& Output : Outputs)
        {
            if (Output.Type == EComfyUINodeOutputType::Mesh && Output.ProcessedAsset)
            {
                return Cast<UStaticMesh>(Output.ProcessedAsset);
            }
        }
        return nullptr;
    }
    
    // 便捷方法：获取所有图像输出
    TArray<UTexture2D*> GetAllImageOutputs() const
    {
        TArray<UTexture2D*> Images;
        for (const auto& Output : Outputs)
        {
            if (Output.Type == EComfyUINodeOutputType::Image && Output.ProcessedAsset)
            {
                if (UTexture2D* Texture = Cast<UTexture2D>(Output.ProcessedAsset))
                {
                    Images.Add(Texture);
                }
            }
        }
        return Images;
    }
};

/**
 * 工作流执行进度信息
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIExecutionProgress
{
    GENERATED_BODY()

    // 当前状态
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    EComfyUIExecutionStatus Status = EComfyUIExecutionStatus::Pending;
    
    // 进度百分比 (0-100)
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    float ProgressPercentage = 0.0f;
    
    // 当前步骤描述
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString CurrentStep;
    
    // 当前处理的节点
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    FString CurrentNode;
    
    // 队列位置
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    int32 QueuePosition = -1;
    
    // 预计剩余时间（秒）
    UPROPERTY(BlueprintReadOnly, Category = "ComfyUI")
    float EstimatedTimeRemaining = 0.0f;

    FComfyUIExecutionProgress() { }
};

/**
 * 工作流执行选项
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIExecutionOptions
{
    GENERATED_BODY()

    // 是否自动验证输入参数
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    bool bValidateInputs = true;
    
    // 是否在失败时自动重试
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    bool bAutoRetry = true;
    
    // 最大重试次数
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    int32 MaxRetries = 3;
    
    // 重试延迟（秒）
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    float RetryDelay = 2.0f;
    
    // 超时时间（秒）
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    float Timeout = 300.0f;
    
    // 是否保存原始输出文件
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    bool bSaveRawOutputs = false;
    
    // 原始输出保存路径
    UPROPERTY(BlueprintReadWrite, Category = "ComfyUI")
    FString RawOutputPath;

    FComfyUIExecutionOptions() { }
};

// 委托定义
DECLARE_DELEGATE_OneParam(FOnComfyUIWorkflowCompleted, const FComfyUIWorkflowResult&);
DECLARE_DELEGATE_OneParam(FOnComfyUIExecutionProgress, const FComfyUIExecutionProgress&);
DECLARE_DELEGATE_TwoParams(FOnComfyUIExecutionError, const FString& /* ErrorMessage */, bool /* bRetryable */);