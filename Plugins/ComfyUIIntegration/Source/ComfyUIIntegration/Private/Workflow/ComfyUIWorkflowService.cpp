#include "Workflow/ComfyUIWorkflowService.h"
#include "Network/ComfyUINetworkManager.h"
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
    if (!Instance || !IsValid(Instance))
    {
        // 使用TransientPackage作为外部对象，避免依赖特定的World上下文
        Instance = NewObject<UComfyUIWorkflowService>(GetTransientPackage());
        if (Instance)
        {
            Instance->AddToRoot(); // 防止被垃圾回收
            Instance->Initialize();
        }
    }
    if (!IsValid(Instance->WorkflowManager))
    {
        UE_LOG(LogTemp, Error, TEXT("UComfyUIWorkflowService: WorkflowManager is not valid, re-initializing"));
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
        WorkflowManager->AddToRoot(); // 防止被垃圾回收
        WorkflowManager->SetFlags(RF_Transient); // 设置为临时对象，避免在编辑器中保存时被序列化
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
        WorkflowManager->RemoveFromRoot(); // 防止内存泄漏
        WorkflowManager = nullptr;
    }
    
    bInitialized = false;
}

void UComfyUIWorkflowService::ShutdownGlobal()
{
    if (Instance)
    {
        Instance->Shutdown();
        Instance->RemoveFromRoot();
        Instance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowService: Global instance shutdown complete"));
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

#pragma optimize("", off)

FString UComfyUIWorkflowService::BuildWorkflowJson(const FString& WorkflowName, 
                                                   const FComfyUIWorkflowInput& Input)
{
    if (!WorkflowManager)
        LOG_AND_RETURN(Error, FString(), "BuildWorkflowJson: WorkflowManager is null");
    
    // 添加文本参数
    for (const auto& Param : Input.TextParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value);
    
    // 添加图像参数
    for (const auto& Param : Input.ImageParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value);
    
    // 添加网格参数
    for (const auto& Param : Input.MeshParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value);
    
    // 添加数值参数（转换为字符串）
    for (const auto& Param : Input.NumericParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, FString::SanitizeFloat(Param.Value));
    
    // 添加布尔参数（转换为字符串）
    for (const auto& Param : Input.BooleanParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value ? TEXT("true") : TEXT("false"));
    
    // 添加选择参数
    for (const auto& Param : Input.ChoiceParameters)
        WorkflowManager->SetWorkflowParameter(WorkflowName, Param.Key, Param.Value);
    
    // 构建并返回工作流JSON
    return WorkflowManager->BuildWorkflowJson(WorkflowName);
}

#pragma optimize("", on)

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

EComfyUIWorkflowType UComfyUIWorkflowService::DetectWorkflowType(const FString &WorkflowName)
{
    return WorkflowManager->DetectWorkflowType(WorkflowName);
}