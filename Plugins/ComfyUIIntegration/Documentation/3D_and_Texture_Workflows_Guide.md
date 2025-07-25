# ComfyUI Integration - 3D和纹理生成工作流指南

本指南详细介绍如何使用ComfyUI Integration插件的3D模型生成和纹理生成功能。

## 📋 目录

- [概述](#概述)
- [3D模型生成](#3d模型生成)
  - [文生3D (Text to 3D)](#文生3d-text-to-3d)
  - [图生3D (Image to 3D)](#图生3d-image-to-3d)
- [纹理生成](#纹理生成)
  - [PBR纹理生成](#pbr纹理生成)
  - [纹理类型支持](#纹理类型支持)
- [资产管理](#资产管理)
- [最佳实践](#最佳实践)
- [故障排除](#故障排除)

## 🌟 概述

ComfyUI Integration插件现在支持三种新的AI生成工作流：

1. **文生3D (Text to 3D)**: 从文本描述生成3D模型
2. **图生3D (Image to 3D)**: 从2D图像重建3D模型  
3. **纹理生成 (Texture Generation)**: 生成PBR材质纹理集

这些功能通过专门的资产管理器实现，能够直接将生成的内容集成到UE5项目中。

## 🎯 3D模型生成

### 文生3D (Text to 3D)

#### 功能描述
根据文本描述生成3D模型，支持简单几何体和复杂对象的创建。

#### 使用步骤

1. **选择工作流类型**
   - 在ComfyUI插件界面中选择"文生3D模型"

2. **输入提示词**
   ```
   正面提示词示例：
   - "a modern wooden chair with clean lines"
   - "a fantasy sword with ornate golden handle"  
   - "a simple coffee mug, white ceramic"
   - "a low-poly tree for game environment"
   ```

3. **配置参数**
   - **模型**: sv3d_u.safetensors (专用于3D生成)
   - **步数**: 25 (推荐值)
   - **CFG比例**: 7.5
   - **采样方法**: dpmpp_2m
   - **视角数量**: 21 (生成多视角用于3D重建)
   - **输出格式**: OBJ (支持OBJ/GLB/FBX)

4. **生成和保存**
   - 点击"生成图像"按钮
   - 生成完成后，系统会自动创建UStaticMesh资产
   - 保存到 `/Game/ComfyUI/Generated/Models/` 目录

#### 支持的输出格式
- **OBJ**: 基础网格格式，包含顶点和面信息
- **glTF/GLB**: 现代3D格式，支持材质和动画(开发中)
- **FBX**: 行业标准格式(计划支持)

### 图生3D (Image to 3D)

#### 功能描述
从单张2D图像重建3D几何体，使用深度估计和几何重建技术。

#### 使用步骤

1. **准备输入图像**
   - 选择清晰、高对比度的图像
   - 对象轮廓明确，背景简单
   - 推荐分辨率: 512x512 或更高

2. **配置工作流**
   - 选择"图生3D模型"工作流
   - 上传参考图像
   - 设置深度估计模型: DPT-Hybrid-MiDaS

3. **调整重建参数**
   - **重建方法**: Poisson (泊松重建)
   - **深度缩放**: 0.1 (控制模型厚度)
   - **平滑迭代**: 3 (表面平滑度)
   - **UV映射**: Smart Projection

4. **生成和应用纹理**
   - 系统会自动将原图像作为纹理应用到3D模型
   - 支持UV映射和纹理坐标生成

#### 最佳输入图像特征
- ✅ 高对比度物体
- ✅ 简单背景
- ✅ 明确边缘轮廓
- ✅ 正面或侧面视角
- ❌ 复杂场景
- ❌ 透明物体
- ❌ 模糊图像

## 🎨 纹理生成

### PBR纹理生成

#### 功能概述
生成完整的PBR(物理渲染)纹理集，包括漫反射、法线、粗糙度、金属度、高度等贴图。

#### 使用步骤

1. **选择纹理生成工作流**
   - 在工作流下拉菜单中选择"纹理生成"

2. **输入材质描述**
   ```
   优秀的提示词示例：
   - "weathered concrete wall texture, high detail"
   - "polished marble surface with subtle veins"
   - "rusty old metal plate, industrial look"
   - "wooden planks texture, oak wood grain"
   - "fabric texture, cotton canvas weave"
   ```

3. **配置生成参数**
   - **分辨率**: 1024x1024 (可选512/1024/2048)
   - **模型**: materialDiffusion_v10.safetensors
   - **步数**: 30
   - **CFG比例**: 8.0
   - **采样器**: dpmpp_sde
   - **无缝处理**: 启用

4. **PBR贴图生成**
   系统会自动生成以下贴图类型：
   - **Diffuse/Albedo**: 基础颜色贴图
   - **Normal**: 法线贴图(表面细节)
   - **Roughness**: 粗糙度贴图(表面光滑程度)
   - **Metallic**: 金属度贴图(金属/非金属)
   - **Height**: 高度贴图(位移信息)

#### 纹理类型支持

| 贴图类型 | 描述 | 压缩格式 | sRGB |
|---------|------|----------|------|
| Diffuse/Albedo | 基础颜色 | TC_Default | 是 |
| Normal | 法线向量 | TC_Normalmap | 否 |
| Roughness | 表面粗糙度 | TC_Masks | 否 |
| Metallic | 金属属性 | TC_Masks | 否 |
| Height/Displacement | 高度信息 | TC_Masks | 否 |
| AO (Ambient Occlusion) | 环境遮蔽 | TC_Masks | 否 |

### 材质创建和应用

#### 自动材质生成
插件会自动为生成的纹理创建UE5材质:

1. **PBR材质实例**
   - 基于UE5标准PBR着色模型
   - 自动连接各种贴图输入
   - 设置适当的参数值

2. **材质参数**
   ```cpp
   // 系统会自动设置这些参数
   - BaseColor: 漫反射贴图
   - Normal: 法线贴图  
   - Roughness: 粗糙度贴图
   - Metallic: 金属度贴图
   - WorldPositionOffset: 高度贴图(可选)
   ```

#### 手动材质调整
生成后可以手动调整材质参数:
- 法线强度
- 粗糙度倍数
- 金属度遮罩
- 高度缩放

## 📁 资产管理

### 保存位置
生成的资产会自动保存到以下位置:

```
/Game/ComfyUI/Generated/
├── Models/          # 3D模型资产
│   ├── TextTo3D/
│   └── ImageTo3D/
├── Textures/        # 纹理资产
│   ├── Diffuse/
│   ├── Normal/
│   ├── Roughness/
│   └── Materials/
└── Materials/       # 材质资产
    ├── PBR/
    └── Instances/
```

### 命名规则
- **3D模型**: `ComfyUI_Model_[Timestamp]`
- **纹理**: `ComfyUI_Texture_[Type]_[Timestamp]`
- **材质**: `ComfyUI_Material_PBR_[Timestamp]`

### 资产优化
系统会自动执行以下优化:
- 网格几何优化
- 纹理压缩设置
- LOD生成(计划功能)
- 碰撞体生成(计划功能)

## ✨ 最佳实践

### 3D模型生成
1. **提示词建议**
   - 使用简单明确的描述
   - 避免过于复杂的场景
   - 包含材质和风格信息
   - 指定"low poly"适合游戏使用

2. **后处理建议**
   - 生成后检查网格质量
   - 必要时手动清理几何体
   - 添加适当的碰撞体
   - 设置LOD级别

### 纹理生成
1. **描述技巧**
   - 描述表面质感和细节
   - 包含磨损、老化等特征
   - 指定材质类型(金属、木材、石材等)
   - 添加环境信息

2. **质量优化**
   - 选择合适的分辨率
   - 启用无缝处理
   - 检查UV映射质量
   - 调整压缩设置

## 🔧 故障排除

### 常见问题

#### 3D模型问题
1. **模型不显示或为空**
   - 检查OBJ文件格式
   - 验证顶点和面数据
   - 确认材质分配

2. **网格质量差**
   - 增加采样步数
   - 调整CFG比例
   - 尝试不同的重建方法

3. **纹理映射错误**
   - 检查UV坐标
   - 重新生成UV映射
   - 验证纹理分辨率

#### 纹理生成问题
1. **纹理不无缝**
   - 启用无缝处理选项
   - 增加边缘混合
   - 检查图块设置

2. **材质显示异常**
   - 验证sRGB设置
   - 检查压缩格式
   - 重建材质连接

3. **性能问题**
   - 降低纹理分辨率
   - 启用压缩
   - 使用流送纹理

### 调试工具
插件提供以下调试功能:
- 网格分析工具
- 纹理属性查看器
- 材质验证器
- 性能分析器

### 支持和报告
如遇到问题，请提供:
- 详细的错误信息
- 使用的提示词
- 生成参数配置  
- UE5版本信息

## 📚 相关文档
- [ComfyUI工作流集成指南](./WorkflowIntegration.md)
- [自定义工作流创建](./Custom_Workflow_Guide.md)
- [API文档](./API_Documentation.md)
- [完整用户指南](./Complete_User_Guide.md)

---

*更新时间: 2025年7月24日*
*版本: v1.0.0*
