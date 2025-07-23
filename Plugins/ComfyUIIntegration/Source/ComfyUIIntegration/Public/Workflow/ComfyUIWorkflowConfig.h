#pragma once

#include "CoreMinimal.h"
#include "ComfyUIWorkflowConfig.generated.h"

/**
 * ComfyUI节点输入类型
 */
UENUM(BlueprintType)
enum class EComfyUINodeInputType : uint8
{
    Text        UMETA(DisplayName = "文本"),
    Image       UMETA(DisplayName = "图像"),
    Number      UMETA(DisplayName = "数值"),
    Boolean     UMETA(DisplayName = "布尔"),
    Choice      UMETA(DisplayName = "选择"),
    Model       UMETA(DisplayName = "模型"),
    VAE         UMETA(DisplayName = "VAE"),
    CLIP        UMETA(DisplayName = "CLIP"),
    Conditioning UMETA(DisplayName = "条件"),
    Latent      UMETA(DisplayName = "潜空间"),
    Unknown     UMETA(DisplayName = "未知")
};

/**
 * ComfyUI节点输出类型
 */
UENUM(BlueprintType)
enum class EComfyUINodeOutputType : uint8
{
    Image       UMETA(DisplayName = "图像"),
    Mesh        UMETA(DisplayName = "网格"),
    Model       UMETA(DisplayName = "模型"),
    Texture     UMETA(DisplayName = "纹理"),
    Material    UMETA(DisplayName = "材质"),
    Video       UMETA(DisplayName = "视频"),
    Audio       UMETA(DisplayName = "音频"),
    Unknown     UMETA(DisplayName = "未知")
};

/**
 * 工作流输入参数信息
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FWorkflowInputInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString NodeId;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString ParameterName;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    EComfyUINodeInputType InputType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString PlaceholderValue;  // 如 {POSITIVE_PROMPT}, {INPUT_IMAGE}
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString DisplayName;       // UI显示名称
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Description;       // 参数描述
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    bool bRequired = true;     // 是否必需
    
    // 数值参数的范围
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    float MinValue = 0.0f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    float MaxValue = 100.0f;
    
    // 选择参数的选项
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<FString> ChoiceOptions;

    FWorkflowInputInfo()
        : InputType(EComfyUINodeInputType::Unknown)
        , bRequired(true)
        , MinValue(0.0f)
        , MaxValue(100.0f)
    {
    }
};

/**
 * 工作流输出信息
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FWorkflowOutputInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString NodeId;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString NodeType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    EComfyUINodeOutputType OutputType;

    FWorkflowOutputInfo()
        : OutputType(EComfyUINodeOutputType::Unknown)
    {
    }
};

/**
 * 工作流类型枚举
 */
UENUM(BlueprintType)
enum class EComfyUIWorkflowType : uint8
{
    TextToImage         UMETA(DisplayName = "文生图"),
    ImageToImage        UMETA(DisplayName = "图生图"),
    TextTo3D           UMETA(DisplayName = "文生3D"),
    ImageTo3D          UMETA(DisplayName = "图生3D"),
    TextureGeneration   UMETA(DisplayName = "纹理生成"),
    Custom             UMETA(DisplayName = "自定义工作流"),
    Unknown            UMETA(DisplayName = "未知")
};

/**
 * 工作流参数定义结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FWorkflowParameterDef
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Name;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Type;        // text, number, boolean, choice
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString DefaultValue;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Description;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<FString> ChoiceOptions; // 当Type为choice时的选项
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    float MinValue = 0.0f;         // 当Type为number时的最小值
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    float MaxValue = 100.0f;       // 当Type为number时的最大值

    // 默认构造函数
    FWorkflowParameterDef()
        : MinValue(0.0f), MaxValue(100.0f)
    {
    }
};

/**
 * ComfyUI工作流配置结构
 */
USTRUCT(BlueprintType)
struct COMFYUIINTEGRATION_API FWorkflowConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Name;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Type;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString Description;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString JsonTemplate;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TMap<FString, FString> Parameters;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString TemplateFile;
    
    // 工作流验证信息
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    bool bIsValid = false;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    FString ValidationError;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<FString> RequiredInputs;  // 必需的输入参数
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<FString> OutputNodes;     // 输出节点ID列表
    
    // 检测到的工作流类型
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    EComfyUIWorkflowType DetectedType = EComfyUIWorkflowType::Unknown;
    
    // 参数定义
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<FWorkflowParameterDef> ParameterDefinitions;
    
    // 分析得到的输入输出信息（用于自动生成UI）
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<struct FWorkflowInputInfo> WorkflowInputs;
    
    UPROPERTY(BlueprintReadWrite, Category = "Workflow")
    TArray<struct FWorkflowOutputInfo> WorkflowOutputs;
    
    // 默认构造函数
    FWorkflowConfig()
        : bIsValid(false)
    {
    }
};
