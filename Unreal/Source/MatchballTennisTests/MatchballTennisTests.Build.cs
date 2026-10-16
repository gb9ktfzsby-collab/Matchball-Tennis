// Matchball Tennis - Automation-Tests (README 0.2)
// Die Testquellen in Kern/ laufen identisch auch ohne Unreal (Tools/KernTests).

using UnrealBuildTool;

public class MatchballTennisTests : ModuleRules
{
	public MatchballTennisTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MatchballTennis"
		});
	}
}
