#include "UI/ComfyUIUIGenerator.h"
#include "Workflow/ComfyUINodeAnalyzer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/Texture2D.h"

UComfyUIUIGenerator::UComfyUIUIGenerator()
{
    UIInputWidgets.Empty();
    ParameterToWidgetIndex.Empty();
}

TSharedRef<SWidget> UComfyUIUIGenerator::GenerateWorkflowUI(const TArray<FWorkflowInputInfo>& Inputs)
{
    // 清空现有控件
    UIInputWidgets.Empty();
    ParameterToWidgetIndex.Empty();

    // 创建垂直布局容器
    TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

    // 为每个输入创建对应的UI控件
    for (int32 Index = 0; Index < Inputs.Num(); ++Index)
    {
        const FWorkflowInputInfo& InputInfo = Inputs[Index];
        
        // 创建UI控件信息
        FUIInputWidget UIWidget;
        UIWidget.ParameterName = InputInfo.ParameterName;
        UIWidget.DisplayName = InputInfo.DisplayName;
        UIWidget.InputType = InputInfo.InputType;
        UIWidget.CurrentValue = InputInfo.PlaceholderValue;

        // 根据输入类型创建不同的控件
        TSharedRef<SWidget> InputWidget = CreateSimpleInputWidget(InputInfo);
        UIWidget.Widget = InputWidget;

        // 添加到垂直布局
        VerticalBox->AddSlot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(InputInfo.DisplayName))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(0.7f)
            [
                InputWidget
            ]
        ];

        // 保存控件信息
        UIInputWidgets.Add(UIWidget);
        ParameterToWidgetIndex.Add(InputInfo.ParameterName, Index);
    }

    return VerticalBox;
}

TSharedRef<SWidget> UComfyUIUIGenerator::CreateSimpleInputWidget(const FWorkflowInputInfo& InputInfo)
{
    switch (InputInfo.InputType)
    {
        case EComfyUINodeInputType::Text:
            return SNew(SEditableTextBox)
                .Text(FText::FromString(InputInfo.PlaceholderValue));

        case EComfyUINodeInputType::Number:
            return SNew(SSpinBox<float>)
                .Value(FCString::Atof(*InputInfo.PlaceholderValue));

        case EComfyUINodeInputType::Boolean:
            return SNew(SCheckBox)
                .IsChecked(InputInfo.PlaceholderValue == TEXT("true") ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

        case EComfyUINodeInputType::Choice:
        {
            TArray<TSharedPtr<FString>> Options;
            Options.Add(MakeShareable(new FString(InputInfo.PlaceholderValue)));
            
            return SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&Options)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
                {
                    return SNew(STextBlock).Text(FText::FromString(*Option));
                });
        }

        default:
            return SNew(SEditableTextBox)
                .Text(FText::FromString(InputInfo.PlaceholderValue));
    }
}

bool UComfyUIUIGenerator::SetInputValue(const FString& ParameterName, const FString& Value)
{
    int32* WidgetIndexPtr = ParameterToWidgetIndex.Find(ParameterName);
    if (!WidgetIndexPtr || !UIInputWidgets.IsValidIndex(*WidgetIndexPtr))
    {
        return false;
    }

    FUIInputWidget& UIWidget = UIInputWidgets[*WidgetIndexPtr];
    UIWidget.CurrentValue = Value;

    // 这里应该更新实际的Slate控件，但为了简化暂时只更新内部值
    return true;
}

FString UComfyUIUIGenerator::GetInputValue(const FString& ParameterName) const
{
    const int32* WidgetIndexPtr = ParameterToWidgetIndex.Find(ParameterName);
    if (!WidgetIndexPtr || !UIInputWidgets.IsValidIndex(*WidgetIndexPtr))
    {
        return FString();
    }

    return UIInputWidgets[*WidgetIndexPtr].CurrentValue;
}

TMap<FString, FString> UComfyUIUIGenerator::GetAllInputValues() const
{
    TMap<FString, FString> AllValues;
    
    for (const FUIInputWidget& UIWidget : UIInputWidgets)
    {
        AllValues.Add(UIWidget.ParameterName, UIWidget.CurrentValue);
    }
    
    return AllValues;
}

void UComfyUIUIGenerator::ClearAllInputs()
{
    UIInputWidgets.Empty();
    ParameterToWidgetIndex.Empty();
}
