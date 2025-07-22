## 前置要求

- **UE5蓝图项目**（5.3+版本）
- **ComfyUI服务**（本地或远程运行）

## 快速安装

### 1. 下载发布包
- 下载：`ComfyUIIntegration-v0.8.0-beta-Win64.zip`
- 大小：约15MB

### 2. 一键安装
1. 解压下载的zip文件
2. 双击运行 `Install.bat`
3. 输入你的UE5项目完整路径，例如：
   ```
   C:\MyProjects\MyBlueprintProject
   ```
4. 等待安装完成

### 3. 启动插件
1. 关闭UE5编辑器（如果已打开）
2. 重新打开你的蓝图项目
3. 编辑器会自动检测并加载插件
4. 可通过以下方式打开ComfyUI窗口：
   - **菜单方式**：在菜单栏找到：**Window > ComfyUI 集成**
   - **快捷键方式**：按下 `Ctrl+Shift+Q`（默认快捷键）

## 自定义快捷键

如需自定义打开ComfyUI窗口的快捷键：

1. 在UE5编辑器中，前往：**Edit > Editor Preferences**
2. 在左侧菜单中选择：**General > Keyboard Shortcuts** 
3. 搜索：`ComfyUI`
4. 找到 `ComfyUI 集成` 命令
5. 点击快捷键栏位，输入您偏好的新快捷键组合
6. 点击应用保存设置

**推荐快捷键**：
- `Ctrl+Shift+Q` (默认)
- `Ctrl+Alt+C` 
- `F11`
- `Ctrl+F12`

> **注意**：避免使用UE5、3ds Max、Maya等软件的常用快捷键，以防冲突。

## 常见问题

### Q: 安装失败，提示找不到项目文件？
**A**: 确保输入的路径包含.uproject文件，例如：
```
正确：C:\MyProject\MyGame
错误：C:\MyProject\MyGame\Content
```

### Q: 插件菜单没有出现？
**A**: 
1. 检查 Edit > Plugins 中是否启用了"ComfyUI Integration"
2. 重启编辑器
3. 确认UE版本为5.3或更高

---

*版本: v0.8.0-beta | 更新: 2025年7月21日*
