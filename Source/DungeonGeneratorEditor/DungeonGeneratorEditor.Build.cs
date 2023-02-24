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
        BuildVersion Version;
        if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
        {
            if (Version.MajorVersion == 5)
            {
                PrivateDependencyModuleNames.AddRange(new string[] { "EditorFramework" });
            }
        }
    }
}
