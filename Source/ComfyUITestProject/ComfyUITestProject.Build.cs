using UnrealBuildTool;

public class ComfyUITestProject : ModuleRules
{
    public ComfyUITestProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore" 
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}