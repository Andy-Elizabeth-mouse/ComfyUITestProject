#include "Workflow/ComfyUIWorkflowExecutor.h"
#include "Client/ComfyUIClient.h"
#include "Utils/ComfyUIFileManager.h"
#include "Engine/Texture2D.h"
#include "Workflow/ComfyUIWorkflowService.h"

#pragma optimize("", off)
void FComfyUIWorkflowExecutor::RunGeneration(
    EComfyUIWorkflowType WorkflowType,
    const FString& Prompt,
    const FString& NegativePrompt,
    UTexture2D* InputImage,
    const FString& InputModelPath,
    UComfyUIClient* Client,
    FOnImageGenerated OnImageGenerated,
    FOnMeshGenerated OnMeshGenerated,
    FOnGenerationProgress OnProgress,
    FOnGenerationStarted OnStarted,
    FOnGenerationFailed OnFailed,
    FOnGenerationCompleted OnCompleted)
{
    // 创建参数结构体
    FComfyUIWorkflowExecutorParams Params;
    Params.WorkflowType = WorkflowType;
    
    // 填充输入参数
    if (!Prompt.IsEmpty())
    {
        Params.Input.SetPrompt(Prompt);
    }
    
    if (!NegativePrompt.IsEmpty())
    {
        Params.Input.SetNegativePrompt(NegativePrompt);
    }
    
    // 设置回调
    Params.OnImageGenerated = OnImageGenerated;
    Params.OnMeshGenerated = OnMeshGenerated;
    Params.OnProgress = OnProgress;
    Params.OnStarted = OnStarted;
    Params.OnFailed = OnFailed;
    Params.OnCompleted = OnCompleted;
    
    // 检查是否需要异步上传资源
    bool bNeedImageUpload = InputImage && WorkflowNeedsImageInput(WorkflowType);
    bool bNeedModelUpload = !InputModelPath.IsEmpty() && WorkflowNeedsMeshInput(WorkflowType);
    
    if (!bNeedImageUpload && !bNeedModelUpload)
    {
        // 没有需要上传的资源，直接执行工作流
        return ExecuteWorkflow(Params, Client);
    }
    
    // 需要异步上传资源，使用共享状态来跟踪上传进度和参数
    struct FUploadState
    {
        int32 TotalUploads = 0;
        int32 CompletedUploads = 0;
        bool bHasError = false;
        FString ErrorMessage;
        FComfyUIWorkflowExecutorParams SharedParams; // 共享的参数对象
    };
    
    TSharedPtr<FUploadState> UploadState = MakeShared<FUploadState>();
    UploadState->SharedParams = Params; // 复制参数到共享状态
    
    // 计算需要上传的资源总数
    if (bNeedImageUpload) UploadState->TotalUploads++;
    if (bNeedModelUpload) UploadState->TotalUploads++;
    
    // 定义上传完成后的回调函数
    auto OnUploadCompleted = [Client, UploadState]() mutable {
        UploadState->CompletedUploads++;
        
        if (UploadState->bHasError)
        {
            // 有错误发生，不执行工作流
            return;
        }
        
        if (UploadState->CompletedUploads >= UploadState->TotalUploads)
        {
            ExecuteWorkflow(UploadState->SharedParams, Client);
        }
    };
    
    // 处理图像上传
    if (bNeedImageUpload)
    {
        TArray<uint8> ImageData;
        if (UComfyUIFileManager::ExtractImageDataFromTexture(InputImage, ImageData))
        {
            FString FileName = FString::Printf(TEXT("input_%d.png"), FDateTime::Now().GetTicks());
            
            if (Client)
            {
                Client->UploadImage(ImageData, FileName, 
                    [OnUploadCompleted, UploadState, OnFailed](const FString& ImageName, bool bSuccess) mutable {
                        if (bSuccess && !ImageName.IsEmpty())
                        {
                            UploadState->SharedParams.Input.SetInputImage(ImageName);
                            UE_LOG(LogTemp, Log, TEXT("Image uploaded successfully: %s"), *ImageName);
                        }
                        else
                        {
                            UploadState->bHasError = true;
                            UploadState->ErrorMessage = TEXT("Failed to upload input image");
                            UE_LOG(LogTemp, Warning, TEXT("Failed to upload input image"));
                            
                            FComfyUIError Error(EComfyUIErrorType::ServerError, TEXT("Failed to upload input image"), 0, TEXT(""), false);
                            OnFailed.ExecuteIfBound(Error, false);
                        }
                        
                        OnUploadCompleted();
                    });
            }
            else
            {
                UploadState->bHasError = true;
                UploadState->ErrorMessage = TEXT("Client is null");
                FComfyUIError Error(EComfyUIErrorType::UnknownError, TEXT("Client is null"), 0, TEXT(""), false);
                OnFailed.ExecuteIfBound(Error, false);
            }
        }
        else
        {
            UploadState->bHasError = true;
            UploadState->ErrorMessage = TEXT("Failed to extract image data");
            FComfyUIError Error(EComfyUIErrorType::UnknownError, TEXT("Failed to extract image data"), 0, TEXT(""), false);
            OnFailed.ExecuteIfBound(Error, false);
        }
    }
    
    // 处理3D模型上传
    if (bNeedModelUpload)
    {
        TArray<uint8> ModelData;
        if (UComfyUIFileManager::LoadArrayFromFile(InputModelPath, ModelData))
        {
            FString FileName = FPaths::GetCleanFilename(InputModelPath);
            
            if (Client)
            {
                Client->UploadModel(ModelData, FileName, 
                    [OnUploadCompleted, UploadState, OnFailed](const FString& ModelName, bool bSuccess) mutable {
                        if (bSuccess && !ModelName.IsEmpty())
                        {
                            UploadState->SharedParams.Input.MeshParameters.Add(TEXT("INPUT_MESH"), ModelName);
                            UE_LOG(LogTemp, Log, TEXT("Model uploaded successfully: %s"), *ModelName);
                        }
                        else
                        {
                            UploadState->bHasError = true;
                            UploadState->ErrorMessage = TEXT("Failed to upload input model");
                            UE_LOG(LogTemp, Warning, TEXT("Failed to upload input model"));
                            
                            FComfyUIError Error(EComfyUIErrorType::ServerError, TEXT("Failed to upload input model"), 0, TEXT(""), false);
                            OnFailed.ExecuteIfBound(Error, false);
                        }
                        
                        OnUploadCompleted();
                    });
            }
            else
            {
                UploadState->bHasError = true;
                UploadState->ErrorMessage = TEXT("Client is null");
                FComfyUIError Error(EComfyUIErrorType::UnknownError, TEXT("Client is null"), 0, TEXT(""), false);
                OnFailed.ExecuteIfBound(Error, false);
            }
        }
        else
        {
            UploadState->bHasError = true;
            UploadState->ErrorMessage = FString::Printf(TEXT("Failed to load model file: %s"), *InputModelPath);
            FComfyUIError Error(EComfyUIErrorType::UnknownError, UploadState->ErrorMessage, 0, TEXT(""), false);
            OnFailed.ExecuteIfBound(Error, false);
        }
    }
}

void FComfyUIWorkflowExecutor::ExecuteWorkflow(const FComfyUIWorkflowExecutorParams& Params, UComfyUIClient* Client)
{
    // 1. 验证Client有效性
    if (!Client)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: Client is null"));
        return;
    }
    
    // 2. 使用WorkflowService构建工作流JSON
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (!WorkflowService)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: WorkflowService is not available"));
        return;
    }
    
    // 根据WorkflowType获取对应的工作流名称
    FString WorkflowName = GetWorkflowNameFromType(Params.WorkflowType);
    if (WorkflowName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: Unsupported workflow type"));
        return;
    }
    
    // 为3D生成工作流设置输出文件名参数
    if (Params.WorkflowType == EComfyUIWorkflowType::ImageTo3D || 
        Params.WorkflowType == EComfyUIWorkflowType::TextTo3D)
    {
        // 生成唯一的输出文件名
        FString Timestamp = FDateTime::Now().ToFormattedString(TEXT("%Y%m%d_%H%M%S"));
        FString UniqueId = FString::Printf(TEXT("%d"), FDateTime::Now().GetTicks() % 10000);
        FString OutputFilenamePrefix = FString::Printf(TEXT("Generated3D_%s_%s"), *Timestamp, *UniqueId);
        
        // 设置输出文件名参数
        WorkflowService->SetWorkflowParameter(WorkflowName, TEXT("OUTPUT_FILENAME_PREFIX"), OutputFilenamePrefix);
        
        // 将文件名存储到Input参数中，供后续下载使用
        FComfyUIWorkflowExecutorParams& MutableParams = const_cast<FComfyUIWorkflowExecutorParams&>(Params);
        MutableParams.Input.TextParameters.Add(TEXT("EXPECTED_OUTPUT_FILENAME"), OutputFilenamePrefix + TEXT(".glb"));
        
        UE_LOG(LogTemp, Log, TEXT("Set 3D output filename: %s"), *OutputFilenamePrefix);
    }
    
    // 构建工作流JSON
    // 使用WorkflowService构建JSON，传入完整的Input参数
    FString WorkflowJson = WorkflowService->BuildWorkflowJson(WorkflowName, Params.Input);
    
    if (WorkflowJson.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: Failed to build workflow JSON"));
        return;
    }
    
    // 3. 创建一个共享的结果指针，用于在回调中更新结果
    // TSharedPtr<FComfyUIWorkflowExecutorResult> SharedResult = MakeShared<FComfyUIWorkflowExecutorResult>();
    // SharedResult->bSuccess = false;
    
    // 4. 创建包装后的回调，更新SharedResult
    
    // 5. 通过Client执行工作流
    Client->ExecuteWorkflow(WorkflowJson, 
                           Params.OnStarted,
                           Params.OnProgress,
                           Params.OnImageGenerated,
                           Params.OnMeshGenerated,
                           Params.OnFailed,
                           Params.OnCompleted);
}
#pragma optimize("", on)

bool FComfyUIWorkflowExecutor::WorkflowNeedsImageInput(EComfyUIWorkflowType WorkflowType)
{
    switch (WorkflowType)
    {
    case EComfyUIWorkflowType::ImageToImage:
    case EComfyUIWorkflowType::ImageTo3D:
    case EComfyUIWorkflowType::MeshTexturing:
        return true;
    default:
        return false;
    }
}

bool FComfyUIWorkflowExecutor::WorkflowNeedsMeshInput(EComfyUIWorkflowType WorkflowType)
{
    switch (WorkflowType)
    {
    case EComfyUIWorkflowType::MeshTexturing:
        return true;
    default:
        return false;
    }
}

FString FComfyUIWorkflowExecutor::GetWorkflowNameFromType(EComfyUIWorkflowType WorkflowType)
{
    switch (WorkflowType)
    {
        case EComfyUIWorkflowType::TextToImage:
            return TEXT("basic_txt2img");
        case EComfyUIWorkflowType::ImageToImage:
            return TEXT("img2img");
        case EComfyUIWorkflowType::TextTo3D:
            return TEXT("text_to_3d");
        case EComfyUIWorkflowType::ImageTo3D:
            return TEXT("image_to_3d_full");
        case EComfyUIWorkflowType::MeshTexturing:
            return TEXT("mesh_texturing");
        default:
            return FString();
    }
}