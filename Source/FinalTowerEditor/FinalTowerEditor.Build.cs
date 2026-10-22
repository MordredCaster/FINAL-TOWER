using UnrealBuildTool;

public class FinalTowerEditor : ModuleRules
{
    public FinalTowerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "AssetTools", "AssetRegistry",
            "Paper2D", "Json"
        });
    }
}
