#include "ComfyUIIntegrationStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FComfyUIIntegrationStyle::StyleInstance = nullptr;

void FComfyUIIntegrationStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FComfyUIIntegrationStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FComfyUIIntegrationStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("ComfyUIIntegrationStyle"));
    return StyleSetName;
}

const ISlateStyle& FComfyUIIntegrationStyle::Get()
{
    return *StyleInstance;
}

void FComfyUIIntegrationStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

TSharedRef<FSlateStyleSet> FComfyUIIntegrationStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("ComfyUIIntegrationStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("ComfyUIIntegration")->GetBaseDir() / TEXT("Resources"));

    const FVector2D Icon20x20(20.0f, 20.0f);
    Style->Set("ComfyUIIntegration.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
    
    return Style;
}
