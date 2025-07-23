#include "Workflow/ComfyUINodeAnalyzer.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UComfyUINodeAnalyzer::UComfyUINodeAnalyzer()
{
    // 不在构造函数中初始化映射，避免访问违例
    // InitializeNodeTypeMappings() 将在第一次使用时调用
    bIsInitialized = false;
}

void UComfyUINodeAnalyzer::EnsureInitialized()
{
    if (!bIsInitialized)
    {
        InitializeNodeTypeMappings();
        bIsInitialized = true;
    }
}

void UComfyUINodeAnalyzer::InitializeNodeTypeMappings()
{
    // 清空映射
    NodeTypeToOutputType.Empty();
    TextInputParameters.Empty();
    ImageInputParameters.Empty();
    NumberInputParameters.Empty();

    // 初始化节点类型到输出类型的映射
    NodeTypeToOutputType.Add(TEXT("SaveImage"), EComfyUINodeOutputType::Image);
    NodeTypeToOutputType.Add(TEXT("PreviewImage"), EComfyUINodeOutputType::Image);
    NodeTypeToOutputType.Add(TEXT("VaeImageOutput"), EComfyUINodeOutputType::Image);
    NodeTypeToOutputType.Add(TEXT("VAEDecode"), EComfyUINodeOutputType::Image);
    
    // 3D输出节点
    NodeTypeToOutputType.Add(TEXT("SaveMesh"), EComfyUINodeOutputType::Mesh);
    NodeTypeToOutputType.Add(TEXT("MeshExport"), EComfyUINodeOutputType::Mesh);
    NodeTypeToOutputType.Add(TEXT("GLBExport"), EComfyUINodeOutputType::Mesh);
    NodeTypeToOutputType.Add(TEXT("OBJExport"), EComfyUINodeOutputType::Mesh);
    NodeTypeToOutputType.Add(TEXT("PLYExport"), EComfyUINodeOutputType::Mesh);
    
    // 纹理/材质输出节点
    NodeTypeToOutputType.Add(TEXT("SaveTexture"), EComfyUINodeOutputType::Texture);
    NodeTypeToOutputType.Add(TEXT("SaveMaterial"), EComfyUINodeOutputType::Material);
    NodeTypeToOutputType.Add(TEXT("TextureOutput"), EComfyUINodeOutputType::Texture);
    NodeTypeToOutputType.Add(TEXT("MaterialOutput"), EComfyUINodeOutputType::Material);

    // 初始化已知的输入参数类型
    
    // 文本参数
    TextInputParameters.Add(TEXT("text"));
    TextInputParameters.Add(TEXT("prompt"));
    TextInputParameters.Add(TEXT("positive"));
    TextInputParameters.Add(TEXT("negative"));
    TextInputParameters.Add(TEXT("description"));
    TextInputParameters.Add(TEXT("positive_prompt"));
    TextInputParameters.Add(TEXT("negative_prompt"));
    TextInputParameters.Add(TEXT("filename_prefix"));
    TextInputParameters.Add(TEXT("ckpt_name"));
    TextInputParameters.Add(TEXT("sampler_name"));
    TextInputParameters.Add(TEXT("scheduler"));

    // 图像参数
    ImageInputParameters.Add(TEXT("image"));
    ImageInputParameters.Add(TEXT("input_image"));
    ImageInputParameters.Add(TEXT("source_image"));
    ImageInputParameters.Add(TEXT("init_image"));
    ImageInputParameters.Add(TEXT("mask"));
    ImageInputParameters.Add(TEXT("pixels"));
    ImageInputParameters.Add(TEXT("images"));

    // 数值参数
    NumberInputParameters.Add(TEXT("seed"));
    NumberInputParameters.Add(TEXT("steps"));
    NumberInputParameters.Add(TEXT("cfg"));
    NumberInputParameters.Add(TEXT("denoise"));
    NumberInputParameters.Add(TEXT("strength"));
    NumberInputParameters.Add(TEXT("scale"));
    NumberInputParameters.Add(TEXT("width"));
    NumberInputParameters.Add(TEXT("height"));
    NumberInputParameters.Add(TEXT("batch_size"));
    NumberInputParameters.Add(TEXT("guidance_scale"));
    NumberInputParameters.Add(TEXT("num_inference_steps"));
    NumberInputParameters.Add(TEXT("noise_level"));
}

bool UComfyUINodeAnalyzer::AnalyzeWorkflow(TSharedPtr<FJsonObject> WorkflowJson, TArray<FWorkflowInputInfo>& OutInputs, TArray<FWorkflowOutputInfo>& OutOutputs)
{
    // 确保映射已初始化
    EnsureInitialized();
    
    if (!WorkflowJson.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UComfyUINodeAnalyzer::AnalyzeWorkflow - Invalid workflow JSON"));
        return false;
    }

    OutInputs.Empty();
    OutOutputs.Empty();

    // 遍历所有节点
    for (const auto& NodePair : WorkflowJson->Values)
    {
        const FString& NodeId = NodePair.Key;
        TSharedPtr<FJsonValue> NodeValue = NodePair.Value;

        if (NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
            if (!AnalyzeNode(NodeId, NodeObj, OutInputs, OutOutputs))
            {
                UE_LOG(LogTemp, Warning, TEXT("UComfyUINodeAnalyzer::AnalyzeWorkflow - Failed to analyze node: %s"), *NodeId);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UComfyUINodeAnalyzer::AnalyzeWorkflow - Found %d inputs and %d outputs"), 
           OutInputs.Num(), OutOutputs.Num());

    return true;
}

bool UComfyUINodeAnalyzer::AnalyzeNode(const FString& NodeId, TSharedPtr<FJsonObject> NodeData, TArray<FWorkflowInputInfo>& OutInputs, TArray<FWorkflowOutputInfo>& OutOutputs)
{
    if (!NodeData.IsValid())
    {
        return false;
    }

    // 获取节点类型
    FString ClassType;
    if (!NodeData->TryGetStringField(TEXT("class_type"), ClassType))
    {
        UE_LOG(LogTemp, Warning, TEXT("UComfyUINodeAnalyzer::AnalyzeNode - Node %s missing class_type"), *NodeId);
        return false;
    }

    // 检查是否为输出节点
    EComfyUINodeOutputType* OutputType = NodeTypeToOutputType.Find(ClassType);
    if (OutputType != nullptr)
    {
        FWorkflowOutputInfo OutputInfo;
        OutputInfo.NodeId = NodeId;
        OutputInfo.NodeType = ClassType;
        OutputInfo.OutputType = *OutputType;
        OutOutputs.Add(OutputInfo);
    }

    // 分析输入参数
    const TSharedPtr<FJsonObject>* InputsObjPtr;
    if (NodeData->TryGetObjectField(TEXT("inputs"), InputsObjPtr) && InputsObjPtr)
    {
        TSharedPtr<FJsonObject> InputsObj = *InputsObjPtr;
        for (const auto& InputPair : InputsObj->Values)
        {
            const FString& ParameterName = InputPair.Key;
            TSharedPtr<FJsonValue> InputValue = InputPair.Value;

            // 只分析字符串值，查找占位符
            if (InputValue->Type == EJson::String)
            {
                FString StringValue = InputValue->AsString();
                
                if (IsPlaceholderValue(StringValue))
                {
                    FWorkflowInputInfo InputInfo;
                    InputInfo.NodeId = NodeId;
                    InputInfo.ParameterName = ParameterName;
                    InputInfo.PlaceholderValue = StringValue;
                    InputInfo.InputType = DetermineInputType(ClassType, ParameterName, StringValue);
                    InputInfo.DisplayName = GenerateDisplayName(ParameterName, InputInfo.InputType);
                    
                    // 设置参数的描述和约束
                    SetParameterConstraints(InputInfo, ClassType, ParameterName);
                    
                    OutInputs.Add(InputInfo);
                }
            }
            // 分析数值参数（可能也需要用户输入）
            else if (InputValue->Type == EJson::Number)
            {
                // 检查是否为可调节的数值参数
                if (NumberInputParameters.Contains(ParameterName))
                {
                    FWorkflowInputInfo InputInfo;
                    InputInfo.NodeId = NodeId;
                    InputInfo.ParameterName = ParameterName;
                    InputInfo.PlaceholderValue = FString::Printf(TEXT("{%s}"), *ParameterName.ToUpper());
                    InputInfo.InputType = EComfyUINodeInputType::Number;
                    InputInfo.DisplayName = GenerateDisplayName(ParameterName, InputInfo.InputType);
                    
                    SetParameterConstraints(InputInfo, ClassType, ParameterName);
                    
                    OutInputs.Add(InputInfo);
                }
            }
        }
    }

    return true;
}

void UComfyUINodeAnalyzer::SetParameterConstraints(FWorkflowInputInfo& InputInfo, const FString& NodeType, const FString& ParameterName)
{
    // 根据参数名称和节点类型设置约束
    if (ParameterName == TEXT("steps") || ParameterName == TEXT("num_inference_steps"))
    {
        InputInfo.MinValue = 1.0f;
        InputInfo.MaxValue = 100.0f;
        InputInfo.Description = TEXT("推理步数，影响生成质量和速度");
    }
    else if (ParameterName == TEXT("cfg") || ParameterName == TEXT("guidance_scale"))
    {
        InputInfo.MinValue = 1.0f;
        InputInfo.MaxValue = 20.0f;
        InputInfo.Description = TEXT("CFG引导强度，控制模型遵循提示词的程度");
    }
    else if (ParameterName == TEXT("denoise") || ParameterName == TEXT("strength"))
    {
        InputInfo.MinValue = 0.0f;
        InputInfo.MaxValue = 1.0f;
        InputInfo.Description = TEXT("去噪强度，值越高改变越大");
    }
    else if (ParameterName == TEXT("width") || ParameterName == TEXT("height"))
    {
        InputInfo.MinValue = 64.0f;
        InputInfo.MaxValue = 2048.0f;
        InputInfo.Description = TEXT("图像尺寸（像素）");
    }
    else if (ParameterName == TEXT("batch_size"))
    {
        InputInfo.MinValue = 1.0f;
        InputInfo.MaxValue = 8.0f;
        InputInfo.Description = TEXT("批次大小，一次生成的图像数量");
    }
    else if (ParameterName == TEXT("seed"))
    {
        InputInfo.MinValue = -1.0f;
        InputInfo.MaxValue = 2147483647.0f;
        InputInfo.Description = TEXT("随机种子，-1为随机");
    }
    else if (ParameterName.Contains(TEXT("prompt")) || ParameterName.Contains(TEXT("text")))
    {
        InputInfo.Description = TEXT("文本提示词");
    }
    else if (ParameterName.Contains(TEXT("image")))
    {
        InputInfo.Description = TEXT("输入图像");
    }
    
    // 添加采样器选择
    if (ParameterName == TEXT("sampler_name"))
    {
        InputInfo.InputType = EComfyUINodeInputType::Choice;
        InputInfo.ChoiceOptions = {
            TEXT("euler"), TEXT("euler_ancestral"), TEXT("heun"), TEXT("dpm_2"), TEXT("dpm_2_ancestral"),
            TEXT("lms"), TEXT("dpm_fast"), TEXT("dpm_adaptive"), TEXT("dpmpp_2s_ancestral"), TEXT("dpmpp_sde"),
            TEXT("dpmpp_2m"), TEXT("ddim"), TEXT("uni_pc"), TEXT("uni_pc_bh2")
        };
        InputInfo.Description = TEXT("采样器类型");
    }
    else if (ParameterName == TEXT("scheduler"))
    {
        InputInfo.InputType = EComfyUINodeInputType::Choice;
        InputInfo.ChoiceOptions = {
            TEXT("normal"), TEXT("karras"), TEXT("exponential"), TEXT("polyexponential"), TEXT("sgm_uniform")
        };
        InputInfo.Description = TEXT("调度器类型");
    }
}

bool UComfyUINodeAnalyzer::IsPlaceholderValue(const FString& Value)
{
    return Value.StartsWith(TEXT("{")) && Value.EndsWith(TEXT("}"));
}

FString UComfyUINodeAnalyzer::ExtractPlaceholderName(const FString& Value)
{
    if (IsPlaceholderValue(Value))
    {
        return Value.Mid(1, Value.Len() - 2);  // 移除首尾的大括号
    }
    return Value;
}

EComfyUINodeInputType UComfyUINodeAnalyzer::DetermineInputType(const FString& NodeType, const FString& ParameterName, const FString& Value)
{
    FString LowerParameterName = ParameterName.ToLower();
    
    // 检查已知的参数类型
    if (TextInputParameters.Contains(LowerParameterName))
    {
        return EComfyUINodeInputType::Text;
    }
    
    if (ImageInputParameters.Contains(LowerParameterName))
    {
        return EComfyUINodeInputType::Image;
    }
    
    if (NumberInputParameters.Contains(LowerParameterName))
    {
        return EComfyUINodeInputType::Number;
    }
    
    // 基于占位符名称推断
    if (IsPlaceholderValue(Value))
    {
        FString PlaceholderName = ExtractPlaceholderName(Value).ToLower();
        
        if (PlaceholderName.Contains(TEXT("prompt")) || PlaceholderName.Contains(TEXT("text")))
        {
            return EComfyUINodeInputType::Text;
        }
        
        if (PlaceholderName.Contains(TEXT("image")))
        {
            return EComfyUINodeInputType::Image;
        }
    }
    
    // 基于参数名称的模糊匹配
    if (LowerParameterName.Contains(TEXT("prompt")) || 
        LowerParameterName.Contains(TEXT("text")) ||
        LowerParameterName.Contains(TEXT("description")))
    {
        return EComfyUINodeInputType::Text;
    }
    
    if (LowerParameterName.Contains(TEXT("image")) ||
        LowerParameterName.Contains(TEXT("img")) ||
        LowerParameterName.Contains(TEXT("pixel")))
    {
        return EComfyUINodeInputType::Image;
    }
    
    return EComfyUINodeInputType::Unknown;
}

EComfyUINodeOutputType UComfyUINodeAnalyzer::DetermineOutputType(const FString& NodeType)
{
    EComfyUINodeOutputType* OutputType = NodeTypeToOutputType.Find(NodeType);
    if (OutputType != nullptr)
    {
        return *OutputType;
    }
    
    // 基于节点类型名称的模糊匹配
    FString LowerNodeType = NodeType.ToLower();
    
    if (LowerNodeType.Contains(TEXT("save")) && LowerNodeType.Contains(TEXT("image")))
    {
        return EComfyUINodeOutputType::Image;
    }
    
    if (LowerNodeType.Contains(TEXT("mesh")) || 
        LowerNodeType.Contains(TEXT("3d")) ||
        LowerNodeType.Contains(TEXT("obj")) ||
        LowerNodeType.Contains(TEXT("ply")) ||
        LowerNodeType.Contains(TEXT("glb")))
    {
        return EComfyUINodeOutputType::Mesh;
    }
    
    if (LowerNodeType.Contains(TEXT("texture")) || LowerNodeType.Contains(TEXT("material")))
    {
        return EComfyUINodeOutputType::Texture;
    }
    
    return EComfyUINodeOutputType::Unknown;
}

FString UComfyUINodeAnalyzer::GenerateDisplayName(const FString& ParameterName, EComfyUINodeInputType InputType)
{
    FString LowerName = ParameterName.ToLower();
    
    // 特定参数的中文显示名称
    if (LowerName == TEXT("positive") || LowerName == TEXT("positive_prompt"))
    {
        return TEXT("正向提示词");
    }
    else if (LowerName == TEXT("negative") || LowerName == TEXT("negative_prompt"))
    {
        return TEXT("负向提示词");
    }
    else if (LowerName == TEXT("text") || LowerName == TEXT("prompt"))
    {
        return TEXT("提示词");
    }
    else if (LowerName == TEXT("image") || LowerName == TEXT("input_image"))
    {
        return TEXT("输入图像");
    }
    else if (LowerName == TEXT("steps") || LowerName == TEXT("num_inference_steps"))
    {
        return TEXT("推理步数");
    }
    else if (LowerName == TEXT("cfg") || LowerName == TEXT("guidance_scale"))
    {
        return TEXT("CFG引导强度");
    }
    else if (LowerName == TEXT("denoise") || LowerName == TEXT("strength"))
    {
        return TEXT("去噪强度");
    }
    else if (LowerName == TEXT("width"))
    {
        return TEXT("宽度");
    }
    else if (LowerName == TEXT("height"))
    {
        return TEXT("高度");
    }
    else if (LowerName == TEXT("batch_size"))
    {
        return TEXT("批次大小");
    }
    else if (LowerName == TEXT("seed"))
    {
        return TEXT("随机种子");
    }
    else if (LowerName == TEXT("sampler_name"))
    {
        return TEXT("采样器");
    }
    else if (LowerName == TEXT("scheduler"))
    {
        return TEXT("调度器");
    }
    
    // 基于输入类型生成默认名称
    switch (InputType)
    {
    case EComfyUINodeInputType::Text:
        return FString::Printf(TEXT("文本参数 (%s)"), *ParameterName);
    case EComfyUINodeInputType::Image:
        return FString::Printf(TEXT("图像参数 (%s)"), *ParameterName);
    case EComfyUINodeInputType::Number:
        return FString::Printf(TEXT("数值参数 (%s)"), *ParameterName);
    case EComfyUINodeInputType::Boolean:
        return FString::Printf(TEXT("布尔参数 (%s)"), *ParameterName);
    case EComfyUINodeInputType::Choice:
        return FString::Printf(TEXT("选择参数 (%s)"), *ParameterName);
    default:
        return ParameterName;
    }
}
#pragma optimize("", off)
EComfyUIWorkflowType UComfyUINodeAnalyzer::DetermineWorkflowType(const TArray<FWorkflowInputInfo>& Inputs, const TArray<FWorkflowOutputInfo>& Outputs)
{
    // 确保映射已初始化
    EnsureInitialized();
    
    // 统计输入类型
    bool bHasTextInput = false;
    bool bHasImageInput = false;
    int32 TextInputCount = 0;
    int32 ImageInputCount = 0;
    
    for (const FWorkflowInputInfo& Input : Inputs)
    {
        if (Input.InputType == EComfyUINodeInputType::Text)
        {
            bHasTextInput = true;
            TextInputCount++;
        }
        else if (Input.InputType == EComfyUINodeInputType::Image)
        {
            bHasImageInput = true;
            ImageInputCount++;
        }
    }
    
    // 统计输出类型
    bool bHasImageOutput = false;
    bool bHasMeshOutput = false;
    bool bHasTextureOutput = false;
    
    for (const FWorkflowOutputInfo& Output : Outputs)
    {
        switch (Output.OutputType)
        {
        case EComfyUINodeOutputType::Image:
            bHasImageOutput = true;
            break;
        case EComfyUINodeOutputType::Mesh:
            bHasMeshOutput = true;
            break;
        case EComfyUINodeOutputType::Texture:
        case EComfyUINodeOutputType::Material:
            bHasTextureOutput = true;
            break;
        default:
            break;
        }
    }
    
    // 基于输入输出组合确定工作流类型
    if (bHasMeshOutput)
    {
        // 3D输出工作流
        if (bHasImageInput)
        {
            return EComfyUIWorkflowType::ImageTo3D;
        }
        else if (bHasTextInput)
        {
            return EComfyUIWorkflowType::TextTo3D;
        }
    }
    else if (bHasTextureOutput)
    {
        // 纹理生成工作流
        return EComfyUIWorkflowType::TextureGeneration;
    }
    else if (bHasImageOutput)
    {
        // 图像输出工作流
        if (bHasImageInput && bHasTextInput)
        {
            // 既有图像输入又有文本输入，通常是带提示词的图生图
            return EComfyUIWorkflowType::ImageToImage;
        }
        else if (bHasImageInput)
        {
            // 仅图像输入
            return EComfyUIWorkflowType::ImageToImage;
        }
        else if (bHasTextInput)
        {
            // 仅文本输入
            return EComfyUIWorkflowType::TextToImage;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UComfyUINodeAnalyzer::DetermineWorkflowType - Unable to determine workflow type. Inputs: %d text, %d image. Outputs: %s%s%s"), 
           TextInputCount, ImageInputCount,
           bHasImageOutput ? TEXT("Image ") : TEXT(""),
           bHasMeshOutput ? TEXT("Mesh ") : TEXT(""),
           bHasTextureOutput ? TEXT("Texture ") : TEXT(""));
    
    return EComfyUIWorkflowType::Unknown;
}
#pragma optimize("", on)