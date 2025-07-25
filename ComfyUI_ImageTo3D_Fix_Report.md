# ComfyUI 图生3D工作流问题修复

## 问题描述

ComfyUI检测到图生3D工作流有问题：
```
invalid prompt: {'type': 'invalid_prompt', 'message': 'Cannot execute because node SetNode does not exist.', 'details': "Node ID '#29'", 'extra_info': {}}
```

## 问题分析

对比原始工作流（Full_Workflow.json）和我们的简化工作流（image_to_3d_full.json），发现以下问题：

### 1. 缺少节点连接信息
- **原始工作流**：包含完整的 `links` 数组定义节点间的连接关系
- **简化工作流**：只有节点定义，缺少输入连接

### 2. SetNode/GetNode 节点问题
- 原始工作流使用了 `SetNode` 和 `GetNode` 来传递变量
- 这些节点可能不是标准ComfyUI节点，而是特定扩展的节点
- 我们的简化工作流包含了这些节点但没有正确的连接

### 3. 不必要的复杂性
- 原始工作流包含了很多预览和调试节点
- 对于基本的图生3D功能，这些节点并非必需

## 解决方案

### 1. 修复后的完整工作流（image_to_3d_full.json）
```json
{
  "4": { // VAE加载器
    "inputs": {
      "ckpt_name": "{VAE_MODEL|Hunyuan3D-vae-v2-1-fp16.ckpt}"
    },
    "class_type": "Hy3D21VAELoader"
  },
  "9": { // VAE解码器 - 生成3D网格
    "inputs": {
      "iso_value": "{ISO_VALUE|1.01}",
      "resolution": "{RESOLUTION|256}",
      "max_facenum": "{MAX_FACES|64000}",
      "pad": "{PAD|0}",
      "algo": "{ALGO|mc}",
      "post_process": "{POST_PROCESS|true}",
      "fix_normals": "{FIX_NORMALS|false}",
      "vae": ["4", 0],        // 连接到VAE加载器
      "latents": ["37", 0]    // 连接到网格生成器
    },
    "class_type": "Hy3D21VAEDecode"
  },
  "14": { // 图像加载器
    "inputs": {
      "image": "{INPUT_IMAGE}",
      "upload": "image"
    },
    "class_type": "Hy3D21LoadImageWithTransparency"
  },
  "37": { // 网格生成器
    "inputs": {
      "image": ["14", 2],     // 连接到图像加载器
      "ckpt_name": "{DIT_MODEL|hunyuan3d-dit-v2-1-fp16.ckpt}",
      "steps": "{STEPS|25}",
      "cfg": "{CFG|7.5}",
      "seed": "{SEED|random}"
    },
    "class_type": "Hy3DMeshGenerator"
  },
  "43": { // 网格后处理
    "inputs": {
      "remove_duplicated_faces": "{REMOVE_DUPLICATED|true}",
      "smooth": "{SMOOTH|true}",
      "unwrap": "{UNWRAP|true}",
      "max_facenum": "{MAX_FACES|40000}",
      "simplify": "{SIMPLIFY|false}",
      "trimesh": ["9", 0]     // 连接到VAE解码器
    },
    "class_type": "Hy3D21PostprocessMesh"
  },
  "44": { // 网格导出
    "inputs": {
      "filename_prefix": "{FILENAME_PREFIX|3D/Hy3D}",
      "format": "{FORMAT|glb}",
      "save_texture": "{SAVE_TEXTURE|true}",
      "trimesh": ["43", 0]    // 连接到后处理
    },
    "class_type": "Hy3D21ExportMesh"
  }
}
```

### 2. 创建简化版本（image_to_3d_simple.json）
- 移除了所有 SetNode/GetNode 节点
- 直接使用数组格式 `["节点ID", 输出索引]` 定义连接
- 只保留核心的图生3D功能节点

## 工作流程

1. **图像加载** (节点14)：加载输入图像
2. **网格生成** (节点37)：使用DIT模型从图像生成3D潜在表示
3. **VAE加载** (节点4)：加载VAE模型
4. **VAE解码** (节点9)：将潜在表示解码为3D网格
5. **后处理** (节点43)：优化网格（去重、平滑、UV展开等）
6. **导出** (节点44)：保存为GLB格式文件

## 测试建议

1. 先测试简化版本（image_to_3d_simple.json）
2. 确保ComfyUI环境中已安装所有必需的HunyuanI3D节点
3. 验证输入图像格式正确
4. 检查模型文件路径是否正确

## 注意事项

- 确保ComfyUI服务端已安装 ComfyUI-Hunyuan3d-2-1 扩展
- 模型文件需要放在正确的目录中
- 输入图像建议使用带透明通道的PNG格式以获得最佳效果
