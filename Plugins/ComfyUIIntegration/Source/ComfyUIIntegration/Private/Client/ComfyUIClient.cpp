#include "Client/ComfyUIClient.h"
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
    NetworkManager = NewObject<UComfyUINetworkManager>(this);
    ServerUrl = TEXT("http://127.0.0.1:8188");
    WorldContext = nullptr;
    InitializeWorkflowConfigs();
    LoadWorkflowConfigs();
}

void UComfyUIClient::SetWorldContext(UWorld* InWorld)
{
    WorldContext = InWorld;
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

void UComfyUIClient::InitializeWorkflowConfigs()
{
    // 清空硬编码的工作流配置，改为完全依赖动态加载
    WorkflowConfigs.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("Initialized empty workflow configs - will load custom workflows from files"));
}

void UComfyUIClient::GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                                  EComfyUIWorkflowType WorkflowType, 
                                  const FOnImageGenerated& OnComplete)
{
    // 现在所有工作流都是自定义的，直接调用自定义工作流生成
    if (CustomWorkflowConfigs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No custom workflows available. Please import a ComfyUI workflow first."));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }
    
    // 使用第一个可用的自定义工作流
    FString DefaultWorkflowName = CustomWorkflowConfigs[0].Name;
    UE_LOG(LogTemp, Warning, TEXT("Using default custom workflow: %s"), *DefaultWorkflowName);
    
    GenerateImageWithCustomWorkflow(Prompt, NegativePrompt, DefaultWorkflowName, OnComplete);
}

void UComfyUIClient::GenerateImageWithCustomWorkflow(const FString& Prompt, const FString& NegativePrompt,
                                                     const FString& CustomWorkflowName,
                                                     const FOnImageGenerated& OnComplete)
{
    OnImageGeneratedCallback = OnComplete;
    
    // 保存当前操作上下文
    CurrentPrompt = Prompt;
    CurrentNegativePrompt = NegativePrompt;
    CurrentWorkflowType = EComfyUIWorkflowType::Custom;
    CurrentCustomWorkflowName = CustomWorkflowName;
    
    // 重置重试状态
    ResetRetryState();

    // 构建自定义工作流JSON
    FString WorkflowJson = BuildCustomWorkflowJson(Prompt, NegativePrompt, CustomWorkflowName);
    
    if (WorkflowJson.IsEmpty())
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
    // 刷新工作流列表
    LoadWorkflowConfigs();
}

TArray<FString> UComfyUIClient::GetAvailableWorkflowNames() const
{
    TArray<FString> WorkflowNames;
    
    // 添加预定义工作流
    for (const auto& Workflow : WorkflowConfigs)
    {
        WorkflowNames.Add(Workflow.Value.Name);
    }
    
    // 添加自定义工作流
    for (const auto& CustomWorkflow : CustomWorkflowConfigs)
    {
        WorkflowNames.Add(CustomWorkflow.Name);
    }
    
    return WorkflowNames;
}

FString UComfyUIClient::BuildWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                                         EComfyUIWorkflowType WorkflowType)
{
    // 创建请求JSON对象
    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("client_id"), TEXT("unreal_engine_plugin"));

    // 获取工作流配置
    FWorkflowConfig* Config = WorkflowConfigs.Find(WorkflowType);
    if (!Config)
    {
        UE_LOG(LogTemp, Error, TEXT("Workflow type not found"));
        return TEXT("{}");
    }

    // 解析工作流模板
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Config->JsonTemplate);
    if (!FJsonSerializer::Deserialize(Reader, WorkflowJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse workflow template"));
        return TEXT("{}");
    }

    // 替换模板中的占位符
    FString WorkflowString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&WorkflowString);
    FJsonSerializer::Serialize(WorkflowJson.ToSharedRef(), Writer);

    // 替换提示词占位符
    WorkflowString = WorkflowString.Replace(TEXT("{POSITIVE_PROMPT}"), *Prompt);
    WorkflowString = WorkflowString.Replace(TEXT("{NEGATIVE_PROMPT}"), *NegativePrompt);

    // 重新解析以确保JSON有效
    TSharedPtr<FJsonObject> FinalWorkflowJson;
    TSharedRef<TJsonReader<>> FinalReader = TJsonReaderFactory<>::Create(WorkflowString);
    if (FJsonSerializer::Deserialize(FinalReader, FinalWorkflowJson))
    {
        RequestJson->SetObjectField(TEXT("prompt"), FinalWorkflowJson);
    }

    // 序列化最终的请求JSON
    FString OutputString;
    TSharedRef<TJsonWriter<>> OutputWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), OutputWriter);

    return OutputString;
}

FString UComfyUIClient::BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt,
                                               const FString& CustomWorkflowName)
{
    // 首先查找自定义工作流配置
    FWorkflowConfig* CustomConfig = nullptr;
    for (FWorkflowConfig& Config : CustomWorkflowConfigs)
    {
        if (Config.Name == CustomWorkflowName)
        {
            CustomConfig = &Config;
            break;
        }
    }
    
    if (!CustomConfig)
    {
        UE_LOG(LogTemp, Error, TEXT("Custom workflow not found: %s"), *CustomWorkflowName);
        return TEXT("{}");
    }
    
    // 创建请求JSON对象
    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("client_id"), TEXT("unreal_engine_plugin"));

    // 如果有模板内容，使用模板
    FString WorkflowTemplate;
    if (!CustomConfig->JsonTemplate.IsEmpty())
    {
        WorkflowTemplate = CustomConfig->JsonTemplate;
        UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Using cached template content"));
    }
    else if (!CustomConfig->TemplateFile.IsEmpty())
    {
        // 确保使用绝对路径
        FString TemplateFilePath = CustomConfig->TemplateFile;
        if (!FPaths::IsRelative(TemplateFilePath))
        {
            // 已经是绝对路径
        }
        else
        {
            // 相对路径，需要构造完整路径
            FString PluginDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration");
            FString TemplatesDir = PluginDir / TEXT("Config") / TEXT("Templates");
            TemplateFilePath = TemplatesDir / TemplateFilePath;
        }
        
        UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Loading template from file: %s"), *TemplateFilePath);
        
        // 从文件加载模板
        if (!FFileHelper::LoadFileToString(WorkflowTemplate, *TemplateFilePath))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load workflow template file: %s"), *TemplateFilePath);
            return TEXT("{}");
        }
        
        // 缓存模板内容以供下次使用
        CustomConfig->JsonTemplate = WorkflowTemplate;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No template found for custom workflow: %s"), *CustomWorkflowName);
        return TEXT("{}");
    }

    // 解析工作流模板
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowTemplate);
    if (!FJsonSerializer::Deserialize(Reader, WorkflowJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse custom workflow template: %s"), *CustomWorkflowName);
        return TEXT("{}");
    }

    // 替换模板中的占位符
    FString WorkflowString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&WorkflowString);
    FJsonSerializer::Serialize(WorkflowJson.ToSharedRef(), Writer);

    // 替换提示词占位符
    WorkflowString = WorkflowString.Replace(TEXT("{POSITIVE_PROMPT}"), *Prompt);
    WorkflowString = WorkflowString.Replace(TEXT("{NEGATIVE_PROMPT}"), *NegativePrompt);
    
    // 替换其他自定义参数
    for (const auto& Param : CustomConfig->Parameters)
    {
        FString Placeholder = FString::Printf(TEXT("{%s}"), *Param.Key.ToUpper());
        WorkflowString = WorkflowString.Replace(*Placeholder, *Param.Value);
    }

    // 重新解析以确保JSON有效
    TSharedPtr<FJsonObject> FinalWorkflowJson;
    TSharedRef<TJsonReader<>> FinalReader = TJsonReaderFactory<>::Create(WorkflowString);
    if (FJsonSerializer::Deserialize(FinalReader, FinalWorkflowJson))
    {
        RequestJson->SetObjectField(TEXT("prompt"), FinalWorkflowJson);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize final workflow JSON"));
        return TEXT("{}");
    }

    // 序列化最终的请求JSON
    FString OutputString;
    TSharedRef<TJsonWriter<>> OutputWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), OutputWriter);

    UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Final JSON being sent to ComfyUI:"));
    UE_LOG(LogTemp, Log, TEXT("%s"), *OutputString);

    return OutputString;
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
    UTexture2D* GeneratedTexture = CreateTextureFromImageData(ImageData);
    
    if (GeneratedTexture)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully created texture: %dx%d"), 
               GeneratedTexture->GetSizeX(), GeneratedTexture->GetSizeY());
               
        // 成功完成，重置重试状态
        ResetRetryState();
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

void UComfyUIClient::LoadWorkflowConfigs()
{
    // 获取插件配置目录
    FString PluginDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration");
    FString ConfigPath = PluginDir / TEXT("Config") / TEXT("default_config.json");
    
    // 检查配置文件是否存在
    if (!FPaths::FileExists(ConfigPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Config file not found: %s"), *ConfigPath);
        return;
    }
    
    // 读取配置文件
    FString ConfigContent;
    if (!FFileHelper::LoadFileToString(ConfigContent, *ConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load config file: %s"), *ConfigPath);
        return;
    }
    
    // 解析JSON配置
    TSharedPtr<FJsonObject> ConfigJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ConfigContent);
    if (!FJsonSerializer::Deserialize(Reader, ConfigJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse config JSON"));
        return;
    }
    
    // 读取服务器设置
    if (ConfigJson->HasField(TEXT("server_settings")))
    {
        TSharedPtr<FJsonObject> ServerSettings = ConfigJson->GetObjectField(TEXT("server_settings"));
        if (ServerSettings->HasField(TEXT("default_url")))
        {
            ServerUrl = ServerSettings->GetStringField(TEXT("default_url"));
        }
    }
    
    // 读取工作流配置
    if (ConfigJson->HasField(TEXT("workflows")))
    {
        const TArray<TSharedPtr<FJsonValue>>* WorkflowArray;
        if (ConfigJson->TryGetArrayField(TEXT("workflows"), WorkflowArray))
        {
            for (const auto& WorkflowValue : *WorkflowArray)
            {
                TSharedPtr<FJsonObject> WorkflowObj = WorkflowValue->AsObject();
                if (WorkflowObj.IsValid())
                {
                    FWorkflowConfig CustomConfig;
                    CustomConfig.Name = WorkflowObj->GetStringField(TEXT("name"));
                    CustomConfig.Type = WorkflowObj->GetStringField(TEXT("type"));
                    CustomConfig.Description = WorkflowObj->GetStringField(TEXT("description"));
                    
                    // 读取模板文件路径
                    if (WorkflowObj->HasField(TEXT("template")))
                    {
                        CustomConfig.TemplateFile = WorkflowObj->GetStringField(TEXT("template"));
                    }
                    
                    // 读取参数
                    if (WorkflowObj->HasField(TEXT("parameters")))
                    {
                        TSharedPtr<FJsonObject> ParamsObj = WorkflowObj->GetObjectField(TEXT("parameters"));
                        for (auto& Param : ParamsObj->Values)
                        {
                            FString ValueStr;
                            if (Param.Value->TryGetString(ValueStr))
                            {
                                CustomConfig.Parameters.Add(Param.Key, ValueStr);
                            }
                            else if (Param.Value->Type == EJson::Number)
                            {
                                double NumValue = Param.Value->AsNumber();
                                CustomConfig.Parameters.Add(Param.Key, FString::SanitizeFloat(NumValue));
                            }
                            else if (Param.Value->Type == EJson::Boolean)
                            {
                                bool BoolValue = Param.Value->AsBool();
                                CustomConfig.Parameters.Add(Param.Key, BoolValue ? TEXT("true") : TEXT("false"));
                            }
                        }
                    }
                    
                    CustomWorkflowConfigs.Add(CustomConfig);
                    UE_LOG(LogTemp, Log, TEXT("Loaded custom workflow: %s"), *CustomConfig.Name);
                }
            }
        }
    }
    
    // 加载工作流模板目录中的额外工作流
    FString TemplatesDir = PluginDir / TEXT("Config") / TEXT("Templates");
    if (FPaths::DirectoryExists(TemplatesDir))
    {
        TArray<FString> TemplateFiles;
        IFileManager::Get().FindFiles(TemplateFiles, *(TemplatesDir / TEXT("*.json")), true, false);
        
        for (const FString& TemplateFile : TemplateFiles)
        {
            FString FullPath = TemplatesDir / TemplateFile;
            LoadCustomWorkflowFromFile(FullPath);
        }
    }
}

bool UComfyUIClient::LoadCustomWorkflowFromFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Workflow template file not found: %s"), *FilePath);
        return false;
    }
    
    FString TemplateContent;
    if (!FFileHelper::LoadFileToString(TemplateContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load workflow template: %s"), *FilePath);
        return false;
    }
    
    // 验证JSON格式
    TSharedPtr<FJsonObject> TemplateJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(TemplateContent);
    if (!FJsonSerializer::Deserialize(Reader, TemplateJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid JSON in workflow template: %s"), *FilePath);
        return false;
    }
    
    // 创建工作流配置
    FWorkflowConfig NewWorkflow;
    FString FileName = FPaths::GetBaseFilename(FilePath);
    NewWorkflow.Name = FileName;
    NewWorkflow.Type = TEXT("custom");
    NewWorkflow.Description = FString::Printf(TEXT("Custom workflow loaded from %s"), *FileName);
    NewWorkflow.JsonTemplate = TemplateContent;
    NewWorkflow.TemplateFile = FilePath;
    
    CustomWorkflowConfigs.Add(NewWorkflow);
    UE_LOG(LogTemp, Log, TEXT("Loaded custom workflow template: %s"), *NewWorkflow.Name);
    
    return true;
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

// ========== 工作流验证和管理功能 ==========

bool UComfyUIClient::ValidateWorkflowFile(const FString& FilePath, FString& OutError)
{
    OutError.Empty();
    
    // 检查文件是否存在
    if (!FPaths::FileExists(FilePath))
    {
        OutError = FString::Printf(TEXT("Workflow file not found: %s"), *FilePath);
        return false;
    }
    
    // 读取文件内容
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read workflow file: %s"), *FilePath);
        return false;
    }
    
    // 验证JSON格式和工作流结构
    FWorkflowConfig TempConfig;
    return ValidateWorkflowJson(JsonContent, TempConfig, OutError);
}

bool UComfyUIClient::ImportWorkflowFile(const FString& FilePath, const FString& WorkflowName, FString& OutError)
{
    OutError.Empty();
    
    // 首先验证工作流
    if (!ValidateWorkflowFile(FilePath, OutError))
    {
        return false;
    }
    
    // 读取工作流内容
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        OutError = TEXT("Failed to read workflow file");
        return false;
    }
    
    // 创建工作流配置
    FWorkflowConfig NewWorkflow;
    if (!ValidateWorkflowJson(JsonContent, NewWorkflow, OutError))
    {
        return false;
    }
    
    // 设置工作流信息
    NewWorkflow.Name = SanitizeWorkflowName(WorkflowName);
    NewWorkflow.Type = TEXT("custom");
    NewWorkflow.Description = FString::Printf(TEXT("Custom workflow: %s"), *NewWorkflow.Name);
    NewWorkflow.JsonTemplate = JsonContent;
    NewWorkflow.TemplateFile = FilePath;
    
    // 保存到插件目录
    FString PluginDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration");
    FString TemplatesDir = PluginDir / TEXT("Config") / TEXT("Templates");
    FString TargetFile = TemplatesDir / (NewWorkflow.Name + TEXT(".json"));
    
    UE_LOG(LogTemp, Log, TEXT("ImportWorkflow: Source file: %s"), *FilePath);
    UE_LOG(LogTemp, Log, TEXT("ImportWorkflow: Target directory: %s"), *TemplatesDir);
    UE_LOG(LogTemp, Log, TEXT("ImportWorkflow: Target file: %s"), *TargetFile);
    
    // 确保目录存在
    if (!FPaths::DirectoryExists(TemplatesDir))
    {
        UE_LOG(LogTemp, Log, TEXT("ImportWorkflow: Creating templates directory"));
        if (!IFileManager::Get().MakeDirectory(*TemplatesDir, true))
        {
            OutError = FString::Printf(TEXT("Failed to create templates directory: %s"), *TemplatesDir);
            UE_LOG(LogTemp, Error, TEXT("ImportWorkflow: %s"), *OutError);
            return false;
        }
    }
    
    // 验证源文件存在
    if (!FPaths::FileExists(FilePath))
    {
        OutError = FString::Printf(TEXT("Source workflow file not found: %s"), *FilePath);
        UE_LOG(LogTemp, Error, TEXT("ImportWorkflow: %s"), *OutError);
        return false;
    }
    
    // 使用 FFileHelper 复制文件（更可靠）
    UE_LOG(LogTemp, Log, TEXT("ImportWorkflow: Saving workflow content to target file"));
    if (!FFileHelper::SaveStringToFile(JsonContent, *TargetFile))
    {
        OutError = FString::Printf(TEXT("Failed to save workflow to: %s"), *TargetFile);
        UE_LOG(LogTemp, Error, TEXT("ImportWorkflow: %s"), *OutError);
        return false;
    }
    
    // 验证文件是否成功创建
    if (!FPaths::FileExists(TargetFile))
    {
        OutError = FString::Printf(TEXT("Workflow file was not created at: %s"), *TargetFile);
        UE_LOG(LogTemp, Error, TEXT("ImportWorkflow: %s"), *OutError);
        return false;
    }
    
    // 添加到配置列表
    CustomWorkflowConfigs.Add(NewWorkflow);
    
    UE_LOG(LogTemp, Log, TEXT("Successfully imported workflow: %s"), *NewWorkflow.Name);
    return true;
}

TArray<FString> UComfyUIClient::GetWorkflowParameterNames(const FString& WorkflowName) const
{
    TArray<FString> ParameterNames;
    
    // 查找工作流配置
    const FWorkflowConfig* FoundConfig = nullptr;
    for (const auto& Config : CustomWorkflowConfigs)
    {
        if (Config.Name == WorkflowName)
        {
            FoundConfig = &Config;
            break;
        }
    }
    
    if (FoundConfig)
    {
        for (const auto& ParamDef : FoundConfig->ParameterDefinitions)
        {
            ParameterNames.Add(ParamDef.Name);
        }
    }
    
    return ParameterNames;
}

void UComfyUIClient::TestWorkflowConnection()
{
    // 使用 NetworkManager 测试服务器连接
    if (NetworkManager)
    {
        NetworkManager->TestServerConnection(ServerUrl, 
            [this](bool bSuccess, const FString& ErrorMessage)
            {
                if (bSuccess)
                {
                    UE_LOG(LogTemp, Log, TEXT("ComfyUI server connection successful"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to connect to ComfyUI server: %s"), *ErrorMessage);
                }
            });
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NetworkManager not initialized"));
    }
}

void UComfyUIClient::TestServerConnection(const FOnConnectionTested& OnComplete)
{
    // 使用 NetworkManager 测试服务器连接
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

bool UComfyUIClient::ValidateWorkflowJson(const FString& JsonContent, FWorkflowConfig& OutConfig, FString& OutError)
{
    OutError.Empty();
    
    // 解析JSON
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, WorkflowJson))
    {
        OutError = TEXT("Invalid JSON format");
        return false;
    }
    
    if (!WorkflowJson.IsValid())
    {
        OutError = TEXT("Empty or invalid workflow JSON");
        return false;
    }
    
    // 检查是否是有效的ComfyUI工作流格式
    bool bHasValidNodes = false;
    for (const auto& NodePair : WorkflowJson->Values)
    {
        if (NodePair.Value->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodePair.Value->AsObject();
            if (NodeObj->HasField(TEXT("class_type")))
            {
                bHasValidNodes = true;
                break;
            }
        }
    }
    
    if (!bHasValidNodes)
    {
        OutError = TEXT("No valid ComfyUI nodes found. Make sure this is a ComfyUI workflow export.");
        return false;
    }
    
    // 分析工作流节点
    if (!AnalyzeWorkflowNodes(WorkflowJson, OutConfig))
    {
        OutError = TEXT("Failed to analyze workflow structure");
        return false;
    }
    
    // 查找输入和输出节点
    if (!FindWorkflowInputs(WorkflowJson, OutConfig.RequiredInputs))
    {
        UE_LOG(LogTemp, Warning, TEXT("No input parameters found in workflow"));
    }
    
    if (!FindWorkflowOutputs(WorkflowJson, OutConfig.OutputNodes))
    {
        OutError = TEXT("No output nodes found. Workflow must have at least one SaveImage or PreviewImage node.");
        return false;
    }
    
    OutConfig.bIsValid = true;
    OutConfig.JsonTemplate = JsonContent;
    
    UE_LOG(LogTemp, Log, TEXT("Workflow validation successful. Found %d inputs and %d outputs"), 
           OutConfig.RequiredInputs.Num(), OutConfig.OutputNodes.Num());
    
    return true;
}

bool UComfyUIClient::AnalyzeWorkflowNodes(TSharedPtr<FJsonObject> WorkflowJson, FWorkflowConfig& OutConfig)
{
    if (!WorkflowJson.IsValid())
    {
        return false;
    }
    
    OutConfig.ParameterDefinitions.Empty();
    
    // 遍历所有节点
    for (const auto& NodePair : WorkflowJson->Values)
    {
        FString NodeId = NodePair.Key;
        TSharedPtr<FJsonObject> NodeObj = NodePair.Value->AsObject();
        
        if (!NodeObj.IsValid() || !NodeObj->HasField(TEXT("class_type")))
        {
            continue;
        }
        
        FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
        
        // 检查常见的输入节点类型
        if (ClassType == TEXT("CLIPTextEncode") || ClassType == TEXT("PromptText"))
        {
            // 文本输入节点
            FWorkflowConfig::FParameterDef PromptParam;
            PromptParam.Name = FString::Printf(TEXT("prompt_%s"), *NodeId);
            PromptParam.Type = TEXT("text");
            PromptParam.DefaultValue = TEXT("");
            PromptParam.Description = FString::Printf(TEXT("Text prompt for node %s"), *NodeId);
            OutConfig.ParameterDefinitions.Add(PromptParam);
        }
        else if (ClassType == TEXT("KSampler") || ClassType == TEXT("SamplerCustom"))
        {
            // 采样器节点 - 添加常见参数
            if (NodeObj->HasField(TEXT("inputs")))
            {
                TSharedPtr<FJsonObject> InputsObj = NodeObj->GetObjectField(TEXT("inputs"));
                
                // Steps参数
                if (InputsObj->HasField(TEXT("steps")))
                {
                    FWorkflowConfig::FParameterDef StepsParam;
                    StepsParam.Name = FString::Printf(TEXT("steps_%s"), *NodeId);
                    StepsParam.Type = TEXT("number");
                    StepsParam.DefaultValue = TEXT("20");
                    StepsParam.Description = TEXT("Number of sampling steps");
                    StepsParam.MinValue = 1.0f;
                    StepsParam.MaxValue = 100.0f;
                    OutConfig.ParameterDefinitions.Add(StepsParam);
                }
                
                // CFG参数
                if (InputsObj->HasField(TEXT("cfg")))
                {
                    FWorkflowConfig::FParameterDef CfgParam;
                    CfgParam.Name = FString::Printf(TEXT("cfg_%s"), *NodeId);
                    CfgParam.Type = TEXT("number");
                    CfgParam.DefaultValue = TEXT("7.0");
                    CfgParam.Description = TEXT("CFG Scale");
                    CfgParam.MinValue = 1.0f;
                    CfgParam.MaxValue = 30.0f;
                    OutConfig.ParameterDefinitions.Add(CfgParam);
                }
            }
        }
    }
    
    return true;
}

bool UComfyUIClient::FindWorkflowInputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutInputs)
{
    OutInputs.Empty();
    
    if (!WorkflowJson.IsValid())
    {
        return false;
    }
    
    // 查找包含文本输入的节点
    TArray<FString> TextInputNodes = {
        TEXT("CLIPTextEncode"),
        TEXT("PromptText"),
        TEXT("StringConstant"),
        TEXT("Text")
    };
    
    for (const auto& NodePair : WorkflowJson->Values)
    {
        TSharedPtr<FJsonObject> NodeObj = NodePair.Value->AsObject();
        if (!NodeObj.IsValid())
        {
            continue;
        }
        
        FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
        if (TextInputNodes.Contains(ClassType))
        {
            OutInputs.Add(NodePair.Key);
        }
    }
    
    return OutInputs.Num() > 0;
}

bool UComfyUIClient::FindWorkflowOutputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutOutputs)
{
    OutOutputs.Empty();
    
    if (!WorkflowJson.IsValid())
    {
        return false;
    }
    
    // 查找输出节点
    TArray<FString> OutputNodeTypes = {
        TEXT("SaveImage"),
        TEXT("PreviewImage"),
        TEXT("SaveImageWebsocket"),
        TEXT("ImageOutput")
    };
    
    for (const auto& NodePair : WorkflowJson->Values)
    {
        TSharedPtr<FJsonObject> NodeObj = NodePair.Value->AsObject();
        if (!NodeObj.IsValid())
        {
            continue;
        }
        
        FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
        if (OutputNodeTypes.Contains(ClassType))
        {
            OutOutputs.Add(NodePair.Key);
        }
    }
    
    return OutOutputs.Num() > 0;
}

FString UComfyUIClient::SanitizeWorkflowName(const FString& Name)
{
    FString SanitizedName = Name;
    
    // 移除或替换不安全的字符
    SanitizedName = SanitizedName.Replace(TEXT(" "), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("/"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("\\"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT(":"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("*"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("?"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("\""), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("<"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT(">"), TEXT("_"));
    SanitizedName = SanitizedName.Replace(TEXT("|"), TEXT("_"));
    
    // 确保不为空
    if (SanitizedName.IsEmpty())
    {
        SanitizedName = TEXT("CustomWorkflow");
    }
    
    return SanitizedName;
}

// ========== 错误处理和重试机制 ==========

FComfyUIError UComfyUIClient::AnalyzeHttpError(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // 调用 NetworkManager 的 AnalyzeHttpError
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
