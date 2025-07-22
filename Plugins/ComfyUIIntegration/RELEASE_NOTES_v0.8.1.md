# ComfyUI Integration Plugin v0.8.1 发布说明

*发布日期: 2025年7月21日*

## 🆕 新功能

### 快捷键支持
- **新增可自定义快捷键**：现在可以通过快捷键快速打开ComfyUI窗口
- **默认快捷键**：`Ctrl+Shift+Q`（精心选择，避免与UE5、3ds Max、Maya等常用软件冲突）
- **自定义设置**：用户可通过 Edit > Editor Preferences > Keyboard Shortcuts 自定义快捷键

## 🐛 Bug修复

### 快捷键响应问题修复
- **修复了快捷键不响应的问题**：快捷键现在能在编辑器全局范围内正确响应
- **技术解决方案**：将插件命令绑定到Level Editor的全局命令列表，而不仅仅是插件内部的命令列表
- **参考资料**：基于UE5社区最佳实践和开发者经验
- **测试验证**：在UE5.5环境下通过完整测试

## ⚙️ 技术改进

### 命令系统增强
- 更新 `FComfyUIIntegrationCommands::RegisterCommands()` 方法
- 添加默认快捷键绑定到UI命令
- 保持向后兼容性，不影响现有功能

## 📚 文档更新

### 用户指南更新
- 更新 `BLUEPRINT_USER_GUIDE.md`，添加快捷键使用说明
- 提供自定义快捷键的详细步骤
- 推荐多种替代快捷键组合

### README更新
- 将快捷键支持标记为已完成功能
- 添加版本信息和功能描述

## 🎯 使用方法

### 1. 使用默认快捷键
- 在UE5编辑器中按下 `Ctrl+Shift+Q` 即可打开ComfyUI窗口

### 2. 自定义快捷键
1. 前往：**Edit > Editor Preferences**
2. 选择：**General > Keyboard Shortcuts**
3. 搜索：`ComfyUI`
4. 找到 `ComfyUI 集成` 命令并设置新的快捷键

### 3. 推荐的替代快捷键
- `Ctrl+Alt+C` - 简洁易记
- `F11` - 功能键，不易冲突
- `Ctrl+F12` - 适合高级用户

## 🔧 兼容性

### 支持的UE版本
- Unreal Engine 5.3+
- Unreal Engine 5.4
- Unreal Engine 5.5

### 平台支持
- Windows 10/11 (64位)

## ⚠️ 注意事项

- 默认快捷键 `Ctrl+Shift+Q` 经过精心选择，避免与常见软件冲突
- 如果快捷键不响应，请检查是否有其他插件或软件占用了相同组合
- 快捷键设置保存在UE5编辑器配置中，切换项目时设置会保持

## 🐛 已知问题

- 无已知问题

## 📝 更新说明

此版本专注于用户体验改进，通过添加快捷键支持使ComfyUI窗口的访问更加便捷。这是一个向后兼容的更新，不会影响现有工作流或配置。

---

**完整版本历史**: [查看所有发布说明](./PROJECT_STATUS.md)
