# ComfyUI Integration - 自定义工作流使用指南

## 概述

ComfyUI Integration 插件现在支持完全自定义的工作流，您不再受限于硬编码的工作流。您可以导入、验证和使用任何兼容的ComfyUI工作流。

## 主要功能

### 1. 工作流导入
- 点击"Import"按钮导入ComfyUI工作流JSON文件
- 系统会自动验证工作流的有效性
- 成功导入的工作流会保存到插件的Templates目录

### 2. 工作流验证
- 点击"Validate"按钮验证当前选中的工作流
- 系统会检查：
  - JSON格式是否正确
  - 是否包含有效的ComfyUI节点
  - 是否有输出节点（SaveImage、PreviewImage等）
  - 节点连接是否合理

### 3. 连接测试
- 点击"Test Connection"按钮测试ComfyUI服务器连接
- 验证服务器URL是否正确且可访问

### 4. 动态参数系统
- 工作流中支持占位符：
  - `{POSITIVE_PROMPT}` - 正面提示词
  - `{NEGATIVE_PROMPT}` - 负面提示词
  - 其他自定义参数可在工作流文件中定义

## 使用步骤

### 1. 准备工作流
1. 在ComfyUI Web界面中设计您的工作流
2. 导出工作流为JSON文件（使用"Save (API Format)"）
3. 编辑JSON文件，将文本输入节点的值替换为占位符：
   ```json
   "inputs": {
     "text": "{POSITIVE_PROMPT}"
   }
   ```

### 2. 导入工作流
1. 在插件界面中点击"Import"按钮
2. 选择您的工作流JSON文件
3. 系统会自动验证并导入工作流

### 3. 使用工作流
1. 从下拉列表中选择导入的工作流
2. 输入提示词
3. 点击"Generate"开始生成

## 工作流要求

### 必需组件
- **输出节点**: 至少包含一个SaveImage或PreviewImage节点
- **有效的JSON格式**: 符合ComfyUI API格式

### 推荐组件
- **文本编码节点**: CLIPTextEncode用于处理提示词
- **采样器节点**: KSampler或其他采样器
- **模型加载节点**: CheckpointLoaderSimple等

### 占位符支持
在工作流JSON中使用以下占位符：
- `{POSITIVE_PROMPT}` - 会被用户输入的正面提示词替换
- `{NEGATIVE_PROMPT}` - 会被用户输入的负面提示词替换

## 示例工作流

插件提供了一个基础的文本生图工作流作为参考：
`BasicTextToImage.json`

## 故障排除

### 常见错误
1. **"No valid ComfyUI nodes found"**
   - 确保文件是ComfyUI工作流导出（API格式）
   - 检查JSON格式是否正确

2. **"No output nodes found"**
   - 确保工作流包含SaveImage或PreviewImage节点

3. **"Failed to connect to ComfyUI server"**
   - 检查ComfyUI服务器是否运行
   - 验证服务器URL是否正确（默认：http://127.0.0.1:8188）

### 调试提示
1. 使用"Validate"按钮检查工作流
2. 使用"Test Connection"按钮验证服务器连接
3. 查看UE5输出日志获取详细错误信息

## 高级功能

### 参数化工作流
您可以创建包含多个参数的复杂工作流，系统会自动识别和提取参数定义。

### 工作流分享
导入的工作流文件保存在：
`{ProjectDir}/Plugins/ComfyUIIntegration/Config/Templates/`

您可以直接共享这些JSON文件给其他团队成员。

## 注意事项

1. 确保ComfyUI服务器包含工作流所需的所有模型和节点
2. 工作流中的模型文件名必须与服务器上的文件名匹配
3. 复杂的工作流可能需要更长的生成时间
4. 建议在导入前在ComfyUI Web界面中测试工作流
