#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "ComfyUITypes.h"
#include "ComfyUINetworkManager.generated.h"

// NetworkManager需要反射系统支持，因为它继承自UObject
UCLASS()
class COMFYUIINTEGRATION_API UComfyUINetworkManager : public UObject
{
    GENERATED_BODY()
    
public:
    UComfyUINetworkManager();
    
    // HTTP请求相关方法
    void SendRequest(const FString& Url, const FString& Payload, TFunction<void(const FString& Response, bool bSuccess)> Callback);
    
    // GET请求（用于轮询状态、测试连接等）
    void SendGetRequest(const FString& Url, TFunction<void(const FString& Response, bool bSuccess)> Callback, float TimeoutSeconds = 10.0f);
    
    // 图片下载请求
    void DownloadImage(const FString& Url, TFunction<void(const TArray<uint8>& ImageData, bool bSuccess)> Callback);
    
    // 图片上传请求
    void UploadImage(const FString& ServerUrl, const TArray<uint8>& ImageData, const FString& FileName, TFunction<void(const FString& UploadedImageName, bool bSuccess)> Callback);
    
    // 3D模型上传请求（使用ComfyUI的通用上传端点）
    void UploadModel(const FString& ServerUrl, const TArray<uint8>& ModelData, const FString& FileName, TFunction<void(const FString& UploadedModelName, bool bSuccess)> Callback);
    
    // 3D模型下载请求
    void DownloadModel(const FString& Url, TFunction<void(const TArray<uint8>& ModelData, bool bSuccess)> Callback);
    
    // 轮询状态请求
    void PollQueueStatus(const FString& ServerUrl, const FString& PromptId, TFunction<void(const FString& Response, bool bSuccess)> Callback);
    
    // 测试服务器连接
    void TestServerConnection(const FString& ServerUrl, TFunction<void(bool bSuccess, const FString& ErrorMessage)> Callback);
    
    // 错误处理和分析
    FComfyUIError AnalyzeHttpError(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    
    // 重试机制
    bool ShouldRetryRequest(const FComfyUIError& Error, int32 CurrentRetryCount, int32 MaxRetryAttempts);
    
    // 用户友好的错误消息
    FString GetUserFriendlyErrorMessage(const FComfyUIError& Error);
    
private:
    // HTTP模块引用
    FHttpModule* HttpModule;
};
