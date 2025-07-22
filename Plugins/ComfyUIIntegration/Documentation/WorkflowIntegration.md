# ComfyUI工作流集成指南

本文档介绍如何将现有的ComfyUI工作流集成到Unreal Engine插件中。

## ComfyUI工作流基础

### 工作流结构
ComfyUI工作流本质上是一个JSON对象，定义了节点之间的连接关系：

```json
{
    "node_id": {
        "inputs": {
            "parameter_name": "value_or_connection",
            "model": ["source_node_id", output_index]
        },
        "class_type": "NodeClassName"
    }
}
```

### 核心节点类型

1. **模型加载节点**
   - `CheckpointLoaderSimple`: 加载基础模型
   - `LoraLoader`: 加载LoRA模型
   - `VAELoader`: 加载VAE模型

2. **文本编码节点**
   - `CLIPTextEncode`: 编码正向/负向提示词

3. **采样节点**
   - `KSampler`: 基础采样器
   - `KSamplerAdvanced`: 高级采样器

4. **图像处理节点**
   - `VAEDecode`: 将潜在空间解码为图像
   - `VAEEncode`: 将图像编码到潜在空间
   - `LoadImage`: 加载输入图像
   - `SaveImage`: 保存生成图像

## 工作流集成步骤

### 1. 导出ComfyUI工作流

在ComfyUI界面中：
1. 构建完整的工作流
2. 点击"Save"或使用API导出
3. 获得JSON格式的工作流定义

### 2. 转换为插件模板

将ComfyUI工作流转换为插件可用的模板：

```cpp
FString WorkflowTemplate = TEXT(R"({
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
    "6": {
        "inputs": {
            "text": "{POSITIVE_PROMPT}",
            "clip": ["4", 1]
        },
        "class_type": "CLIPTextEncode"
    }
})");
```

### 3. 添加占位符支持

在模板中使用占位符来支持动态参数：

| 占位符 | 描述 | 使用场景 |
|--------|------|----------|
| `{POSITIVE_PROMPT}` | 正向提示词 | 所有文本引导工作流 |
| `{NEGATIVE_PROMPT}` | 负向提示词 | 所有文本引导工作流 |
| `{INPUT_IMAGE}` | 输入图像路径 | 图生图工作流 |
| `{SEED}` | 随机种子 | 可重复生成 |
| `{STEPS}` | 采样步数 | 质量控制 |
| `{CFG_SCALE}` | CFG比例 | 提示词遵循度 |
| `{WIDTH}` | 图像宽度 | 分辨率控制 |
| `{HEIGHT}` | 图像高度 | 分辨率控制 |

## 常用工作流模板

### 1. 基础文生图工作流

```json
{
    "3": {
        "inputs": {
            "seed": "{SEED}",
            "steps": "{STEPS}",
            "cfg": "{CFG_SCALE}",
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
            "width": "{WIDTH}",
            "height": "{HEIGHT}",
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
}
```

### 2. 图生图工作流

```json
{
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
}
```

### 3. LoRA增强工作流

```json
{
    "12": {
        "inputs": {
            "lora_name": "{LORA_MODEL}",
            "strength_model": "{LORA_STRENGTH}",
            "strength_clip": "{LORA_STRENGTH}",
            "model": ["4", 0],
            "clip": ["4", 1]
        },
        "class_type": "LoraLoader"
    }
}
```

## 高级工作流示例

### 1. 多阶段精炼工作流

```json
{
    "first_pass": {
        "inputs": {
            "steps": 10,
            "cfg": 7,
            "denoise": 1
        },
        "class_type": "KSampler"
    },
    "refine_pass": {
        "inputs": {
            "steps": 15,
            "cfg": 8,
            "denoise": 0.4,
            "latent_image": ["first_pass", 0]
        },
        "class_type": "KSampler"
    }
}
```

### 2. 风格转换工作流

```json
{
    "style_model": {
        "inputs": {
            "ckpt_name": "style_model.safetensors"
        },
        "class_type": "CheckpointLoaderSimple"
    },
    "style_encode": {
        "inputs": {
            "text": "{STYLE_PROMPT}",
            "clip": ["style_model", 1]
        },
        "class_type": "CLIPTextEncode"
    }
}
```

### 3. 批量生成工作流

```json
{
    "batch_sampler": {
        "inputs": {
            "seed": "{BATCH_SEED}",
            "steps": "{STEPS}",
            "batch_size": "{BATCH_SIZE}"
        },
        "class_type": "KSampler"
    }
}
```

## 参数配置指南

### 1. 采样器选择

| 采样器 | 特点 | 推荐用途 |
|--------|------|----------|
| `euler` | 快速，质量中等 | 快速预览 |
| `euler_a` | 更好的细节 | 一般生成 |
| `dpmpp_2m` | 高质量，较慢 | 最终输出 |
| `ddim` | 稳定，可重复 | 测试用途 |

### 2. 调度器配置

| 调度器 | 效果 | 适用场景 |
|--------|------|----------|
| `normal` | 标准效果 | 大多数情况 |
| `karras` | 更平滑 | 人像生成 |
| `exponential` | 强对比 | 艺术风格 |

### 3. CFG比例设置

- **1-3**: 很少约束，更有创意但可能偏离提示
- **7-8**: 平衡创意和准确性（推荐）
- **12-15**: 严格遵循提示，但可能过度饱和
- **15+**: 可能产生伪影

## 集成到插件代码

### 1. 创建工作流配置类

```cpp
struct FComfyUIWorkflow
{
    FString Name;
    FString Description;
    FString JsonTemplate;
    TMap<FString, FString> DefaultParameters;
    TArray<FString> RequiredInputs;
    
    bool IsValid() const
    {
        return !Name.IsEmpty() && !JsonTemplate.IsEmpty();
    }
};
```

### 2. 工作流管理器

```cpp
class UComfyUIWorkflowManager : public UObject
{
public:
    void LoadWorkflowFromFile(const FString& FilePath);
    void SaveWorkflowToFile(const FString& FilePath, const FComfyUIWorkflow& Workflow);
    FString ProcessWorkflowTemplate(const FComfyUIWorkflow& Workflow, const TMap<FString, FString>& Parameters);
    
private:
    TMap<FString, FComfyUIWorkflow> LoadedWorkflows;
};
```

### 3. 动态参数替换

```cpp
FString UComfyUIWorkflowManager::ProcessWorkflowTemplate(const FComfyUIWorkflow& Workflow, const TMap<FString, FString>& Parameters)
{
    FString ProcessedJson = Workflow.JsonTemplate;
    
    for (const auto& Param : Parameters)
    {
        FString Placeholder = FString::Printf(TEXT("{%s}"), *Param.Key);
        ProcessedJson = ProcessedJson.Replace(*Placeholder, *Param.Value);
    }
    
    return ProcessedJson;
}
```

## 调试和测试

### 1. 工作流验证

在集成前，先在ComfyUI中验证工作流：
1. 导入JSON到ComfyUI
2. 手动执行确保无错误
3. 检查输出质量

### 2. 参数范围测试

测试不同参数组合：
```cpp
void TestWorkflowParameters()
{
    TArray<int32> StepValues = {10, 20, 30, 50};
    TArray<float> CFGValues = {5.0f, 7.0f, 10.0f, 15.0f};
    
    for (int32 Steps : StepValues)
    {
        for (float CFG : CFGValues)
        {
            // 测试参数组合
        }
    }
}
```

### 3. 错误处理

```cpp
bool ValidateWorkflowJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogComfyUI, Error, TEXT("Invalid JSON format"));
        return false;
    }
    
    // 验证必需节点
    if (!JsonObject->HasField(TEXT("save_node")))
    {
        UE_LOG(LogComfyUI, Error, TEXT("Missing save node"));
        return false;
    }
    
    return true;
}
```

通过遵循这个指南，您可以有效地将ComfyUI工作流集成到Unreal Engine插件中，实现强大的AI内容生成功能。
