using UnrealBuildTool;

public class FinalTower : ModuleRules
{
    public FinalTower(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Paper2D"
        });
    }
}
