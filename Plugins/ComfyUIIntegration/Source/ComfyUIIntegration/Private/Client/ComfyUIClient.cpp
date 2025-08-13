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
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"

// 单例实例声明
UComfyUIClient* UComfyUIClient::Instance = nullptr;

UComfyUIClient* UComfyUIClient::GetInstance()
{
    if (!Instance)
    {
        // 在编辑器模式下，使用Transient包创建单例
        Instance = NewObject<UComfyUIClient>(GetTransientPackage(), UComfyUIClient::StaticClass());
        if (Instance)
        {
            // 设置默认服务器URL
            Instance->SetServerUrl(TEXT("http://192.168.2.169:8188"));
                
            // 添加到根引用以防止被垃圾回收
            Instance->AddToRoot();
        }
    }
    return Instance;
}

void UComfyUIClient::DestroyInstance()
{
    // 在插件环境中，不主动操作根引用
    // Instance 由 UE 的 GC 系统自动管理
    Instance = nullptr;
}

UComfyUIClient::UComfyUIClient()
{
    HttpModule = &FHttpModule::Get();
    // 初始化网络管理器，用于封装 HTTP 请求
    // 注意：在默认构造函数中不能创建默认子对象，需要使用带ObjectInitializer的构造函数
    NetworkManager = nullptr;
    ServerUrl = TEXT("http://192.168.2.169:8188");
    LastPollTime = 0.0f;
    bIsPolling = false;
}

UComfyUIClient::UComfyUIClient(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    HttpModule = &FHttpModule::Get();
    // 使用ObjectInitializer创建默认子对象
    NetworkManager = ObjectInitializer.CreateDefaultSubobject<UComfyUINetworkManager>(this, TEXT("NetworkManager"));
    ServerUrl = TEXT("http://192.168.2.169:8188");
    LastPollTime = 0.0f;
    bIsPolling = false;
}

void UComfyUIClient::EnsureNetworkManagerInitialized()
{
    if (!NetworkManager)
    {
        NetworkManager = NewObject<UComfyUINetworkManager>(this);
        UE_LOG(LogTemp, Warning, TEXT("NetworkManager was created using NewObject - this should be done via ObjectInitializer in constructor"));
    }
}

void UComfyUIClient::SetServerUrl(const FString& Url)
{
    ServerUrl = Url;
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

void UComfyUIClient::PollGenerationStatus()
{
    PollGenerationStatus(CurrentPromptId);
}
#pragma optimize("", off)
void UComfyUIClient::OnQueueStatusChecked(const FString& ResponseContent, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        StopAsyncPolling();
        FComfyUIError Error(EComfyUIErrorType::ConnectionFailed, TEXT("无法获取生成状态"), 0, TEXT("检查网络连接"), true);
        HandleRequestError(Error, [this]() { PollGenerationStatus(CurrentPromptId); });
        return;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("Queue status response: %s"), *ResponseContent);
    
    // 解析队列状态并更新进度
    FComfyUIProgressInfo ProgressInfo = ParseQueueStatus(ResponseContent);
    OnGenerationProgressCallback.ExecuteIfBound(ProgressInfo);
    
    // 如果任务被取消，停止处理
    if (bIsCancelled) return;
    
    // 解析响应检查是否完成
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    
    bool bGenerationComplete = false;
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        // 检查是否有历史记录（表示完成）
        if (JsonObject->HasField(CurrentPromptId))
        {
            bGenerationComplete = true;
            
            // 生成完成，获取输出信息
            TSharedPtr<FJsonObject> PromptHistory = JsonObject->GetObjectField(CurrentPromptId);
            if (PromptHistory.IsValid() && PromptHistory->HasField(TEXT("outputs")))
            {
                TSharedPtr<FJsonObject> Outputs = PromptHistory->GetObjectField(TEXT("outputs"));
                UE_LOG(LogTemp, Log, TEXT("Generation completed with outputs: %s"), *ResponseContent);
                
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
                        
                        // 检查文本输出（ShowText节点输出的文件路径）
                        if (OutputNode->HasField(TEXT("text")))
                        {
                            const TArray<TSharedPtr<FJsonValue>>* TextArray;
                            if (OutputNode->TryGetArrayField(TEXT("text"), TextArray) && TextArray->Num() > 0)
                            {
                                FString TextOutput = (*TextArray)[0]->AsString();
                                if (!TextOutput.IsEmpty() && (TextOutput.EndsWith(TEXT(".glb")) || TextOutput.EndsWith(TEXT(".gltf"))))
                                {
                                    // 从文本输出中提取文件名
                                    MeshFilename = FPaths::GetCleanFilename(TextOutput);
                                    MeshSubfolder = TEXT(""); // ShowText输出的是完整路径，通常不包含子文件夹
                                    
                                    UE_LOG(LogTemp, Log, TEXT("Found 3D model path from text output: %s"), *TextOutput);
                                }
                            }
                        }
                        
                        // 如果没有从文本输出找到，检查传统的3D模型输出格式
                        if (MeshFilename.IsEmpty())
                        {
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
    }
    else
    {
        // JSON解析失败，但在生成过程中这可能是正常的（空响应）
        UE_LOG(LogTemp, Log, TEXT("JSON parsing failed - this is normal during generation, continuing to poll"));
    }
    
    // 无论JSON解析是否成功，只要生成未完成，就继续轮询
    if (!bGenerationComplete)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Generation still in progress, continuing to poll..."));
        
        // 使用异步轮询代替计时器
        if (!bIsPolling) StartAsyncPolling();
    }
    else
    {
        // 生成完成，停止轮询
        StopAsyncPolling();
        UE_LOG(LogTemp, Log, TEXT("Generation completed, stopped async polling"));
        
        // 重置重试状态
        ResetRetryState();
        
        // 通知生成完成
        OnGenerationCompletedCallback.ExecuteIfBound();
    }
}
#pragma optimize("", on)
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
        // OnGenerationCompletedCallback.ExecuteIfBound();
        
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

void UComfyUIClient::HandleRequestError(const FComfyUIError& Error, TFunction<void()> RetryFunction)
{
    EnsureNetworkManagerInitialized();
    LastErrorType = Error.ErrorType;
    LastErrorMessage = Error.ErrorMessage;
    
    UE_LOG(LogTemp, Error, TEXT("ComfyUI Request Error: %s (Type: %d, HTTP: %d)"), 
           *Error.ErrorMessage, (int32)Error.ErrorType, Error.HttpStatusCode);
           
    if (NetworkManager->ShouldRetryRequest(Error, CurrentRetryCount, MaxRetryAttempts))
    {
        CurrentRetryCount++;
        
        UE_LOG(LogTemp, Warning, TEXT("Retrying request... Attempt %d/%d"), 
               CurrentRetryCount, MaxRetryAttempts);
               
        OnRetryAttemptCallback.ExecuteIfBound(CurrentRetryCount);
        
        // 使用异步重试代替计时器
        StartAsyncRetry(RetryFunction, RetryDelaySeconds);
    }
    else
    {
        // 不能重试或达到最大重试次数
        FString UserFriendlyMessage = GetUserFriendlyErrorMessage(Error);
        
        UE_LOG(LogTemp, Error, TEXT("Final error after %d attempts: %s"), 
               CurrentRetryCount, *UserFriendlyMessage);
               
        OnGenerationFailedCallback.ExecuteIfBound(Error, false);
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        
        StopAsyncPolling();
        StopAsyncRetry();
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
    
    // 停止异步轮询和重试
    StopAsyncPolling();
    StopAsyncRetry();
    
    // 取消当前HTTP请求
    if (CurrentRequest.IsValid())
    {
        CurrentRequest->CancelRequest();
        CurrentRequest.Reset();
    }
    
    // 重置状态
    CurrentPromptId.Empty();
    ResetRetryState();
    
    // 通知UI生成已完成（被取消），以便重新启用按钮
    OnGenerationCompletedCallback.ExecuteIfBound();
    
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

void UComfyUIClient::UploadModel(const TArray<uint8>& ModelData, const FString& FileName, 
                                TFunction<void(const FString& UploadedModelName, bool bSuccess)> Callback)
{
    // 确保NetworkManager已初始化
    EnsureNetworkManagerInitialized();
    
    // 使用NetworkManager上传3D模型
    NetworkManager->UploadModel(ServerUrl, ModelData, FileName, Callback);
}

void UComfyUIClient::DownloadModel(const FString& Url, 
                                  TFunction<void(const TArray<uint8>& ModelData, bool bSuccess)> Callback)
{
    // 确保NetworkManager已初始化
    EnsureNetworkManagerInitialized();
    
    // 使用NetworkManager下载3D模型
    NetworkManager->DownloadModel(Url, Callback);
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
        NetworkManager->DownloadModel(ModelUrl, [this, Filename](const TArray<uint8>& ModelData, bool bSuccess) {
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
        // OnGenerationCompletedCallback.ExecuteIfBound();
        
        // 最后通知3D模型生成完成
        OnMeshGeneratedCallback.ExecuteIfBound(GeneratedMesh, ModelData, FileExtension);
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

// 异步轮询相关方法实现
void UComfyUIClient::StartAsyncPolling()
{
    if (!bIsPolling)
    {
        bIsPolling = true;
        LastPollTime = FPlatformTime::Seconds();
        
        // 使用 FTSTicker 进行异步轮询
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UComfyUIClient::HandleAsyncPoll),
            0.0f  // 每帧检查
        );
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Started async polling"));
    }
}

void UComfyUIClient::StopAsyncPolling()
{
    if (bIsPolling)
    {
        bIsPolling = false;
        
        if (TickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
            TickerHandle.Reset();
        }
        
        UE_LOG(LogTemp, VeryVerbose, TEXT("Stopped async polling"));
    }
}

bool UComfyUIClient::HandleAsyncPoll(float DeltaTime)
{
    if (!bIsPolling || bIsCancelled)
    {
        return false; // 停止 ticker
    }
    
    float CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastPollTime >= PollInterval)
    {
        LastPollTime = CurrentTime;
        
        // 执行轮询逻辑
        UE_LOG(LogTemp, VeryVerbose, TEXT("Async poll: checking generation status"));
        PollGenerationStatus();
    }
    
    return true; // 继续 ticker
}

// 异步重试相关方法实现
void UComfyUIClient::StartAsyncRetry(TFunction<void()> RetryFunction, float DelaySeconds)
{
    PendingRetryFunction = RetryFunction;
    RetryStartTime = FPlatformTime::Seconds();
    
    // 使用 FTSTicker 进行异步重试
    RetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UComfyUIClient::HandleAsyncRetry),
        0.0f  // 每帧检查
    );
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Started async retry with delay %.2f seconds"), DelaySeconds);
}

void UComfyUIClient::StopAsyncRetry()
{
    if (RetryTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(RetryTickerHandle);
        RetryTickerHandle.Reset();
        PendingRetryFunction = nullptr;
    }
}

bool UComfyUIClient::HandleAsyncRetry(float DeltaTime)
{
    float CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - RetryStartTime >= RetryDelaySeconds)
    {
        // 时间到了，执行重试
        if (PendingRetryFunction)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("Async retry: executing retry function"));
            PendingRetryFunction();
            PendingRetryFunction = nullptr;
        }
        
        return false; // 停止 ticker
    }
    
    return true; // 继续等待
}
