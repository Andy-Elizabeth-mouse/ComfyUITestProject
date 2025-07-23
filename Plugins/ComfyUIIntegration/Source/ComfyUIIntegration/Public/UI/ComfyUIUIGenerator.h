#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Workflow/ComfyUIWorkflowConfig.h"
#include "ComfyUIUIGenerator.generated.h"

class SWidget;
class SVerticalBox;
class SEditableTextBox;
template<typename NumericType> class SSpinBox;
class SCheckBox;
template<typename OptionType> class SComboBox;

/**
 * UI输入控件信息
 */
struct COMFYUIINTEGRATION_API FUIInputWidget
{
    FString ParameterName;
    FString DisplayName;
    EComfyUINodeInputType InputType;
    FString CurrentValue;
    
    // 对应的UI控件指针（C++层面使用）
    TSharedPtr<SWidget> Widget;
    TSharedPtr<SEditableTextBox> TextWidget;
    TSharedPtr<SSpinBox<float>> NumberWidget;
    TSharedPtr<SCheckBox> BoolWidget;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> ChoiceWidget;

    FUIInputWidget()
        : InputType(EComfyUINodeInputType::Unknown)
    {
    }
};

/**
 * ComfyUI UI生成器
 * 根据工作流输入信息自动生成相应的UI控件
 */
UCLASS(BlueprintType)
class COMFYUIINTEGRATION_API UComfyUIUIGenerator : public UObject
{
    GENERATED_BODY()

public:
    UComfyUIUIGenerator();

    /**
     * 根据工作流输入信息生成UI控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> GenerateWorkflowUI(const TArray<FWorkflowInputInfo>& Inputs);

    /**
     * 获取当前所有输入的值
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|UI")
    TMap<FString, FString> GetAllInputValues() const;

    /**
     * 设置输入值
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|UI")
    bool SetInputValue(const FString& ParameterName, const FString& Value);

    /**
     * 获取输入值
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|UI")
    FString GetInputValue(const FString& ParameterName) const;

    /**
     * 清除所有输入
     */
    UFUNCTION(BlueprintCallable, Category = "ComfyUI|UI")
    void ClearAllInputs();

private:
    /**
     * 创建简化的输入控件（避免复杂的事件处理）
     */
    TSharedRef<SWidget> CreateSimpleInputWidget(const FWorkflowInputInfo& InputInfo);
    /**
     * 为单个输入创建UI控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateInputWidget(const FWorkflowInputInfo& InputInfo);

    /**
     * 创建文本输入控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateTextInput(const FWorkflowInputInfo& InputInfo);

    /**
     * 创建数值输入控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateNumberInput(const FWorkflowInputInfo& InputInfo);

    /**
     * 创建布尔输入控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateBooleanInput(const FWorkflowInputInfo& InputInfo);

    /**
     * 创建选择输入控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateChoiceInput(const FWorkflowInputInfo& InputInfo);

    /**
     * 创建图像输入控件
     * 注意：由于使用TSharedRef，此函数不能标记为UFUNCTION
     */
    TSharedRef<SWidget> CreateImageInput(const FWorkflowInputInfo& InputInfo);

    /**
     * 文本改变回调
     */
    void OnTextChanged(const FText& Text, const FString& ParameterName);

    /**
     * 数值改变回调
     */
    void OnNumberChanged(float Value, const FString& ParameterName);

    /**
     * 布尔值改变回调
     */
    void OnBooleanChanged(ECheckBoxState State, const FString& ParameterName);

    /**
     * 选择改变回调
     */
    void OnChoiceChanged(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo, const FString& ParameterName);

private:
    /** 所有UI输入控件 */
    TArray<FUIInputWidget> UIInputWidgets;

    /** 参数名到控件的映射 */
    TMap<FString, int32> ParameterToWidgetIndex;

    /** 主容器 */
    TSharedPtr<SVerticalBox> MainContainer;
};
