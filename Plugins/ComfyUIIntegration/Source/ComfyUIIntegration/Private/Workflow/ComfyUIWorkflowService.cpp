#include "Workflow/ComfyUIWorkflowService.h"
#include "Workflow/ComfyUIWorkflowManager.h"
#include "Utils/Defines.h"

// 静态成员变量定义
TObjectPtr<UComfyUIWorkflowService> UComfyUIWorkflowService::Instance = nullptr;

UComfyUIWorkflowService::UComfyUIWorkflowService()
{
    bInitialized = false;
}

// ========== 单例模式 ==========

UComfyUIWorkflowService* UComfyUIWorkflowService::Get()
{
    if (!Instance)
    {
        Instance = NewObject<UComfyUIWorkflowService>();
        Instance->AddToRoot(); // 防止被垃圾回收
        Instance->Initialize();
    }
    return Instance;
}

void UComfyUIWorkflowService::Initialize()
{
    if (bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("UComfyUIWorkflowService: Already initialized"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Initializing workflow service"));
    
    // 创建工作流管理器
    WorkflowManager = NewObject<UComfyUIWorkflowManager>(this);
    if (WorkflowManager)
    {
        // 加载工作流配置
        WorkflowManager->LoadWorkflowConfigs();
        bInitialized = true;
        
        UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Successfully initialized"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UComfyUIWorkflowService: Failed to create workflow manager"));
    }
}

void UComfyUIWorkflowService::Shutdown()
{
    if (!bInitialized)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Shutting down workflow service"));
    
    if (WorkflowManager)
    {
        WorkflowManager->ClearWorkflowConfigs();
        WorkflowManager = nullptr;
    }
    
    bInitialized = false;
    
    if (Instance == this)
    {
        Instance->RemoveFromRoot();
        Instance = nullptr;
    }
}

// ========== 工作流管理接口 ==========

TArray<FString> UComfyUIWorkflowService::GetAvailableWorkflowNames() const
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, TArray<FString>(), "GetAvailableWorkflowNames: WorkflowManager is null");
    
    return WorkflowManager->GetAvailableWorkflowNames();
}

bool UComfyUIWorkflowService::GetWorkflowConfig(const FString& WorkflowName, FWorkflowConfig& OutConfig) const
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, false, "GetWorkflowConfig: WorkflowManager is null");
    
    return WorkflowManager->FindWorkflowConfig(WorkflowName, OutConfig);
}

void UComfyUIWorkflowService::RefreshWorkflows()
{
    if (!WorkflowManager)
    {
        UE_LOG(LogTemp, Error, TEXT("RefreshWorkflows: WorkflowManager is null"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Refreshing workflows"));
    WorkflowManager->RefreshWorkflowConfigs();
}

bool UComfyUIWorkflowService::ValidateWorkflow(const FString& FilePath, FString& OutError)
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, false, "ValidateWorkflow: WorkflowManager is null");
    
    return WorkflowManager->ValidateWorkflowFile(FilePath, OutError);
}

bool UComfyUIWorkflowService::ImportWorkflow(const FString& FilePath, const FString& WorkflowName, FString& OutError)
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, false, "ImportWorkflow: WorkflowManager is null");
    
    bool bSuccess = WorkflowManager->ImportWorkflowFile(FilePath, WorkflowName, OutError);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Successfully imported workflow: %s"), *WorkflowName);
    }
    
    return bSuccess;
}

// ========== JSON构建接口 ==========

FString UComfyUIWorkflowService::BuildWorkflowJson(const FString& WorkflowName, 
                                                   const FString& Prompt, 
                                                   const FString& NegativePrompt,
                                                   const TMap<FString, FString>& Parameters)
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, FString(), "BuildWorkflowJson: WorkflowManager is null");
    
    // 设置自定义参数
    for (const auto& Param : Parameters)
    {
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value);
    }
    
    return WorkflowManager->BuildCustomWorkflowJson(Prompt, NegativePrompt, WorkflowName);
}

FString UComfyUIWorkflowService::BuildWorkflowJson(const FString& WorkflowName, 
                                                   const FString& Prompt, 
                                                   const FString& NegativePrompt)
{
    TMap<FString, FString> EmptyParameters;
    return BuildWorkflowJson(WorkflowName, Prompt, NegativePrompt, EmptyParameters);
}

// ========== 参数管理接口 ==========

TArray<FString> UComfyUIWorkflowService::GetWorkflowParameters(const FString& WorkflowName) const
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, TArray<FString>(), "GetWorkflowParameters: WorkflowManager is null");
    
    return WorkflowManager->GetWorkflowParameterNames(WorkflowName);
}

bool UComfyUIWorkflowService::SetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName, const FString& Value)
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, false, "SetWorkflowParameter: WorkflowManager is null");
    
    return WorkflowManager->SetWorkflowParameter(WorkflowName, ParameterName, Value);
}

FString UComfyUIWorkflowService::GetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName) const
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, FString(), "GetWorkflowParameter: WorkflowManager is null");
    
    return WorkflowManager->GetWorkflowParameter(WorkflowName, ParameterName);
}

// ========== 便捷接口 ==========

bool UComfyUIWorkflowService::IsWorkflowValid(const FString& WorkflowName) const
{
    if (!WorkflowManager)
        return false;
    
    FWorkflowConfig Config;
    if (WorkflowManager->FindWorkflowConfig(WorkflowName, Config))
    {
        return Config.bIsValid;
    }
    
    return false;
}

FString UComfyUIWorkflowService::GetWorkflowDescription(const FString& WorkflowName) const
{
    if (!WorkflowManager)
        return FString();
    
    FWorkflowConfig Config;
    if (WorkflowManager->FindWorkflowConfig(WorkflowName, Config))
    {
        return Config.Description;
    }
    
    return FString();
}

FString UComfyUIWorkflowService::GetWorkflowType(const FString& WorkflowName) const
{
    if (!WorkflowManager)
        return FString();
    
    FWorkflowConfig Config;
    if (WorkflowManager->FindWorkflowConfig(WorkflowName, Config))
    {
        return Config.Type;
    }
    
    return FString();
}
