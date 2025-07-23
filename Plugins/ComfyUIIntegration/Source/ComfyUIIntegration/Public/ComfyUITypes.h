#pragma once

#include "CoreMinimal.h"
#include "ComfyUITypes.generated.h"

/**
 * ComfyUI 错误类型枚举
 */
UENUM(BlueprintType)
enum class EComfyUIErrorType : uint8
{
    None = 0,          // 无错误
    ConnectionFailed,  // 连接失败
    ServerError,       // 服务器错误
    InvalidWorkflow,   // 工作流无效
    JsonParsingError,  // JSON解析错误
    ImageDownloadFailed, // 图像下载失败
    Timeout,           // 请求超时
    ServerUnavailable, // 服务器不可用
    AuthenticationFailed, // 认证失败
    InsufficientResources, // 资源不足
    UnknownError       // 未知错误
};

/**
 * ComfyUI 错误信息结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIError
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EComfyUIErrorType ErrorType = EComfyUIErrorType::None;
    
    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;
    
    UPROPERTY(BlueprintReadOnly)
    int32 HttpStatusCode = 0;
    
    UPROPERTY(BlueprintReadOnly)
    FString SuggestedSolution;
    
    UPROPERTY(BlueprintReadOnly)
    bool bCanRetry = false;

    FComfyUIError() = default;
    
    FComfyUIError(EComfyUIErrorType Type, const FString& Message, int32 StatusCode = 0, const FString& Solution = TEXT(""), bool CanRetry = false)
        : ErrorType(Type), ErrorMessage(Message), HttpStatusCode(StatusCode), SuggestedSolution(Solution), bCanRetry(CanRetry) {}
};

/**
 * 生成进度信息结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FComfyUIProgressInfo
{
    GENERATED_BODY()

    /** 队列中的位置 (0表示正在执行) */
    UPROPERTY(BlueprintReadOnly)
    int32 QueuePosition = 0;

    /** 总体进度百分比 (0.0 - 1.0) */
    UPROPERTY(BlueprintReadOnly)
    float ProgressPercentage = 0.0f;

    /** 当前执行的节点名称 */
    UPROPERTY(BlueprintReadOnly)
    FString CurrentNodeName;

    /** 当前状态描述 */
    UPROPERTY(BlueprintReadOnly)
    FString StatusMessage;

    /** 是否正在执行 */
    UPROPERTY(BlueprintReadOnly)
    bool bIsExecuting = false;

    FComfyUIProgressInfo()
    {
        QueuePosition = 0;
        ProgressPercentage = 0.0f;
        CurrentNodeName = TEXT("");
        StatusMessage = TEXT("等待中...");
        bIsExecuting = false;
    }

    FComfyUIProgressInfo(int32 InQueuePosition, float InProgressPercentage, const FString& InCurrentNodeName, const FString& InStatusMessage, bool InbIsExecuting)
        : QueuePosition(InQueuePosition)
        , ProgressPercentage(InProgressPercentage)
        , CurrentNodeName(InCurrentNodeName)
        , StatusMessage(InStatusMessage)
        , bIsExecuting(InbIsExecuting)
    {
    }
};
