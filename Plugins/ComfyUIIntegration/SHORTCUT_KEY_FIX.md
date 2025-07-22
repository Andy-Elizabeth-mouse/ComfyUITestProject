# ComfyUI快捷键功能测试指南

## 快捷键修复说明

### 问题原因
根据UE5插件开发的经验，快捷键不响应的主要原因是：
- 快捷键命令没有绑定到全局命令列表上下文
- 仅在特定窗口或菜单中的命令绑定无法在全局响应

### 解决方案
我们采用了以下修复方案：

1. **保持原有的命令映射**：
   ```cpp
   PluginCommands->MapAction(
       FComfyUIIntegrationCommands::Get().OpenPluginWindow,
       FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::PluginButtonClicked),
       FCanExecuteAction());
   ```

2. **将命令绑定到Level Editor的全局命令列表**：
   ```cpp
   // 将命令绑定到Level Editor的CommandList，使快捷键在全局生效
   FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
   LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());
   ```

3. **设置合适的默认快捷键**：
   ```cpp
   UI_COMMAND(OpenPluginWindow, "ComfyUI 集成", "打开ComfyUI集成窗口", EUserInterfaceActionType::Button, 
       FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Q));
   ```

## 测试步骤

### 1. 启动项目
1. 启动UE5编辑器
2. 打开ComfyUITestProject项目
3. 等待插件加载完成

### 2. 验证快捷键功能
1. **测试默认快捷键**：
   - 在编辑器任意位置按下 `Ctrl+Shift+Q`
   - 应该能看到ComfyUI窗口弹出

2. **测试菜单访问**：
   - 通过菜单：**Window > ComfyUI 集成**
   - 应该能看到ComfyUI窗口弹出

### 3. 自定义快捷键测试
1. 前往：**Edit > Editor Preferences**
2. 选择：**General > Keyboard Shortcuts**
3. 搜索："ComfyUI"
4. 找到 `ComfyUI 集成` 命令
5. 修改为其他快捷键组合（如 `Ctrl+Alt+C`）
6. 保存设置并测试新的快捷键

### 4. 冲突检测测试
测试以下快捷键组合，确认不与常用软件冲突：

#### UE5常用快捷键（应避免）：
- `Ctrl+S` (保存)
- `Ctrl+Z` (撤销) 
- `Ctrl+Y` (重做)
- `Ctrl+C/V/X` (复制/粘贴/剪切)
- `F5` (播放)
- `Alt+P` (PIE模式)

#### 3ds Max常用快捷键（应避免）：
- `Alt+W` (最大化视口)
- `Ctrl+Alt+Z` (场景撤销)
- `F10` (渲染设置)

#### Maya常用快捷键（应避免）：
- `Alt+鼠标` (视图导航)
- `Ctrl+D` (复制)
- `G` (重复上次操作)

## 推荐快捷键组合

### 安全的快捷键选择：
1. **`Ctrl+Shift+Q`** （默认）- 很少被其他软件使用
2. **`Ctrl+Alt+C`** - C代表ComfyUI
3. **`F11`** - 功能键通常较安全
4. **`Ctrl+F12`** - 组合功能键

### 避免使用的快捷键：
- 单一修饰键组合 (如 `Ctrl+Q`)
- 常见的文本操作快捷键
- 各大3D软件的核心功能快捷键

## 故障排除

### 如果快捷键仍然不响应：
1. **检查编辑器焦点**：确保UE5编辑器窗口处于活动状态
2. **检查快捷键冲突**：查看Editor Preferences中是否有其他命令使用了相同快捷键
3. **重启编辑器**：重新启动UE5编辑器以确保插件正确加载
4. **检查插件状态**：在Edit > Plugins中确认"ComfyUI Integration"插件已启用

### 调试信息：
- 查看Output Log中的插件加载信息
- 检查是否有错误或警告消息
- 确认插件模块已正确加载

## 版本信息
- **修复版本**: v0.8.1
- **修复日期**: 2025年7月21日
- **UE版本支持**: 5.3+
- **默认快捷键**: `Ctrl+Shift+Q`

---

*此文档基于UE5插件开发最佳实践和社区经验编写*
