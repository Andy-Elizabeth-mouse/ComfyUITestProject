# HunyuanI3D 节点参数更新报告

## 问题描述
ComfyUI-Hunyuan3d-2-1 插件的节点接口发生了更新，导致多个节点的必需参数缺失或参数名称变更。

## 节点参数变更详情

### 1. Hy3D21VAELoader (节点4)
**旧参数** → **新参数**
- `ckpt_name` → `model_name`

### 2. Hy3D21VAEDecode (节点9)
**新增必需参数**：
- `mc_algo` (替代 `algo`)
- `mc_level` (替代 `pad`)
- `octree_resolution` (替代 `resolution`)
- `box_v` (新参数，默认1.1)
- `num_chunks` (新参数，默认1)

### 3. Hy3DMeshGenerator (节点37)
**参数变更**：
- `ckpt_name` → `model`
- `cfg` → `guidance_scale`
- **新增必需参数**：`attention_mode` (默认"math")

### 4. Hy3D21PostprocessMesh (节点43)
**新增必需参数**：
- `remove_degenerate_faces` (默认true)
- `remove_floaters` (默认true)
- `reduce_faces` (默认true)
- `smooth_normals` (替代 `smooth`)

### 5. Hy3D21CameraConfig (节点19)
**参数变更**：
- `azimuth_list` → `camera_azimuths`
- `elevation_list` → `camera_elevations`
- `distance_list` → `view_weights`
- **新增必需参数**：`ortho_scale` (默认1.0)

### 6. Hy3DMultiViewsGenerator (节点20)
**参数变更**：
- `resolution` → `view_size`
- `size` → `texture_size`
- **新增必需参数**：
  - `steps` (默认25)
  - `guidance_scale` (默认7.5)
  - `unwrap_mesh` (默认true)

## 修复后的工作流

### 简化版本 (image_to_3d_simple.json)
```json
{
  "4": {
    "inputs": {
      "model_name": "Hunyuan3D-vae-v2-1-fp16.ckpt"
    },
    "class_type": "Hy3D21VAELoader"
  },
  "9": {
    "inputs": {
      "iso_value": 1.01,
      "octree_resolution": 256,
      "max_facenum": 64000,
      "mc_level": 0,
      "mc_algo": "mc",
      "post_process": true,
      "fix_normals": false,
      "box_v": 1.1,
      "num_chunks": 1,
      "vae": ["4", 0],
      "latents": ["37", 0]
    },
    "class_type": "Hy3D21VAEDecode"
  },
  "37": {
    "inputs": {
      "image": ["14", 2],
      "model": "hunyuan3d-dit-v2-1-fp16.ckpt",
      "steps": 25,
      "guidance_scale": 7.5,
      "seed": 42,
      "attention_mode": "math"
    },
    "class_type": "Hy3DMeshGenerator"
  },
  "43": {
    "inputs": {
      "remove_duplicated_faces": true,
      "remove_degenerate_faces": true,
      "remove_floaters": true,
      "smooth_normals": true,
      "reduce_faces": true,
      "unwrap": true,
      "max_facenum": 40000,
      "simplify": false,
      "trimesh": ["9", 0]
    },
    "class_type": "Hy3D21PostprocessMesh"
  }
}
```

## 验证步骤

1. **模型文件检查**：确保以下模型文件存在于ComfyUI的models目录中
   - `Hunyuan3D-vae-v2-1-fp16.ckpt` (VAE模型)
   - `hunyuan3d-dit-v2-1-fp16.ckpt` (DIT模型)

2. **节点版本验证**：确认ComfyUI-Hunyuan3d-2-1插件为最新版本

3. **参数测试**：使用简化版本测试基本图生3D功能

## 关键修复点

1. **参数名称标准化**：所有参数名称与最新节点接口保持一致
2. **必需参数完整性**：补充所有新增的必需参数
3. **默认值合理性**：使用经过验证的默认参数值
4. **数据类型正确性**：确保所有参数使用正确的JSON数据类型

## 测试建议

1. **基础功能测试**：使用`image_to_3d_simple.json`测试核心图生3D功能
2. **完整工作流测试**：使用`image_to_3d_full.json`测试包含纹理生成的完整流程
3. **模型兼容性测试**：验证不同输入图像的处理效果
4. **输出格式验证**：确认生成的GLB文件格式正确

## 注意事项

- 确保ComfyUI服务端已正确安装HunyuanI3D相关模型文件
- 注意新参数的取值范围和有效性
- 如遇到问题，可参考原始Full_Workflow.json中的参数配置
