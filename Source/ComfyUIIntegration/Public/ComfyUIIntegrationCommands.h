#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ComfyUIIntegrationStyle.h"

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
    TSharedPtr<FUICommandInfo> OpenPluginWindow;
};
