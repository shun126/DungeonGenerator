/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

using UnrealBuildTool;

public class DungeonGeneratorEditor : ModuleRules
{
	public DungeonGeneratorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {});
		PrivateIncludePaths.AddRange(new string[] {});

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"PropertyEditor",
				"DungeonGenerator",
			}
		);
		
		//DynamicallyLoadedModuleNames.AddRange(new string[] {});
	}
}
