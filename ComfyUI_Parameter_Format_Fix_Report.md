# ComfyUI 工作流参数格式修复报告

## 问题描述

ComfyUI 报告工作流验证失败：
```
Value not in list: file_format: '{FORMAT|glb}' not in ['glb', 'obj', 'ply', 'stl', '3mf', 'dae']
```

## 根本原因

1. **参数替换系统限制**：当前的参数替换系统只支持简单的 `{KEY}` 格式，不支持 `{KEY|default}` 格式
2. **枚举值验证**：ComfyUI 的 `Hy3D21ExportMesh` 节点对 `file_format` 参数进行严格的枚举值验证
3. **模板格式不匹配**：工作流模板使用了系统不支持的参数格式

## 参数替换系统分析

在 `ComfyUIWorkflowManager.cpp` 中的 `ReplaceWorkflowPlaceholders` 方法：

```cpp
FString UComfyUIWorkflowManager::ReplaceWorkflowPlaceholders(const FString& WorkflowTemplate, 
                                                           const FString& Prompt, 
                                                           const FString& NegativePrompt,
                                                           const TMap<FString, FString>& CustomParameters)
{
    FString ProcessedTemplate = WorkflowTemplate;
    
    // 只替换简单格式
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
```

**支持的格式**：`{KEY}`  
**不支持的格式**：`{KEY|default}`

## 解决方案

### 1. 修复工作流文件
将所有 `{KEY|default}` 格式的参数替换为固定值：

**修复前**：
```json
{
  "inputs": {
    "file_format": "{FORMAT|glb}",
    "save_texture": "{SAVE_TEXTURE|true}",
    "steps": "{STEPS|25}"
  }
}
```

**修复后**：
```json
{
  "inputs": {
    "file_format": "glb",
    "save_texture": true,
    "steps": 25
  }
}
```

### 2. 数据类型正确性
确保所有参数使用正确的 JSON 数据类型：
- 字符串：`"glb"`
- 布尔值：`true`/`false`
- 数值：`25`、`7.5`

### 3. 枚举值合规性
`file_format` 参数只接受以下值：
- `"glb"`
- `"obj"`
- `"ply"`
- `"stl"`
- `"3mf"`
- `"dae"`

## 修复的文件

### 1. image_to_3d_full.json
- 移除所有 `{KEY|default}` 格式参数
- 使用固定的合理默认值
- 保留 `{INPUT_IMAGE}` 占位符用于图像输入

### 2. image_to_3d_simple.json
- 简化的工作流版本
- 只保留核心节点
- 使用固定参数值

## 工作流节点参数总结

| 节点ID | 节点类型 | 关键参数 | 修复说明 |
|--------|----------|----------|----------|
| 4 | Hy3D21VAELoader | ckpt_name | 固定为 "Hunyuan3D-vae-v2-1-fp16.ckpt" |
| 9 | Hy3D21VAEDecode | file_format相关 | 使用数值类型而非字符串 |
| 37 | Hy3DMeshGenerator | steps, cfg, seed | 使用数值类型 |
| 43 | Hy3D21PostprocessMesh | 布尔参数 | 使用 true/false 而非字符串 |
| 44 | Hy3D21ExportMesh | file_format | 固定为 "glb" |

## 测试建议

1. **基础测试**：使用简化版本 (`image_to_3d_simple.json`) 测试基本功能
2. **完整测试**：使用完整版本 (`image_to_3d_full.json`) 测试所有功能
3. **参数验证**：确认所有枚举参数都在有效范围内
4. **类型检查**：验证 JSON 参数类型正确

## 后续改进建议

1. **扩展参数系统**：如果需要支持默认值，可以扩展 `ReplaceWorkflowPlaceholders` 方法
2. **参数验证**：添加工作流参数验证机制
3. **模板标准化**：建立统一的工作流模板格式标准
