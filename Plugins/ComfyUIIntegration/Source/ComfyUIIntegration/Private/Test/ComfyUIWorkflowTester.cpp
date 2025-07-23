#include "Test/ComfyUIWorkflowTester.h"
#include "Workflow/ComfyUIWorkflowManager.h"
#include "Workflow/ComfyUINodeAnalyzer.h"
#include "UI/ComfyUIUIGenerator.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UComfyUIWorkflowTester::UComfyUIWorkflowTester()
{
}

bool UComfyUIWorkflowTester::TestWorkflowTypeDetection()
{
    UE_LOG(LogTemp, Warning, TEXT("========== 开始工作流类型检测测试 =========="));

    bool bAllTestsPassed = true;

    // 测试基本文生图工作流
    UE_LOG(LogTemp, Log, TEXT("测试 1: 基本文生图工作流"));
    if (!TestBasicTextToImageWorkflow())
    {
        bAllTestsPassed = false;
        UE_LOG(LogTemp, Error, TEXT("基本文生图工作流测试失败"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("基本文生图工作流测试通过"));
    }

    // 测试图生图工作流
    UE_LOG(LogTemp, Log, TEXT("测试 2: 图生图工作流"));
    if (!TestImageToImageWorkflow())
    {
        bAllTestsPassed = false;
        UE_LOG(LogTemp, Error, TEXT("图生图工作流测试失败"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("图生图工作流测试通过"));
    }

    // 测试数字命名节点的工作流（这是用户报告的问题）
    UE_LOG(LogTemp, Log, TEXT("测试 3: 数字命名节点工作流"));
    FString NumberedWorkflow = CreateTestNumberedNodeWorkflow();
    
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    TSharedPtr<FJsonObject> WorkflowJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NumberedWorkflow);
    
    if (FJsonSerializer::Deserialize(Reader, WorkflowJson) && WorkflowJson.IsValid())
    {
        TArray<FWorkflowInputInfo> Inputs;
        TArray<FWorkflowOutputInfo> Outputs;
        
        if (NodeAnalyzer->AnalyzeWorkflow(WorkflowJson, Inputs, Outputs))
        {
            EComfyUIWorkflowType DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
            
            UE_LOG(LogTemp, Log, TEXT("数字命名节点工作流分析结果:"));
            UE_LOG(LogTemp, Log, TEXT("  - 检测到的类型: %d"), (int32)DetectedType);
            UE_LOG(LogTemp, Log, TEXT("  - 输入数量: %d"), Inputs.Num());
            UE_LOG(LogTemp, Log, TEXT("  - 输出数量: %d"), Outputs.Num());
            
            for (const FWorkflowInputInfo& Input : Inputs)
            {
                UE_LOG(LogTemp, Log, TEXT("  - 输入: %s (%s) 类型: %d"), 
                       *Input.ParameterName, *Input.PlaceholderValue, (int32)Input.InputType);
            }
            
            // 验证是否正确检测为文生图
            if (DetectedType == EComfyUIWorkflowType::TextToImage)
            {
                UE_LOG(LogTemp, Log, TEXT("数字命名节点工作流测试通过"));
            }
            else
            {
                bAllTestsPassed = false;
                UE_LOG(LogTemp, Error, TEXT("数字命名节点工作流测试失败 - 期望类型: %d, 实际类型: %d"), 
                       (int32)EComfyUIWorkflowType::TextToImage, (int32)DetectedType);
            }
        }
        else
        {
            bAllTestsPassed = false;
            UE_LOG(LogTemp, Error, TEXT("数字命名节点工作流分析失败"));
        }
    }
    else
    {
        bAllTestsPassed = false;
        UE_LOG(LogTemp, Error, TEXT("数字命名节点工作流JSON解析失败"));
    }

    UE_LOG(LogTemp, Warning, TEXT("========== 工作流类型检测测试完成 =========="));
    UE_LOG(LogTemp, Warning, TEXT("整体测试结果: %s"), bAllTestsPassed ? TEXT("通过") : TEXT("失败"));

    return bAllTestsPassed;
}

bool UComfyUIWorkflowTester::TestBasicTextToImageWorkflow()
{
    FString WorkflowJson = CreateTestTextToImageWorkflow();
    
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    TSharedPtr<FJsonObject> WorkflowData;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowJson);
    
    if (!FJsonSerializer::Deserialize(Reader, WorkflowData) || !WorkflowData.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("TestBasicTextToImageWorkflow: JSON解析失败"));
        return false;
    }
    
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (!NodeAnalyzer->AnalyzeWorkflow(WorkflowData, Inputs, Outputs))
    {
        UE_LOG(LogTemp, Error, TEXT("TestBasicTextToImageWorkflow: 工作流分析失败"));
        return false;
    }
    
    EComfyUIWorkflowType DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
    
    // 验证检测结果
    bool bHasTextInput = false;
    bool bHasImageOutput = false;
    
    for (const FWorkflowInputInfo& Input : Inputs)
    {
        if (Input.InputType == EComfyUINodeInputType::Text)
        {
            bHasTextInput = true;
        }
    }
    
    for (const FWorkflowOutputInfo& Output : Outputs)
    {
        if (Output.OutputType == EComfyUINodeOutputType::Image)
        {
            bHasImageOutput = true;
        }
    }
    
    bool bTestPassed = (DetectedType == EComfyUIWorkflowType::TextToImage) && bHasTextInput && bHasImageOutput;
    
    if (bTestPassed)
    {
        UE_LOG(LogTemp, Log, TEXT("基本文生图工作流测试通过 - 检测到 %d 个输入, %d 个输出"), 
               Inputs.Num(), Outputs.Num());
    }
    
    return bTestPassed;
}

bool UComfyUIWorkflowTester::TestImageToImageWorkflow()
{
    FString WorkflowJson = CreateTestImageToImageWorkflow();
    
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    TSharedPtr<FJsonObject> WorkflowData;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowJson);
    
    if (!FJsonSerializer::Deserialize(Reader, WorkflowData) || !WorkflowData.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("TestImageToImageWorkflow: JSON解析失败"));
        return false;
    }
    
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (!NodeAnalyzer->AnalyzeWorkflow(WorkflowData, Inputs, Outputs))
    {
        UE_LOG(LogTemp, Error, TEXT("TestImageToImageWorkflow: 工作流分析失败"));
        return false;
    }
    
    EComfyUIWorkflowType DetectedType = NodeAnalyzer->DetermineWorkflowType(Inputs, Outputs);
    
    // 验证检测结果
    bool bHasTextInput = false;
    bool bHasImageInput = false;
    bool bHasImageOutput = false;
    
    for (const FWorkflowInputInfo& Input : Inputs)
    {
        if (Input.InputType == EComfyUINodeInputType::Text)
        {
            bHasTextInput = true;
        }
        else if (Input.InputType == EComfyUINodeInputType::Image)
        {
            bHasImageInput = true;
        }
    }
    
    for (const FWorkflowOutputInfo& Output : Outputs)
    {
        if (Output.OutputType == EComfyUINodeOutputType::Image)
        {
            bHasImageOutput = true;
        }
    }
    
    bool bTestPassed = (DetectedType == EComfyUIWorkflowType::ImageToImage) && 
                       bHasTextInput && bHasImageInput && bHasImageOutput;
    
    if (bTestPassed)
    {
        UE_LOG(LogTemp, Log, TEXT("图生图工作流测试通过 - 检测到 %d 个输入, %d 个输出"), 
               Inputs.Num(), Outputs.Num());
    }
    
    return bTestPassed;
}

bool UComfyUIWorkflowTester::TestUIGeneration()
{
    UE_LOG(LogTemp, Warning, TEXT("========== 开始UI生成测试 =========="));

    FString WorkflowJson = CreateTestTextToImageWorkflow();
    
    UComfyUINodeAnalyzer* NodeAnalyzer = NewObject<UComfyUINodeAnalyzer>();
    TSharedPtr<FJsonObject> WorkflowData;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowJson);
    
    if (!FJsonSerializer::Deserialize(Reader, WorkflowData) || !WorkflowData.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("TestUIGeneration: JSON解析失败"));
        return false;
    }
    
    TArray<FWorkflowInputInfo> Inputs;
    TArray<FWorkflowOutputInfo> Outputs;
    
    if (!NodeAnalyzer->AnalyzeWorkflow(WorkflowData, Inputs, Outputs))
    {
        UE_LOG(LogTemp, Error, TEXT("TestUIGeneration: 工作流分析失败"));
        return false;
    }
    
    // 测试UI生成
    UComfyUIUIGenerator* UIGenerator = NewObject<UComfyUIUIGenerator>();
    TSharedRef<SWidget> GeneratedUI = UIGenerator->GenerateWorkflowUI(Inputs);
    
    // 测试设置和获取值
    bool bValueTestPassed = true;
    
    for (const FWorkflowInputInfo& Input : Inputs)
    {
        FString TestValue = FString::Printf(TEXT("Test_%s"), *Input.ParameterName);
        
        if (UIGenerator->SetInputValue(Input.ParameterName, TestValue))
        {
            FString RetrievedValue = UIGenerator->GetInputValue(Input.ParameterName);
            if (RetrievedValue != TestValue)
            {
                bValueTestPassed = false;
                UE_LOG(LogTemp, Error, TEXT("UI值测试失败 - 参数: %s, 设置值: %s, 获取值: %s"), 
                       *Input.ParameterName, *TestValue, *RetrievedValue);
            }
        }
        else
        {
            bValueTestPassed = false;
            UE_LOG(LogTemp, Error, TEXT("UI设置值失败 - 参数: %s"), *Input.ParameterName);
        }
    }
    
    // 测试获取所有值
    TMap<FString, FString> AllValues = UIGenerator->GetAllInputValues();
    
    UE_LOG(LogTemp, Log, TEXT("UI生成测试结果:"));
    UE_LOG(LogTemp, Log, TEXT("  - 生成的输入控件数量: %d"), Inputs.Num());
    UE_LOG(LogTemp, Log, TEXT("  - 值设置/获取测试: %s"), bValueTestPassed ? TEXT("通过") : TEXT("失败"));
    UE_LOG(LogTemp, Log, TEXT("  - 所有值数量: %d"), AllValues.Num());
    
    bool bTestPassed = (Inputs.Num() > 0) && bValueTestPassed && (AllValues.Num() == Inputs.Num());
    
    UE_LOG(LogTemp, Warning, TEXT("UI生成测试结果: %s"), bTestPassed ? TEXT("通过") : TEXT("失败"));
    
    return bTestPassed;
}

void UComfyUIWorkflowTester::PrintWorkflowAnalysis(const FString& WorkflowName)
{
    UComfyUIWorkflowManager* WorkflowManager = NewObject<UComfyUIWorkflowManager>();
    WorkflowManager->LoadWorkflowConfigs();
    
    FWorkflowConfig Config;
    if (WorkflowManager->FindWorkflowConfig(WorkflowName, Config))
    {
        UE_LOG(LogTemp, Warning, TEXT("========== 工作流分析: %s =========="), *WorkflowName);
        UE_LOG(LogTemp, Log, TEXT("工作流类型: %d"), (int32)Config.DetectedType);
        UE_LOG(LogTemp, Log, TEXT("输入数量: %d"), Config.WorkflowInputs.Num());
        UE_LOG(LogTemp, Log, TEXT("输出数量: %d"), Config.WorkflowOutputs.Num());
        
        for (const FWorkflowInputInfo& Input : Config.WorkflowInputs)
        {
            UE_LOG(LogTemp, Log, TEXT("输入: %s - 类型: %d - 显示名: %s"), 
                   *Input.ParameterName, (int32)Input.InputType, *Input.DisplayName);
        }
        
        for (const FWorkflowOutputInfo& Output : Config.WorkflowOutputs)
        {
            UE_LOG(LogTemp, Log, TEXT("输出: %s - 节点类型: %s - 输出类型: %d"), 
                   *Output.NodeId, *Output.NodeType, (int32)Output.OutputType);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("未找到工作流: %s"), *WorkflowName);
    }
}

FString UComfyUIWorkflowTester::CreateTestTextToImageWorkflow()
{
    return TEXT(R"({
        "3": {
            "inputs": {
                "seed": 156680208700286,
                "steps": 20,
                "cfg": 8,
                "sampler_name": "euler",
                "scheduler": "normal",
                "denoise": 1,
                "model": ["4", 0],
                "positive": ["6", 0],
                "negative": ["7", 0],
                "latent_image": ["5", 0]
            },
            "class_type": "KSampler"
        },
        "4": {
            "inputs": {
                "ckpt_name": "v1-5-pruned-emaonly-fp16.safetensors"
            },
            "class_type": "CheckpointLoaderSimple"
        },
        "5": {
            "inputs": {
                "width": 512,
                "height": 512,
                "batch_size": 1
            },
            "class_type": "EmptyLatentImage"
        },
        "6": {
            "inputs": {
                "text": "{POSITIVE_PROMPT}",
                "clip": ["4", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "7": {
            "inputs": {
                "text": "{NEGATIVE_PROMPT}",
                "clip": ["4", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "8": {
            "inputs": {
                "samples": ["3", 0],
                "vae": ["4", 2]
            },
            "class_type": "VAEDecode"
        },
        "9": {
            "inputs": {
                "filename_prefix": "ComfyUI",
                "images": ["8", 0]
            },
            "class_type": "SaveImage"
        }
    })");
}

FString UComfyUIWorkflowTester::CreateTestImageToImageWorkflow()
{
    return TEXT(R"({
        "3": {
            "inputs": {
                "seed": 156680208700286,
                "steps": 20,
                "cfg": 7,
                "sampler_name": "euler",
                "scheduler": "normal",
                "denoise": 0.75,
                "model": ["4", 0],
                "positive": ["6", 0],
                "negative": ["7", 0],
                "latent_image": ["10", 0]
            },
            "class_type": "KSampler"
        },
        "4": {
            "inputs": {
                "ckpt_name": "v1-5-pruned-emaonly-fp16.safetensors"
            },
            "class_type": "CheckpointLoaderSimple"
        },
        "6": {
            "inputs": {
                "text": "{POSITIVE_PROMPT}",
                "clip": ["4", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "7": {
            "inputs": {
                "text": "{NEGATIVE_PROMPT}",
                "clip": ["4", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "8": {
            "inputs": {
                "samples": ["3", 0],
                "vae": ["4", 2]
            },
            "class_type": "VAEDecode"
        },
        "9": {
            "inputs": {
                "filename_prefix": "Img2Img_ComfyUI",
                "images": ["8", 0]
            },
            "class_type": "SaveImage"
        },
        "10": {
            "inputs": {
                "pixels": ["11", 0],
                "vae": ["4", 2]
            },
            "class_type": "VAEEncode"
        },
        "11": {
            "inputs": {
                "image": "{INPUT_IMAGE}"
            },
            "class_type": "LoadImage"
        }
    })");
}

FString UComfyUIWorkflowTester::CreateTestNumberedNodeWorkflow()
{
    // 这是一个模拟的数字命名节点工作流，类似用户测试中使用的
    return TEXT(R"({
        "1": {
            "inputs": {
                "text": "{PROMPT_TEXT}",
                "clip": ["2", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "2": {
            "inputs": {
                "ckpt_name": "model.safetensors"
            },
            "class_type": "CheckpointLoaderSimple"
        },
        "3": {
            "inputs": {
                "width": 512,
                "height": 512,
                "batch_size": 1
            },
            "class_type": "EmptyLatentImage"
        },
        "4": {
            "inputs": {
                "seed": "{SEED_VALUE}",
                "steps": "{INFERENCE_STEPS}",
                "cfg": "{CFG_SCALE}",
                "sampler_name": "euler",
                "scheduler": "normal",
                "denoise": 1,
                "model": ["2", 0],
                "positive": ["1", 0],
                "negative": ["5", 0],
                "latent_image": ["3", 0]
            },
            "class_type": "KSampler"
        },
        "5": {
            "inputs": {
                "text": "{NEGATIVE_PROMPT}",
                "clip": ["2", 1]
            },
            "class_type": "CLIPTextEncode"
        },
        "6": {
            "inputs": {
                "samples": ["4", 0],
                "vae": ["2", 2]
            },
            "class_type": "VAEDecode"
        },
        "7": {
            "inputs": {
                "filename_prefix": "output",
                "images": ["6", 0]
            },
            "class_type": "SaveImage"
        }
    })");
}
