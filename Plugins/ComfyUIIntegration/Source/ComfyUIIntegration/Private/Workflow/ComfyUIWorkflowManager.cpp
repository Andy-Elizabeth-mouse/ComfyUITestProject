#include "Workflow/ComfyUIWorkflowManager.h"
#include "Workflow/ComfyUINodeAnalyzer.h"
#include "Test/ComfyUIWorkflowTester.h"
#include "Utils/ComfyUIFileManager.h"
#include "Utils/Defines.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

UComfyUIWorkflowManager::UComfyUIWorkflowManager()
{
    // 初始化空的工作流配置列表
    CustomWorkflowConfigs.Empty();
}

// ========== 工作流加载和管理 ==========

void UComfyUIWorkflowManager::LoadWorkflowConfigs()
{
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowManager: Loading workflow configurations"));
    
    // 清空现有配置
    CustomWorkflowConfigs.Empty();
    
    // 从配置文件加载
    FString ConfigPath = UComfyUIFileManager::GetConfigDirectory() / TEXT("default_config.json");
    
    if (FPaths::FileExists(ConfigPath))
    {
        FString ConfigContent;
        if (UComfyUIFileManager::LoadJsonFromFile(ConfigPath, ConfigContent))
        {
            TSharedPtr<FJsonObject> ConfigJson;
            if (UComfyUIFileManager::ParseJsonString(ConfigContent, ConfigJson))
            {
                LoadWorkflowsFromConfigJson(ConfigJson);
            }
        }
    }
    
    // 从模板目录加载
    LoadTemplateDirectoryWorkflows();
    
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowManager: Loaded %d workflow configurations"), CustomWorkflowConfigs.Num());
}

bool UComfyUIWorkflowManager::LoadCustomWorkflowFromFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
        LOG_AND_RETURN(Warning, false, "LoadCustomWorkflowFromFile: Workflow template file not found: %s", *FilePath);
    
    FString TemplateContent;
    if (!UComfyUIFileManager::LoadJsonFromFile(FilePath, TemplateContent))
        LOG_AND_RETURN(Error, false, "LoadCustomWorkflowFromFile: Failed to load workflow template: %s", *FilePath);
    
    // 验证JSON格式
    TSharedPtr<FJsonObject> TemplateJson;
    if (!UComfyUIFileManager::ParseJsonString(TemplateContent, TemplateJson))
        LOG_AND_RETURN(Error, false, "LoadCustomWorkflowFromFile: Invalid JSON in workflow template: %s", *FilePath);
    
    // 创建工作流配置
    FWorkflowConfig NewWorkflow;
    FString FileName = FPaths::GetBaseFilename(FilePath);
    NewWorkflow.Name = FileName;
    NewWorkflow.Type = TEXT("custom");
    NewWorkflow.Description = FString::Printf(TEXT("Custom workflow loaded from %s"), *FileName);
    NewWorkflow.JsonTemplate = TemplateContent;
    NewWorkflow.TemplateFile = FilePath;
    
    // 验证工作流
    FString ValidationError;
    if (ValidateWorkflowJson(TemplateContent, NewWorkflow, ValidationError))
    {
        CustomWorkflowConfigs.Add(NewWorkflow);
        UE_LOG(LogTemp, Log, TEXT("LoadCustomWorkflowFromFile: Successfully loaded workflow: %s"), *NewWorkflow.Name);
        return true;
    }
    else
    {
        LOG_AND_RETURN(Warning, false, "LoadCustomWorkflowFromFile: Workflow validation failed for %s: %s", *FileName, *ValidationError);
    }
}

void UComfyUIWorkflowManager::LoadTemplateDirectoryWorkflows()
{
    FString TemplatesDir = UComfyUIFileManager::GetTemplatesDirectory();
    
    if (!FPaths::DirectoryExists(TemplatesDir))
    {
        UE_LOG(LogTemp, Warning, TEXT("UComfyUIWorkflowManager: Templates directory does not exist: %s"), *TemplatesDir);
        return;
    }
    
    TArray<FString> TemplateFiles = UComfyUIFileManager::ScanFilesInDirectory(TemplatesDir, TEXT(".json"));
    
    for (const FString& TemplateFile : TemplateFiles)
    {
        FString FullPath = TemplatesDir / TemplateFile;
        LoadCustomWorkflowFromFile(FullPath);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowManager: Loaded %d workflows from templates directory"), TemplateFiles.Num());
}

TArray<FString> UComfyUIWorkflowManager::GetAvailableWorkflowNames() const
{
    TArray<FString> WorkflowNames;
    
    for (const FWorkflowConfig& Config : CustomWorkflowConfigs)
    {
        WorkflowNames.Add(Config.Name);
    }
    
    return WorkflowNames;
}

bool UComfyUIWorkflowManager::FindWorkflowConfig(const FString& WorkflowName, FWorkflowConfig& OutConfig) const
{
    const FWorkflowConfig* FoundConfig = FindWorkflowConfigInternal(WorkflowName);
    if (FoundConfig)
    {
        OutConfig = *FoundConfig;
        return true;
    }
    return false;
}

// ========== 工作流验证 ==========

bool UComfyUIWorkflowManager::ValidateWorkflowFile(const FString& FilePath, FString& OutError)
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
    if (!UComfyUIFileManager::LoadJsonFromFile(FilePath, JsonContent))
    {
        OutError = FString::Printf(TEXT("Failed to read workflow file: %s"), *FilePath);
        return false;
    }
    
    // 验证JSON格式和工作流结构
    FWorkflowConfig TempConfig;
    return ValidateWorkflowJson(JsonContent, TempConfig, OutError);
}

bool UComfyUIWorkflowManager::ValidateWorkflowJson(const FString& JsonContent, FWorkflowConfig& OutConfig, FString& OutError)
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
        OutError = TEXT("Failed to parse JSON object");
        return false;
    }
    
    // 检查是否是有效的ComfyUI工作流格式
    bool bHasValidNodes = false;
    for (const auto& NodePair : WorkflowJson->Values)
    {
        const TSharedPtr<FJsonValue>& NodeValue = NodePair.Value;
        if (NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
            if (NodeObj->HasField(TEXT("class_type")) && NodeObj->HasField(TEXT("inputs")))
            {
                bHasValidNodes = true;
                break;
            }
        }
    }
    
    if (!bHasValidNodes)
    {
        OutError = TEXT("Invalid ComfyUI workflow format: no valid nodes found");
        return false;
    }
    
    // 分析工作流节点
    if (!AnalyzeWorkflowNodes(WorkflowJson, OutConfig))
    {
        OutError = TEXT("Failed to analyze workflow nodes");
        return false;
    }
    
    // 查找输入和输出节点（保留旧方法作为后备）
    if (!FindWorkflowInputs(WorkflowJson, OutConfig.RequiredInputs))
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateWorkflowJson: No input nodes found"));
    }
    
    if (!FindWorkflowOutputs(WorkflowJson, OutConfig.OutputNodes))
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateWorkflowJson: No output nodes found"));
    }
    
    // 使用新的节点分析器来分析工作流输入输出
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (NodeAnalyzer->AnalyzeWorkflow(WorkflowJson, Inputs, Outputs))
    {
        OutConfig.WorkflowInputs = Inputs;
        OutConfig.WorkflowOutputs = Outputs;
        OutConfig.DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
        
        UE_LOG(LogTemp, Log, TEXT("ValidateWorkflowJson: Node analysis successful. Found %d workflow inputs and %d outputs"), 
               Inputs.Num(), Outputs.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateWorkflowJson: Node analysis failed, using legacy method"));
        OutConfig.DetectedType = EComfyUIWorkflowType::Unknown;
    }
    
    OutConfig.bIsValid = true;
    OutConfig.JsonTemplate = JsonContent;
    
    UE_LOG(LogTemp, Log, TEXT("ValidateWorkflowJson: Validation successful. Found %d inputs and %d outputs"), 
           OutConfig.RequiredInputs.Num(), OutConfig.OutputNodes.Num());
    
    return true;
}

// ========== 工作流导入和导出 ==========

bool UComfyUIWorkflowManager::ImportWorkflowFile(const FString& FilePath, const FString& WorkflowName, FString& OutError)
{
    OutError.Empty();
    
    // 首先验证工作流
    if (!ValidateWorkflowFile(FilePath, OutError))
    {
        return false;
    }
    
    // 读取工作流内容
    FString JsonContent;
    if (!UComfyUIFileManager::LoadJsonFromFile(FilePath, JsonContent))
    {
        OutError = FString::Printf(TEXT("Failed to read workflow file: %s"), *FilePath);
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
    
    // 使用FileManager保存到插件目录
    FString ImportError;
    if (!UComfyUIFileManager::ImportWorkflowTemplate(FilePath, NewWorkflow.Name, ImportError))
    {
        OutError = ImportError;
        return false;
    }
    
    // 添加到配置列表
    CustomWorkflowConfigs.Add(NewWorkflow);
    
    UE_LOG(LogTemp, Log, TEXT("ImportWorkflowFile: Successfully imported workflow: %s"), *NewWorkflow.Name);
    return true;
}

bool UComfyUIWorkflowManager::ExportWorkflowConfig(const FWorkflowConfig& Config, const FString& FilePath, FString& OutError)
{
    OutError.Empty();
    
    if (Config.JsonTemplate.IsEmpty())
    {
        OutError = TEXT("Workflow configuration has no template content");
        return false;
    }
    
    // 确保目录存在
    FString FileDirectory = FPaths::GetPath(FilePath);
    if (!UComfyUIFileManager::EnsureDirectoryExists(FileDirectory))
    {
        OutError = FString::Printf(TEXT("Failed to create directory: %s"), *FileDirectory);
        return false;
    }
    
    // 保存工作流内容到文件
    if (!UComfyUIFileManager::SaveJsonToFile(Config.JsonTemplate, FilePath))
    {
        OutError = FString::Printf(TEXT("Failed to save workflow to file: %s"), *FilePath);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ExportWorkflowConfig: Successfully exported workflow %s to %s"), *Config.Name, *FilePath);
    return true;
}

// ========== 工作流JSON构建 ==========

FString UComfyUIWorkflowManager::BuildCustomWorkflowJson(const FString& Prompt, const FString& NegativePrompt, 
                                                         const FString& CustomWorkflowName)
{
    // 查找自定义工作流配置
    FWorkflowConfig* CustomConfig = FindWorkflowConfigInternal(CustomWorkflowName);
    if (!CustomConfig)
        LOG_AND_RETURN(Error, TEXT("{}"), "BuildCustomWorkflowJson: Custom workflow not found: %s", *CustomWorkflowName);
    
    // 创建请求JSON对象
    TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
    RequestJson->SetStringField(TEXT("client_id"), TEXT("unreal_engine_plugin"));

    // 获取工作流模板
    FString WorkflowTemplate;
    if (!CustomConfig->JsonTemplate.IsEmpty())
    {
        WorkflowTemplate = CustomConfig->JsonTemplate;
        UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Using cached template content"));
    }
    else if (!CustomConfig->TemplateFile.IsEmpty())
    {
        // 从文件加载模板
        FString TemplateFilePath = CustomConfig->TemplateFile;
        if (FPaths::IsRelative(TemplateFilePath))
        {
            FString TemplatesDir = UComfyUIFileManager::GetTemplatesDirectory();
            TemplateFilePath = TemplatesDir / TemplateFilePath;
        }
        
        UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Loading template from file: %s"), *TemplateFilePath);
        
        if (!UComfyUIFileManager::LoadJsonFromFile(TemplateFilePath, WorkflowTemplate))
            LOG_AND_RETURN(Error, TEXT("{}"), "BuildCustomWorkflowJson: Failed to load workflow template file: %s", *TemplateFilePath);
        
        // 缓存模板内容以供下次使用
        CustomConfig->JsonTemplate = WorkflowTemplate;
    }
    else
    {
        LOG_AND_RETURN(Error, TEXT("{}"), "BuildCustomWorkflowJson: No template found for custom workflow: %s", *CustomWorkflowName);
    }

    // 替换占位符
    FString ProcessedWorkflow = ReplaceWorkflowPlaceholders(WorkflowTemplate, Prompt, NegativePrompt, CustomConfig->Parameters);
    
    // 解析处理后的工作流JSON
    TSharedPtr<FJsonObject> FinalWorkflowJson;
    TSharedRef<TJsonReader<>> FinalReader = TJsonReaderFactory<>::Create(ProcessedWorkflow);
    if (FJsonSerializer::Deserialize(FinalReader, FinalWorkflowJson))
    {
        RequestJson->SetObjectField(TEXT("prompt"), FinalWorkflowJson);
    }
    else
    {
        LOG_AND_RETURN(Error, TEXT("{}"), "BuildCustomWorkflowJson: Failed to deserialize final workflow JSON");
    }

    // 序列化最终的请求JSON
    FString OutputString;
    TSharedRef<TJsonWriter<>> OutputWriter = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestJson.ToSharedRef(), OutputWriter);

    UE_LOG(LogTemp, Log, TEXT("BuildCustomWorkflowJson: Successfully built workflow JSON for: %s"), *CustomWorkflowName);
    return OutputString;
}

FString UComfyUIWorkflowManager::ReplaceWorkflowPlaceholders(const FString& WorkflowTemplate, 
                                                           const FString& Prompt, 
                                                           const FString& NegativePrompt,
                                                           const TMap<FString, FString>& CustomParameters)
{
    FString ProcessedTemplate = WorkflowTemplate;
    
    // 替换基本占位符
    ProcessedTemplate = ProcessedTemplate.Replace(TEXT("{POSITIVE_PROMPT}"), *Prompt);
    ProcessedTemplate = ProcessedTemplate.Replace(TEXT("{NEGATIVE_PROMPT}"), *NegativePrompt);
    
    // 替换自定义参数
    for (const auto& Param : CustomParameters)
    {
        FString Placeholder = FString::Printf(TEXT("{%s}"), *Param.Key.ToUpper());
        ProcessedTemplate = ProcessedTemplate.Replace(*Placeholder, *Param.Value);
    }
    
    return ProcessedTemplate;
}

FString UComfyUIWorkflowManager::ReplaceWorkflowPlaceholders(const FString& WorkflowTemplate, 
                                                           const FString& Prompt, 
                                                           const FString& NegativePrompt)
{
    TMap<FString, FString> EmptyParameters;
    return ReplaceWorkflowPlaceholders(WorkflowTemplate, Prompt, NegativePrompt, EmptyParameters);
}

// ========== 工作流参数管理 ==========

TArray<FString> UComfyUIWorkflowManager::GetWorkflowParameterNames(const FString& WorkflowName) const
{
    TArray<FString> ParameterNames;
    
    const FWorkflowConfig* FoundConfig = FindWorkflowConfigInternal(WorkflowName);
    if (FoundConfig)
    {
        for (const auto& Param : FoundConfig->Parameters)
        {
            ParameterNames.Add(Param.Key);
        }
    }
    
    return ParameterNames;
}

bool UComfyUIWorkflowManager::SetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName, const FString& Value)
{
    FWorkflowConfig* FoundConfig = FindWorkflowConfigInternal(WorkflowName);
    if (FoundConfig)
    {
        FoundConfig->Parameters.Add(ParameterName, Value);
        UE_LOG(LogTemp, Log, TEXT("SetWorkflowParameter: Set %s.%s = %s"), *WorkflowName, *ParameterName, *Value);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("SetWorkflowParameter: Workflow not found: %s"), *WorkflowName);
    return false;
}

FString UComfyUIWorkflowManager::GetWorkflowParameter(const FString& WorkflowName, const FString& ParameterName) const
{
    const FWorkflowConfig* FoundConfig = FindWorkflowConfigInternal(WorkflowName);
    if (FoundConfig)
    {
        if (const FString* FoundValue = FoundConfig->Parameters.Find(ParameterName))
        {
            return *FoundValue;
        }
    }
    
    return FString();
}

// ========== 工具函数 ==========

void UComfyUIWorkflowManager::ClearWorkflowConfigs()
{
    CustomWorkflowConfigs.Empty();
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowManager: Cleared all workflow configurations"));
}

void UComfyUIWorkflowManager::RefreshWorkflowConfigs()
{
    UE_LOG(LogTemp, Log, TEXT("UComfyUIWorkflowManager: Refreshing workflow configurations"));
    ClearWorkflowConfigs();
    LoadWorkflowConfigs();
}

// ========== 私有函数 ==========

bool UComfyUIWorkflowManager::AnalyzeWorkflowNodes(TSharedPtr<FJsonObject> WorkflowJson, FWorkflowConfig& OutConfig)
{
    if (!WorkflowJson.IsValid())
    {
        return false;
    }
    
    OutConfig.ParameterDefinitions.Empty();
    
    // 遍历所有节点
    for (const auto& NodePair : WorkflowJson->Values)
    {
        const TSharedPtr<FJsonValue>& NodeValue = NodePair.Value;
        if (NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
            
            // 分析节点类型和参数
            if (NodeObj->HasField(TEXT("class_type")))
            {
                FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
                
                // 根据节点类型分析可配置参数
                if (ClassType == TEXT("CLIPTextEncode") || ClassType == TEXT("PromptText"))
                {
                    // 文本输入节点
                    FWorkflowParameterDef PromptParam;
                    PromptParam.Name = TEXT("prompt");
                    PromptParam.Type = TEXT("text");
                    PromptParam.Description = TEXT("Text prompt for generation");
                    OutConfig.ParameterDefinitions.Add(PromptParam);
                }
                // 可以添加更多节点类型的分析
            }
        }
    }
    
    return true;
}

bool UComfyUIWorkflowManager::FindWorkflowInputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutInputs)
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
        const TSharedPtr<FJsonValue>& NodeValue = NodePair.Value;
        if (NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
            if (NodeObj->HasField(TEXT("class_type")))
            {
                FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
                if (TextInputNodes.Contains(ClassType))
                {
                    OutInputs.Add(NodePair.Key);
                }
            }
        }
    }
    
    return OutInputs.Num() > 0;
}

bool UComfyUIWorkflowManager::FindWorkflowOutputs(TSharedPtr<FJsonObject> WorkflowJson, TArray<FString>& OutOutputs)
{
    OutOutputs.Empty();
    
    if (!WorkflowJson.IsValid())
    {
        return false;
    }
    
    // 查找输出节点
    TArray<FString> OutputNodes = {
        TEXT("SaveImage"),
        TEXT("PreviewImage"),
        TEXT("ImageOutput")
    };
    
    for (const auto& NodePair : WorkflowJson->Values)
    {
        const TSharedPtr<FJsonValue>& NodeValue = NodePair.Value;
        if (NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
            if (NodeObj->HasField(TEXT("class_type")))
            {
                FString ClassType = NodeObj->GetStringField(TEXT("class_type"));
                if (OutputNodes.Contains(ClassType))
                {
                    OutOutputs.Add(NodePair.Key);
                }
            }
        }
    }
    
    return OutOutputs.Num() > 0;
}

FString UComfyUIWorkflowManager::SanitizeWorkflowName(const FString& Name) const
{
    FString CleanName = Name;
    
    // 移除非法字符
    CleanName = CleanName.Replace(TEXT(" "), TEXT("_"));
    CleanName = CleanName.Replace(TEXT("-"), TEXT("_"));
    CleanName = CleanName.Replace(TEXT("."), TEXT(""));
    CleanName = CleanName.Replace(TEXT(":"), TEXT(""));
    CleanName = CleanName.Replace(TEXT("\\"), TEXT(""));
    CleanName = CleanName.Replace(TEXT("/"), TEXT(""));
    
    // 确保名称不为空
    if (CleanName.IsEmpty())
    {
        CleanName = TEXT("CustomWorkflow");
    }
    
    return CleanName;
}

FWorkflowConfig* UComfyUIWorkflowManager::FindWorkflowConfigInternal(const FString& WorkflowName)
{
    for (FWorkflowConfig& Config : CustomWorkflowConfigs)
    {
        if (Config.Name == WorkflowName)
        {
            return &Config;
        }
    }
    return nullptr;
}

const FWorkflowConfig* UComfyUIWorkflowManager::FindWorkflowConfigInternal(const FString& WorkflowName) const
{
    for (const FWorkflowConfig& Config : CustomWorkflowConfigs)
    {
        if (Config.Name == WorkflowName)
        {
            return &Config;
        }
    }
    return nullptr;
}

void UComfyUIWorkflowManager::LoadWorkflowsFromConfigJson(TSharedPtr<FJsonObject> ConfigJson)
{
    if (!ConfigJson.IsValid() || !ConfigJson->HasField(TEXT("workflows")))
    {
        return;
    }
    
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
                UE_LOG(LogTemp, Log, TEXT("LoadWorkflowsFromConfigJson: Loaded workflow from config: %s"), *CustomConfig.Name);
            }
        }
    }
}

// ========== 工作流类型检测 ==========

EComfyUIWorkflowType UComfyUIWorkflowManager::DetectWorkflowType(const FString& WorkflowName)
{
    // 默认类型
    EComfyUIWorkflowType DetectedType = EComfyUIWorkflowType::Unknown;
    
    if (WorkflowName.IsEmpty()) return DetectedType;
    
    // 构建工作流文件路径
    FString TemplatesDir = FPaths::ProjectPluginsDir() / TEXT("ComfyUIIntegration/Config/Templates");
    FString WorkflowFile = TemplatesDir / (WorkflowName + TEXT(".json"));
    
    // 检查文件是否存在
    if (!FPaths::FileExists(WorkflowFile))
    {
        UE_LOG(LogTemp, Warning, TEXT("DetectWorkflowType: Workflow file not found: %s"), *WorkflowFile);
        return DetectedType;
    }
    
    // 读取并验证工作流文件
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *WorkflowFile))
    {
        UE_LOG(LogTemp, Error, TEXT("DetectWorkflowType: Failed to load workflow file: %s"), *WorkflowFile);
        return DetectedType;
    }
    
    // 验证和分析工作流
    FWorkflowConfig Config;
    FString ValidationError;
    
    bool bIsValid = ValidateWorkflowJson(JsonContent, Config, ValidationError);
    
    if (bIsValid)
    {
        // 分析工作流类型
        DetectedType = AnalyzEComfyUIWorkflowTypeFromConfig(Config);
        
        UE_LOG(LogTemp, Log, TEXT("DetectWorkflowType: Detected workflow type for '%s': %d"), 
               *WorkflowName, (int32)DetectedType);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DetectWorkflowType: Failed to validate workflow '%s': %s"), 
               *WorkflowName, *ValidationError);
    }
    
    return DetectedType;
}

EComfyUIWorkflowType UComfyUIWorkflowManager::AnalyzEComfyUIWorkflowTypeFromConfig(const FWorkflowConfig& Config)
{
    // 使用新的节点分析器来分析工作流类型
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    
    // 解析工作流JSON
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Config.JsonTemplate);
    
    if (!FJsonSerializer::Deserialize(Reader, WorkflowJson) || !WorkflowJson.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("AnalyzEComfyUIWorkflowTypeFromConfig: Failed to parse workflow JSON for config: %s"), *Config.Name);
        return EComfyUIWorkflowType::Unknown;
    }
    
    // 分析工作流输入输出
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (!NodeAnalyzer->AnalyzeWorkflow(WorkflowJson, Inputs, Outputs))
    {
        UE_LOG(LogTemp, Error, TEXT("AnalyzEComfyUIWorkflowTypeFromConfig: Failed to analyze workflow for config: %s"), *Config.Name);
        return EComfyUIWorkflowType::Unknown;
    }
    
    // 确定工作流类型
    EComfyUIWorkflowType DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
    
    UE_LOG(LogTemp, Log, TEXT("AnalyzEComfyUIWorkflowTypeFromConfig: Analyzed workflow '%s' - Found %d inputs, %d outputs, Type: %d"), 
           *Config.Name, Inputs.Num(), Outputs.Num(), (int32)DetectedType);
    
    // 打印详细的输入输出信息
    for (const FWorkflowInputInfo& Input : Inputs)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("  Input: %s (%s) - Type: %d - Display: %s"), 
               *Input.ParameterName, *Input.PlaceholderValue, (int32)Input.InputType, *Input.DisplayName);
    }
    
    for (const FWorkflowOutputInfo& Output : Outputs)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("  Output: %s (%s) - Type: %d"), 
               *Output.NodeId, *Output.NodeType, (int32)Output.OutputType);
    }
    
    return DetectedType;
}

bool UComfyUIWorkflowManager::UpdateWorkflowInputOutputInfo(const FString& WorkflowName)
{
    FWorkflowConfig* Config = FindWorkflowConfigInternal(WorkflowName);
    if (!Config)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateWorkflowInputOutputInfo: Workflow config not found: %s"), *WorkflowName);
        return false;
    }
    
    // 使用节点分析器来分析工作流
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    
    // 解析工作流JSON
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Config->JsonTemplate);
    
    if (!FJsonSerializer::Deserialize(Reader, WorkflowJson) || !WorkflowJson.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateWorkflowInputOutputInfo: Failed to parse workflow JSON for: %s"), *WorkflowName);
        return false;
    }
    
    // 分析工作流输入输出
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (!NodeAnalyzer->AnalyzeWorkflow(WorkflowJson, Inputs, Outputs))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateWorkflowInputOutputInfo: Failed to analyze workflow: %s"), *WorkflowName);
        return false;
    }
    
    // 更新配置
    Config->WorkflowInputs = Inputs;
    Config->WorkflowOutputs = Outputs;
    Config->DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
    
    UE_LOG(LogTemp, Log, TEXT("UpdateWorkflowInputOutputInfo: Updated workflow '%s' - %d inputs, %d outputs, type: %d"), 
           *WorkflowName, Inputs.Num(), Outputs.Num(), (int32)Config->DetectedType);
    
    return true;
}

bool UComfyUIWorkflowManager::RunWorkflowTests()
{
    UE_LOG(LogTemp, Warning, TEXT("开始运行ComfyUI工作流测试..."));
    
    // 创建测试器并运行测试
    UComfyUIWorkflowTester* Tester = NewObject<UComfyUIWorkflowTester>(this);
    
    bool bTypeDetectionPassed = Tester->TestWorkflowTypeDetection();
    bool bUIGenerationPassed = Tester->TestUIGeneration();
    
    bool bAllTestsPassed = bTypeDetectionPassed && bUIGenerationPassed;
    
    UE_LOG(LogTemp, Warning, TEXT("ComfyUI工作流测试完成:"));
    UE_LOG(LogTemp, Warning, TEXT("  - 类型检测测试: %s"), bTypeDetectionPassed ? TEXT("通过") : TEXT("失败"));
    UE_LOG(LogTemp, Warning, TEXT("  - UI生成测试: %s"), bUIGenerationPassed ? TEXT("通过") : TEXT("失败"));
    UE_LOG(LogTemp, Warning, TEXT("  - 整体结果: %s"), bAllTestsPassed ? TEXT("通过") : TEXT("失败"));
    
    return bAllTestsPassed;
}
