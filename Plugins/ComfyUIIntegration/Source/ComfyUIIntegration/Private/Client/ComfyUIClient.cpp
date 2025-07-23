#include "Client/ComfyUIClient.h"
#include "Utils/ComfyUIFileManager.h"
#include "Workflow/ComfyUIWorkflowService.h"
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

void UComfyUIClient::GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                                  EComfyUIWorkflowType WorkflowType, 
                                  const FOnImageGenerated& OnComplete,
                                  const FOnGenerationProgress& OnProgress,
                                  const FOnGenerationStarted& OnStarted,
                                  const FOnGenerationCompleted& OnCompleted)
{
    // 现在所有工作流都是自定义的，直接调用自定义工作流生成
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (!WorkflowService)
    {
        UE_LOG(LogTemp, Error, TEXT("WorkflowService not available"));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }

    TArray<FString> AvailableWorkflows = WorkflowService->GetAvailableWorkflowNames();
    if (AvailableWorkflows.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No custom workflows available. Please import a ComfyUI workflow first."));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }
    
    // 使用第一个可用的自定义工作流
    FString DefaultWorkflowName = AvailableWorkflows[0];
    UE_LOG(LogTemp, Warning, TEXT("Using default custom workflow: %s"), *DefaultWorkflowName);
    
    GenerateImageWithCustomWorkflow(Prompt, NegativePrompt, DefaultWorkflowName, OnComplete, OnProgress, OnStarted, OnCompleted);
}

void UComfyUIClient::GenerateImageWithCustomWorkflow(const FString& Prompt, const FString& NegativePrompt,
                                                     const FString& CustomWorkflowName,
                                                     const FOnImageGenerated& OnComplete,
                                                     const FOnGenerationProgress& OnProgress,
                                                     const FOnGenerationStarted& OnStarted,
                                                     const FOnGenerationCompleted& OnCompleted)
{
    OnImageGeneratedCallback = OnComplete;
    OnGenerationProgressCallback = OnProgress;
    OnGenerationStartedCallback = OnStarted;
    OnGenerationCompletedCallback = OnCompleted;
    
    // 重置取消状态
    bIsCancelled = false;
    
    // 保存当前操作上下文
    CurrentPrompt = Prompt;
    CurrentNegativePrompt = NegativePrompt;
    CurrentWorkflowType = EComfyUIWorkflowType::Custom;
    CurrentCustomWorkflowName = CustomWorkflowName;
    
    // 重置重试状态
    ResetRetryState();

    // 使用工作流服务构建自定义工作流JSON
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (!WorkflowService)
    {
        FComfyUIError Error(EComfyUIErrorType::InvalidWorkflow, 
                           TEXT("工作流服务不可用"));
        OnImageGenerationFailedCallback.ExecuteIfBound(Error, false);
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        return;
    }

    FString WorkflowJson = WorkflowService->BuildWorkflowJson(CustomWorkflowName, Prompt, NegativePrompt);
    
    if (WorkflowJson.IsEmpty() || WorkflowJson == TEXT("{}"))
    {
        FComfyUIError Error(EComfyUIErrorType::InvalidWorkflow, 
                           FString::Printf(TEXT("无法构建自定义工作流JSON: %s"), *CustomWorkflowName));
        OnImageGenerationFailedCallback.ExecuteIfBound(Error, false);
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        return;
    }

    // 使用 NetworkManager 发送请求
    FString Url = ServerUrl + TEXT("/prompt");
    UE_LOG(LogTemp, Log, TEXT("GenerateImageWithCustomWorkflow: Sending prompt via NetworkManager to %s"), *Url);
    
    EnsureNetworkManagerInitialized();
    NetworkManager->SendRequest(Url, WorkflowJson,
        [this](const FString& ResponseContent, bool bSuccess)
        {
            // 调用统一的响应处理
            OnPromptResponse(ResponseContent, bSuccess);
        }
    );
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
        HandleRequestError(Error, [this]() { RetryCurrentOperation(); });
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
                
                // 查找图像输出节点（通常是SaveImage节点）
                for (auto& OutputPair : Outputs->Values)
                {
                    TSharedPtr<FJsonObject> OutputNode = OutputPair.Value->AsObject();
                    if (OutputNode.IsValid() && OutputNode->HasField(TEXT("images")))
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
                                return;
                            }
                        }
                    }
                }
            }
            
            // 如果没有找到图片输出，报告错误
            FComfyUIError OutputError(EComfyUIErrorType::ServerError, 
                                    TEXT("生成完成但未找到图像输出"), 
                                    0,
                                    TEXT("检查工作流是否包含SaveImage节点"), false);
            HandleRequestError(OutputError, [this]() { RetryCurrentOperation(); });
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

// ========== 新的3D和纹理生成方法实现 ==========

void UComfyUIClient::Generate3DModel(const FString& Prompt, const FString& NegativePrompt,
                                     const FOn3DModelGenerated& OnComplete,
                                     const FOnGenerationProgress& OnProgress,
                                     const FOnGenerationStarted& OnStarted,
                                     const FOnGenerationCompleted& OnCompleted)
{
    UE_LOG(LogTemp, Log, TEXT("Starting 3D model generation with prompt: %s"), *Prompt);
    
    // 设置回调
    On3DModelGeneratedCallback = OnComplete;
    OnGenerationProgressCallback = OnProgress;
    OnGenerationStartedCallback = OnStarted;
    OnGenerationCompletedCallback = OnCompleted;
    
    // 设置生成类型
    CurrentGenerationType = EGenerationType::Model3D;
    
    // 开始生成
    StartGeneration(EComfyUIWorkflowType::TextTo3D, Prompt, NegativePrompt);
}

void UComfyUIClient::Generate3DModelFromImage(const FString& Prompt, const FString& NegativePrompt,
                                             const FString& InputImagePath,
                                             const FOn3DModelGenerated& OnComplete,
                                             const FOnGenerationProgress& OnProgress,
                                             const FOnGenerationStarted& OnStarted,
                                             const FOnGenerationCompleted& OnCompleted)
{
    UE_LOG(LogTemp, Log, TEXT("Starting image-to-3D generation with prompt: %s, image: %s"), *Prompt, *InputImagePath);
    
    // 验证输入图像文件存在
    if (!FPaths::FileExists(InputImagePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Input image file does not exist: %s"), *InputImagePath);
        FComfyUIError ImageError(EComfyUIErrorType::InvalidWorkflow,
                               FString::Printf(TEXT("输入图像文件不存在: %s"), *InputImagePath),
                               0,
                               TEXT("请检查图像文件路径是否正确"));
        OnComplete.ExecuteIfBound(TArray<uint8>());
        return;
    }
    
    // 设置回调
    On3DModelGeneratedCallback = OnComplete;
    OnGenerationProgressCallback = OnProgress;
    OnGenerationStartedCallback = OnStarted;
    OnGenerationCompletedCallback = OnCompleted;
    
    // 设置生成类型
    CurrentGenerationType = EGenerationType::Model3D;
    
    // 开始生成
    StartGeneration(EComfyUIWorkflowType::ImageTo3D, Prompt, NegativePrompt, InputImagePath);
}

void UComfyUIClient::GeneratePBRTextures(const FString& Prompt, const FString& NegativePrompt,
                                         const FOnTextureGenerated& OnComplete,
                                         const FOnGenerationProgress& OnProgress,
                                         const FOnGenerationStarted& OnStarted,
                                         const FOnGenerationCompleted& OnCompleted)
{
    UE_LOG(LogTemp, Log, TEXT("Starting PBR texture generation with prompt: %s"), *Prompt);
    
    // 设置回调
    OnTextureGeneratedCallback = OnComplete;
    OnGenerationProgressCallback = OnProgress;
    OnGenerationStartedCallback = OnStarted;
    OnGenerationCompletedCallback = OnCompleted;
    
    // 设置生成类型
    CurrentGenerationType = EGenerationType::PBRTextures;
    
    // 开始生成
    StartGeneration(EComfyUIWorkflowType::TextureGeneration, Prompt, NegativePrompt);
}

void UComfyUIClient::StartGeneration(EComfyUIWorkflowType WorkflowType, const FString& Prompt, const FString& NegativePrompt, const FString& InputImagePath)
{
    // 保存当前生成参数
    CurrentPrompt = Prompt;
    CurrentNegativePrompt = NegativePrompt;
    CurrentWorkflowType = WorkflowType;
    CurrentInputImagePath = InputImagePath;
    
    // 重置重试状态
    ResetRetryState();
    
    // 根据工作流类型选择合适的模板
    FString WorkflowName;
    switch (WorkflowType)
    {
    case EComfyUIWorkflowType::TextTo3D:
        WorkflowName = TEXT("text_to_3d");
        break;
    case EComfyUIWorkflowType::ImageTo3D:
        WorkflowName = TEXT("image_to_3d");
        break;
    case EComfyUIWorkflowType::TextureGeneration:
        WorkflowName = TEXT("texture_generation");
        break;
    default:
        UE_LOG(LogTemp, Error, TEXT("Unsupported workflow type for this generation method"));
        return;
    }
    
    // 使用自定义工作流生成
    GenerateImageWithCustomWorkflow(Prompt, NegativePrompt, WorkflowName,
        FOnImageGenerated(), // 不使用图像回调
        OnGenerationProgressCallback,
        OnGenerationStartedCallback,
        OnGenerationCompletedCallback
    );
}

void UComfyUIClient::OnDataDownloaded(const TArray<uint8>& Data, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        FComfyUIError DownloadError(EComfyUIErrorType::ImageDownloadFailed,
                                  TEXT("数据下载失败"),
                                  0,
                                  TEXT("检查服务器状态，数据可能已生成但下载失败"), true);
        HandleRequestError(DownloadError, [this]() { 
            RetryCurrentOperation(); 
        });
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Downloaded data: %d bytes, type: %d"), Data.Num(), (int32)CurrentGenerationType);
    
    if (Data.Num() == 0)
    {
        FComfyUIError EmptyDataError(EComfyUIErrorType::ImageDownloadFailed,
                                   TEXT("下载的数据为空"),
                                   0,
                                   TEXT("检查生成的数据是否有效"), true);
        HandleRequestError(EmptyDataError, [this]() { RetryCurrentOperation(); });
        return;
    }
    
    // 根据当前生成类型处理数据
    switch (CurrentGenerationType)
    {
    case EGenerationType::Image:
        ProcessImageData(Data);
        break;
    case EGenerationType::Model3D:
        Process3DModelData(Data);
        break;
    case EGenerationType::PBRTextures:
        // TODO: 实现PBR纹理数据处理
        ProcessPBRTextureData(Data, TEXT("diffuse"));
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown generation type, treating as image"));
        ProcessImageData(Data);
        break;
    }
}

void UComfyUIClient::ProcessImageData(const TArray<uint8>& ImageData)
{
    // 使用原有的图像处理逻辑
    OnImageDownloaded(ImageData, true);
}

void UComfyUIClient::Process3DModelData(const TArray<uint8>& ModelData)
{
    UE_LOG(LogTemp, Log, TEXT("Processing 3D model data: %d bytes"), ModelData.Num());
    
    // 验证模型数据格式（假设为GLB格式）
    if (!UComfyUIFileManager::ValidateF3DFormat(ModelData, EComfyUI3DFormat::GLB))
    {
        UE_LOG(LogTemp, Warning, TEXT("3D model format validation failed, proceeding anyway"));
    }
    
    // 成功完成，重置重试状态
    ResetRetryState();
    
    // 通知生成完成
    OnGenerationCompletedCallback.ExecuteIfBound();
    
    // 通知3D模型生成完成
    On3DModelGeneratedCallback.ExecuteIfBound(ModelData);
}

void UComfyUIClient::ProcessPBRTextureData(const TArray<uint8>& TextureData, const FString& TextureType)
{
    UE_LOG(LogTemp, Log, TEXT("Processing PBR texture data (%s): %d bytes"), *TextureType, TextureData.Num());
    
    // 简化处理，创建单个纹理
    UTexture2D* GeneratedTexture = UComfyUIFileManager::CreateTextureFromImageData(TextureData);
    if (GeneratedTexture)
    {
        // 成功完成，重置重试状态
        ResetRetryState();
        
        // 通知生成完成
        OnGenerationCompletedCallback.ExecuteIfBound();
        
        // 通知纹理生成完成
        OnTextureGeneratedCallback.ExecuteIfBound(GeneratedTexture);
    }
    else
    {
        FComfyUIError TextureError(EComfyUIErrorType::ImageDownloadFailed,
                                 TEXT("无法从纹理数据创建纹理"),
                                 0,
                                 TEXT("检查纹理格式是否支持"), true);
        HandleRequestError(TextureError, [this]() { RetryCurrentOperation(); });
    }
}

UTexture2D* UComfyUIClient::CreateTextureFromImageData(const TArray<uint8>& ImageData)
{
    if (ImageData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Empty image data"));
        return nullptr;
    }

    // 使用ImageWrapper模块来解码图像
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    
    // 尝试不同的图像格式
    TArray<EImageFormat> FormatsToTry = {
        EImageFormat::PNG,
        EImageFormat::JPEG,
        EImageFormat::BMP,
        EImageFormat::TIFF
    };
    
    for (EImageFormat Format : FormatsToTry)
    {
        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
        if (!ImageWrapper.IsValid())
        {
            continue;
        }

        if (ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
        {
            UE_LOG(LogTemp, Log, TEXT("Successfully decoded image as format: %d"), static_cast<int32>(Format));
            
            TArray<uint8> UncompressedBGRA;
            if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
            {
                // 创建纹理
                int32 Width = ImageWrapper->GetWidth();
                int32 Height = ImageWrapper->GetHeight();
                
                UE_LOG(LogTemp, Log, TEXT("Creating texture: %dx%d"), Width, Height);
                
                // 创建一个可持久化的纹理，而不是临时纹理
                UTexture2D* Texture = NewObject<UTexture2D>();
                if (Texture)
                {
                    // 初始化纹理源数据
                    Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, UncompressedBGRA.GetData());
                    
                    // 设置纹理属性
                    Texture->CompressionSettings = TextureCompressionSettings::TC_Default;
                    Texture->Filter = TextureFilter::TF_Default;
                    Texture->AddressX = TextureAddress::TA_Clamp;
                    Texture->AddressY = TextureAddress::TA_Clamp;
                    Texture->LODGroup = TextureGroup::TEXTUREGROUP_UI;
                    Texture->SRGB = true;
                    Texture->MipGenSettings = TMGS_NoMipmaps;
                    
                    // 标记为需要更新，并触发立即构建
                    Texture->PostEditChange();
                    Texture->UpdateResource();
                    
                    UE_LOG(LogTemp, Log, TEXT("Persistent texture created successfully with %d mips"), 
                           Texture->GetPlatformData() ? Texture->GetPlatformData()->Mips.Num() : 0);
                    return Texture;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to create UTexture2D"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get raw image data"));
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to decode image data with any supported format"));
    return nullptr;
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
               
        OnImageGenerationFailedCallback.ExecuteIfBound(Error, false);
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        
        ResetRetryState();
    }
}

void UComfyUIClient::RetryCurrentOperation()
{
    if (CurrentWorkflowType == EComfyUIWorkflowType::Custom && !CurrentCustomWorkflowName.IsEmpty())
    {
        GenerateImageWithCustomWorkflow(CurrentPrompt, CurrentNegativePrompt, 
                                       CurrentCustomWorkflowName, OnImageGeneratedCallback);
    }
    else
    {
        GenerateImage(CurrentPrompt, CurrentNegativePrompt, 
                     CurrentWorkflowType, OnImageGeneratedCallback);
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
