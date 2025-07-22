#pragma once

#include "CoreMinimal.h"

/**
 * ComfyUI工作流配置结构
 */
struct COMFYUIINTEGRATION_API FWorkflowConfig
{
    FString Name;
    FString Type;
    FString Description;
    FString JsonTemplate;
    TMap<FString, FString> Parameters;
    FString TemplateFile;
    
    // 工作流验证信息
    bool bIsValid = false;
    FString ValidationError;
    TArray<FString> RequiredInputs;  // 必需的输入参数
    TArray<FString> OutputNodes;     // 输出节点ID列表
    
    // 参数定义
    struct FParameterDef
    {
        FString Name;
        FString Type;        // text, number, boolean, choice
        FString DefaultValue;
        FString Description;
        TArray<FString> ChoiceOptions; // 当Type为choice时的选项
        float MinValue = 0.0f;         // 当Type为number时的最小值
        float MaxValue = 100.0f;       // 当Type为number时的最大值
    };
    TArray<FParameterDef> ParameterDefinitions;
    
    // 默认构造函数
    FWorkflowConfig()
        : bIsValid(false)
    {
    }
};
