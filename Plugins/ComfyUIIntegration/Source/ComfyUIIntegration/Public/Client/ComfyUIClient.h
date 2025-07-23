#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "Engine/Texture2D.h"
#include "ComfyUIDelegates.h"
#include "ComfyUITypes.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "Network/ComfyUINetworkManager.h"

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

    /** 设置世界上下文 */
    void SetWorldContext(UWorld* InWorld);

    /** 设置重试配置 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetRetryConfiguration(int32 MaxRetries, float RetryDelay);

    /** 设置请求超时时间 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetRequestTimeout(float TimeoutSeconds);

    /** 生成图像 - C++专用版本，使用委托回调 */
    void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                      EComfyUIWorkflowType WorkflowType, 
                      const FOnImageGenerated& OnComplete,
                      const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                      const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                      const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());

    /** 使用自定义工作流生成图像 - C++专用版本 */
    void GenerateImageWithCustomWorkflow(const FString& Prompt, const FString& NegativePrompt,
                                        const FString& CustomWorkflowName,
                                        const FOnImageGenerated& OnComplete,
                                        const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                                        const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                                        const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());

    /** 生成3D模型 - 文生3D */
    void Generate3DModel(const FString& Prompt, const FString& NegativePrompt,
                        const FOn3DModelGenerated& OnComplete,
                        const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                        const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                        const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());

    /** 生成3D模型 - 图生3D */
    void Generate3DModelFromImage(const FString& Prompt, const FString& NegativePrompt,
                                 const FString& InputImagePath,
                                 const FOn3DModelGenerated& OnComplete,
                                 const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                                 const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                                 const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());

    /** 生成PBR纹理集 */
    void GeneratePBRTextures(const FString& Prompt, const FString& NegativePrompt,
                            const FOnTextureGenerated& OnComplete,
                            const FOnGenerationProgress& OnProgress = FOnGenerationProgress(),
                            const FOnGenerationStarted& OnStarted = FOnGenerationStarted(),
                            const FOnGenerationCompleted& OnCompleted = FOnGenerationCompleted());

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
    
    /** 测试服务器连接 - 带回调 */
    void TestServerConnection(const FOnConnectionTested& OnComplete);
    
    /** 从图像数据创建纹理 */
    UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);

private:
    /** 网络通信管理器，封装 HTTP 请求 */
    UPROPERTY()
    UComfyUINetworkManager* NetworkManager;

    /** ComfyUI服务器URL */
    FString ServerUrl;

    /** HTTP模块引用 */
    FHttpModule* HttpModule;

    /** 世界上下文引用 */
    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldContext;

    /** 当前请求 */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;

    /** 生成完成回调 */
    FOnImageGenerated OnImageGeneratedCallback;
    FOnImageGenerationFailed OnImageGenerationFailedCallback;
    FOnRetryAttempt OnRetryAttemptCallback;
    FOnGenerationProgress OnGenerationProgressCallback;
    FOnGenerationStarted OnGenerationStartedCallback;
    FOnGenerationCompleted OnGenerationCompletedCallback;

    /** 3D模型生成回调 */
    FOn3DModelGenerated On3DModelGeneratedCallback;
    FOn3DModelImported On3DModelImportedCallback;

    /** PBR纹理集生成回调 */
    FOnTextureGenerated OnTextureGeneratedCallback;
    FOnMaterialCreated OnMaterialCreatedCallback;

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
    FString CurrentInputImagePath; // 用于图生3D

    /** 当前正在进行的生成类型 */
    enum class EGenerationType : uint8
    {
        None,
        Image,
        Model3D,
        PBRTextures
    };
    EGenerationType CurrentGenerationType = EGenerationType::None;
    
    /** 构建自定义工作流JSON - 改用工作流服务 */
    FString BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt,
                                   const FString& CustomWorkflowName);

    /** 使用 NetworkManager 发送 Prompt 请求后的回调处理 */
    void OnPromptResponse(const FString& ResponseContent, bool bWasSuccessful);

    /** HTTP响应处理 */
    void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnImageDownloaded(const TArray<uint8>& ImageData, bool bWasSuccessful);
    void OnQueueStatusChecked(const FString& ResponseContent, bool bWasSuccessful);

    /** 新的下载处理方法，根据生成类型处理不同数据 */
    void OnDataDownloaded(const TArray<uint8>& Data, bool bWasSuccessful);
    void ProcessImageData(const TArray<uint8>& ImageData);
    void Process3DModelData(const TArray<uint8>& ModelData);
    void ProcessPBRTextureData(const TArray<uint8>& TextureData, const FString& TextureType);

    /** 3D和纹理生成的通用处理方法 */
    void StartGeneration(EComfyUIWorkflowType WorkflowType, const FString& Prompt, const FString& NegativePrompt, const FString& InputImagePath = TEXT(""));

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
