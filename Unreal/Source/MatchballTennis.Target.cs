// Matchball Tennis - Spiel-Ziel (gepackte "Matchball Tennis.exe", README 2.1)

using UnrealBuildTool;

public class MatchballTennisTarget : TargetRules
{
	public MatchballTennisTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MatchballTennis");
	}
}
