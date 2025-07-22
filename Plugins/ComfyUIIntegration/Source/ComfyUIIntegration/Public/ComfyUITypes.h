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
