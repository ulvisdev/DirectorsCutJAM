using UnrealBuildTool;
using System.Collections.Generic;
public class DirectorsCutJAMTarget : TargetRules
{
    public DirectorsCutJAMTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        ExtraModuleNames.Add("DirectorsCutJAM");
    }
}

