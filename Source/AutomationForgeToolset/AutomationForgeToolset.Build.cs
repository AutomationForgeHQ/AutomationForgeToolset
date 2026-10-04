using UnrealBuildTool;

public class AutomationForgeToolset : ModuleRules
{
	public AutomationForgeToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ToolsetRegistry",
				"AutomationForge",
				"AutomationForgePipelines",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",         // IPluginManager, so GetToolsetVersion() reads the descriptor
			}
			);

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
