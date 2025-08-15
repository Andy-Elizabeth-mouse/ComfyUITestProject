#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UICommandList.h"

DECLARE_LOG_CATEGORY_EXTERN(LogComfyUIIntegration, Log, All);

class FComfyUIIntegrationModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void RegisterMenusInternal();
    void UnregisterMenus();
    void PluginButtonClicked();
    
    /** 创建ComfyUI工具窗口 */
    TSharedRef<class SDockTab> OnSpawnComfyUITab(const class FSpawnTabArgs& SpawnTabArgs);
    
    /** 菜单扩展委托句柄 */
    TSharedPtr<class FUICommandList> PluginCommands;
};
