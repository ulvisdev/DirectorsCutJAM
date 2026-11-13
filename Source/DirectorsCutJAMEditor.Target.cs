using UnrealBuildTool;
using System.Collections.Generic;
public class DirectorsCutJAMEditorTarget : TargetRules
{
    public DirectorsCutJAMEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        ExtraModuleNames.Add("DirectorsCutJAM");
    }
}

