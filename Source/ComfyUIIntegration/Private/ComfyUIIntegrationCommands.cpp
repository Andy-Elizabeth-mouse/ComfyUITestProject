#include "ComfyUIIntegrationCommands.h"

#define LOCTEXT_NAMESPACE "FComfyUIIntegrationModule"

void FComfyUIIntegrationCommands::RegisterCommands()
{
    // 使用Ctrl+Shift+Q作为默认快捷键，这个组合在UE、3ds Max、Maya等软件中不常用
    UI_COMMAND(OpenPluginWindow, "ComfyUI 集成", "打开ComfyUI集成窗口", EUserInterfaceActionType::Button, 
        FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Q));
}

#undef LOCTEXT_NAMESPACE
