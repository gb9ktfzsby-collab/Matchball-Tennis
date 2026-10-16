// Matchball Tennis - Editor-Ziel (inklusive Automation-Tests)

using UnrealBuildTool;

public class MatchballTennisEditorTarget : TargetRules
{
	public MatchballTennisEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "MatchballTennis", "MatchballTennisTests" });
	}
}
