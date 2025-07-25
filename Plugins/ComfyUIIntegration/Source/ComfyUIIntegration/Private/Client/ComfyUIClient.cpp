#include "Client/ComfyUIClient.h"
#include "Utils/ComfyUIFileManager.h"
#include "Workflow/ComfyUIWorkflowService.h"
#include "Asset/ComfyUI3DAssetManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UComfyUIClient::UComfyUIClient()
{
    HttpModule = &FHttpModule::Get();
    // 初始化网络管理器，用于封装 HTTP 请求
    // 注意：在默认构造函数中不能创建默认子对象，需要使用带ObjectInitializer的构造函数
    NetworkManager = nullptr;
    ServerUrl = TEXT("http://127.0.0.1:8188");
    WorldContext = nullptr;
}

UComfyUIClient::UComfyUIClient(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    HttpModule = &FHttpModule::Get();
    // 使用ObjectInitializer创建默认子对象
    NetworkManager = ObjectInitializer.CreateDefaultSubobject<UComfyUINetworkManager>(this, TEXT("NetworkManager"));
    ServerUrl = TEXT("http://127.0.0.1:8188");
    WorldContext = nullptr;
}

void UComfyUIClient::SetWorldContext(UWorld* InWorld)
{
    WorldContext = InWorld;
}

void UComfyUIClient::EnsureNetworkManagerInitialized()
{
    if (!NetworkManager)
    {
        // 如果使用了默认构造函数，需要在这里创建NetworkManager
        NetworkManager = NewObject<UComfyUINetworkManager>(this);
        UE_LOG(LogTemp, Warning, TEXT("NetworkManager was created using NewObject - this should be done via ObjectInitializer in constructor"));
    }
}

void UComfyUIClient::SetServerUrl(const FString& Url)
{
    ServerUrl = Url;
}

void UComfyUIClient::SetRetryConfiguration(int32 MaxRetries, float RetryDelay)
{
    MaxRetryAttempts = FMath::Max(0, MaxRetries);
    RetryDelaySeconds = FMath::Max(0.1f, RetryDelay);
    
    UE_LOG(LogTemp, Log, TEXT("Set retry configuration: MaxRetries=%d, RetryDelay=%.2fs"), MaxRetryAttempts, RetryDelaySeconds);
}

void UComfyUIClient::SetRequestTimeout(float TimeoutSeconds)
{
    RequestTimeoutSeconds = FMath::Max(5.0f, TimeoutSeconds);
    
    UE_LOG(LogTemp, Log, TEXT("Set request timeout: %.2fs"), RequestTimeoutSeconds);
}

void UComfyUIClient::CheckServerStatus()
{
    // 使用 NetworkManager 检查服务器状态
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        FString StatusUrl = ServerUrl + TEXT("/system_stats");
        NetworkManager->SendGetRequest(StatusUrl, 
            [this](const FString& Response, bool bSuccess)
            {
                if (bSuccess)
                {
                    UE_LOG(LogTemp, Log, TEXT("Server status check successful: %s"), *Response);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Server status check failed"));
                }
            });
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkManager not initialized"));
    }
}

void UComfyUIClient::GetAvailableWorkflows()
{
    // 工作流列表现在通过工作流服务管理
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (WorkflowService)
    {
        WorkflowService->RefreshWorkflows();
    }
}

TArray<FString> UComfyUIClient::GetAvailableWorkflowNames() const
{
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (WorkflowService)
    {
        return WorkflowService->GetAvailableWorkflowNames();
    }
    return TArray<FString>();
}

void UComfyUIClient::OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FComfyUIError Error = AnalyzeHttpError(Request, Response, bWasSuccessful);
    
    if (Error.ErrorType != EComfyUIErrorType::None)
    {
        HandleRequestError(Error, nullptr);
        return;
    }

    FString ResponseContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("Prompt submitted successfully: %s"), *ResponseContent);

    // 解析响应获取prompt_id
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        CurrentPromptId = JsonObject->GetStringField(TEXT("prompt_id"));
        if (!CurrentPromptId.IsEmpty())
        {
            // 重置重试状态，因为这一步成功了
            ResetRetryState();
            // 开始轮询生成状态
            PollGenerationStatus(CurrentPromptId);
        }
        else
        {
            FComfyUIError JsonError(EComfyUIErrorType::InvalidWorkflow, 
                                  TEXT("服务器响应中缺少prompt_id字段"), 
                                  0,
                                  TEXT("检查ComfyUI服务器版本是否兼容"), false);
            HandleRequestError(JsonError, [this]() { RetryCurrentOperation(); });
        }
    }
    else
    {
        FComfyUIError JsonError(EComfyUIErrorType::InvalidWorkflow, 
                              TEXT("无法解析服务器响应JSON"),
                              0,
                              TEXT("检查服务器响应格式"), false);
        HandleRequestError(JsonError, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::PollGenerationStatus(const FString& PromptId)
{
    // 使用 NetworkManager 进行状态轮询
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        NetworkManager->PollQueueStatus(ServerUrl, PromptId, 
            [this](const FString& Response, bool bSuccess)
            {
                OnQueueStatusChecked(Response, bSuccess);
            });
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkManager not initialized"));
        FComfyUIError Error(EComfyUIErrorType::UnknownError, TEXT("NetworkManager 未初始化"), 0, TEXT("请检查插件初始化流程"), false);
        HandleRequestError(Error, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::OnQueueStatusChecked(const FString& ResponseContent, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        FComfyUIError Error(EComfyUIErrorType::ConnectionFailed, TEXT("无法获取生成状态"), 0, TEXT("检查网络连接"), true);
        
        // 对于状态检查失败，我们可以重试状态查询而不是整个生成过程
        if (ShouldRetryRequest(Error))
        {
            CurrentRetryCount++;
            OnRetryAttemptCallback.ExecuteIfBound(CurrentRetryCount);
            
            UE_LOG(LogTemp, Warning, TEXT("Retrying status check... Attempt %d/%d"), 
                   CurrentRetryCount, MaxRetryAttempts);
            
            if (NetworkManager)
            {
                NetworkManager->ScheduleRetry(WorldContext, [this]() { PollGenerationStatus(CurrentPromptId); }, RetryDelaySeconds);
            }
            else
            {
                PollGenerationStatus(CurrentPromptId);
            }
        }
        else
        {
            HandleRequestError(Error, [this]() { RetryCurrentOperation(); });
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Queue status response: %s"), *ResponseContent);
    
    // 解析队列状态并更新进度
    FComfyUIProgressInfo ProgressInfo = ParseQueueStatus(ResponseContent);
    OnGenerationProgressCallback.ExecuteIfBound(ProgressInfo);
    
    // 如果任务被取消，停止处理
    if (bIsCancelled)
    {
        return;
    }
    
    // 解析响应检查是否完成
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // 检查是否有历史记录（表示完成）
        if (JsonObject->HasField(CurrentPromptId))
        {
            // 生成完成，获取输出信息
            TSharedPtr<FJsonObject> PromptHistory = JsonObject->GetObjectField(CurrentPromptId);
            if (PromptHistory.IsValid() && PromptHistory->HasField(TEXT("outputs")))
            {
                TSharedPtr<FJsonObject> Outputs = PromptHistory->GetObjectField(TEXT("outputs"));
                
                // 查找输出节点
                bool bFoundOutput = false;
                for (auto& OutputPair : Outputs->Values)
                {
                    TSharedPtr<FJsonObject> OutputNode = OutputPair.Value->AsObject();
                    if (OutputNode.IsValid())
                    {
                        // 检查图像输出
                        if (OutputNode->HasField(TEXT("images")))
                        {
                            const TArray<TSharedPtr<FJsonValue>>* ImagesArray;
                            if (OutputNode->TryGetArrayField(TEXT("images"), ImagesArray) && ImagesArray->Num() > 0)
                            {
                                // 获取第一张图片的信息
                                TSharedPtr<FJsonObject> ImageInfo = (*ImagesArray)[0]->AsObject();
                                if (ImageInfo.IsValid())
                                {
                                    FString Filename = ImageInfo->GetStringField(TEXT("filename"));
                                    FString Subfolder = ImageInfo->GetStringField(TEXT("subfolder"));
                                    FString Type = ImageInfo->GetStringField(TEXT("type"));

                                    UE_LOG(LogTemp, Log, TEXT("Found generated image: %s in %s"), *Filename, *Subfolder);

                                    // 重置重试状态，因为状态检查成功
                                    ResetRetryState();
                                    // 下载图片
                                    DownloadGeneratedImage(Filename, Subfolder, Type);
                                    bFoundOutput = true;
                                }
                            }
                        }
                    
                        // 检查3D模型输出
                        FString MeshFilename;
                        FString MeshSubfolder;
                        
                        // 检查不同的可能输出格式 - 统一处理逻辑
                        TArray<FString> MeshFieldNames = {TEXT("gltf"), TEXT("glb"), TEXT("meshes"), TEXT("mesh")};
                        
                        for (const FString& FieldName : MeshFieldNames)
                        {
                            if (OutputNode->HasField(FieldName))
                            {
                                const TArray<TSharedPtr<FJsonValue>>* MeshArray;
                                if (OutputNode->TryGetArrayField(FieldName, MeshArray) && MeshArray->Num() > 0)
                                {
                                    TSharedPtr<FJsonObject> MeshInfo = (*MeshArray)[0]->AsObject();
                                    if (MeshInfo.IsValid())
                                    {
                                        MeshFilename = MeshInfo->GetStringField(TEXT("filename"));
                                        MeshSubfolder = MeshInfo->GetStringField(TEXT("subfolder"));
                                        break; // 找到第一个有效的就停止
                                    }
                                }
                            }
                        }
                        
                        if (!MeshFilename.IsEmpty())
                        {
                            UE_LOG(LogTemp, Log, TEXT("Found generated 3D model: %s in %s"), *MeshFilename, *MeshSubfolder);
                            
                            // 重置重试状态
                            ResetRetryState();
                            // 下载3D模型
                            DownloadGenerated3DModel(MeshFilename, MeshSubfolder);
                            bFoundOutput = true;
                        }
                    }
                }
                
                if (!bFoundOutput)
                {
                    // 如果没有找到任何输出，报告错误
                    FComfyUIError OutputError(EComfyUIErrorType::ServerError, 
                                            TEXT("生成完成但未找到输出"), 
                                            0,
                                            TEXT("检查工作流是否包含输出节点"), false);
                    HandleRequestError(OutputError, [this]() { RetryCurrentOperation(); });
                }
            }
        }
        else
        {
            // 还未完成，继续轮询
            if (WorldContext.IsValid())
            {
                WorldContext->GetTimerManager().SetTimer(StatusPollTimer, 
                    FTimerDelegate::CreateLambda([this]() { PollGenerationStatus(CurrentPromptId); }),
                    2.0f, false); // 2秒后再次检查
            }
            else
            {
                FComfyUIError ContextError(EComfyUIErrorType::UnknownError, 
                                        TEXT("无效的世界上下文，无法管理计时器"), 0,
                                        TEXT("重新启动生成过程"), true);
                HandleRequestError(ContextError, [this]() { RetryCurrentOperation(); });
            }
        }
    }
    else
    {
        FComfyUIError JsonError(EComfyUIErrorType::JsonParsingError, 
                                TEXT("无法解析状态响应的JSON格式"), 
                                0,
                                TEXT("检查ComfyUI服务器状态"), true);
        HandleRequestError(JsonError, [this]() { PollGenerationStatus(CurrentPromptId); });
    }
}

void UComfyUIClient::OnImageDownloaded(const TArray<uint8>& ImageData, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        FComfyUIError DownloadError(EComfyUIErrorType::ImageDownloadFailed, 
                                  TEXT("图像下载失败"),
                                  0,
                                  TEXT("检查服务器状态，图像可能已生成但下载失败"), true);
        HandleRequestError(DownloadError, [this]() { 
            // 对于下载失败，重试整个操作
            RetryCurrentOperation(); 
        });
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Downloaded image data: %d bytes"), ImageData.Num());
    
    if (ImageData.Num() == 0)
    {
        FComfyUIError EmptyImageError(EComfyUIErrorType::ImageDownloadFailed, 
                                    TEXT("下载的图像数据为空"), 
                                    0,
                                    TEXT("检查生成的图像是否有效"), true);
        HandleRequestError(EmptyImageError, [this]() { RetryCurrentOperation(); });
        return;
    }
    
    // 创建纹理
    UTexture2D* GeneratedTexture = UComfyUIFileManager::CreateTextureFromImageData(ImageData);
    
    if (GeneratedTexture)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully created texture: %dx%d"), 
               GeneratedTexture->GetSizeX(), GeneratedTexture->GetSizeY());
               
        // 成功完成，重置重试状态
        ResetRetryState();
        
        // 通知生成完成
        OnGenerationCompletedCallback.ExecuteIfBound();
        
        // 最后通知图像生成完成
        OnImageGeneratedCallback.ExecuteIfBound(GeneratedTexture);
    }
    else
    {
        FComfyUIError TextureError(EComfyUIErrorType::ImageDownloadFailed, 
                                 TEXT("无法从图像数据创建纹理"), 
                                 0,
                                 TEXT("检查图像格式是否支持，或尝试重新生成"), true);
        HandleRequestError(TextureError, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::DownloadGeneratedImage(const FString& Filename, const FString& Subfolder, const FString& Type)
{
    // 构建图片下载URL
    FString ImageUrl = ServerUrl + TEXT("/view");
    
    // 添加查询参数
    TArray<FString> QueryParams;
    QueryParams.Add(FString::Printf(TEXT("filename=%s"), *FGenericPlatformHttp::UrlEncode(Filename)));
    
    if (!Subfolder.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("subfolder=%s"), *FGenericPlatformHttp::UrlEncode(Subfolder)));
    }
    
    if (!Type.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("type=%s"), *FGenericPlatformHttp::UrlEncode(Type)));
    }
    
    if (QueryParams.Num() > 0)
    {
        ImageUrl += TEXT("?") + FString::Join(QueryParams, TEXT("&"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Downloading image from: %s"), *ImageUrl);
    
    // 使用 NetworkManager 下载图片
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        NetworkManager->DownloadImage(ImageUrl, 
            [this](const TArray<uint8>& ImageData, bool bSuccess)
            {
                OnImageDownloaded(ImageData, bSuccess);
            });
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkManager not initialized"));
        FComfyUIError Error(EComfyUIErrorType::UnknownError, TEXT("NetworkManager 未初始化"), 0, TEXT("请检查插件初始化流程"), false);
        HandleRequestError(Error, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::TestServerConnection(const FOnConnectionTested& OnComplete)
{
    // 使用 NetworkManager 测试服务器连接
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        NetworkManager->TestServerConnection(ServerUrl, 
            [OnComplete](bool bSuccess, const FString& ErrorMessage)
            {
                OnComplete.ExecuteIfBound(bSuccess, ErrorMessage);
            });
    }
    else
    {
        OnComplete.ExecuteIfBound(false, TEXT("NetworkManager 未初始化"));
    }
}

// ========== 错误处理和重试机制 ==========

FComfyUIError UComfyUIClient::AnalyzeHttpError(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // 调用 NetworkManager 的 AnalyzeHttpError
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        return NetworkManager->AnalyzeHttpError(Request, Response, bWasSuccessful);
    }
    // 兜底逻辑（防止 NetworkManager 未初始化）
    return FComfyUIError(EComfyUIErrorType::UnknownError, TEXT("NetworkManager 未初始化"), 0, TEXT("请检查插件初始化流程"), false);
}

bool UComfyUIClient::ShouldRetryRequest(const FComfyUIError& Error)
{
    // 调用 NetworkManager 的 ShouldRetryRequest
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        return NetworkManager->ShouldRetryRequest(Error, CurrentRetryCount, MaxRetryAttempts);
    }
    return false;
}

void UComfyUIClient::HandleRequestError(const FComfyUIError& Error, TFunction<void()> RetryFunction)
{
    LastErrorType = Error.ErrorType;
    LastErrorMessage = Error.ErrorMessage;
    
    UE_LOG(LogTemp, Error, TEXT("ComfyUI Request Error: %s (Type: %d, HTTP: %d)"), 
           *Error.ErrorMessage, (int32)Error.ErrorType, Error.HttpStatusCode);
           
    if (ShouldRetryRequest(Error))
    {
        CurrentRetryCount++;
        
        UE_LOG(LogTemp, Warning, TEXT("Retrying request... Attempt %d/%d"), 
               CurrentRetryCount, MaxRetryAttempts);
               
        OnRetryAttemptCallback.ExecuteIfBound(CurrentRetryCount);
        
        // 延迟重试由 NetworkManager 统一调度
        if (NetworkManager)
        {
            NetworkManager->ScheduleRetry(WorldContext, RetryFunction, RetryDelaySeconds);
        }
        else
        {
            // 没有NetworkManager时的兜底处理
            RetryFunction();
        }
    }
    else
    {
        // 不能重试或达到最大重试次数
        FString UserFriendlyMessage = GetUserFriendlyErrorMessage(Error);
        
        UE_LOG(LogTemp, Error, TEXT("Final error after %d attempts: %s"), 
               CurrentRetryCount, *UserFriendlyMessage);
               
        OnGenerationFailedCallback.ExecuteIfBound(Error, false);
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        
        ResetRetryState();
    }
}

void UComfyUIClient::ResetRetryState()
{
    CurrentRetryCount = 0;
    LastErrorMessage.Empty();
    LastErrorType = EComfyUIErrorType::None;
}

FString UComfyUIClient::GetUserFriendlyErrorMessage(const FComfyUIError& Error)
{
    // 调用 NetworkManager 的 GetUserFriendlyErrorMessage
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        return NetworkManager->GetUserFriendlyErrorMessage(Error);
    }
    // 兜底逻辑
    return Error.ErrorMessage;
}

// NetworkManager 回调：处理 Prompt 提交响应
void UComfyUIClient::OnPromptResponse(const FString& ResponseContent, bool bSuccess)
{
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("OnPromptResponse: 请求失败"));
        FComfyUIError Error(EComfyUIErrorType::ConnectionFailed, TEXT("无法连接到 ComfyUI 服务器"), 0, TEXT("检查服务器URL和网络连接"), true);
        HandleRequestError(Error, [this]() { RetryCurrentOperation(); });
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Prompt submitted successfully: %s"), *ResponseContent);

    // 解析响应获取prompt_id
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        CurrentPromptId = JsonObject->GetStringField(TEXT("prompt_id"));
        if (!CurrentPromptId.IsEmpty())
        {
            // 重置重试状态，因为这一步成功了
            ResetRetryState();
            
            // 通知生成开始
            OnGenerationStartedCallback.ExecuteIfBound(CurrentPromptId);
            
            // 开始轮询生成状态
            PollGenerationStatus(CurrentPromptId);
        }
        else
        {
            FComfyUIError JsonError(EComfyUIErrorType::InvalidWorkflow, 
                                  TEXT("服务器响应中缺少prompt_id字段"), 
                                  0,
                                  TEXT("检查工作流是否正确配置"), false);
            HandleRequestError(JsonError, [this]() { RetryCurrentOperation(); });
        }
    }
    else
    {
        FComfyUIError JsonError(EComfyUIErrorType::InvalidWorkflow, 
                              TEXT("无法解析服务器响应JSON"),
                              0,
                              TEXT("检查服务器响应格式"), false);
        HandleRequestError(JsonError, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::CancelCurrentGeneration()
{
    bIsCancelled = true;
    
    // 清除计时器
    if (WorldContext.IsValid())
    {
        WorldContext->GetTimerManager().ClearTimer(StatusPollTimer);
    }
    
    // 取消当前HTTP请求
    if (CurrentRequest.IsValid())
    {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }
    
    // 重置状态
    CurrentPromptId.Empty();
    ResetRetryState();
    
    UE_LOG(LogTemp, Log, TEXT("Generation cancelled"));
}

FComfyUIProgressInfo UComfyUIClient::ParseQueueStatus(const FString& ResponseContent)
{
    FComfyUIProgressInfo ProgressInfo;
    
    // 检查是否被取消
    if (bIsCancelled)
    {
        ProgressInfo.StatusMessage = TEXT("已取消");
        return ProgressInfo;
    }
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        ProgressInfo.StatusMessage = TEXT("解析状态响应失败");
        return ProgressInfo;
    }

    // 检查是否在队列中运行
    if (JsonObject->HasField(TEXT("queue_running")))
    {
        const TArray<TSharedPtr<FJsonValue>>* QueueRunningArray;
        if (JsonObject->TryGetArrayField(TEXT("queue_running"), QueueRunningArray))
        {
            for (int32 i = 0; i < QueueRunningArray->Num(); ++i)
            {
                TSharedPtr<FJsonObject> QueueItem = (*QueueRunningArray)[i]->AsObject();
                if (QueueItem.IsValid() && QueueItem->HasField(TEXT("1")))
                {
                    FString QueuePromptId = QueueItem->GetStringField(TEXT("1"));
                    if (QueuePromptId == CurrentPromptId)
                    {
                        ProgressInfo.QueuePosition = 0; // 正在执行
                        ProgressInfo.bIsExecuting = true;
                        ProgressInfo.StatusMessage = TEXT("正在执行...");
                        ProgressInfo.ProgressPercentage = 0.5f; // 假设50%进度当正在执行时
                        return ProgressInfo;
                    }
                }
            }
        }
    }

    // 检查是否在等待队列中
    if (JsonObject->HasField(TEXT("queue_pending")))
    {
        const TArray<TSharedPtr<FJsonValue>>* QueuePendingArray;
        if (JsonObject->TryGetArrayField(TEXT("queue_pending"), QueuePendingArray))
        {
            for (int32 i = 0; i < QueuePendingArray->Num(); ++i)
            {
                TSharedPtr<FJsonObject> QueueItem = (*QueuePendingArray)[i]->AsObject();
                if (QueueItem.IsValid() && QueueItem->HasField(TEXT("1")))
                {
                    FString QueuePromptId = QueueItem->GetStringField(TEXT("1"));
                    if (QueuePromptId == CurrentPromptId)
                    {
                        ProgressInfo.QueuePosition = i + 1; // 队列位置从1开始
                        ProgressInfo.bIsExecuting = false;
                        ProgressInfo.StatusMessage = FString::Printf(TEXT("队列等待中.. (位置: %d)"), ProgressInfo.QueuePosition);
                        ProgressInfo.ProgressPercentage = 0.0f;
                        return ProgressInfo;
                    }
                }
            }
        }
    }

    // 如果都没找到，可能已经完成或出错
    ProgressInfo.StatusMessage = TEXT("检查完成状态..");
    ProgressInfo.ProgressPercentage = 0.8f;
    return ProgressInfo;
}
void UComfyUIClient::RetryCurrentOperation()
{
    // 重新提交当前的提示请求
    // 注意：这是一个简化的重试实现，应该根据实际的操作类型来决定重试逻辑
    if (!CurrentPromptId.IsEmpty())
    {
        // 如果有当前的提示ID，继续轮询状态
        PollGenerationStatus(CurrentPromptId);
    }
    else
    {
        // 如果没有提示ID，说明可能是提交提示失败了
        // 这里应该重新提交提示，但需要更多上下文信息
        // 暂时记录错误并调用失败回调
        UE_LOG(LogTemp, Warning, TEXT("RetryCurrentOperation called but no operation context available"));
        
        FComfyUIError Error;
        Error.ErrorType = EComfyUIErrorType::UnknownError;
        Error.ErrorMessage = TEXT("No operation context for retry");
        
        OnGenerationFailedCallback.ExecuteIfBound(Error, false);
    }
}

void UComfyUIClient::ExecuteWorkflow(const FString& WorkflowJson, 
                                   const FOnGenerationStarted& OnStarted,
                                   const FOnGenerationProgress& OnProgress,
                                   const FOnImageGenerated& OnImageGenerated,
                                   const FOnMeshGenerated& OnMeshGenerated,
                                   const FOnGenerationFailed& OnFailed,
                                   const FOnGenerationCompleted& OnCompleted)
{
    // 设置回调
    OnGenerationStartedCallback = OnStarted;
    OnGenerationProgressCallback = OnProgress;
    OnImageGeneratedCallback = OnImageGenerated;
    OnMeshGeneratedCallback = OnMeshGenerated;
    OnGenerationFailedCallback = OnFailed;
    OnGenerationCompletedCallback = OnCompleted;
    
    // 重置状态
    ResetRetryState();
    bIsCancelled = false;
    
    // 确保NetworkManager已初始化
    EnsureNetworkManagerInitialized();
    
    // 触发开始回调
    if (OnGenerationStartedCallback.IsBound())
    {
        OnGenerationStartedCallback.ExecuteIfBound(TEXT("workflow_execution_started"));
    }
    
    // 发送工作流JSON到ComfyUI服务器
    FString PromptEndpoint = ServerUrl + TEXT("/prompt");
    NetworkManager->SendRequest(PromptEndpoint, WorkflowJson, [this](const FString& Response, bool bSuccess) {
        if (!bIsCancelled)
        {
            OnPromptResponse(Response, bSuccess);
        }
    });
}

void UComfyUIClient::UploadImage(const TArray<uint8>& ImageData, const FString& FileName, 
                                TFunction<void(const FString& UploadedImageName, bool bSuccess)> Callback)
{
    // 确保NetworkManager已初始化
    EnsureNetworkManagerInitialized();
    
    // 使用NetworkManager上传图像
    NetworkManager->UploadImage(ServerUrl, ImageData, FileName, Callback);
}

void UComfyUIClient::DownloadGenerated3DModel(const FString& Filename, const FString& Subfolder)
{
    // 构建3D模型下载URL
    FString ModelUrl = ServerUrl + TEXT("/view");
    
    // 添加查询参数
    TArray<FString> QueryParams;
    QueryParams.Add(FString::Printf(TEXT("filename=%s"), *Filename));
    if (!Subfolder.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("subfolder=%s"), *Subfolder));
    }
    QueryParams.Add(TEXT("type=output"));
    
    if (QueryParams.Num() > 0)
    {
        ModelUrl += TEXT("?") + FString::Join(QueryParams, TEXT("&"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("Downloading 3D model from: %s"), *ModelUrl);
    
    // 使用NetworkManager下载3D模型
    EnsureNetworkManagerInitialized();
    if (NetworkManager)
    {
        NetworkManager->DownloadImage(ModelUrl, [this, Filename](const TArray<uint8>& ModelData, bool bSuccess) {
            On3DModelDownloaded(ModelData, bSuccess, Filename);
        });
    }
    else
    {
        FComfyUIError NetworkError(EComfyUIErrorType::UnknownError, 
                                 TEXT("NetworkManager未初始化"), 
                                 0,
                                 TEXT("请检查插件初始化流程"), false);
        HandleRequestError(NetworkError, [this]() { RetryCurrentOperation(); });
    }
}

void UComfyUIClient::On3DModelDownloaded(const TArray<uint8>& ModelData, bool bWasSuccessful, const FString& Filename)
{
    if (!bWasSuccessful || ModelData.Num() == 0)
    {
        FComfyUIError DownloadError(EComfyUIErrorType::ServerError, 
                                  TEXT("无法下载3D模型文件"), 
                                  0,
                                  TEXT("检查网络连接和文件权限"), true);
        HandleRequestError(DownloadError, [this]() { RetryCurrentOperation(); });
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Successfully downloaded 3D model: %d bytes"), ModelData.Num());
    
    // 从文件名判断格式
    FString FileExtension = FPaths::GetExtension(Filename).ToLower();
    
    // 使用3D资产管理器创建StaticMesh
    UComfyUI3DAssetManager* AssetManager = NewObject<UComfyUI3DAssetManager>();
    UStaticMesh* GeneratedMesh = AssetManager->CreateStaticMeshFromData(ModelData, FileExtension);
    
    if (GeneratedMesh)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully created 3D mesh from downloaded data"));
        
        // 成功完成，重置重试状态
        ResetRetryState();
        
        // 通知生成完成
        OnGenerationCompletedCallback.ExecuteIfBound();
        
        // 最后通知3D模型生成完成
        OnMeshGeneratedCallback.ExecuteIfBound(GeneratedMesh);
    }
    else
    {
        FComfyUIError MeshError(EComfyUIErrorType::ServerError, 
                              TEXT("无法从3D模型数据创建StaticMesh"), 
                              0,
                              TEXT("检查模型格式是否支持，或尝试重新生成"), true);
        HandleRequestError(MeshError, [this]() { RetryCurrentOperation(); });
    }
}
