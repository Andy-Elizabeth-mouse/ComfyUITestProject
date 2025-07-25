#include "Workflow/ComfyUIWorkflowExecutor.h"
#include "Client/ComfyUIClient.h"
#include "Utils/ComfyUIFileManager.h"
#include "Engine/Texture2D.h"
#include "Workflow/ComfyUIWorkflowService.h"

FComfyUIWorkflowExecutorResult FComfyUIWorkflowExecutor::RunGeneration(
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
    
    // 处理图像输入
    if (InputImage && WorkflowNeedsImageInput(WorkflowType))
    {
        // 将UTexture2D转换为图像数据并通过Client上传
        TArray<uint8> ImageData;
        if (UComfyUIFileManager::ExtractImageDataFromTexture(InputImage, ImageData))
        {
            FString FileName = FString::Printf(TEXT("input_%d.png"), FDateTime::Now().GetTicks());
            
            // 使用Client上传图像（同步方式，简化处理）
            bool bUploadSuccess = false;
            FString UploadedImageName;
            
            if (Client)
            {
                Client->UploadImage(ImageData, FileName, [&](const FString& ImageName, bool bSuccess) {
                    bUploadSuccess = bSuccess;
                    UploadedImageName = ImageName;
                });
                
                // 简单的同步等待
                if (bUploadSuccess && !UploadedImageName.IsEmpty())
                {
                    Params.Input.SetInputImage(UploadedImageName);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to upload input image, using default name"));
                    Params.Input.SetInputImage(TEXT("uploaded_input.png"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Client is null, cannot upload image"));
                Params.Input.SetInputImage(TEXT("uploaded_input.png"));
            }
        }
    }
    
    // 处理3D模型输入
    if (!InputModelPath.IsEmpty() && WorkflowNeedsMeshInput(WorkflowType))
    {
        Params.Input.MeshParameters.Add(TEXT("INPUT_MESH"), InputModelPath);
    }
    
    // 设置回调
    Params.OnImageGenerated = OnImageGenerated;
    Params.OnMeshGenerated = OnMeshGenerated;
    Params.OnProgress = OnProgress;
    Params.OnStarted = OnStarted;
    Params.OnFailed = OnFailed;
    Params.OnCompleted = OnCompleted;
    
    // 调用执行函数，传递Client实例
    return ExecuteWorkflow(Params, Client);
}

FComfyUIWorkflowExecutorResult FComfyUIWorkflowExecutor::ExecuteWorkflow(const FComfyUIWorkflowExecutorParams& Params, UComfyUIClient* Client)
{
    FComfyUIWorkflowExecutorResult Result;
    Result.bSuccess = false;
    
    // 1. 验证Client有效性
    if (!Client)
    {
        Result.ErrorMessage = TEXT("Client is null");
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: %s"), *Result.ErrorMessage);
        return Result;
    }
    
    // 2. 使用WorkflowService构建工作流JSON
    UComfyUIWorkflowService* WorkflowService = UComfyUIWorkflowService::Get();
    if (!WorkflowService)
    {
        Result.ErrorMessage = TEXT("WorkflowService is not available");
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: %s"), *Result.ErrorMessage);
        return Result;
    }
    
    // 根据WorkflowType获取对应的工作流名称
    FString WorkflowName = GetWorkflowNameFromType(Params.WorkflowType);
    if (WorkflowName.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Unsupported workflow type");
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: %s"), *Result.ErrorMessage);
        return Result;
    }
    
    // 构建工作流JSON
    FString WorkflowJson = WorkflowService->BuildWorkflowJson(
        WorkflowName,
        Params.Input.TextParameters.FindRef(TEXT("POSITIVE_PROMPT")), // Prompt
        Params.Input.TextParameters.FindRef(TEXT("NEGATIVE_PROMPT")), // Negative prompt
        Params.Input.NumericParameters
    );
    
    if (WorkflowJson.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to build workflow JSON");
        UE_LOG(LogTemp, Error, TEXT("ExecuteWorkflow: %s"), *Result.ErrorMessage);
        return Result;
    }
    
    // 3. 创建一个共享的结果指针，用于在回调中更新结果
    TSharedPtr<FComfyUIWorkflowExecutorResult> SharedResult = MakeShared<FComfyUIWorkflowExecutorResult>();
    SharedResult->bSuccess = false;
    
    // 4. 创建包装后的回调，更新SharedResult
    FOnImageGenerated WrappedOnImageGenerated;
    if (Params.OnImageGenerated.IsBound())
    {
        WrappedOnImageGenerated.BindLambda([SharedResult, OriginalCallback = Params.OnImageGenerated](UTexture2D* GeneratedTexture) {
            SharedResult->GeneratedTexture = GeneratedTexture;
            SharedResult->GeneratedImage = GeneratedTexture;  // 保持一致性
            OriginalCallback.ExecuteIfBound(GeneratedTexture);
        });
    }
    
    FOnMeshGenerated WrappedOnMeshGenerated;
    if (Params.OnMeshGenerated.IsBound())
    {
        WrappedOnMeshGenerated.BindLambda([SharedResult, OriginalCallback = Params.OnMeshGenerated](UStaticMesh* GeneratedMesh) {
            SharedResult->GeneratedMesh = GeneratedMesh;
            OriginalCallback.ExecuteIfBound(GeneratedMesh);
        });
    }
    
    FOnGenerationCompleted WrappedOnCompleted;
    if (Params.OnCompleted.IsBound())
    {
        WrappedOnCompleted.BindLambda([SharedResult, OriginalCallback = Params.OnCompleted]() {
            SharedResult->bSuccess = true;
            SharedResult->ErrorMessage = TEXT("Workflow completed successfully");
            OriginalCallback.ExecuteIfBound();
        });
    }
    
    FOnGenerationFailed WrappedOnFailed;
    if (Params.OnFailed.IsBound())
    {
        WrappedOnFailed.BindLambda([SharedResult, OriginalCallback = Params.OnFailed](const FComfyUIError& Error, bool bCanRetry) {
            SharedResult->bSuccess = false;
            SharedResult->ErrorMessage = Error.ErrorMessage;
            OriginalCallback.ExecuteIfBound(Error, bCanRetry);
        });
    }
    else
    {
        // 如果没有提供OnFailed回调，创建一个默认的
        WrappedOnFailed.BindLambda([SharedResult](const FComfyUIError& Error, bool bCanRetry) {
            SharedResult->bSuccess = false;
            SharedResult->ErrorMessage = Error.ErrorMessage;
        });
    }
    
    // 5. 通过Client执行工作流
    Client->ExecuteWorkflow(WorkflowJson, 
                           Params.OnStarted,
                           Params.OnProgress,
                           WrappedOnImageGenerated,
                           WrappedOnMeshGenerated,
                           WrappedOnFailed,
                           WrappedOnCompleted);
    
    // 6. 返回初始结果（异步操作，实际结果在回调中更新）
    Result.bSuccess = true;
    Result.ErrorMessage = TEXT("Workflow execution initiated successfully");
    UE_LOG(LogTemp, Log, TEXT("ExecuteWorkflow: %s"), *Result.ErrorMessage);
    
    return Result;
}

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