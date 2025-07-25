#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "Engine/Texture2D.h"
#include "ComfyUIDelegates.h"
#include "ComfyUITypes.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "Network/ComfyUINetworkManager.h"
#include "ComfyUIExecutionTypes.h"

#include "ComfyUIClient.generated.h"

/**
 * ComfyUI HTTP客户端，用于与ComfyUI服务器通信
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUIClient : public UObject
{
    GENERATED_BODY()

public:
    UComfyUIClient();
    UComfyUIClient(const FObjectInitializer& ObjectInitializer);

    /** 设置ComfyUI服务器URL */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetServerUrl(const FString& Url);

    /** 设置世界上下文 - C++专用 */
    void SetWorldContext(UWorld* InWorld);

    /** 设置重试配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetRetryConfiguration(int32 MaxRetries, float RetryDelay);

    /** 设置请求超时时间 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetRequestTimeout(float TimeoutSeconds);

    /** 取消当前生成任务 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void CancelCurrentGeneration();

    /** 检查服务器状态 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void CheckServerStatus();

    /** 获取可用的工作流列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void GetAvailableWorkflows();

    /** 获取所有可用的工作流名称 - 通过工作流服务 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    TArray<FString> GetAvailableWorkflowNames() const;
    
    /** 执行工作流 - C++专用 */
    void ExecuteWorkflow(const FString& WorkflowJson, 
                        const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                        const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                        const FOnImageGenerated& OnImageGenerated = FOnImageGenerated(),
                        const FOnMeshGenerated& OnMeshGenerated = FOnMeshGenerated(),
                        const FOnGenerationFailed& OnFailed = FOnGenerationFailed(),
                        const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());
    
    /** 上传图像并获取图像名称 - C++专用 */
    void UploadImage(const TArray<uint8>& ImageData, const FString& FileName, 
                    TFunction<void(const FString& UploadedImageName, bool bSuccess)> Callback);
    
    /** 测试服务器连接 - 带回调，C++专用 */
    void TestServerConnection(const FOnConnectionTested& OnComplete);
    
    /** 从图像数据创建纹理 - C++专用 */
    UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);

private:
    /** 网络通信管理器，封装 HTTP 请求 */
    UComfyUINetworkManager* NetworkManager;

    /** ComfyUI服务器URL */
    FString ServerUrl;

    /** HTTP模块引用 */
    FHttpModule* HttpModule;

    /** 世界上下文引用 */
    TWeakObjectPtr<UWorld> WorldContext;

    /** 当前请求 */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;

    /** 生成完成回调 */
    FOnImageGenerated OnImageGeneratedCallback;
    FOnMeshGenerated OnMeshGeneratedCallback;
    FOnGenerationFailed OnGenerationFailedCallback;
    FOnRetryAttempt OnRetryAttemptCallback;
    FOnGenerationProgress OnGenerationProgressCallback;
    FOnGenerationStarted OnGenerationStartedCallback;
    FOnGenerationCompleted OnGenerationCompletedCallback;

    /** 重试配置 */
    int32 MaxRetryAttempts = 3;
    float RetryDelaySeconds = 2.0f;
    float RequestTimeoutSeconds = 30.0f;

    /** 当前请求状态 */
    int32 CurrentRetryCount = 0;
    FString LastErrorMessage;
    EComfyUIErrorType LastErrorType = EComfyUIErrorType::None;

    /** 当前操作的上下文信息 */
    FString CurrentPrompt;
    FString CurrentNegativePrompt;
    EComfyUIWorkflowType CurrentWorkflowType;
    FString CurrentCustomWorkflowName;
    
    /** 构建自定义工作流JSON - 改用工作流服务 */
    FString BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt,
                                   const FString& CustomWorkflowName);

    /** 使用 NetworkManager 发送 Prompt 请求后的回调处理 */
    void OnPromptResponse(const FString& ResponseContent, bool bWasSuccessful);

    /** HTTP响应处理 */
    void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnImageDownloaded(const TArray<uint8>& ImageData, bool bWasSuccessful);
    void On3DModelDownloaded(const TArray<uint8>& ModelData, bool bWasSuccessful, const FString& Filename);
    void OnQueueStatusChecked(const FString& ResponseContent, bool bWasSuccessful);

    /** 错误处理和重试机制 */
    FComfyUIError AnalyzeHttpError(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    bool ShouldRetryRequest(const FComfyUIError& Error);
    void HandleRequestError(const FComfyUIError& Error, TFunction<void()> RetryFunction);
    void RetryCurrentOperation();
    void ResetRetryState();
    FString GetUserFriendlyErrorMessage(const FComfyUIError& Error);

    /** 工具函数 */
    void PollGenerationStatus(const FString& PromptId);
    void DownloadGeneratedImage(const FString& Filename, const FString& Subfolder, const FString& Type);
    void DownloadGenerated3DModel(const FString& Filename, const FString& Subfolder);
    FComfyUIProgressInfo ParseQueueStatus(const FString& ResponseContent);
    
    /** 确保NetworkManager被正确初始化 */
    void EnsureNetworkManagerInitialized();

    /** 当前生成的提示ID */
    FString CurrentPromptId;

    /** 轮询计时器 */
    FTimerHandle StatusPollTimer;
    
    /** 当前生成任务是否被取消 */
    bool bIsCancelled = false;
};
