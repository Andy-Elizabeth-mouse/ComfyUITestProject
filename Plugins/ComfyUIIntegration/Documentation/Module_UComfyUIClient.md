# UComfyUIClient - HTTP客户端模块开发文档

## 概述

`UComfyUIClient`是ComfyUI Integration插件的网络通信核心，负责与ComfyUI服务器的HTTP通信。该类继承自`UObject`，支持Blueprint调用，提供了完整的ComfyUI API封装。

## 文件位置

- **头文件**: `Source/ComfyUIIntegration/Public/ComfyUIClient.h`
- **实现文件**: `Source/ComfyUIIntegration/Private/ComfyUIClient.cpp`

## 类结构

```cpp
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUIClient : public UObject
{
    GENERATED_BODY()

public:
    // 构造函数
    UComfyUIClient();

    // 配置API
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetServerUrl(const FString& Url);

    // 核心功能API
    void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                      SComfyUIWidget::EWorkflowType WorkflowType, 
                      const FOnImageGenerated& OnComplete);

    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void CheckServerStatus(const FOnImageGeneratedDynamic& OnComplete);

    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void GetAvailableWorkflows();

private:
    // 成员变量
    FString ServerUrl;
    FHttpModule* HttpModule;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;
    FOnImageGenerated OnImageGeneratedCallback;
    FString CurrentPromptId;
    FTimerHandle StatusPollTimer;
    TMap<SComfyUIWidget::EWorkflowType, FWorkflowConfig> WorkflowConfigs;

    // 私有方法
    void InitializeWorkflowConfigs();
    FString BuildWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                             SComfyUIWidget::EWorkflowType WorkflowType);
    void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnImageDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnQueueStatusChecked(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);
    void PollGenerationStatus(const FString& PromptId);
};
```

## 核心功能

### 1. 构造函数和初始化

#### UComfyUIClient()

```cpp
UComfyUIClient::UComfyUIClient()
{
    HttpModule = &FHttpModule::Get();
    ServerUrl = TEXT("http://127.0.0.1:8188");
    InitializeWorkflowConfigs();
}
```

**功能描述**：
- 初始化HTTP模块引用
- 设置默认服务器URL
- 初始化工作流配置

**默认配置**：
- 服务器地址：`http://127.0.0.1:8188`
- 支持的工作流类型：TextToImage, ImageToImage, TextTo3D, ImageTo3D, TextureGeneration

### 2. 配置管理

#### SetServerUrl()

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void SetServerUrl(const FString& Url);
```

**功能描述**：
- 设置ComfyUI服务器地址
- 支持Blueprint调用

**参数**：
- `Url`：服务器地址（例如："http://192.168.1.100:8188"）

**使用示例**：
```cpp
// C++调用
UComfyUIClient* Client = NewObject<UComfyUIClient>();
Client->SetServerUrl(TEXT("http://192.168.1.100:8188"));

// Blueprint调用
// 在Blueprint中直接设置Server URL节点
```

### 3. 核心生成功能

#### GenerateImage()

```cpp
void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                  SComfyUIWidget::EWorkflowType WorkflowType, 
                  const FOnImageGenerated& OnComplete);
```

**功能描述**：
- 执行AI图像生成任务
- 异步操作，通过回调返回结果

**参数**：
- `Prompt`：正面提示词（描述想要生成的内容）
- `NegativePrompt`：负面提示词（描述不想要的内容）
- `WorkflowType`：工作流类型枚举
- `OnComplete`：完成回调委托

**工作流程**：
1. 根据工作流类型构建JSON请求
2. 发送HTTP POST请求到ComfyUI服务器
3. 解析响应获取任务ID
4. 开始轮询任务状态
5. 任务完成后下载生成的图像
6. 将图像数据转换为UTexture2D
7. 调用完成回调

**使用示例**：
```cpp
// 设置完成回调
FOnImageGenerated OnComplete;
OnComplete.BindUObject(this, &AMyActor::OnImageGenerated);

// 生成图像
Client->GenerateImage(
    TEXT("a beautiful landscape with mountains and lakes, highly detailed, 4k"),
    TEXT("blurry, low quality, distorted, ugly"),
    SComfyUIWidget::EWorkflowType::TextToImage,
    OnComplete
);

// 回调函数
void AMyActor::OnImageGenerated(UTexture2D* GeneratedTexture)
{
    if (GeneratedTexture)
    {
        UE_LOG(LogTemp, Log, TEXT("图像生成成功"));
        // 使用生成的纹理
        MyMaterialInstance->SetTextureParameterValue(TEXT("BaseTexture"), GeneratedTexture);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("图像生成失败"));
    }
}
```

### 4. 服务器状态检查

#### CheckServerStatus()

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void CheckServerStatus(const FOnImageGeneratedDynamic& OnComplete);
```

**功能描述**：
- 检查ComfyUI服务器是否在线
- 验证服务器响应和API可用性
- 支持Blueprint调用

**参数**：
- `OnComplete`：状态检查完成回调

**检查内容**：
- 服务器连接状态
- API端点可用性
- 响应时间

**使用示例**：
```cpp
// C++调用
FOnImageGeneratedDynamic OnStatusCheck;
OnStatusCheck.BindUFunction(this, TEXT("OnServerStatusChecked"));
Client->CheckServerStatus(OnStatusCheck);

// Blueprint调用
// 使用Check Server Status节点
```

### 5. 工作流管理

#### GetAvailableWorkflows()

```cpp
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void GetAvailableWorkflows();
```

**功能描述**：
- 获取服务器可用的工作流列表
- 动态发现新的工作流模板

**用途**：
- 界面动态更新工作流选项
- 验证工作流可用性
- 获取工作流参数信息

## 工作流配置系统

### 工作流配置结构

```cpp
struct FWorkflowConfig
{
    FString Name;                           // 工作流名称
    FString JsonTemplate;                   // JSON模板
    TMap<FString, FString> Parameters;      // 参数映射
};
```

### InitializeWorkflowConfigs()

```cpp
void InitializeWorkflowConfigs();
```

**功能描述**：
- 初始化预定义的工作流配置
- 设置各种AI生成工作流的JSON模板

**支持的工作流类型**：

#### 1. TextToImage（文生图）
```json
{
    "3": {
        "inputs": {
            "seed": 156680208700286,
            "steps": 20,
            "cfg": 8,
            "sampler_name": "euler",
            "scheduler": "normal",
            "denoise": 1,
            "model": ["4", 0],
            "positive": ["6", 0],
            "negative": ["7", 0],
            "latent_image": ["5", 0]
        },
        "class_type": "KSampler"
    },
    "4": {
        "inputs": {
            "ckpt_name": "v1-5-pruned-emaonly-fp16.safetensors"
        },
        "class_type": "CheckpointLoaderSimple"
    },
    "5": {
        "inputs": {
            "width": 512,
            "height": 512,
            "batch_size": 1
        },
        "class_type": "EmptyLatentImage"
    },
    "6": {
        "inputs": {
            "text": "{POSITIVE_PROMPT}",
            "clip": ["4", 1]
        },
        "class_type": "CLIPTextEncode"
    },
    "7": {
        "inputs": {
            "text": "{NEGATIVE_PROMPT}",
            "clip": ["4", 1]
        },
        "class_type": "CLIPTextEncode"
    },
    "8": {
        "inputs": {
            "samples": ["3", 0],
            "vae": ["4", 2]
        },
        "class_type": "VAEDecode"
    },
    "9": {
        "inputs": {
            "filename_prefix": "ComfyUI",
            "images": ["8", 0]
        },
        "class_type": "SaveImage"
    }
}
```

#### 2. ImageToImage（图生图）
- 支持输入图像作为参考
- 可调整变化强度
- 支持局部修改

#### 3. TextTo3D（文生3D）
- 实验性功能
- 生成3D模型
- 支持多种3D格式

#### 4. ImageTo3D（图生3D）
- 从2D图像生成3D模型
- 支持深度估计
- 纹理映射

#### 5. TextureGeneration（纹理生成）
- 专门用于游戏纹理生成
- 支持无缝贴图
- 可配置分辨率

### BuildWorkflowJson()

```cpp
FString BuildWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                         SComfyUIWidget::EWorkflowType WorkflowType);
```

**功能描述**：
- 根据用户输入构建ComfyUI工作流JSON
- 替换模板中的占位符
- 返回可直接提交的JSON字符串

**参数**：
- `Prompt`：正面提示词
- `NegativePrompt`：负面提示词
- `WorkflowType`：工作流类型

**返回值**：
- 完整的ComfyUI工作流JSON字符串

**模板占位符**：
- `{POSITIVE_PROMPT}`：正面提示词
- `{NEGATIVE_PROMPT}`：负面提示词
- `{IMAGE_WIDTH}`：图像宽度
- `{IMAGE_HEIGHT}`：图像高度
- `{STEPS}`：生成步数
- `{CFG_SCALE}`：CFG缩放

## HTTP请求处理

### 1. 提交工作流请求

#### OnPromptSubmitted()

```cpp
void OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```

**功能描述**：
- 处理工作流提交的HTTP响应
- 解析服务器返回的任务ID
- 开始状态轮询

**响应处理**：
```cpp
void UComfyUIClient::OnPromptSubmitted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        FString ResponseString = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
        
        if (FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            FString PromptId = JsonObject->GetStringField(TEXT("prompt_id"));
            CurrentPromptId = PromptId;
            
            // 开始轮询状态
            PollGenerationStatus(PromptId);
        }
    }
    else
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("工作流提交失败"));
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
    }
}
```

### 2. 状态轮询

#### PollGenerationStatus()

```cpp
void PollGenerationStatus(const FString& PromptId);
```

**功能描述**：
- 定期查询生成任务状态
- 使用定时器实现非阻塞轮询
- 检测任务完成并触发图像下载

**轮询逻辑**：
```cpp
void UComfyUIClient::PollGenerationStatus(const FString& PromptId)
{
    // 创建状态查询请求
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();
    Request->SetURL(ServerUrl + TEXT("/history/") + PromptId);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // 设置响应处理
    Request->OnProcessRequestComplete().BindUObject(this, &UComfyUIClient::OnQueueStatusChecked);
    
    // 发送请求
    Request->ProcessRequest();
}
```

#### OnQueueStatusChecked()

```cpp
void OnQueueStatusChecked(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```

**功能描述**：
- 处理状态查询响应
- 判断任务是否完成
- 触发图像下载或继续轮询

**状态判断逻辑**：
- **运行中**：继续轮询
- **已完成**：开始下载图像
- **失败**：报告错误

### 3. 图像下载

#### OnImageDownloaded()

```cpp
void OnImageDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
```

**功能描述**：
- 处理图像下载响应
- 将图像数据转换为UTexture2D
- 调用完成回调

**实现细节**：
```cpp
void UComfyUIClient::OnImageDownloaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        TArray<uint8> ImageData = Response->GetContent();
        UTexture2D* GeneratedTexture = CreateTextureFromImageData(ImageData);
        
        if (GeneratedTexture)
        {
            UE_LOG(LogComfyUIIntegration, Log, TEXT("图像生成成功"));
            OnImageGeneratedCallback.ExecuteIfBound(GeneratedTexture);
        }
        else
        {
            UE_LOG(LogComfyUIIntegration, Error, TEXT("图像数据转换失败"));
            OnImageGeneratedCallback.ExecuteIfBound(nullptr);
        }
    }
    else
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("图像下载失败"));
        OnImageGeneratedCallback.ExecuteIfBound(nullptr);
    }
}
```

## 图像处理

### CreateTextureFromImageData()

```cpp
UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData);
```

**功能描述**：
- 将原始图像数据转换为UE5可用的纹理
- 支持多种图像格式（PNG、JPEG、WebP等）
- 自动设置纹理参数

**实现步骤**：
1. 检测图像格式
2. 使用IImageWrapper解码图像
3. 创建UTexture2D对象
4. 设置纹理数据和参数
5. 更新纹理资源

**关键代码**：
```cpp
UTexture2D* UComfyUIClient::CreateTextureFromImageData(const TArray<uint8>& ImageData)
{
    // 获取图像包装器模块
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    
    // 检测图像格式
    EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(ImageData.GetData(), ImageData.Num());
    
    if (ImageFormat == EImageFormat::Invalid)
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("无效的图像格式"));
        return nullptr;
    }
    
    // 创建图像包装器
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
    
    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("无法创建图像包装器"));
        return nullptr;
    }
    
    // 设置图像数据
    if (!ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("图像数据设置失败"));
        return nullptr;
    }
    
    // 获取原始图像数据
    TArray<uint8> UncompressedBGRA;
    if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("图像解压失败"));
        return nullptr;
    }
    
    // 创建纹理
    UTexture2D* Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
    
    if (!Texture)
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("纹理创建失败"));
        return nullptr;
    }
    
    // 设置纹理数据
    void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_WRITE_ONLY);
    FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
    
    // 更新纹理
    Texture->UpdateResource();
    
    return Texture;
}
```

## 委托和回调

### 委托定义

```cpp
// 标准委托（C++使用）
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*);

// 动态委托（Blueprint使用）
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnImageGeneratedDynamic, UTexture2D*, GeneratedImage);
```

### 回调使用示例

```cpp
// 绑定成员函数
FOnImageGenerated OnComplete;
OnComplete.BindUObject(this, &AMyActor::OnImageGenerated);

// 绑定Lambda表达式
OnComplete.BindLambda([this](UTexture2D* GeneratedTexture)
{
    if (GeneratedTexture)
    {
        // 处理生成的纹理
        HandleGeneratedTexture(GeneratedTexture);
    }
});

// 绑定静态函数
OnComplete.BindStatic(&MyClass::StaticHandleImageGenerated);
```

## 错误处理

### 错误类型

1. **网络错误**：
   - 连接超时
   - 服务器不可达
   - DNS解析失败

2. **API错误**：
   - 无效的工作流JSON
   - 参数错误
   - 权限问题

3. **数据处理错误**：
   - 图像格式不支持
   - 内存不足
   - 文件系统错误

### 错误处理策略

```cpp
// 在关键函数中添加错误处理
void UComfyUIClient::GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                                   SComfyUIWidget::EWorkflowType WorkflowType, 
                                   const FOnImageGenerated& OnComplete)
{
    // 验证输入参数
    if (Prompt.IsEmpty())
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("提示词不能为空"));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }
    
    // 验证服务器URL
    if (ServerUrl.IsEmpty())
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("服务器URL未设置"));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }
    
    // 验证HTTP模块
    if (!HttpModule)
    {
        UE_LOG(LogComfyUIIntegration, Error, TEXT("HTTP模块未初始化"));
        OnComplete.ExecuteIfBound(nullptr);
        return;
    }
    
    // 继续执行正常流程...
}
```

## 性能优化

### 1. 请求复用

```cpp
// 复用HTTP请求对象
if (CurrentRequest.IsValid())
{
    CurrentRequest->CancelRequest();
}
CurrentRequest = HttpModule->CreateRequest();
```

### 2. 内存管理

```cpp
// 使用智能指针管理HTTP请求
TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule->CreateRequest();

// 及时释放大型纹理
void UComfyUIClient::CleanupTextures()
{
    if (GeneratedTexture)
    {
        GeneratedTexture->ConditionalBeginDestroy();
        GeneratedTexture = nullptr;
    }
}
```

### 3. 异步处理

```cpp
// 使用异步任务处理图像转换
AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, ImageData]()
{
    UTexture2D* Texture = CreateTextureFromImageData(ImageData);
    
    // 回到主线程执行回调
    AsyncTask(ENamedThreads::GameThread, [this, Texture]()
    {
        OnImageGeneratedCallback.ExecuteIfBound(Texture);
    });
});
```

## 调试和日志

### 详细日志记录

```cpp
// 请求日志
UE_LOG(LogComfyUIIntegration, Log, TEXT("发送HTTP请求: %s"), *RequestUrl);

// 响应日志
UE_LOG(LogComfyUIIntegration, Log, TEXT("收到HTTP响应: %d"), Response->GetResponseCode());

// 数据日志
UE_LOG(LogComfyUIIntegration, Verbose, TEXT("响应数据大小: %d bytes"), Response->GetContentLength());

// 错误日志
UE_LOG(LogComfyUIIntegration, Error, TEXT("请求失败: %s"), *ErrorMessage);
```

### 调试工具

```cpp
// 启用HTTP请求日志
void UComfyUIClient::EnableDebugLogging(bool bEnable)
{
    if (bEnable)
    {
        HttpModule->SetHttpTimeout(0.0f); // 禁用超时用于调试
    }
}

// 获取当前请求状态
FString UComfyUIClient::GetRequestStatus() const
{
    if (CurrentRequest.IsValid())
    {
        return FString::Printf(TEXT("请求状态: %s"), *CurrentRequest->GetURL());
    }
    return TEXT("无活动请求");
}
```

## 扩展指南

### 添加新的工作流类型

1. 在`SComfyUIWidget::EWorkflowType`枚举中添加新类型
2. 在`InitializeWorkflowConfigs()`中添加配置
3. 在`BuildWorkflowJson()`中处理新类型

```cpp
// 添加新的工作流类型
enum class EWorkflowType
{
    // 现有类型...
    VideoGeneration,    // 新增：视频生成
    AudioGeneration     // 新增：音频生成
};

// 在InitializeWorkflowConfigs()中添加配置
void UComfyUIClient::InitializeWorkflowConfigs()
{
    // 现有配置...
    
    // 视频生成配置
    FWorkflowConfig VideoConfig;
    VideoConfig.Name = TEXT("Text to Video");
    VideoConfig.JsonTemplate = TEXT("{ /* 视频生成工作流JSON */ }");
    WorkflowConfigs.Add(SComfyUIWidget::EWorkflowType::VideoGeneration, VideoConfig);
}
```

### 自定义HTTP处理

```cpp
// 创建自定义HTTP处理器
class FCustomHttpHandler
{
public:
    static void ProcessResponse(FHttpResponsePtr Response, TFunction<void(bool, FString)> OnComplete)
    {
        // 自定义响应处理逻辑
        if (Response.IsValid())
        {
            FString Content = Response->GetContentAsString();
            // 处理特殊格式响应
            OnComplete(true, Content);
        }
        else
        {
            OnComplete(false, TEXT("请求失败"));
        }
    }
};
```

## 常见问题解决

### 1. 连接超时

**问题**：请求经常超时
**解决方案**：
```cpp
// 增加超时时间
Request->SetTimeout(300.0f); // 5分钟超时

// 实现重试机制
void UComfyUIClient::RetryRequest(int32 MaxRetries)
{
    if (RetryCount < MaxRetries)
    {
        RetryCount++;
        UE_LOG(LogComfyUIIntegration, Warning, TEXT("重试请求，第 %d 次"), RetryCount);
        // 重新发送请求
    }
}
```

### 2. 内存泄漏

**问题**：长时间运行后内存占用过高
**解决方案**：
```cpp
// 在完成回调后清理资源
void UComfyUIClient::OnImageGenerationComplete(UTexture2D* GeneratedTexture)
{
    // 执行回调
    OnImageGeneratedCallback.ExecuteIfBound(GeneratedTexture);
    
    // 清理资源
    CurrentRequest.Reset();
    CurrentPromptId.Empty();
    
    // 清理定时器
    if (StatusPollTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(StatusPollTimer);
    }
}
```

### 3. 图像质量问题

**问题**：生成的图像质量不佳
**解决方案**：
```cpp
// 调整纹理创建参数
UTexture2D* UComfyUIClient::CreateHighQualityTexture(const TArray<uint8>& ImageData)
{
    // 使用更高质量的像素格式
    UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
    
    // 设置纹理参数
    Texture->SRGB = true;
    Texture->Filter = TF_Linear;
    Texture->CompressionSettings = TC_Default;
    
    // 禁用压缩以保持质量
    Texture->CompressionNoAlpha = false;
    
    return Texture;
}
```

## 最佳实践

1. **资源管理**：及时释放HTTP请求和纹理资源
2. **错误处理**：为所有异步操作提供错误处理
3. **性能优化**：使用对象池复用HTTP请求
4. **线程安全**：确保UI更新在主线程执行
5. **用户体验**：提供进度反馈和取消功能
