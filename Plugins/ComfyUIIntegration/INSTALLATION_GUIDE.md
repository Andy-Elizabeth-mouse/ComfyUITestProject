# ComfyUI Integration Plugin - 用户安装指南

## 📋 系统要求

### 必需软件
- **Unreal Engine 5.3+** (推荐 5.4 或 5.5)
- **ComfyUI** (最新稳定版)
- **Windows 10/11** (主要支持平台)

### 硬件要求
- **内存**: 至少 8GB RAM (推荐 16GB+)
- **显卡**: 支持 CUDA 的 NVIDIA 显卡 (用于 ComfyUI)
- **存储**: 至少 2GB 可用空间

## 🚀 安装步骤

### 步骤 1: 准备 ComfyUI 环境

1. **下载和安装 ComfyUI**
   ```bash
   # 方法一：使用 ComfyUI Portable (推荐新手)
   # 下载 ComfyUI_windows_portable.zip
   # 解压到合适的位置

   # 方法二：从源码安装
   git clone https://github.com/comfyanonymous/ComfyUI.git
   cd ComfyUI
   pip install -r requirements.txt
   ```

2. **启动 ComfyUI 服务**
   ```bash
   # 在 ComfyUI 目录下运行
   python main.py --listen 127.0.0.1 --port 8188
   ```

3. **验证 ComfyUI 运行**
   - 浏览器访问：`http://127.0.0.1:8188`
   - 应该能看到 ComfyUI 的 Web 界面

### 步骤 2: 安装插件到 UE5 项目

#### 🎯 项目类型兼容性

本插件支持以下项目类型：
- ✅ **蓝图项目**：完全支持，推荐使用预编译包
- ✅ **C++项目**：完全支持，可使用预编译包或源码编译
- ✅ **混合项目**：完全支持

> **蓝图用户注意**：本插件为编辑器插件，无需项目具备C++编译环境即可使用。

#### 方法一：预编译包安装 (推荐所有用户)

1. **下载预编译包**
   ```
   ComfyUIIntegration-v0.8.0-beta-Win64.zip
   ```

2. **自动安装（推荐）**
   ```bash
   # 解压后运行
   Install.bat
   # 按提示输入项目路径即可
   ```

3. **手动安装**
   ```bash
   # 解压到项目的 Plugins 目录
   你的UE5项目/Plugins/ComfyUIIntegration/
   ```

4. **重启 UE5 编辑器**
   - 关闭虚幻编辑器
   - 重新打开项目
   - 编辑器会自动加载插件

#### 方法二：从源码安装 (开发者推荐)

1. **下载插件源码**
   ```bash
   # 下载或克隆插件代码
   # 确保获得完整的 ComfyUIIntegration 文件夹
   ```

2. **复制到项目**
   ```bash
   # 将 ComfyUIIntegration 文件夹复制到：
   你的UE5项目/Plugins/ComfyUIIntegration/
   ```

3. **重新生成项目文件**
   ```bash
   # 右键点击 .uproject 文件
   # 选择 "Generate Visual Studio project files"
   ```

4. **编译项目**
   ```bash
   # 在 Visual Studio 中编译项目
   # 或使用命令行：
   "E:\ue\UE_5.5\Engine\Build\BatchFiles\Build.bat" YourProjectEditor Win64 Development "C:\Path\To\Your\Project.uproject" -waitmutex
   ```

#### 方法二：预编译包安装 (用户推荐)

1. **下载预编译包**
   ```
   ComfyUIIntegration-v0.8.0-beta-UE5.x.zip
   ```

2. **解压安装**
   ```bash
   # 解压到项目的 Plugins 目录
   你的UE5项目/Plugins/ComfyUIIntegration/
   ```

3. **重启 UE5 编辑器**
   - 关闭虚幻编辑器
   - 重新打开项目
   - 编辑器会自动加载插件

### 步骤 3: 启用插件

1. **打开插件管理器**
   - 在 UE5 中：**Edit > Plugins**

2. **找到 ComfyUI Integration**
   - 搜索 "ComfyUI" 或在 "Editor" 分类下找到
   - 勾选 "Enabled" 复选框

3. **重启编辑器**
   - 点击 "Restart Now" 按钮

## ⚙️ 配置指南

### 基础配置

1. **打开插件界面**
   - **Tools > ComfyUI Integration**

2. **配置 ComfyUI 服务器**
   - 服务器地址：`http://127.0.0.1:8188`
   - 点击 "Test Connection" 验证连接

3. **选择工作流类型**
   - 文生图：Text to Image
   - 图生图：Image to Image  
   - 其他类型根据需要选择

### 高级配置

1. **工作流模板配置**
   ```bash
   # 将自定义工作流文件 (.json) 放置到：
   你的项目/Content/ComfyUI/Workflows/
   ```

2. **输出目录配置**
   ```bash
   # 生成的内容默认保存到：
   你的项目/Content/ComfyUI/Generated/
   ```

## 🎯 第一次使用

### 测试文生图功能

1. **启动插件**
   - Tools > ComfyUI Integration

2. **选择工作流类型**
   - 选择 "Text to Image"

3. **输入提示词**
   ```
   正向提示词: a beautiful landscape, mountain, river, sunset, photorealistic
   负向提示词: blurry, low quality, bad anatomy
   ```

4. **生成图像**
   - 点击 "Generate" 按钮
   - 等待生成完成

5. **保存结果**
   - 点击 "Save" 保存到项目资产

### 测试图生图功能（重点）

1. **准备输入图像**
   - 在内容浏览器中导入一张纹理
   
2. **选择工作流类型**
   - 选择 "Image to Image"

3. **设置输入图像** 🆕
   - **方法一**: 点击 "Load Image" 按钮选择文件
   - **方法二**: 直接从内容浏览器拖拽纹理资产到输入框 ⭐

4. **输入提示词并生成**

## 🔧 常见问题解决

### 安装问题

**Q: 插件无法加载？**
```bash
A: 检查以下事项：
1. UE5 版本是否兼容 (5.3+)
2. 插件文件是否完整
3. 重新生成项目文件
4. 清理编译缓存后重新编译
```

**Q: 编译错误？**
```bash
A: 常见解决方法：
1. 确保 Visual Studio 已安装 UE5 工作负载
2. 检查 Windows SDK 版本
3. 清理 Binaries 和 Intermediate 文件夹
4. 重新生成项目文件
```

### 连接问题

**Q: 无法连接 ComfyUI？**
```bash
A: 检查清单：
1. ComfyUI 是否正在运行
2. 端口 8188 是否被占用
3. 防火墙设置
4. ComfyUI 启动参数是否正确
```

**Q: 生成速度慢？**
```bash
A: 优化建议：
1. 确保使用支持 CUDA 的显卡
2. 调整 ComfyUI 模型设置
3. 减少输出分辨率进行测试
4. 检查系统资源使用情况
```

### 使用问题

**Q: 拖拽功能不工作？**
```bash
A: 检查事项：
1. 拖拽的是否为纹理资产 (Texture2D)
2. 当前工作流是否支持图像输入
3. 纹理格式是否支持
4. 重启编辑器后重试
```

## 📞 获取帮助

### 联系方式
- **技术支持**: [支持邮箱]
- **用户社区**: [Discord/论坛链接]
- **Bug 报告**: [GitHub Issues]

### 日志文件位置
```bash
# UE5 日志文件
你的项目/Saved/Logs/

# ComfyUI 日志
ComfyUI安装目录/logs/
```

## 📚 更多资源

- **使用教程**: [链接]
- **最佳实践**: [链接]  
- **API 文档**: [链接]
- **示例工作流**: [链接]

---

**祝您使用愉快！如有问题请随时联系我们。**

*版本: v0.8.0-beta | 更新时间: 2025年7月21日*
