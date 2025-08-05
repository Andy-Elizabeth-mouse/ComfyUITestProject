#include "Network/ComfyUINetworkManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/World.h"
#include "TimerManager.h"

UComfyUINetworkManager::UComfyUINetworkManager()
{
    HttpModule = &FHttpModule::Get();
}

void UComfyUINetworkManager::SendRequest(const FString& Url, const FString& Payload, TFunction<void(const FString& Response, bool bSuccess)> Callback)
{
    // 构建HTTP请求
    if (!HttpModule)
    {
        HttpModule = &FHttpModule::Get();
    }
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Payload);

    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
                Callback(Resp->GetContentAsString(), true);
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager: HTTP Request failed - %s"), *Error.ErrorMessage);
                Callback(TEXT(""), false);
            }
        }
    );
    Request->ProcessRequest();
}

void UComfyUINetworkManager::SendGetRequest(const FString& Url, TFunction<void(const FString& Response, bool bSuccess)> Callback, float TimeoutSeconds)
{
    if (!HttpModule) HttpModule = &FHttpModule::Get();
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(TimeoutSeconds);

    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            // 分析HTTP错误
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
            {
                // 请求成功
                Callback(Resp->GetContentAsString(), true);
            }
            else
            {
                // 请求失败，记录错误
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager GET Request failed: %s"), *Error.ErrorMessage);
                Callback(TEXT(""), false);
            }
        }
    );
    Request->ProcessRequest();
}

void UComfyUINetworkManager::DownloadImage(const FString& Url, TFunction<void(const TArray<uint8>& ImageData, bool bSuccess)> Callback)
{
    if (!HttpModule) HttpModule = &FHttpModule::Get();
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(30.0f);

    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
            {
                TArray<uint8> ImageData = Resp->GetContent();
                Callback(ImageData, true);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager Image Download failed: %s"), *Error.ErrorMessage);
                TArray<uint8> EmptyData;
                Callback(EmptyData, false);
            }
        }
    );
    Request->ProcessRequest();
}
#pragma optimize("", off)
void UComfyUINetworkManager::UploadImage(const FString& ServerUrl, const TArray<uint8>& ImageData, const FString& FileName, TFunction<void(const FString& UploadedImageName, bool bSuccess)> Callback)
{
    if (!HttpModule) HttpModule = &FHttpModule::Get();
    
    // 构建上传URL
    FString UploadUrl = ServerUrl;
    if (!UploadUrl.EndsWith(TEXT("/"))) UploadUrl += TEXT("/"); 
    UploadUrl += TEXT("upload/image");
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(UploadUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetTimeout(60.0f); // 图片上传需要更长时间
    
    // 创建multipart/form-data格式的内容
    FString Boundary = FString::Printf(TEXT("----UnrealEngine4FormBoundary%d"), FMath::Rand());
    FString ContentType = FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary);
    Request->SetHeader(TEXT("Content-Type"), ContentType);
    
    // 构建multipart内容
    TArray<uint8> FormData;
    
    // 添加文件字段
    FString FileHeader = FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"image\"; filename=\"%s\"\r\nContent-Type: image/png\r\n\r\n"), *Boundary, *FileName);
    FormData.Append((uint8*)TCHAR_TO_UTF8(*FileHeader), FileHeader.Len());
    FormData.Append(ImageData);
    
    // 添加结束边界
    FString EndBoundary = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary);
    FormData.Append((uint8*)TCHAR_TO_UTF8(*EndBoundary), EndBoundary.Len());
    
    Request->SetContent(FormData);

    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            // 分析HTTP错误
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
            {
                // 解析响应JSON获取上传的图片名称
                FString ResponseContent = Resp->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
                
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString UploadedImageName;
                    if (JsonObject->TryGetStringField(TEXT("name"), UploadedImageName))
                    {
                        UE_LOG(LogTemp, Log, TEXT("Image uploaded successfully: %s"), *UploadedImageName);
                        Callback(UploadedImageName, true);
                        return;
                    }
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Image uploaded but failed to parse response: %s"), *ResponseContent);
                Callback(TEXT(""), false);
            }
            else
            {
                // 请求失败，记录错误
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager Image Upload failed: %s"), *Error.ErrorMessage);
                Callback(TEXT(""), false);
            }
        }
    );
    Request->ProcessRequest();
}

void UComfyUINetworkManager::UploadModel(const FString& ServerUrl, const TArray<uint8>& ModelData, const FString& FileName, TFunction<void(const FString& UploadedModelName, bool bSuccess)> Callback)
{
    if (!HttpModule) HttpModule = &FHttpModule::Get();
    
    // 构建上传URL
    FString UploadUrl = ServerUrl;
    if (!UploadUrl.EndsWith(TEXT("/"))) UploadUrl += TEXT("/");
    UploadUrl += TEXT("upload/model");
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(UploadUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetTimeout(120.0f);
    
    // 创建multipart/form-data格式的内容
    FString Boundary = FString::Printf(TEXT("----UnrealEngine4FormBoundary%d"), FMath::Rand());
    FString ContentType = FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary);
    Request->SetHeader(TEXT("Content-Type"), ContentType);
    
    // 构建multipart内容
    TArray<uint8> FormData;
    
    // 根据文件扩展名确定Content-Type
    FString ModelContentType = TEXT("application/octet-stream");
    if (FileName.EndsWith(TEXT(".glb")))
        ModelContentType = TEXT("model/gltf-binary");
    else if (FileName.EndsWith(TEXT(".gltf")))
        ModelContentType = TEXT("model/gltf+json");
    else if (FileName.EndsWith(TEXT(".obj")))
        ModelContentType = TEXT("text/plain");
    else if (FileName.EndsWith(TEXT(".ply")))
        ModelContentType = TEXT("application/octet-stream");
    
    // 添加文件字段
    FString FileHeader = FString::Printf(TEXT("--%s\r\nContent-Disposition: form-data; name=\"model\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n"), 
                                        *Boundary, *FileName, *ModelContentType);
    FormData.Append((uint8*)TCHAR_TO_UTF8(*FileHeader), FileHeader.Len());
    FormData.Append(ModelData);
    
    // 添加结束边界
    FString EndBoundary = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary);
    FormData.Append((uint8*)TCHAR_TO_UTF8(*EndBoundary), EndBoundary.Len());
    
    Request->SetContent(FormData);

    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            // 分析HTTP错误
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
            {
                // 解析响应JSON获取上传的模型名称
                FString ResponseContent = Resp->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
                
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString UploadedModelName;
                    if (JsonObject->TryGetStringField(TEXT("name"), UploadedModelName))
                    {
                        UE_LOG(LogTemp, Log, TEXT("Model uploaded successfully: %s"), *UploadedModelName);
                        Callback(UploadedModelName, true);
                        return;
                    }
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Model uploaded but failed to parse response: %s"), *ResponseContent);
                Callback(TEXT(""), false);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager Model Upload failed: %s"), *Error.ErrorMessage);
                Callback(TEXT(""), false);
            }
        }
    );
    Request->ProcessRequest();
}

void UComfyUINetworkManager::DownloadModel(const FString& Url, TFunction<void(const TArray<uint8>& ModelData, bool bSuccess)> Callback)
{
    if (!HttpModule) HttpModule = &FHttpModule::Get();
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetTimeout(120.0f);
    
    Request->OnProcessRequestComplete().BindLambda(
        [this, Callback](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            // 分析HTTP错误
            FComfyUIError Error = AnalyzeHttpError(Req, Resp, bSuccess);
            
            if (Error.ErrorType == EComfyUIErrorType::None)
            {
                // 获取模型数据
                TArray<uint8> ModelData = Resp->GetContent();
                
                if (ModelData.Num() > 0)
                {
                    UE_LOG(LogTemp, Log, TEXT("Model downloaded successfully: %d bytes"), ModelData.Num());
                    Callback(ModelData, true);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Model download completed but no data received"));
                    Callback(TArray<uint8>(), false);
                }
            }
            else
            {
                // 请求失败，记录错误
                UE_LOG(LogTemp, Warning, TEXT("NetworkManager Model Download failed: %s"), *Error.ErrorMessage);
                Callback(TArray<uint8>(), false);
            }
        }
    );
    Request->ProcessRequest();
}

#pragma optimize("", on)
void UComfyUINetworkManager::PollQueueStatus(const FString& ServerUrl, const FString& PromptId, TFunction<void(const FString& Response, bool bSuccess)> Callback)
{
    FString StatusUrl = ServerUrl;
    if (!StatusUrl.EndsWith(TEXT("/")))
        StatusUrl += TEXT("/");
    StatusUrl += TEXT("history/") + PromptId;
    
    SendGetRequest(StatusUrl, Callback, 10.0f);
}

void UComfyUINetworkManager::TestServerConnection(const FString& ServerUrl, TFunction<void(bool bSuccess, const FString& ErrorMessage)> Callback)
{
    if (ServerUrl.IsEmpty())
    {
        Callback(false, TEXT("服务器URL为空"));
        return;
    }

    // 首先尝试最基本的连接测试
    FString TestUrl = ServerUrl;
    if (!TestUrl.EndsWith(TEXT("/")))
    {
        TestUrl += TEXT("/");
    }
    
    SendGetRequest(TestUrl, [this, ServerUrl, Callback](const FString& Response, bool bSuccess)
    {
        if (bSuccess)
        {
            // 检查响应内容是否表明这是ComfyUI服务器
            if (Response.Contains(TEXT("ComfyUI")) || Response.Contains(TEXT("comfyui")))
            {
                Callback(true, TEXT(""));
                return;
            }
        }
        
        // 如果根路径不行，试试/system_stats端点（ComfyUI的标准API端点）
        FString StatsUrl = ServerUrl;
        if (!StatsUrl.EndsWith(TEXT("/")))
        {
            StatsUrl += TEXT("/");
        }
        StatsUrl += TEXT("system_stats");
        
        SendGetRequest(StatsUrl, [Callback](const FString& StatsResponse, bool bStatsSuccess)
        {
            if (bStatsSuccess && !StatsResponse.IsEmpty())
            {
                // 检查是否包含ComfyUI系统信息的特征
                if (StatsResponse.Contains(TEXT("system")) || StatsResponse.Contains(TEXT("memory")))
                {
                    Callback(true, TEXT(""));
                }
                else
                {
                    Callback(false, TEXT("服务器响应但不是ComfyUI服务器"));
                }
            }
            else
            {
                Callback(false, TEXT("ComfyUI服务器未响应或不可用"));
            }
        }, 5.0f);
    }, 5.0f);
}

FComfyUIError UComfyUINetworkManager::AnalyzeHttpError(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        if (!Request.IsValid())
        {
            return FComfyUIError(EComfyUIErrorType::UnknownError, 
                               TEXT("HTTP请求对象无效"), 0, 
                               TEXT("检查网络连接并重试"), true);
        }

        // 检查是否超时或连接错误
        if (Request->GetStatus() == EHttpRequestStatus::Failed)
        {
            EHttpFailureReason FailureReason = Request->GetFailureReason();
            if (FailureReason == EHttpFailureReason::ConnectionError)
            {
                return FComfyUIError(EComfyUIErrorType::ConnectionFailed, 
                                   TEXT("连接到ComfyUI服务器失败"), 0, 
                                   TEXT("请检查服务器是否运行，URL是否正确"), true);
            }
            else if (FailureReason == EHttpFailureReason::TimedOut)
            {
                return FComfyUIError(EComfyUIErrorType::Timeout, 
                                   TEXT("请求超时"), 0, 
                                   TEXT("服务器响应时间过长，可以尝试增加超时时间或稍后重试"), true);
            }
        }

        return FComfyUIError(EComfyUIErrorType::ConnectionFailed, 
                           TEXT("网络请求失败"), 0, 
                           TEXT("检查网络连接状态和服务器运行状态"), true);
    }

    if (!Response.IsValid())
    {
        return FComfyUIError(EComfyUIErrorType::ServerError, 
                           TEXT("服务器响应无效"), 0, 
                           TEXT("服务器可能出现内部错误，请稍后重试"), true);
    }

    int32 StatusCode = Response->GetResponseCode();
    FString ResponseContent = Response->GetContentAsString();

    // 分析HTTP状态码
    switch (StatusCode)
    {
        case 200: // 成功
            return FComfyUIError(EComfyUIErrorType::None, TEXT(""));
            
        case 400: // 错误请求
            return FComfyUIError(EComfyUIErrorType::InvalidWorkflow, 
                               FString::Printf(TEXT("请求格式错误 (400): %s"), *ResponseContent), StatusCode,
                               TEXT("检查工作流JSON格式是否正确"), false);
            
        case 401: // 未授权
            return FComfyUIError(EComfyUIErrorType::AuthenticationFailed, 
                               TEXT("身份验证失败 (401)"), StatusCode,
                               TEXT("检查API密钥或登录凭据"), false);
            
        case 404: // 未找到
            return FComfyUIError(EComfyUIErrorType::ServerError, 
                               TEXT("请求的资源未找到 (404)"), StatusCode,
                               TEXT("检查API端点URL是否正确"), false);
            
        case 429: // 请求过多
            return FComfyUIError(EComfyUIErrorType::ServerUnavailable, 
                               TEXT("请求过于频繁 (429)"), StatusCode,
                               TEXT("请稍后再试，或联系服务器管理员"), true);
            
        case 500: // 服务器内部错误
            return FComfyUIError(EComfyUIErrorType::ServerError, 
                               FString::Printf(TEXT("服务器内部错误 (500): %s"), *ResponseContent), StatusCode,
                               TEXT("服务器出现内部错误，请稍后重试"), true);
            
        case 502: // 错误网关
        case 503: // 服务不可用
        case 504: // 网关超时
            return FComfyUIError(EComfyUIErrorType::ServerUnavailable, 
                               FString::Printf(TEXT("服务器暂时不可用 (%d)"), StatusCode), StatusCode,
                               TEXT("服务器暂时不可用，请稍后重试"), true);
                               
        default:
            if (StatusCode >= 400 && StatusCode < 500)
            {
                return FComfyUIError(EComfyUIErrorType::InvalidWorkflow, 
                                   FString::Printf(TEXT("客户端错误 (%d): %s"), StatusCode, *ResponseContent), StatusCode,
                                   TEXT("检查请求参数和格式"), false);
            }
            else if (StatusCode >= 500)
            {
                return FComfyUIError(EComfyUIErrorType::ServerError, 
                                   FString::Printf(TEXT("服务器错误 (%d): %s"), StatusCode, *ResponseContent), StatusCode,
                                   TEXT("服务器出现错误，请稍后重试"), true);
            }
            
            return FComfyUIError(EComfyUIErrorType::UnknownError, 
                               FString::Printf(TEXT("未知HTTP状态码: %d"), StatusCode), StatusCode,
                               TEXT("请联系技术支持"), false);
    }
}

bool UComfyUINetworkManager::ShouldRetryRequest(const FComfyUIError& Error, int32 CurrentRetryCount, int32 MaxRetryAttempts)
{
    return Error.bCanRetry && CurrentRetryCount < MaxRetryAttempts;
}

FString UComfyUINetworkManager::GetUserFriendlyErrorMessage(const FComfyUIError& Error)
{
    FString UserMessage;
    
    switch (Error.ErrorType)
    {
        case EComfyUIErrorType::ConnectionFailed:
            UserMessage = TEXT("无法连接到ComfyUI服务器。请确保服务器正在运行并检查网络连接。");
            break;
            
        case EComfyUIErrorType::ServerError:
            UserMessage = TEXT("ComfyUI服务器出现错误。请稍后重试或联系管理员。");
            break;
            
        case EComfyUIErrorType::InvalidWorkflow:
            UserMessage = TEXT("工作流配置有误。请检查工作流文件是否正确。");
            break;
            
        case EComfyUIErrorType::Timeout:
            UserMessage = TEXT("请求超时。服务器可能正在处理其他任务，请稍后重试。");
            break;
            
        case EComfyUIErrorType::ServerUnavailable:
            UserMessage = TEXT("服务器暂时不可用。请稍后重试。");
            break;
            
        case EComfyUIErrorType::AuthenticationFailed:
            UserMessage = TEXT("身份验证失败。请检查API密钥或登录信息。");
            break;
            
        case EComfyUIErrorType::InsufficientResources:
            UserMessage = TEXT("服务器资源不足。请稍后重试或减少任务复杂度。");
            break;
            
        case EComfyUIErrorType::ImageDownloadFailed:
            UserMessage = TEXT("图像下载失败。生成可能成功但图像传输出现问题。");
            break;
            
        default:
            UserMessage = FString::Printf(TEXT("发生未知错误: %s"), *Error.ErrorMessage);
            break;
    }
    
    if (!Error.SuggestedSolution.IsEmpty())
    {
        UserMessage += FString::Printf(TEXT("\n\n建议解决方案: %s"), *Error.SuggestedSolution);
    }
    
    return UserMessage;
}
