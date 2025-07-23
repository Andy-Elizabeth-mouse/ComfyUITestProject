#include "Utils/ComfyUIConfigManager.h"
#include "Utils/ComfyUIFileManager.h"
#include "Utils/Defines.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString UComfyUIConfigManager::ConfigFilePath;
TSharedPtr<FJsonObject> UComfyUIConfigManager::DefaultConfigJson;

bool UComfyUIConfigManager::LoadDefaultConfig()
{
    if (!EnsureConfigFileExists())
        LOG_AND_RETURN(Error, false, "Failed to ensure config file exists");

    FString ConfigContent;
    if (!UComfyUIFileManager::LoadJsonFromFile(ConfigFilePath, ConfigContent))
        LOG_AND_RETURN(Error, false, "Failed to load config file: %s", *ConfigFilePath);

    if (!UComfyUIFileManager::ParseJsonString(ConfigContent, DefaultConfigJson))
        LOG_AND_RETURN(Error, false, "Failed to parse config JSON");

    return true;
}

bool UComfyUIConfigManager::SaveDefaultConfig()
{
    if (!DefaultConfigJson.IsValid())
        DefaultConfigJson = CreateDefaultConfigJson();

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (!FJsonSerializer::Serialize(DefaultConfigJson.ToSharedRef(), Writer))
        LOG_AND_RETURN(Error, false, "Failed to serialize config JSON");

    return UComfyUIFileManager::SaveJsonToFile(OutputString, ConfigFilePath);
}

TArray<FWorkflowConfig> UComfyUIConfigManager::LoadWorkflowConfigs()
{
    TArray<FWorkflowConfig> Configs;

    if (!LoadDefaultConfig()) return Configs;

    // 从配置JSON中读取工作流配置
    if (DefaultConfigJson->HasField(TEXT("workflows")))
    {
        const TArray<TSharedPtr<FJsonValue>>* WorkflowArray;
        if (DefaultConfigJson->TryGetArrayField(TEXT("workflows"), WorkflowArray))
        {
            for (const TSharedPtr<FJsonValue>& WorkflowValue : *WorkflowArray)
            {
                const TSharedPtr<FJsonObject>* WorkflowObject;
                if (WorkflowValue->TryGetObject(WorkflowObject))
                {
                    FWorkflowConfig Config;
                    Config.Name = (*WorkflowObject)->GetStringField(TEXT("name"));
                    Config.Type = (*WorkflowObject)->GetStringField(TEXT("type"));
                    Config.Description = (*WorkflowObject)->GetStringField(TEXT("description"));
                    Config.JsonTemplate = (*WorkflowObject)->GetStringField(TEXT("json_template"));
                    Config.TemplateFile = (*WorkflowObject)->GetStringField(TEXT("template_file"));
                    
                    Configs.Add(Config);
                }
            }
        }
    }

    return Configs;
}

bool UComfyUIConfigManager::SaveWorkflowConfig(const FWorkflowConfig& Config)
{
    if (!LoadDefaultConfig()) return false;

    // 获取或创建工作流数组
    TArray<TSharedPtr<FJsonValue>> WorkflowArray;
    if (DefaultConfigJson->HasField(TEXT("workflows")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ExistingArray;
        if (DefaultConfigJson->TryGetArrayField(TEXT("workflows"), ExistingArray))
            WorkflowArray = *ExistingArray;
    }

    // 创建工作流JSON对象
    TSharedPtr<FJsonObject> WorkflowObject = MakeShareable(new FJsonObject);
    WorkflowObject->SetStringField(TEXT("name"), Config.Name);
    WorkflowObject->SetStringField(TEXT("type"), Config.Type);
    WorkflowObject->SetStringField(TEXT("description"), Config.Description);
    WorkflowObject->SetStringField(TEXT("json_template"), Config.JsonTemplate);
    WorkflowObject->SetStringField(TEXT("template_file"), Config.TemplateFile);

    WorkflowArray.Add(MakeShareable(new FJsonValueObject(WorkflowObject)));
    DefaultConfigJson->SetArrayField(TEXT("workflows"), WorkflowArray);

    return SaveDefaultConfig();
}

bool UComfyUIConfigManager::GetServerSettings(FString& OutServerAddress, int32& OutServerPort)
{
    if (!LoadDefaultConfig()) return false;

    if (DefaultConfigJson->HasField(TEXT("server_settings")))
    {
        const TSharedPtr<FJsonObject>* ServerSettings;
        if (DefaultConfigJson->TryGetObjectField(TEXT("server_settings"), ServerSettings))
        {
            OutServerAddress = (*ServerSettings)->GetStringField(TEXT("address"));
            OutServerPort = (*ServerSettings)->GetIntegerField(TEXT("port"));
            return true;
        }
    }

    // 使用默认值
    OutServerAddress = TEXT("127.0.0.1");
    OutServerPort = 8188;
    return false;
}

bool UComfyUIConfigManager::SetServerSettings(const FString& ServerAddress, int32 ServerPort)
{
    if (!LoadDefaultConfig()) return false;

    TSharedPtr<FJsonObject> ServerSettings = MakeShareable(new FJsonObject);
    ServerSettings->SetStringField(TEXT("address"), ServerAddress);
    ServerSettings->SetNumberField(TEXT("port"), ServerPort);
    
    DefaultConfigJson->SetObjectField(TEXT("server_settings"), ServerSettings);

    return SaveDefaultConfig();
}

bool UComfyUIConfigManager::EnsureConfigFileExists()
{
    if (ConfigFilePath.IsEmpty())
        ConfigFilePath = UComfyUIFileManager::GetConfigDirectory() / TEXT("default_config.json");

    if (!FPaths::FileExists(ConfigFilePath))
    {
        // 创建默认配置文件
        DefaultConfigJson = CreateDefaultConfigJson();
        return SaveDefaultConfig();
    }

    return true;
}

TSharedPtr<FJsonObject> UComfyUIConfigManager::CreateDefaultConfigJson()
{
    TSharedPtr<FJsonObject> ConfigJson = MakeShareable(new FJsonObject);

    // 服务器设置
    TSharedPtr<FJsonObject> ServerSettings = MakeShareable(new FJsonObject);
    ServerSettings->SetStringField(TEXT("address"), TEXT("127.0.0.1"));
    ServerSettings->SetNumberField(TEXT("port"), 8188);
    ConfigJson->SetObjectField(TEXT("server_settings"), ServerSettings);

    // 工作流配置数组
    TArray<TSharedPtr<FJsonValue>> WorkflowArray;
    ConfigJson->SetArrayField(TEXT("workflows"), WorkflowArray);

    // 其他设置
    ConfigJson->SetBoolField(TEXT("auto_save_generated_images"), true);
    ConfigJson->SetStringField(TEXT("default_output_format"), TEXT("png"));

    return ConfigJson;
}
