using UnrealBuildTool;

public class ComfyUIIntegration : ModuleRules
{
    public ComfyUIIntegration(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                
                "EditorWidgets",
                "ToolMenus",
                "Slate",
                "SlateCore",
                "HTTP",
                "WebSockets",
                "Json",
                "JsonUtilities",
                "ImageWrapper",
                "AssetRegistry",
                
                // 3D和材质相关
                "MeshDescription",
                "StaticMeshDescription",
                "StaticMeshEditor",
                "MaterialEditor",
                
                // 资产导入相关
                "AssetTools"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "EditorSubsystem",
                "ToolWidgets",
                "PropertyEditor",
                "LevelEditor",
                "ContentBrowser",
                "AssetTools",
                "DeveloperSettings",
                "Projects",
                "EditorStyle",
                "ApplicationCore",
                "InputCore",
                "RenderCore",
                "RHI",
                "DesktopPlatform",
                
                // 3D模型处理相关
                "MeshUtilities",
                "StaticMeshEditor",
                "MeshBuilder",
                
                // 资产导入相关
                "UnrealEd"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );
    }
}
