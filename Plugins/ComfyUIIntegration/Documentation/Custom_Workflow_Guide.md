# ComfyUI自定义工作流导入指南

## 概述

ComfyUI Integration插件现在支持导入和使用自定义工作流，让你能够使用任何ComfyUI工作流来生成内容。

## 🔧 功能特性

### 1. 配置系统
- 自动读取 `Config/default_config.json` 中的服务器设置和预定义工作流
- 支持从配置文件加载工作流参数
- 动态服务器URL配置

### 2. 工作流模板支持
- 支持JSON格式的ComfyUI工作流文件
- 自动扫描 `Config/Templates/` 目录中的工作流模板
- 支持占位符替换（如 `{POSITIVE_PROMPT}`, `{NEGATIVE_PROMPT}`）

### 3. 自定义工作流导入
- 通过UI界面导入外部工作流JSON文件
- 自动复制到Templates目录进行管理
- 实时刷新工作流列表

## 📋 使用方法

### 基础工作流使用
1. 在工作流类型下拉框中选择预定义工作流（如"Text to Image", "Image to Image"）
2. 输入提示词和负面提示词
3. 点击"Generate"按钮生成图像

### 自定义工作流使用
1. 在工作流类型下拉框中选择"Custom Workflow"
2. 在自定义工作流下拉框中选择要使用的工作流
3. 输入提示词和负面提示词
4. 点击"Generate"按钮生成图像

### 导入新的工作流
1. 点击"Import"按钮
2. 选择ComfyUI导出的JSON工作流文件
3. 文件将自动复制到Templates目录
4. 点击"Refresh"按钮更新工作流列表

## 📁 目录结构

```
Plugins/ComfyUIIntegration/
├── Config/
│   ├── default_config.json          # 主配置文件
│   └── Templates/                   # 工作流模板目录
│       ├── basic_txt2img.json       # 基础文生图工作流
│       ├── high_quality_txt2img.json # 高质量文生图工作流
│       ├── img2img.json             # 图生图工作流
│       └── [自定义工作流...]         # 用户导入的工作流
```

## 🔧 工作流模板格式

### 占位符支持
工作流模板支持以下占位符：
- `{POSITIVE_PROMPT}` - 正面提示词
- `{NEGATIVE_PROMPT}` - 负面提示词
- `{INPUT_IMAGE}` - 输入图像（图生图工作流）
- 其他自定义参数占位符

### 示例模板
```json
{
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
    }
}
```

## ⚙️ 配置文件格式

### default_config.json
```json
{
    "workflows": [
        {
            "name": "工作流名称",
            "type": "工作流类型",
            "description": "工作流描述",
            "parameters": {
                "参数名": "参数值"
            },
            "template": "模板文件名.json"
        }
    ],
    "server_settings": {
        "default_url": "http://127.0.0.1:8188",
        "timeout": 300,
        "max_retries": 3,
        "poll_interval": 2.0
    },
    "ui_settings": {
        "default_workflow": "Basic Text-to-Image",
        "auto_save": true,
        "preview_size": "medium",
        "show_advanced_options": false
    }
}
```

## 🚀 高级使用

### 创建自定义工作流
1. 在ComfyUI中设计和测试你的工作流
2. 使用ComfyUI的"Save (API Format)"功能导出JSON
3. 编辑JSON文件，将提示词替换为占位符
4. 通过插件界面导入或直接放置到Templates目录

### 工作流参数化
你可以在配置文件中定义参数，然后在工作流模板中使用：
```json
"parameters": {
    "model_name": "v1-5-pruned-emaonly-fp16.safetensors",
    "width": "512",
    "height": "512"
}
```

在模板中使用：
```json
{
    "inputs": {
        "ckpt_name": "{MODEL_NAME}",
        "width": "{WIDTH}",
        "height": "{HEIGHT}"
    }
}
```

## 🔍 故障排除

### 常见问题
1. **工作流不显示** - 检查JSON文件格式是否正确
2. **生成失败** - 确保ComfyUI服务器正在运行且模型文件存在
3. **占位符未替换** - 检查占位符格式是否正确（大写，用花括号包围）

### 调试信息
插件会在UE5的输出日志中显示详细的调试信息，包括：
- 配置文件加载状态
- 工作流模板解析结果
- HTTP请求和响应详情

## 📝 最佳实践

1. **工作流命名** - 使用描述性的文件名，如 `portrait_realistic_v1.json`
2. **参数配置** - 在配置文件中预设常用参数组合
3. **模板验证** - 导入前在ComfyUI中测试工作流
4. **备份管理** - 定期备份Templates目录和配置文件
