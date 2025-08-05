#include "ComfyUIIntegration.h"
#include "ComfyUIIntegrationStyle.h"
#include "ComfyUIIntegrationCommands.h"
#include "Workflow/ComfyUIWorkflowService.h"
#include "Client/ComfyUIClient.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "UI/ComfyUIWidget.h"

static const FName ComfyUIIntegrationTabName("ComfyUIIntegration");

#define LOCTEXT_NAMESPACE "FComfyUIIntegrationModule"

DEFINE_LOG_CATEGORY(LogComfyUIIntegration);

void FComfyUIIntegrationModule::StartupModule()
{
    UE_LOG(LogComfyUIIntegration, Warning, TEXT("ComfyUI Integration module has been loaded"));
    
    // 初始化样式
    FComfyUIIntegrationStyle::Initialize();
    FComfyUIIntegrationStyle::ReloadTextures();

    // 初始化命令
    FComfyUIIntegrationCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FComfyUIIntegrationCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FComfyUIIntegrationModule::PluginButtonClicked),
        FCanExecuteAction());

    // 注册菜单
    RegisterMenus();
    
    // 将命令绑定到Level Editor的CommandList，使快捷键在全局生效
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());
    
    // 注册Tab生成器
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ComfyUIIntegrationTabName,
        FOnSpawnTab::CreateRaw(this, &FComfyUIIntegrationModule::OnSpawnComfyUITab))
        .SetDisplayName(LOCTEXT("FComfyUIIntegrationTabTitle", "ComfyUI 集成"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
        
    // 初始化 ComfyUI 客户端单例
    UComfyUIClient::GetInstance();
}

void FComfyUIIntegrationModule::ShutdownModule()
{
    UE_LOG(LogComfyUIIntegration, Warning, TEXT("ComfyUI Integration module has been unloaded"));
    
    // 销毁 ComfyUI 客户端单例
    UComfyUIClient::DestroyInstance();
    
    // 注销Tab生成器
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ComfyUIIntegrationTabName);
    
    // 清理
    UnregisterMenus();
    UComfyUIWorkflowService::ShutdownGlobal();
    FComfyUIIntegrationStyle::Shutdown();
    FComfyUIIntegrationCommands::Unregister();
}

void FComfyUIIntegrationModule::RegisterMenus()
{
    // 仅在编辑器中注册菜单
    if (!IsRunningCommandlet())
    {
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FComfyUIIntegrationModule::RegisterMenusInternal));
    }
}

void FComfyUIIntegrationModule::RegisterMenusInternal()
{
    // 在Level Editor菜单栏中添加ComfyUI菜单
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    
    Section.AddMenuEntryWithCommandList(FComfyUIIntegrationCommands::Get().OpenPluginWindow, PluginCommands);
}

void FComfyUIIntegrationModule::UnregisterMenus()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

void FComfyUIIntegrationModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ComfyUIIntegrationTabName);
}

TSharedRef<SDockTab> FComfyUIIntegrationModule::OnSpawnComfyUITab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            // 创建ComfyUI主界面Widget
            SNew(SComfyUIWidget)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FComfyUIIntegrationModule, ComfyUIIntegration)
