# ComfyUI Integration Plugin for Unreal Engine 5

这是一个Unreal Engine 5编辑器插件，允许直接在UE5编辑器中集成和使用ComfyUI工作流，实现AI驱动的内容生成功能。

> **⚠️ 项目状态**: 当前版本为 v1.0.0-alpha，核心功能已实现，部分高级功能正在开发中。请查看[项目状态](./PROJECT_STATUS.md)了解详细进度。

## 功能特性

### 🎨 AI内容生成
- **✅ 文生图 (Text-to-Image)**: 基于文本描述生成图像 *(已实现)*
- **✅ 图生图 (Image-to-Image)**: 基于输入图像和提示词生成新图像 *(已实现)*
- **🔄 文生3D (Text-to-3D)**: 从文本描述生成3D模型 *(开发中)*
- **🔄 图生3D (Image-to-3D)**: 从图像生成3D模型 *(开发中)*
- **🔄 纹理生成**: 为3D模型生成材质和纹理 *(计划中)*

### 🖥️ 自定义编辑器界面
- **✅ 直观的Slate UI界面** *(已实现)*
- **✅ 拖拽纹理资产支持**: 从内容浏览器直接拖拽纹理到输入框 *(v1.0.1新增)*
- **🔄 实时预览生成的内容** *(基础版本已实现，完善中)*
- **✅ 可配置的工作流参数** *(已实现)*
- **🔄 批量生成和管理功能** *(计划中)*

### 💾 资产管理
- **🔄 保存生成的资产到项目中** *(开发中)*
- **🔄 另存为功能支持** *(开发中)*
- **🔄 自动资产导入和管理** *(开发中)*
- **🔄 与Content Browser集成** *(计划中)*

### 🔧 高级功能
- **✅ 可配置的ComfyUI服务器连接** *(已实现)*
- **✅ 自定义工作流模板支持** *(基础版本已实现)*
- **✅ 快捷键支持**: 可自定义快捷键打开ComfyUI窗口 (默认: Ctrl+Shift+Q) *(v0.8.1新增)*
- **🔄 批处理和队列管理** *(计划中)*
- **✅ 错误处理和状态监控** *(基础版本已实现)*

### 图例说明
- **✅** = 已完成实现
- **🔄** = 正在开发中
- **📋** = 计划中/待实现

## 🚀 开发状态与待办事项

### 当前已实现的核心功能
- ✅ **插件架构**: 完整的UE5插件框架
- ✅ **Slate UI界面**: 用户友好的操作界面
- ✅ **HTTP通信**: 与ComfyUI服务器的完整通信
- ✅ **工作流系统**: 文生图和图生图工作流
- ✅ **模板系统**: 可配置的工作流模板
- ✅ **错误处理**: 基础的错误处理和状态监控

### 🔄 正在开发中的功能
1. **图像资产保存系统**
   - 实现图像下载和导入到UE5项目
   - 集成Content Browser
   - 支持多种图像格式

2. **高级UI功能**
   - 进度条和状态指示器
   - 预览图像的缩放和导航
   - 批量操作界面

3. **3D内容生成**
   - 文生3D工作流集成
   - 图生3D工作流实现
   - 3D模型导入系统

### 📋 待实现的功能
1. **资产管理增强**
   - [ ] 另存为功能
   - [ ] 资产版本控制
   - [ ] 批量导入/导出
   - [ ] 资产预览缩略图

2. **纹理和材质生成**
   - [ ] PBR纹理生成工作流
   - [ ] 自动材质创建
   - [ ] 纹理质量优化
   - [ ] 材质预设管理

3. **批处理和自动化**
   - [ ] 任务队列管理
   - [ ] 批量参数配置
   - [ ] 自动化脚本支持
   - [ ] 定时任务功能

4. **工作流编辑器**
   - [ ] 可视化工作流编辑器
   - [ ] 自定义节点支持
   - [ ] 工作流版本控制
   - [ ] 社区工作流分享

5. **高级功能**
   - [ ] 插件设置持久化
   - [ ] 历史记录功能
   - [ ] 参数预设管理
   - [x] 快捷键支持 (默认: Ctrl+Shift+Q)

### 🔧 技术改进计划
- [ ] 内存管理优化
- [ ] 性能profiling和优化
- [ ] 单元测试覆盖
- [ ] 代码文档完善
- [ ] 多平台支持

### ⚠️ 已知限制
- 当前图像处理使用占位符，需要实现实际下载
- 3D模型生成功能尚未实现
- 保存功能需要与UE5资产系统完全集成
- 大型图像可能导致内存问题

**详细状态请查看**: [PROJECT_STATUS.md](./PROJECT_STATUS.md)

## 快速开始

### 方法一：使用自动化脚本（推荐）

1. **运行测试项目创建脚本**：
   ```powershell
   # 进入Scripts目录
   cd Scripts
   
   # 运行批处理文件
   ./Create-TestProject.bat
   
   # 或直接运行PowerShell脚本
   powershell -ExecutionPolicy Bypass -File "Create-TestProject.ps1"
   ```

2. **脚本会自动**：
   - 创建名为`ComfyUITestProject`的UE5空白项目
   - 复制插件文件到项目的Plugins目录
   - 生成Visual Studio项目文件
   - 配置必要的构建设置

### 方法二：手动集成

1. **创建UE5测试项目**：
   - 打开UE5编辑器
   - 选择"Games" -> "Blank"
   - 项目类型：C++
   - 命名：`ComfyUITestProject`

2. **集成插件**：
   ```powershell
   # 创建Plugins目录
   mkdir ComfyUITestProject\Plugins
   
   # 复制插件文件
   xcopy "d:\forcopilot" "ComfyUITestProject\Plugins\ComfyUIIntegration" /s /e /h
   ```

3. **生成项目文件**：
   - 右键.uproject文件
   - 选择"Generate Visual Studio project files"

## 编译和调试

### 1. Visual Studio设置
```
Configuration: Development Editor
Platform: Win64
StartUp Project: ComfyUITestProject
```

### 2. 编译项目
- 按F5启动调试
- 或右键解决方案选择"Build Solution"

### 3. 启用插件
1. UE5编辑器启动后
2. 进入"Edit" -> "Plugins"
3. 搜索"ComfyUI Integration"
4. 勾选"Enabled"并重启编辑器

### 4. 验证功能
- 查看菜单栏是否有"ComfyUI"菜单
- 点击菜单项打开ComfyUI Widget
- 检查Output Log确认插件正常加载

## 调试技巧

### 启用详细日志
在`Config/DefaultEngine.ini`中添加：
```ini
[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
```

### 断点调试
```cpp
// 在关键函数中设置断点
void UComfyUIClient::Initialize()
{
    // 设置断点在这里
    UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Client Initialized"));
}
```

### 性能分析
```cpp
// 添加性能统计
DECLARE_CYCLE_STAT(TEXT("ComfyUI API Call"), STAT_ComfyUIAPICall, STATGROUP_ComfyUI);

void UComfyUIClient::CallAPI()
{
    SCOPE_CYCLE_COUNTER(STAT_ComfyUIAPICall);
    // API调用逻辑
}
```
├── Resources/                          # 插件资源
│   └── (图标和其他资源文件)
└── README.md                          # 本文件
```

## 开发要求

### 环境要求
- Unreal Engine 5.1 或更高版本
- Visual Studio 2019/2022 with C++ support
- ComfyUI服务器 (本地或远程)

### 依赖模块
- Core
- CoreUObject
- Engine
- UnrealEd
- Slate/SlateCore
- HTTP
- Json
- EditorStyle
- ToolMenus

## 安装说明

### 1. 插件安装
1. 将插件文件夹复制到UE5项目的 `Plugins` 目录
2. 重新生成项目文件
3. 编译项目
4. 在插件管理器中启用 "ComfyUI Integration"

### 2. ComfyUI服务器设置
1. 安装并启动ComfyUI服务器
2. 确保服务器运行在默认端口 `8188`
3. 在插件设置中配置服务器URL

### 3. 使用方法

#### 基础使用流程
1. 在UE5编辑器中，通过 `Window > ComfyUI Integration` 打开插件界面
2. 配置ComfyUI服务器URL
3. 选择工作流类型
4. 输入提示词和参数
5. 点击生成开始AI内容创建

#### 🆕 图像输入功能 (v1.0.1新增)
对于支持图像输入的工作流（图生图、图生3D等），现在可以：
- **拖拽资产**: 直接从内容浏览器拖拽纹理资产到输入框
- **加载文件**: 使用"加载图像"按钮从磁盘选择图片文件
- **清除图像**: 使用"清除图像"按钮移除当前输入图像

**拖拽使用方法**:
1. 在内容浏览器中找到纹理资产（.uasset文件）
2. 点击并拖拽到"输入图像"区域
3. 悬停时会看到蓝色高亮边框
4. 释放鼠标完成拖拽
5. 图像立即显示在输入框中

> 📝 **提示**: 支持拖拽的资产类型为UTexture2D纹理资产。更多详细信息请参考 [DRAG_DROP_FEATURE.md](./DRAG_DROP_FEATURE.md)

## 开发指南

### 添加新的工作流类型

1. 在 `ComfyUIWidget.h` 中的 `EWorkflowType` 枚举添加新类型
2. 在 `ComfyUIClient.cpp` 的 `InitializeWorkflowConfigs()` 中添加配置
3. 更新UI文本和界面逻辑

### 自定义工作流模板

工作流模板使用JSON格式，支持占位符：
- `{POSITIVE_PROMPT}`: 正向提示词
- `{NEGATIVE_PROMPT}`: 负向提示词
- `{INPUT_IMAGE}`: 输入图像 (用于图生图工作流)

### 扩展UI功能

在 `ComfyUIWidget.cpp` 中添加新的UI组件和交互逻辑。使用Slate框架构建响应式界面。

## API参考

### UComfyUIClient

主要的ComfyUI通信客户端类：

```cpp
// 设置服务器URL
void SetServerUrl(const FString& Url);

// 生成图像
void GenerateImage(const FString& Prompt, const FString& NegativePrompt, 
                  EWorkflowType WorkflowType, const FOnImageGenerated& OnComplete);

// 检查服务器状态
void CheckServerStatus();
```

### SComfyUIWidget

主要的UI组件类，提供完整的用户交互界面。

## 故障排除

### 🔧 常见问题及解决方案

#### 1. **无法连接到ComfyUI服务器**
- ✅ **检查服务器状态**: 确保ComfyUI服务器正在运行
- ✅ **验证URL配置**: 默认URL为 `http://127.0.0.1:8188`
- ✅ **检查防火墙**: 确保端口8188未被阻止
- ✅ **测试连接**: 在浏览器中访问ComfyUI URL

#### 2. **生成失败或超时**
- ✅ **检查工作流**: 确保ComfyUI加载了正确的模型文件
- ✅ **验证提示词**: 检查提示词格式和内容
- ✅ **查看日志**: 在UE5 Output Log中查看详细错误信息
- 🔄 **内存不足**: 大型图像生成可能需要更多GPU内存

#### 3. **编译错误**
- ✅ **检查UE5版本**: 确保使用UE5.1或更高版本
- ✅ **重新生成项目**: 右键.uproject文件重新生成VS项目
- ✅ **清理构建**: 删除Binaries和Intermediate文件夹后重新编译
- ✅ **检查依赖**: 确保所有依赖模块正确配置

#### 4. **插件未加载**
- ✅ **启用插件**: 在Edit->Plugins中搜索并启用ComfyUI Integration
- ✅ **重启编辑器**: 启用插件后需要重启UE5编辑器
- ✅ **检查日志**: 查看启动日志中的插件加载信息

### 🐛 调试技巧

#### 启用详细日志记录
在 `Config/DefaultEngine.ini` 中添加：
```ini
[Core.Log]
LogComfyUIIntegration=Verbose
LogSlate=Log
LogHttp=Log
LogJson=Log
```

#### 使用断点调试
```cpp
// 在关键函数中设置断点进行调试
void UComfyUIClient::Initialize()
{
    // 在这里设置断点
    UE_LOG(LogComfyUIIntegration, Log, TEXT("ComfyUI Client Initialized"));
}
```

#### 性能分析
```cpp
// 添加性能统计宏
DECLARE_CYCLE_STAT(TEXT("ComfyUI API Call"), STAT_ComfyUIAPICall, STATGROUP_ComfyUI);

void UComfyUIClient::CallAPI()
{
    SCOPE_CYCLE_COUNTER(STAT_ComfyUIAPICall);
    // API调用逻辑
}
```

### 📞 获取帮助

如果遇到问题：
1. **查看文档**: 首先查看本README和PROJECT_STATUS.md
2. **检查日志**: UE5 Output Log通常包含有用的错误信息
3. **搜索Issues**: 在GitHub Issues中搜索类似问题
4. **提交Issue**: 提供详细的错误信息和重现步骤
5. **社区支持**: 参与GitHub Discussions

## 📚 相关文档

- **[项目状态](./PROJECT_STATUS.md)** - 详细的开发进度和计划
- **[工作流集成指南](./Documentation/workflow-integration-guide.md)** - 如何添加自定义工作流
- **[开发者指南](./Documentation/developer-guide.md)** - 插件开发和扩展指南

## 🔄 版本历史

### v1.0.0-alpha (2025年7月18日)
- ✅ 初始版本发布
- ✅ 实现基础插件架构
- ✅ 完成Slate UI界面
- ✅ 支持文生图和图生图工作流
- ✅ HTTP通信和错误处理
- 🔄 图像保存功能开发中

### 计划中的版本
- **v1.1.0** - 完整的图像保存和资产管理
- **v1.2.0** - 3D模型生成支持
- **v1.3.0** - 纹理生成和材质系统
- **v2.0.0** - 工作流编辑器和高级功能

## 🤝 贡献指南

### 如何贡献
1. **Fork项目** - 在GitHub上fork这个项目
2. **创建分支** - 为你的功能创建一个新分支
3. **开发功能** - 实现功能或修复bug
4. **测试代码** - 确保代码能正常编译和运行
5. **提交PR** - 创建Pull Request并描述你的更改

### 开发规范
- 📖 **遵循UE5 C++编码规范**
- 📝 **保持代码可读性和可维护性**
- 💬 **添加适当的注释和文档**
- 🧪 **进行充分的测试**
- ✨ **保持与现有代码风格一致**

### 贡献类型
- **🐛 Bug修复**: 报告和修复问题
- **✨ 新功能**: 实现新的功能特性
- **📚 文档**: 改进文档和说明
- **🎨 UI/UX**: 改进用户界面和体验
- **⚡ 性能**: 优化性能和内存使用
- **🧪 测试**: 添加测试覆盖

## 📄 许可证

本项目采用 **MIT许可证** - 查看 [LICENSE](./LICENSE) 文件了解详情。

## 🙏 致谢

- **Unreal Engine团队** - 提供优秀的开发框架
- **ComfyUI社区** - 创新的AI工具和生态系统
- **所有贡献者** - 感谢每一位参与项目的开发者
- **测试人员** - 感谢提供反馈和bug报告

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- **GitHub Issues** - 报告bug和功能请求
- **GitHub Discussions** - 技术讨论和社区交流
- **电子邮件** - [your-email@example.com] *(请替换为实际邮箱)*

## ⚠️ 免责声明

**注意**: 这是一个开发中的项目，当前版本为alpha测试版。某些功能可能还在完善中，建议在生产环境使用前进行充分测试。

### 使用建议
- 🧪 **测试环境**: 建议先在测试项目中试用
- 💾 **备份数据**: 使用前备份重要项目文件
- 📋 **查看状态**: 定期查看项目状态文档了解最新进展
- 🐛 **报告问题**: 遇到问题请及时反馈

---

**最后更新**: 2025年7月18日  
**版本**: v1.0.0-alpha  
**维护状态**: 🔄 活跃开发中

感谢使用ComfyUI Integration Plugin! 🚀
