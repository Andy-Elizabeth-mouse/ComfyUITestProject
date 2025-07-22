using UnrealBuildTool;
using System.Collections.Generic;

public class ComfyUITestProjectEditorTarget : TargetRules
{
    public ComfyUITestProjectEditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange( new string[] { "ComfyUITestProject" } );
    }
}