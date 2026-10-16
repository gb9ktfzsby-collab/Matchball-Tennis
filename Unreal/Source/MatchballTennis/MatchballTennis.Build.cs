// Matchball Tennis - Hauptmodul (README 2.3)
//
// Aufbau: Unterordner je Bereich (Core, Match, Physics, Player, ...). Dateien in */Kern/ sind reines C++
// ohne Unreal-Abhaengigkeit und werden zusaetzlich ausserhalb von Unreal getestet (Tools/KernTests).

using UnrealBuildTool;

public class MatchballTennis : ModuleRules
{
	public MatchballTennis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Includes relativ zum Modulordner, z. B. "Physics/Kern/MBBallPhysik.h"
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});
	}
}
