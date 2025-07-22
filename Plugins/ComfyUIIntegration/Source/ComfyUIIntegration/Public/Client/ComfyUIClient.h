#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "Engine/Texture2D.h"
#include "ComfyUIDelegates.h"
#include "ComfyUITypes.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "Network/ComfyUINetworkManager.h"

/** 工作流类型枚举 */
UENUM(BlueprintType)
enum class EComfyUIWorkflowType : uint8
{
    TextToImage,      // 文生图
    ImageToImage,     // 图生图
    TextTo3D,         // 文生3D
    ImageTo3D,        // 图生3D
    TextureGeneration, // 贴图生成
    Custom            // 自定义工作流
};

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
                      const FOnImageGenerated& OnComplete);

    /** 使用自定义工作流生成图像 - C++专用版本 */
    void GenerateImageWithCustomWorkflow(const FString& Prompt, const FString& NegativePrompt,
                                        const FString& CustomWorkflowName,
                                        const FOnImageGenerated& OnComplete);

    /** 检查服务器状态 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void CheckServerStatus();

    /** 获取可用的工作流列表 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void GetAvailableWorkflows();

    /** 获取所有可用的工作流名称 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    TArray<FString> GetAvailableWorkflowNames() const;
    
    /** 验证工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    bool ValidateWorkflowFile(const FString& FilePath, FString& OutError);
    
    /** 导入工作流文件 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    bool ImportWorkflowFile(const FString& FilePath, const FString& WorkflowName, FString& OutError);
    
    /** 获取工作流参数定义 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    TArray<FString> GetWorkflowParameterNames(const FString& WorkflowName) const;
    
    /** 测试工作流连接 */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void TestWorkflowConnection();
    
    /** 测试服务器连接 - 带回调 */
    void TestServerConnection(const FOnConnectionTested& OnComplete);
    
    /** 从图像数据创建纹理 */
    UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);
    
    /** 验证工作流JSON */
    bool ValidateWorkflowJson(const FString& JsonContent, FWorkflowConfig& OutConfig, FString& OutError);

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

    /** 预定义工作流配置 */
    TMap<EComfyUIWorkflowType, FWorkflowConfig> WorkflowConfigs;
    
    /** 自定义工作流配置 */
    TArray<FWorkflowConfig> CustomWorkflowConfigs;

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

    /** 初始化工作流配置 */
    void InitializeWorkflowConfigs();
    
    /** 从配置文件加载工作流 */
    void LoadWorkflowConfigs();
    
    /** 加载自定义工作流模板 */
    bool LoadCustomWorkflowFromFile(const FString& FilePath);

    /** 构建工作流JSON */
    FString BuildWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                             EComfyUIWorkflowType WorkflowType);
                             
    /** 构建自定义工作流JSON */
    FString BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt,
                                   const FString& CustomWorkflowName);

    /** 使用 NetworkManager 发送 Prompt 请求后的回调处理 */
    void OnPromptResponse(const FString& ResponseContent, bool bWasSuccessful);

    /** HTTP响应处理 */
    void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnImageDownloaded(const TArray<uint8>& ImageData, bool bWasSuccessful);
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
    
    /** 工作流验证功能 */
    bool AnalyzeWorkflowNodes(TSharedPtr<FJsonObject> WorkflowJson, FWorkflowConfig& OutConfig);
    bool FindWorkflowInputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutInputs);
    bool FindWorkflowOutputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutOutputs);
    FString SanitizeWorkflowName(const FString& Name);

    /** 当前生成的提示ID */
    FString CurrentPromptId;

    /** 轮询计时器 */
    FTimerHandle StatusPollTimer;
};
