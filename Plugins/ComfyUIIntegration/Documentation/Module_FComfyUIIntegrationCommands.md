# FComfyUIIntegrationCommands - 命令系统模块开发文档

## 概述

`FComfyUIIntegrationCommands`是ComfyUI Integration插件的命令系统，负责管理插件的UI命令、菜单项和快捷键绑定。该类继承自`TCommands`模板类，提供了标准的UE5命令系统集成。

## 文件位置

- **头文件**: `Source/ComfyUIIntegration/Public/ComfyUIIntegrationCommands.h`
- **实现文件**: `Source/ComfyUIIntegration/Private/ComfyUIIntegrationCommands.cpp`

## 类结构

```cpp
class FComfyUIIntegrationCommands : public TCommands<FComfyUIIntegrationCommands>
{
public:
    FComfyUIIntegrationCommands()
        : TCommands<FComfyUIIntegrationCommands>(TEXT("ComfyUIIntegration"), 
            NSLOCTEXT("Contexts", "ComfyUIIntegration", "ComfyUI Integration Plugin"), 
            NAME_None, FComfyUIIntegrationStyle::GetStyleSetName())
    {
    }

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    // 命令定义
    TSharedPtr<FUICommandInfo> OpenPluginWindow;
    TSharedPtr<FUICommandInfo> OpenSettings;
    TSharedPtr<FUICommandInfo> RefreshConnection;
    TSharedPtr<FUICommandInfo> ShowHelp;
    TSharedPtr<FUICommandInfo> ToggleDebugMode;
};
```

## 核心功能

### 1. 构造函数和初始化

#### 构造函数

```cpp
FComfyUIIntegrationCommands()
    : TCommands<FComfyUIIntegrationCommands>(TEXT("ComfyUIIntegration"), 
        NSLOCTEXT("Contexts", "ComfyUIIntegration", "ComfyUI Integration Plugin"), 
        NAME_None, FComfyUIIntegrationStyle::GetStyleSetName())
{
}
```

**参数说明**：
- **CommandName**: `TEXT("ComfyUIIntegration")` - 命令上下文名称
- **ContextDesc**: `NSLOCTEXT("Contexts", "ComfyUIIntegration", "ComfyUI Integration Plugin")` - 本地化的上下文描述
- **ParentContext**: `NAME_None` - 父命令上下文（无）
- **StyleSetName**: `FComfyUIIntegrationStyle::GetStyleSetName()` - 关联的样式集名称

**功能描述**：
- 初始化命令系统上下文
- 设置本地化支持
- 关联样式集用于图标和视觉效果

### 2. 命令注册

#### RegisterCommands()

```cpp
void FComfyUIIntegrationCommands::RegisterCommands()
{
    // 注册主要命令
    UI_COMMAND(OpenPluginWindow, 
        "ComfyUI Integration", 
        "Open ComfyUI Integration window", 
        EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C));
    
    UI_COMMAND(OpenSettings, 
        "Settings", 
        "Open ComfyUI Integration settings", 
        EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::S));
    
    UI_COMMAND(RefreshConnection, 
        "Refresh Connection", 
        "Refresh connection to ComfyUI server", 
        EUserInterfaceActionType::Button, 
        FInputChord(EKeys::F5));
    
    UI_COMMAND(ShowHelp, 
        "Help", 
        "Show ComfyUI Integration help", 
        EUserInterfaceActionType::Button, 
        FInputChord(EKeys::F1));
    
    UI_COMMAND(ToggleDebugMode, 
        "Debug Mode", 
        "Toggle ComfyUI Integration debug mode", 
        EUserInterfaceActionType::ToggleButton, 
        FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::D));
}
```

**UI_COMMAND宏参数**：
- **CommandVariable**: 命令变量名
- **FriendlyName**: 友好显示名称
- **Description**: 命令描述
- **CommandType**: 命令类型（按钮、切换按钮等）
- **InputChord**: 默认快捷键组合

## 命令详细说明

### 1. 主窗口命令

#### OpenPluginWindow

```cpp
TSharedPtr<FUICommandInfo> OpenPluginWindow;
```

**功能描述**：
- 打开ComfyUI Integration主窗口
- 提供主要的AI生成功能界面

**快捷键**：
- `Ctrl + Shift + C`

**使用场景**：
- 用户需要使用ComfyUI生成功能时
- 从菜单或快捷键快速访问

**实现示例**：
```cpp
// 在模块中映射命令
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().OpenPluginWindow,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenPluginWindow),
    FCanExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::CanOpenPluginWindow)
);

// 执行函数
void FComfyUIIntegrationModule::OpenPluginWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ComfyUIIntegrationTabName);
}

// 可执行条件检查
bool FComfyUIIntegrationModule::CanOpenPluginWindow()
{
    return true; // 始终可执行
}
```

### 2. 设置命令

#### OpenSettings

```cpp
TSharedPtr<FUICommandInfo> OpenSettings;
```

**功能描述**：
- 打开插件设置界面
- 允许用户配置服务器地址、默认参数等

**快捷键**：
- `Ctrl + Alt + S`

**使用场景**：
- 首次使用时配置服务器
- 调整生成参数
- 管理工作流模板

**实现示例**：
```cpp
// 映射设置命令
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().OpenSettings,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenSettings),
    FCanExecuteAction()
);

// 执行函数
void FComfyUIIntegrationModule::OpenSettings()
{
    // 创建设置窗口
    TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
        .Title(LOCTEXT("SettingsWindowTitle", "ComfyUI Integration Settings"))
        .ClientSize(FVector2D(600, 400))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
    
    // 创建设置Widget
    TSharedRef<SWidget> SettingsWidget = SNew(SComfyUISettingsWidget);
    
    SettingsWindow->SetContent(SettingsWidget);
    FSlateApplication::Get().AddWindow(SettingsWindow);
}
```

### 3. 连接刷新命令

#### RefreshConnection

```cpp
TSharedPtr<FUICommandInfo> RefreshConnection;
```

**功能描述**：
- 刷新与ComfyUI服务器的连接
- 重新检测服务器状态和可用工作流

**快捷键**：
- `F5`

**使用场景**：
- 服务器连接出现问题时
- 服务器重启后重新连接
- 检查新的工作流模板

**实现示例**：
```cpp
// 映射刷新命令
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().RefreshConnection,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::RefreshConnection),
    FCanExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::CanRefreshConnection)
);

// 执行函数
void FComfyUIIntegrationModule::RefreshConnection()
{
    // 获取当前活动的ComfyUI客户端
    UComfyUIClient* Client = GetActiveClient();
    if (Client)
    {
        // 检查服务器状态
        Client->CheckServerStatus(FOnImageGeneratedDynamic::CreateRaw(this, &FComfyUIIntegrationModule::OnConnectionRefreshed));
        
        // 获取可用工作流
        Client->GetAvailableWorkflows();
    }
}

// 可执行条件检查
bool FComfyUIIntegrationModule::CanRefreshConnection()
{
    return GetActiveClient() != nullptr;
}
```

### 4. 帮助命令

#### ShowHelp

```cpp
TSharedPtr<FUICommandInfo> ShowHelp;
```

**功能描述**：
- 显示插件帮助信息
- 提供使用指南和故障排除

**快捷键**：
- `F1`

**使用场景**：
- 新用户学习插件使用
- 查找功能说明
- 故障排除

**实现示例**：
```cpp
// 映射帮助命令
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().ShowHelp,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ShowHelp),
    FCanExecuteAction()
);

// 执行函数
void FComfyUIIntegrationModule::ShowHelp()
{
    // 打开帮助文档
    FString HelpUrl = TEXT("https://github.com/your-repo/ComfyUI-Integration/wiki");
    FPlatformProcess::LaunchURL(*HelpUrl, nullptr, nullptr);
    
    // 或者显示内置帮助窗口
    TSharedRef<SWindow> HelpWindow = SNew(SWindow)
        .Title(LOCTEXT("HelpWindowTitle", "ComfyUI Integration Help"))
        .ClientSize(FVector2D(800, 600))
        .SupportsMaximize(true)
        .SupportsMinimize(true);
    
    TSharedRef<SWidget> HelpWidget = SNew(SComfyUIHelpWidget);
    HelpWindow->SetContent(HelpWidget);
    FSlateApplication::Get().AddWindow(HelpWindow);
}
```

### 5. 调试模式命令

#### ToggleDebugMode

```cpp
TSharedPtr<FUICommandInfo> ToggleDebugMode;
```

**功能描述**：
- 切换插件调试模式
- 启用/禁用详细日志和调试信息

**快捷键**：
- `Ctrl + Shift + D`

**使用场景**：
- 开发和调试时使用
- 排查问题时启用详细日志
- 性能分析

**实现示例**：
```cpp
// 映射调试命令
PluginCommands->MapAction(
    FComfyUIIntegrationCommands::Get().ToggleDebugMode,
    FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ToggleDebugMode),
    FCanExecuteAction(),
    FIsActionChecked::CreateRaw(this, &FComfyUIIntegrationModule::IsDebugModeEnabled)
);

// 执行函数
void FComfyUIIntegrationModule::ToggleDebugMode()
{
    bDebugModeEnabled = !bDebugModeEnabled;
    
    if (bDebugModeEnabled)
    {
        UE_LOG(LogComfyUIIntegration, Warning, TEXT("Debug mode enabled"));
        
        // 启用详细日志
        LogComfyUIIntegration.SetVerbosity(ELogVerbosity::VeryVerbose);
        
        // 启用UI调试
        FSlateApplication::Get().EnableVisualDebugger();
    }
    else
    {
        UE_LOG(LogComfyUIIntegration, Warning, TEXT("Debug mode disabled"));
        
        // 恢复正常日志级别
        LogComfyUIIntegration.SetVerbosity(ELogVerbosity::Warning);
        
        // 禁用UI调试
        FSlateApplication::Get().DisableVisualDebugger();
    }
}

// 检查状态
bool FComfyUIIntegrationModule::IsDebugModeEnabled() const
{
    return bDebugModeEnabled;
}
```

## 命令绑定和使用

### 1. 在模块中使用命令

```cpp
// 在StartupModule()中注册和绑定命令
void FComfyUIIntegrationModule::StartupModule()
{
    // 注册命令
    FComfyUIIntegrationCommands::Register();
    
    // 创建命令列表
    PluginCommands = MakeShareable(new FUICommandList);
    
    // 绑定所有命令
    BindCommands();
}

void FComfyUIIntegrationModule::BindCommands()
{
    // 绑定主窗口命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenPluginWindow),
        FCanExecuteAction()
    );
    
    // 绑定设置命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().OpenSettings,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenSettings),
        FCanExecuteAction()
    );
    
    // 绑定刷新命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().RefreshConnection,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::RefreshConnection),
        FCanExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::CanRefreshConnection)
    );
    
    // 绑定帮助命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().ShowHelp,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ShowHelp),
        FCanExecuteAction()
    );
    
    // 绑定调试命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().ToggleDebugMode,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ToggleDebugMode),
        FCanExecuteAction(),
        FIsActionChecked::CreateRaw(this, &FComfyUIIntegrationModule::IsDebugModeEnabled)
    );
}
```

### 2. 在菜单中使用命令

```cpp
// 在RegisterMenusInternal()中添加菜单项
void FComfyUIIntegrationModule::RegisterMenusInternal()
{
    // 主菜单
    UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& MainSection = MainMenu->FindOrAddSection("WindowLayout");
    
    // 添加主窗口菜单项
    MainSection.AddMenuEntryWithCommandList(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow, 
        PluginCommands
    );
    
    // 创建ComfyUI子菜单
    FToolMenuSection& ComfyUISection = MainMenu->AddSection("ComfyUI", LOCTEXT("ComfyUIMenu", "ComfyUI"));
    
    // 添加设置菜单项
    ComfyUISection.AddMenuEntryWithCommandList(
        FComfyUIIntegrationCommands::Get().OpenSettings,
        PluginCommands
    );
    
    // 添加分隔符
    ComfyUISection.AddSeparator(NAME_None);
    
    // 添加刷新菜单项
    ComfyUISection.AddMenuEntryWithCommandList(
        FComfyUIIntegrationCommands::Get().RefreshConnection,
        PluginCommands
    );
    
    // 添加帮助菜单项
    ComfyUISection.AddMenuEntryWithCommandList(
        FComfyUIIntegrationCommands::Get().ShowHelp,
        PluginCommands
    );
}
```

### 3. 在工具栏中使用命令

```cpp
// 创建工具栏
void FComfyUIIntegrationModule::CreateToolbar()
{
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("ComfyUI");
    
    // 添加主要按钮
    ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow,
        LOCTEXT("ComfyUIToolbarLabel", "ComfyUI"),
        LOCTEXT("ComfyUIToolbarTooltip", "Open ComfyUI Integration"),
        FSlateIcon(FComfyUIIntegrationStyle::GetStyleSetName(), "ComfyUI.Icon.Small")
    ));
    
    // 添加下拉菜单
    ToolbarSection.AddEntry(FToolMenuEntry::InitComboButton(
        "ComfyUITools",
        FUIAction(),
        FOnGetContent::CreateRaw(this, &FComfyUIIntegrationModule::CreateToolbarMenu),
        LOCTEXT("ComfyUIToolsLabel", "ComfyUI Tools"),
        LOCTEXT("ComfyUIToolsTooltip", "ComfyUI Tools and Settings"),
        FSlateIcon(FComfyUIIntegrationStyle::GetStyleSetName(), "ComfyUI.Icon.Small")
    ));
}

// 创建工具栏下拉菜单
TSharedRef<SWidget> FComfyUIIntegrationModule::CreateToolbarMenu()
{
    FMenuBuilder MenuBuilder(true, PluginCommands);
    
    MenuBuilder.AddMenuEntry(FComfyUIIntegrationCommands::Get().OpenSettings);
    MenuBuilder.AddSeparator();
    MenuBuilder.AddMenuEntry(FComfyUIIntegrationCommands::Get().RefreshConnection);
    MenuBuilder.AddMenuEntry(FComfyUIIntegrationCommands::Get().ShowHelp);
    MenuBuilder.AddSeparator();
    MenuBuilder.AddMenuEntry(FComfyUIIntegrationCommands::Get().ToggleDebugMode);
    
    return MenuBuilder.MakeWidget();
}
```

## 快捷键管理

### 1. 默认快捷键

```cpp
// 在RegisterCommands()中定义的默认快捷键
FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C)  // 打开主窗口
FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::S)    // 打开设置
FInputChord(EKeys::F5)                                              // 刷新连接
FInputChord(EKeys::F1)                                              // 显示帮助
FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::D)  // 切换调试模式
```

### 2. 自定义快捷键

```cpp
// 允许用户自定义快捷键
class FComfyUIKeybindingCustomization
{
public:
    static void RegisterCustomKeybindings()
    {
        // 从配置文件读取自定义快捷键
        FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("ComfyUI_Keybindings.ini");
        
        if (FPaths::FileExists(ConfigPath))
        {
            LoadCustomKeybindings(ConfigPath);
        }
    }
    
    static void LoadCustomKeybindings(const FString& ConfigPath)
    {
        TSharedPtr<FJsonObject> JsonObject;
        FString JsonString;
        
        if (FFileHelper::LoadFileToString(JsonString, *ConfigPath))
        {
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
            
            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                // 解析自定义快捷键
                ParseKeybindings(JsonObject);
            }
        }
    }
    
    static void ParseKeybindings(TSharedPtr<FJsonObject> JsonObject)
    {
        // 解析JSON格式的快捷键配置
        for (auto& Pair : JsonObject->Values)
        {
            FString CommandName = Pair.Key;
            FString KeyString = Pair.Value->AsString();
            
            // 解析键位组合
            FInputChord InputChord = ParseKeyString(KeyString);
            
            // 更新命令的快捷键
            UpdateCommandKeybinding(CommandName, InputChord);
        }
    }
};
```

### 3. 快捷键冲突检测

```cpp
// 检测快捷键冲突
class FComfyUIKeybindingValidator
{
public:
    static bool ValidateKeybindings(const TSharedPtr<FUICommandList>& CommandList)
    {
        TMap<FInputChord, FString> KeyMap;
        bool bHasConflicts = false;
        
        // 检查所有命令的快捷键
        for (const auto& Command : GetAllCommands())
        {
            FInputChord Chord = Command->GetActiveChord();
            
            if (KeyMap.Contains(Chord))
            {
                UE_LOG(LogComfyUIIntegration, Warning, 
                    TEXT("Keybinding conflict detected: %s and %s both use %s"),
                    *KeyMap[Chord], *Command->GetLabel().ToString(), *Chord.GetInputText().ToString());
                
                bHasConflicts = true;
            }
            else
            {
                KeyMap.Add(Chord, Command->GetLabel().ToString());
            }
        }
        
        return !bHasConflicts;
    }
};
```

## 本地化支持

### 1. 本地化字符串

```cpp
// 定义本地化命名空间
#define LOCTEXT_NAMESPACE "ComfyUIIntegrationCommands"

// 使用本地化文本
NSLOCTEXT("Contexts", "ComfyUIIntegration", "ComfyUI Integration Plugin")
LOCTEXT("OpenPluginWindowLabel", "ComfyUI Integration")
LOCTEXT("OpenPluginWindowTooltip", "Open ComfyUI Integration window")
LOCTEXT("OpenSettingsLabel", "Settings")
LOCTEXT("OpenSettingsTooltip", "Open ComfyUI Integration settings")
LOCTEXT("RefreshConnectionLabel", "Refresh Connection")
LOCTEXT("RefreshConnectionTooltip", "Refresh connection to ComfyUI server")
LOCTEXT("ShowHelpLabel", "Help")
LOCTEXT("ShowHelpTooltip", "Show ComfyUI Integration help")
LOCTEXT("ToggleDebugModeLabel", "Debug Mode")
LOCTEXT("ToggleDebugModeTooltip", "Toggle ComfyUI Integration debug mode")

#undef LOCTEXT_NAMESPACE
```

### 2. 多语言支持

```cpp
// 动态本地化
class FComfyUILocalization
{
public:
    static FText GetCommandText(const FString& CommandKey)
    {
        static TMap<FString, FText> LocalizationMap;
        
        if (LocalizationMap.IsEmpty())
        {
            InitializeLocalization();
        }
        
        if (LocalizationMap.Contains(CommandKey))
        {
            return LocalizationMap[CommandKey];
        }
        
        return FText::FromString(CommandKey);
    }
    
    static void InitializeLocalization()
    {
        // 根据当前语言加载本地化文本
        FString CurrentCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
        
        if (CurrentCulture.StartsWith(TEXT("zh")))
        {
            LoadChineseLocalization();
        }
        else if (CurrentCulture.StartsWith(TEXT("ja")))
        {
            LoadJapaneseLocalization();
        }
        else
        {
            LoadEnglishLocalization();
        }
    }
};
```

## 命令状态管理

### 1. 动态启用/禁用

```cpp
// 根据条件动态启用/禁用命令
class FComfyUICommandStateManager
{
public:
    static void UpdateCommandStates(const TSharedPtr<FUICommandList>& CommandList)
    {
        // 检查服务器连接状态
        bool bServerConnected = IsServerConnected();
        
        // 更新刷新命令状态
        CommandList->SetCanExecuteAction(
            FComfyUIIntegrationCommands::Get().RefreshConnection,
            FCanExecuteAction::CreateStatic(&FComfyUICommandStateManager::CanRefreshConnection)
        );
        
        // 更新其他命令状态
        UpdateOtherCommandStates(CommandList, bServerConnected);
    }
    
    static bool CanRefreshConnection()
    {
        // 检查是否有活动的客户端连接
        return GetActiveComfyUIClient() != nullptr;
    }
    
    static bool IsServerConnected()
    {
        UComfyUIClient* Client = GetActiveComfyUIClient();
        return Client && Client->IsConnected();
    }
};
```

### 2. 命令执行状态反馈

```cpp
// 执行命令时的状态反馈
class FComfyUICommandExecutor
{
public:
    static void ExecuteCommandWithFeedback(const FString& CommandName, TFunction<void()> ExecuteFunc)
    {
        // 显示开始执行的通知
        FNotificationInfo StartInfo(FText::FromString(FString::Printf(TEXT("Executing %s..."), *CommandName)));
        StartInfo.ExpireDuration = 2.0f;
        TSharedPtr<SNotificationItem> StartNotification = FSlateNotificationManager::Get().AddNotification(StartInfo);
        
        // 异步执行命令
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ExecuteFunc, StartNotification, CommandName]()
        {
            try
            {
                ExecuteFunc();
                
                // 回到主线程显示成功通知
                AsyncTask(ENamedThreads::GameThread, [StartNotification, CommandName]()
                {
                    if (StartNotification.IsValid())
                    {
                        StartNotification->ExpireAndFadeout();
                    }
                    
                    FNotificationInfo SuccessInfo(FText::FromString(FString::Printf(TEXT("%s completed successfully"), *CommandName)));
                    SuccessInfo.ExpireDuration = 3.0f;
                    FSlateNotificationManager::Get().AddNotification(SuccessInfo);
                });
            }
            catch (const std::exception& e)
            {
                // 回到主线程显示错误通知
                AsyncTask(ENamedThreads::GameThread, [StartNotification, CommandName, e]()
                {
                    if (StartNotification.IsValid())
                    {
                        StartNotification->ExpireAndFadeout();
                    }
                    
                    FNotificationInfo ErrorInfo(FText::FromString(FString::Printf(TEXT("%s failed: %s"), *CommandName, UTF8_TO_TCHAR(e.what()))));
                    ErrorInfo.ExpireDuration = 5.0f;
                    FSlateNotificationManager::Get().AddNotification(ErrorInfo);
                });
            }
        });
    }
};
```

## 扩展指南

### 1. 添加新命令

```cpp
// 步骤1：在头文件中声明新命令
class FComfyUIIntegrationCommands : public TCommands<FComfyUIIntegrationCommands>
{
public:
    // 现有命令...
    
    // 新命令
    TSharedPtr<FUICommandInfo> ExportWorkflow;
    TSharedPtr<FUICommandInfo> ImportWorkflow;
    TSharedPtr<FUICommandInfo> OpenWorkflowEditor;
};

// 步骤2：在RegisterCommands()中注册新命令
void FComfyUIIntegrationCommands::RegisterCommands()
{
    // 现有命令注册...
    
    // 注册新命令
    UI_COMMAND(ExportWorkflow, 
        "Export Workflow", 
        "Export current workflow to file", 
        EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control, EKeys::E));
    
    UI_COMMAND(ImportWorkflow, 
        "Import Workflow", 
        "Import workflow from file", 
        EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control, EKeys::I));
    
    UI_COMMAND(OpenWorkflowEditor, 
        "Workflow Editor", 
        "Open workflow editor", 
        EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::W));
}

// 步骤3：在模块中绑定新命令
void FComfyUIIntegrationModule::BindCommands()
{
    // 现有命令绑定...
    
    // 绑定新命令
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().ExportWorkflow,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ExportWorkflow),
        FCanExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::CanExportWorkflow)
    );
    
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().ImportWorkflow,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::ImportWorkflow),
        FCanExecuteAction()
    );
    
    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().OpenWorkflowEditor,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::OpenWorkflowEditor),
        FCanExecuteAction()
    );
}
```

### 2. 创建自定义命令类型

```cpp
// 创建自定义命令类型
class FComfyUIWorkflowCommands : public TCommands<FComfyUIWorkflowCommands>
{
public:
    FComfyUIWorkflowCommands()
        : TCommands<FComfyUIWorkflowCommands>(TEXT("ComfyUIWorkflow"), 
            NSLOCTEXT("Contexts", "ComfyUIWorkflow", "ComfyUI Workflow Commands"), 
            NAME_None, FComfyUIIntegrationStyle::GetStyleSetName())
    {
    }

    virtual void RegisterCommands() override
    {
        UI_COMMAND(CreateTextToImageWorkflow, "Text to Image", "Create a text to image workflow", EUserInterfaceActionType::Button, FInputChord());
        UI_COMMAND(CreateImageToImageWorkflow, "Image to Image", "Create an image to image workflow", EUserInterfaceActionType::Button, FInputChord());
        UI_COMMAND(CreateTextTo3DWorkflow, "Text to 3D", "Create a text to 3D workflow", EUserInterfaceActionType::Button, FInputChord());
    }

public:
    TSharedPtr<FUICommandInfo> CreateTextToImageWorkflow;
    TSharedPtr<FUICommandInfo> CreateImageToImageWorkflow;
    TSharedPtr<FUICommandInfo> CreateTextTo3DWorkflow;
};
```

### 3. 命令组和上下文

```cpp
// 创建命令组
class FComfyUICommandGroup
{
public:
    static void RegisterCommandGroups()
    {
        // 注册主命令组
        FComfyUIIntegrationCommands::Register();
        
        // 注册工作流命令组
        FComfyUIWorkflowCommands::Register();
        
        // 注册调试命令组
        FComfyUIDebugCommands::Register();
    }
    
    static void CreateContextualMenus()
    {
        // 创建右键菜单
        CreateAssetContextMenu();
        CreateViewportContextMenu();
        CreateContentBrowserContextMenu();
    }
    
    static void CreateAssetContextMenu()
    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.Texture2D");
        FToolMenuSection& Section = Menu->FindOrAddSection("ComfyUI");
        
        Section.AddMenuEntry(
            "GenerateVariations",
            LOCTEXT("GenerateVariationsLabel", "Generate Variations"),
            LOCTEXT("GenerateVariationsTooltip", "Generate variations of this texture using ComfyUI"),
            FSlateIcon(FComfyUIIntegrationStyle::GetStyleSetName(), "ComfyUI.Icon.Small"),
            FUIAction(FExecuteAction::CreateStatic(&FComfyUIIntegrationModule::GenerateTextureVariations))
        );
    }
};
```

## 调试和测试

### 1. 命令调试

```cpp
// 启用命令调试
class FComfyUICommandDebugger
{
public:
    static void EnableCommandLogging(bool bEnable)
    {
        if (bEnable)
        {
            // 监听命令执行
            FCoreDelegates::OnBeginFrame.AddStatic(&FComfyUICommandDebugger::LogCommandExecution);
        }
        else
        {
            FCoreDelegates::OnBeginFrame.RemoveAll(nullptr);
        }
    }
    
    static void LogCommandExecution()
    {
        // 记录当前活动的命令
        TSharedPtr<FUICommandList> ActiveCommandList = FSlateApplication::Get().GetActiveModalWindow().IsValid() 
            ? FSlateApplication::Get().GetActiveModalWindow()->GetCommandList() 
            : nullptr;
        
        if (ActiveCommandList.IsValid())
        {
            // 记录命令列表状态
            UE_LOG(LogComfyUIIntegration, VeryVerbose, TEXT("Active command list: %s"), 
                ActiveCommandList.IsValid() ? TEXT("Valid") : TEXT("Invalid"));
        }
    }
};
```

### 2. 性能监控

```cpp
// 监控命令执行性能
class FComfyUICommandProfiler
{
public:
    static void ProfileCommand(const FString& CommandName, TFunction<void()> CommandFunc)
    {
        double StartTime = FPlatformTime::Seconds();
        
        CommandFunc();
        
        double EndTime = FPlatformTime::Seconds();
        double ExecutionTime = EndTime - StartTime;
        
        UE_LOG(LogComfyUIIntegration, Log, TEXT("Command '%s' executed in %.3f seconds"), 
            *CommandName, ExecutionTime);
        
        // 记录性能数据
        RecordPerformanceData(CommandName, ExecutionTime);
    }
    
    static void RecordPerformanceData(const FString& CommandName, double ExecutionTime)
    {
        static TMap<FString, TArray<double>> PerformanceData;
        
        if (!PerformanceData.Contains(CommandName))
        {
            PerformanceData.Add(CommandName, TArray<double>());
        }
        
        PerformanceData[CommandName].Add(ExecutionTime);
        
        // 如果数据足够，计算平均值
        if (PerformanceData[CommandName].Num() >= 10)
        {
            double AverageTime = 0.0;
            for (double Time : PerformanceData[CommandName])
            {
                AverageTime += Time;
            }
            AverageTime /= PerformanceData[CommandName].Num();
            
            UE_LOG(LogComfyUIIntegration, Log, TEXT("Command '%s' average execution time: %.3f seconds"), 
                *CommandName, AverageTime);
        }
    }
};
```

## 最佳实践

1. **命令命名**：使用清晰、描述性的命令名称
2. **快捷键设计**：避免与UE5内置快捷键冲突
3. **本地化支持**：所有用户可见文本都应支持本地化
4. **状态管理**：正确处理命令的启用/禁用状态
5. **错误处理**：为命令执行提供适当的错误处理
6. **用户反馈**：提供命令执行状态的及时反馈
7. **性能考虑**：避免在命令执行中进行耗时操作
8. **可扩展性**：设计时考虑未来的扩展需求
