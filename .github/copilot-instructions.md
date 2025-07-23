<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# ComfyUI Integration Plugin for Unreal Engine 5

这是一个Unreal Engine 5编辑器插件项目，用于集成ComfyUI工作流功能。

## 项目特性

- 集成ComfyUI API以支持AI驱动的内容生成
- 支持多种工作流类型：文生图、图生图、文生3D、图生3D、纹理生成
- 自定义Slate UI界面用于交互式内容生成
- 内置资产管理和预览功能
- 与Unreal Engine编辑器无缝集成

## 代码规范

在为此项目生成代码时，请遵循以下准则：

1. **Unreal Engine C++规范**：
   - 使用Unreal Engine的命名约定（PascalCase for classes, camelCase for variables）
   - 正确使用UE5的反射系统（UCLASS, UFUNCTION, UPROPERTY等）
   - 遵循UE5的内存管理模式（UObject, TSharedPtr, TUniquePtr等）

2. **插件架构**：
   - 保持模块化设计，清晰分离UI、网络通信和业务逻辑
   - 使用适当的UE5编辑器扩展API
   - 确保线程安全，特别是在处理HTTP请求时

3. **Slate UI开发**：
   - 使用Slate框架构建UI组件
   - 保持响应式设计和良好的用户体验
   - 正确处理本地化文本

4. **ComfyUI集成**：
   - 使用UE5的HTTP模块进行API通信
   - 正确处理异步操作和错误情况
   - 支持可配置的工作流模板

5. **资产管理**：
   - 正确处理纹理创建和内存管理
   - 支持多种图像格式
   - 实现保存/加载功能

6. **文档创建**:
   - 为所有重要功能、API及其更新编写详细的文档
   - 维护API变更日志和版本兼容性说明

7. **任何时候都不要轻易重新创建项目或删除并重新创建文件**，除非确实需要。

请确保生成的代码能够编译并与UE5编辑器正确集成。

附：编译命令
```powershell
& "E:\ue\UE_5.5\Engine\Build\BatchFiles\Build.bat" ComfyUITestProjectEditor Win64 Development "C:\UnrealProjects\ComfyUITestProject\ComfyUITestProject.uproject" -waitmutex
```
重新生成项目文件
```cmd
E:\ue\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe -projectfiles -project="C:\UnrealProjects\ComfyUITestProject\ComfyUITestProject.uproject" -game -rocket -progress -engine -VSCode
```