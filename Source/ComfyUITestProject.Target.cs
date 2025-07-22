using UnrealBuildTool;
using System.Collections.Generic;

public class ComfyUITestProjectTarget : TargetRules
{
    public ComfyUITestProjectTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange( new string[] { "ComfyUITestProject" } );
    }
}