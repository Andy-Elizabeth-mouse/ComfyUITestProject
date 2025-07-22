# ComfyUI Integration 插件使用指南

## 概述

本文档为ComfyUI Integration插件提供完整的使用指南，包括安装、配置、使用和故障排除。该插件将ComfyUI的AI生成功能集成到Unreal Engine 5编辑器中，支持多种AI内容生成工作流。

## 目录

1. [快速开始](#快速开始)
2. [详细功能介绍](#详细功能介绍)
3. [工作流详解](#工作流详解)
4. [高级功能](#高级功能)
5. [故障排除](#故障排除)
6. [最佳实践](#最佳实践)
7. [API参考](#api参考)

---

## 快速开始

### 1. 安装要求

#### 软件要求
- **Unreal Engine 5.0+**: 主要开发环境
- **ComfyUI**: AI生成后端服务
- **Python 3.8+**: ComfyUI运行环境

#### 硬件要求
- **CPU**: Intel i5 或 AMD Ryzen 5 以上
- **内存**: 16GB RAM 以上
- **显卡**: GTX 1060 或 RX 580 以上（推荐RTX 3060以上）
- **存储**: 10GB 可用空间

### 2. 安装步骤

#### 步骤1：安装ComfyUI
```bash
# 克隆ComfyUI仓库
git clone https://github.com/comfyanonymous/ComfyUI.git
cd ComfyUI

# 安装依赖
pip install -r requirements.txt

# 启动ComfyUI服务器
python main.py --listen 0.0.0.0 --port 8188
```

#### 步骤2：安装插件
1. 将ComfyUIIntegration文件夹复制到项目的`Plugins`目录
2. 右键点击项目的`.uproject`文件，选择"Generate Visual Studio project files"
3. 在Visual Studio中构建项目
4. 启动Unreal Engine编辑器
5. 在插件管理器中启用"ComfyUI Integration"插件

#### 步骤3：配置连接
1. 在UE5编辑器中，从菜单栏选择`Window > ComfyUI Integration`
2. 在服务器地址栏输入ComfyUI服务器地址（默认：`http://127.0.0.1:8188`）
3. 点击"测试连接"按钮验证连接

### 3. 第一次使用

#### 生成第一张图像
1. 选择工作流类型为"文生图"
2. 在提示词输入框中输入：`"a beautiful landscape with mountains and lakes, highly detailed, 4k"`
3. 在负面提示词输入框中输入：`"blurry, low quality, distorted"`
4. 点击"生成"按钮
5. 等待生成完成，查看预览结果
6. 点击"保存"按钮将纹理保存到项目中

---

## 详细功能介绍

### 1. 主界面介绍

#### 工作流选择区域
- **下拉菜单**: 选择不同的AI生成工作流
- **支持类型**: 文生图、图生图、文生3D、图生3D、纹理生成
- **实时切换**: 可随时切换工作流类型

#### 提示词输入区域
- **正面提示词**: 描述想要生成的内容
- **负面提示词**: 描述不想要的内容
- **字符限制**: 支持最多2000字符的提示词
- **历史记录**: 自动保存最近使用的提示词

#### 服务器配置区域
- **服务器地址**: ComfyUI服务器的URL地址
- **连接状态**: 实时显示连接状态
- **测试连接**: 验证服务器连接的可用性

#### 控制按钮区域
- **生成按钮**: 开始AI生成任务
- **保存按钮**: 保存生成的纹理到项目
- **另存为按钮**: 以指定名称保存纹理
- **预览按钮**: 在新窗口中预览生成结果

#### 图像预览区域
- **实时预览**: 显示生成的图像结果
- **缩放功能**: 支持图像缩放查看
- **信息显示**: 显示图像尺寸和格式信息

### 2. 设置和配置

#### 服务器设置
```json
{
    "server_url": "http://127.0.0.1:8188",
    "timeout_seconds": 300,
    "max_retries": 3,
    "connection_check_interval": 30
}
```

#### 工作流设置
```json
{
    "default_workflow": "TextToImage",
    "default_steps": 20,
    "default_cfg_scale": 8.0,
    "default_sampler": "euler",
    "default_scheduler": "normal"
}
```

#### 输出设置
```json
{
    "output_directory": "Generated",
    "max_image_size": 2048,
    "default_format": "png",
    "auto_save": false,
    "save_metadata": true
}
```

---

## 工作流详解

### 1. 文生图 (Text to Image)

#### 功能描述
根据文本描述生成图像，是最基础也是最常用的AI生成功能。

#### 使用方法
1. 选择工作流类型为"文生图"
2. 输入详细的正面提示词
3. 输入负面提示词（可选）
4. 点击生成按钮

#### 提示词技巧
```
正面提示词示例：
"a majestic dragon flying over a medieval castle, golden hour lighting, highly detailed, fantasy art, 8k resolution"

负面提示词示例：
"blurry, low quality, distorted, bad anatomy, text, watermark"
```

#### 参数调整
- **步数**: 20-50（更多步数=更高质量，但更慢）
- **CFG Scale**: 7-12（控制提示词的影响程度）
- **尺寸**: 512x512, 768x768, 1024x1024

### 2. 图生图 (Image to Image)

#### 功能描述
基于输入图像和文本提示生成新的图像，可以进行风格转换、细节修改等。

#### 使用方法
1. 选择工作流类型为"图生图"
2. 导入参考图像
3. 输入修改指令的提示词
4. 调整变化强度（0.1-1.0）
5. 点击生成按钮

#### 应用场景
- **风格转换**: 将照片转换为不同艺术风格
- **细节修改**: 修改图像中的特定元素
- **质量提升**: 提高图像分辨率和细节

#### 参数说明
- **Denoising Strength**: 0.3-0.8（控制变化程度）
- **Seed**: 控制随机性（-1为随机）

### 3. 文生3D (Text to 3D)

#### 功能描述
根据文本描述生成3D模型，实验性功能，支持简单的3D对象生成。

#### 使用方法
1. 选择工作流类型为"文生3D"
2. 输入3D对象的详细描述
3. 选择输出格式（OBJ、FBX、GLB）
4. 点击生成按钮

#### 提示词建议
```
适合的提示词：
"a simple wooden chair, clean design, minimal style"
"a fantasy sword with ornate handle, medieval style"
"a cute cartoon character, low poly style"
```

#### 注意事项
- 生成时间较长（5-15分钟）
- 适合简单几何形状
- 需要后期处理和优化

### 4. 图生3D (Image to 3D)

#### 功能描述
从2D图像生成3D模型，使用深度估计和形状推断技术。

#### 使用方法
1. 选择工作流类型为"图生3D"
2. 导入清晰的2D图像
3. 选择视角和深度设置
4. 点击生成按钮

#### 最佳实践
- 使用高对比度的图像
- 选择有明确轮廓的对象
- 避免复杂的背景

### 5. 纹理生成 (Texture Generation)

#### 功能描述
生成可用于3D模型的无缝纹理，支持多种材质类型。

#### 使用方法
1. 选择工作流类型为"纹理生成"
2. 描述纹理类型和特征
3. 设置纹理尺寸（512x512到2048x2048）
4. 启用无缝贴图选项
5. 点击生成按钮

#### 纹理类型
- **Diffuse**: 基础颜色贴图
- **Normal**: 法线贴图
- **Roughness**: 粗糙度贴图
- **Metallic**: 金属度贴图
- **Height**: 高度贴图

#### 提示词示例
```
纹理提示词：
"seamless brick wall texture, weathered, high detail"
"wood grain texture, oak, natural pattern"
"metal surface texture, brushed steel, industrial"
```

---

## 高级功能

### 1. 批量生成

#### 功能描述
一次性生成多个变体，提高工作效率。

#### 使用方法
```cpp
// C++代码示例
void ABatchGenerator::GenerateMultipleImages()
{
    TArray<FString> Prompts = {
        TEXT("forest scene, morning light"),
        TEXT("forest scene, sunset"),
        TEXT("forest scene, night time"),
        TEXT("forest scene, winter")
    };
    
    for (const FString& Prompt : Prompts)
    {
        Client->GenerateImage(Prompt, NegativePrompt, WorkflowType, OnBatchComplete);
    }
}
```

### 2. 自定义工作流

#### 创建自定义工作流
1. 在ComfyUI中设计工作流
2. 导出工作流JSON
3. 在插件中注册新工作流

```cpp
// 注册自定义工作流
FWorkflowConfig CustomWorkflow;
CustomWorkflow.Name = TEXT("Custom Art Style");
CustomWorkflow.JsonTemplate = TEXT("{ /* 自定义JSON */ }");
WorkflowConfigs.Add(EWorkflowType::Custom, CustomWorkflow);
```

### 3. 参数控制

#### 高级参数调整
- **采样器选择**: euler, ddim, dpm++, etc.
- **调度器选择**: normal, karras, exponential
- **模型选择**: 不同的AI模型
- **LoRA权重**: 风格控制参数

### 4. 质量控制

#### 生成质量优化
- **提示词优化**: 使用详细、准确的描述
- **参数调整**: 根据需求调整步数和CFG
- **模型选择**: 选择适合的AI模型
- **后处理**: 使用图像处理技术

---

## 故障排除

### 1. 连接问题

#### 问题：无法连接到ComfyUI服务器
**可能原因**：
- ComfyUI服务器未启动
- 网络连接问题
- 防火墙阻止连接
- 服务器地址错误

**解决方案**：
1. 确认ComfyUI服务器正在运行
2. 检查服务器地址是否正确
3. 测试网络连接：`ping 127.0.0.1`
4. 检查防火墙设置
5. 尝试使用不同的端口

#### 问题：连接超时
**解决方案**：
1. 增加超时时间设置
2. 检查服务器性能
3. 减少并发请求数量

### 2. 生成问题

#### 问题：生成失败或错误
**可能原因**：
- 提示词包含无效字符
- 服务器资源不足
- 模型文件损坏
- 工作流配置错误

**解决方案**：
1. 检查提示词内容
2. 验证工作流JSON格式
3. 重启ComfyUI服务器
4. 检查模型文件完整性

#### 问题：生成质量差
**解决方案**：
1. 优化提示词描述
2. 增加生成步数
3. 调整CFG Scale参数
4. 使用更高质量的模型

### 3. 性能问题

#### 问题：生成速度慢
**优化方案**：
1. 使用更快的硬件
2. 优化ComfyUI配置
3. 减少图像尺寸
4. 使用批量生成

#### 问题：内存不足
**解决方案**：
1. 减少图像尺寸
2. 关闭不必要的程序
3. 增加虚拟内存
4. 使用内存优化设置

### 4. 文件问题

#### 问题：无法保存生成结果
**解决方案**：
1. 检查文件权限
2. 确认磁盘空间充足
3. 验证保存路径
4. 检查文件格式支持

---

## 最佳实践

### 1. 提示词编写技巧

#### 基本原则
- **详细描述**: 提供具体、详细的描述
- **关键词优先**: 将重要信息放在前面
- **风格指定**: 明确指定艺术风格
- **质量标识**: 添加质量相关关键词

#### 提示词结构
```
[主题描述] + [风格指定] + [质量标识] + [技术参数]

示例：
"a majestic dragon, fantasy art style, highly detailed, 8k resolution"
```

#### 高级技巧
- **权重控制**: 使用括号调整权重 `(important:1.2)`
- **风格混合**: 结合多种风格 `oil painting + digital art`
- **构图控制**: 指定视角和构图 `bird's eye view, centered composition`

### 2. 工作流优化

#### 效率提升
- **模板使用**: 创建常用的提示词模板
- **批量处理**: 一次性生成多个变体
- **参数预设**: 保存常用的参数组合
- **快捷键**: 使用键盘快捷键快速操作

#### 质量保证
- **多次尝试**: 生成多个版本选择最佳结果
- **迭代优化**: 基于结果不断调整提示词
- **A/B测试**: 比较不同参数的效果

### 3. 资源管理

#### 文件组织
- **分类保存**: 按类型和用途分类保存
- **命名规范**: 使用统一的命名规范
- **版本控制**: 保留不同版本的生成结果
- **备份策略**: 定期备份重要的生成结果

#### 性能优化
- **缓存清理**: 定期清理临时文件
- **资源监控**: 监控内存和存储使用情况
- **硬件升级**: 根据需求升级硬件配置

---

## API参考

### 1. 主要类和接口

#### UComfyUIClient
```cpp
class UComfyUIClient : public UObject
{
public:
    // 设置服务器地址
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void SetServerUrl(const FString& Url);
    
    // 生成图像
    void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                      EWorkflowType WorkflowType, const FOnImageGenerated& OnComplete);
    
    // 检查服务器状态
    UFUNCTION(BlueprintCallable, Category = "ComfyUI")
    void CheckServerStatus(const FOnImageGeneratedDynamic& OnComplete);
};
```

#### SComfyUIWidget
```cpp
class SComfyUIWidget : public SCompoundWidget
{
public:
    // 工作流类型枚举
    enum class EWorkflowType
    {
        TextToImage,
        ImageToImage,
        TextTo3D,
        ImageTo3D,
        TextureGeneration
    };
    
    // 构造Widget
    void Construct(const FArguments& InArgs);
};
```

### 2. 委托和回调

#### 图像生成回调
```cpp
// C++委托
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*);

// Blueprint委托
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnImageGeneratedDynamic, UTexture2D*, GeneratedImage);
```

### 3. 配置系统

#### 配置管理器
```cpp
class FComfyUIConfigManager
{
public:
    // 获取配置值
    template<typename T>
    T GetConfigValue(const FString& Section, const FString& Key, const T& DefaultValue);
    
    // 设置配置值
    template<typename T>
    void SetConfigValue(const FString& Section, const FString& Key, const T& Value);
    
    // 保存配置
    void SaveConfig();
};
```

### 4. 使用示例

#### 基本使用
```cpp
// 创建客户端
UComfyUIClient* Client = NewObject<UComfyUIClient>();
Client->SetServerUrl(TEXT("http://127.0.0.1:8188"));

// 设置回调
FOnImageGenerated OnComplete;
OnComplete.BindLambda([](UTexture2D* GeneratedTexture)
{
    if (GeneratedTexture)
    {
        // 处理生成的纹理
        UE_LOG(LogTemp, Log, TEXT("Image generated successfully"));
    }
});

// 生成图像
Client->GenerateImage(
    TEXT("a beautiful landscape"),
    TEXT("blurry, low quality"),
    SComfyUIWidget::EWorkflowType::TextToImage,
    OnComplete
);
```

#### Blueprint使用
```cpp
// Blueprint函数
UFUNCTION(BlueprintCallable, Category = "ComfyUI")
void GenerateImageFromBlueprint(const FString& Prompt, const FString& NegativePrompt)
{
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    
    FOnImageGeneratedDynamic OnComplete;
    OnComplete.BindUFunction(this, TEXT("OnImageGenerated"));
    
    Client->GenerateImage(Prompt, NegativePrompt, EWorkflowType::TextToImage, OnComplete);
}

UFUNCTION(BlueprintImplementableEvent, Category = "ComfyUI")
void OnImageGenerated(UTexture2D* GeneratedTexture);
```

---

## 更新和维护

### 1. 版本更新

#### 检查更新
1. 访问项目GitHub页面
2. 查看最新发布版本
3. 下载更新包
4. 按照更新指南操作

#### 更新流程
1. 备份当前配置
2. 下载新版本
3. 替换插件文件
4. 重新生成项目文件
5. 测试功能正常

### 2. 反馈和支持

#### 问题报告
- **GitHub Issues**: 提交Bug报告和功能请求
- **社区论坛**: 参与讨论和分享经验
- **文档Wiki**: 查阅和贡献文档

#### 贡献指南
1. Fork项目仓库
2. 创建特性分支
3. 提交代码更改
4. 创建Pull Request
5. 等待审核合并

---

## 许可证和版权

本插件采用MIT许可证，允许自由使用、修改和分发。请参阅LICENSE文件了解详细信息。

---

## 联系方式

- **项目主页**: https://github.com/your-repo/ComfyUI-Integration
- **文档Wiki**: https://github.com/your-repo/ComfyUI-Integration/wiki
- **问题报告**: https://github.com/your-repo/ComfyUI-Integration/issues
- **讨论论坛**: https://github.com/your-repo/ComfyUI-Integration/discussions

---

*本文档将持续更新，请定期查看最新版本。*
