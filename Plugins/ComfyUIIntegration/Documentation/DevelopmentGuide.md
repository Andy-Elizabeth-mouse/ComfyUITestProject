# ComfyUI Integration Development Guide

## 架构概述

本插件采用模块化架构设计，主要包含以下组件：

### 1. 核心模块 (FComfyUIIntegrationModule)
- 负责插件的初始化和清理
- 注册编辑器菜单和工具窗口
- 管理插件生命周期

### 2. UI组件 (SComfyUIWidget)
- 基于Slate框架的用户界面
- 提供工作流选择、参数输入和结果预览
- 支持响应式布局和本地化

### 3. 网络客户端 (UComfyUIClient)
- 处理与ComfyUI服务器的HTTP通信
- 管理异步请求和响应
- 支持多种工作流类型的JSON构建

### 4. 样式和命令系统
- 统一的视觉样式定义
- 键盘快捷键和菜单命令
- 图标和资源管理

## 技术实现细节

### HTTP通信流程

1. **提交工作流**
   ```cpp
   CurrentRequest->SetURL(ServerUrl + TEXT("/prompt"));
   CurrentRequest->SetVerb(TEXT("POST"));
   CurrentRequest->SetContentAsString(WorkflowJson);
   ```

2. **轮询生成状态**
   ```cpp
   StatusRequest->SetURL(ServerUrl + TEXT("/history/") + PromptId);
   StatusRequest->SetVerb(TEXT("GET"));
   ```

3. **下载生成结果**
   - 从ComfyUI输出目录获取生成的图像
   - 转换为UE5纹理格式
   - 更新UI预览组件

### Slate UI组件结构

```cpp
SVerticalBox
├── 标题区域 (STextBlock)
├── 服务器配置 (SEditableTextBox)
├── 工作流选择 (SComboBox)
├── 提示词输入 (SEditableTextBox)
├── 控制按钮 (SHorizontalBox)
│   ├── 生成按钮 (SButton)
│   ├── 保存按钮 (SButton)
│   ├── 另存为按钮 (SButton)
│   └── 预览按钮 (SButton)
└── 图像预览 (SScrollBox > SImage)
```

### 工作流配置系统

每个工作流类型都有对应的JSON模板：

```json
{
  "3": {
    "inputs": {
      "seed": 156680208700286,
      "steps": 20,
      "cfg": 8,
      "model": ["4", 0],
      "positive": ["6", 0],
      "negative": ["7", 0]
    },
    "class_type": "KSampler"
  }
}
```

支持占位符替换：
- `{POSITIVE_PROMPT}`: 正向提示词
- `{NEGATIVE_PROMPT}`: 负向提示词  
- `{INPUT_IMAGE}`: 输入图像路径

## 扩展开发

### 添加新工作流类型

1. **扩展枚举定义**
   ```cpp
   enum class EWorkflowType
   {
       TextToImage,
       ImageToImage,
       TextTo3D,
       ImageTo3D,
       TextureGeneration,
       NewWorkflowType  // 新增类型
   };
   ```

2. **添加工作流配置**
   ```cpp
   void UComfyUIClient::InitializeWorkflowConfigs()
   {
       FWorkflowConfig NewConfig;
       NewConfig.Name = TEXT("New Workflow");
       NewConfig.JsonTemplate = TEXT("{ ... }");
       WorkflowConfigs.Add(EWorkflowType::NewWorkflowType, NewConfig);
   }
   ```

3. **更新UI文本**
   ```cpp
   case EWorkflowType::NewWorkflowType:
       return LOCTEXT("NewWorkflow", "New Workflow Type");
   ```

### 自定义UI组件

创建新的Slate组件：

```cpp
class SCustomWidget : public SCompoundWidget
{
    SLATE_BEGIN_ARGS(SCustomWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};
```

### 添加新的API端点

扩展ComfyUIClient支持更多API：

```cpp
void UComfyUIClient::GetModelList(const FOnModelsReceived& OnComplete)
{
    TSharedPtr<IHttpRequest> Request = HttpModule->CreateRequest();
    Request->SetURL(ServerUrl + TEXT("/object_info"));
    Request->SetVerb(TEXT("GET"));
    // ... 实现逻辑
}
```

## 性能优化

### 1. 内存管理
- 使用TSharedPtr管理UI组件生命周期
- 及时释放大型纹理资源
- 避免在UI线程进行重度计算

### 2. 异步处理
- 所有网络请求使用异步方式
- UI更新在主线程执行
- 使用定时器进行状态轮询

### 3. 缓存策略
- 缓存工作流模板减少解析开销
- 复用HTTP连接池
- 预加载常用资源

## 调试和测试

### 日志系统

使用UE5的日志系统进行调试：

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogComfyUIIntegration, Log, All);

UE_LOG(LogComfyUIIntegration, Warning, TEXT("Debug message: %s"), *DebugString);
```

### 单元测试

为关键功能编写单元测试：

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComfyUIClientTest, "ComfyUIIntegration.Client.Basic", 
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FComfyUIClientTest::RunTest(const FString& Parameters)
{
    UComfyUIClient* Client = NewObject<UComfyUIClient>();
    Client->SetServerUrl(TEXT("http://localhost:8188"));
    
    // 测试逻辑...
    
    return true;
}
```

### 性能分析

使用UE5的性能分析工具：

```cpp
SCOPE_CYCLE_COUNTER(STAT_ComfyUIImageGeneration);
```

## 部署和打包

### 1. 插件打包
```bash
# 打包插件
Engine/Build/BatchFiles/RunUAT.bat BuildPlugin 
    -Plugin="Path/To/ComfyUIIntegration.uplugin" 
    -Package="Output/Directory"
```

### 2. 依赖检查
确保所有必需的模块都在.uplugin文件中声明：

```json
{
    "Modules": [
        {
            "Name": "ComfyUIIntegration",
            "Type": "Editor",
            "LoadingPhase": "Default"
        }
    ]
}
```

### 3. 版本兼容性
测试插件在不同UE5版本下的兼容性，更新Build.cs文件中的版本要求。

## 最佳实践

1. **错误处理**: 始终检查HTTP请求结果和JSON解析状态
2. **用户体验**: 提供进度指示和错误提示
3. **配置管理**: 使用UE5的设置系统保存用户配置
4. **本地化**: 所有用户界面文本都应支持本地化
5. **文档**: 保持代码注释和文档的同步更新

通过遵循这些指南，可以确保插件的稳定性、可维护性和扩展性。
