using UnrealBuildTool;
using System.Collections.Generic;

public class FinalTowerEditorTarget : TargetRules
{
    public FinalTowerEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("FinalTower");
        ExtraModuleNames.Add("FinalTowerEditor");
    }
}
