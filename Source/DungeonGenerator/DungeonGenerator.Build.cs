/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

using UnrealBuildTool;

public class DungeonGenerator : ModuleRules
{
	public DungeonGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
			});

		PrivateIncludePaths.AddRange(
			new string[]
			{
			});

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "AIModule"
            });

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"NavigationSystem",
                "NetCore",
				"SlateCore",
				"UMG",
                "Foliage"
            });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnrealEd",
	                "JsonUtilities",
                });
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			});

		//CppStandard = CppStandardVersion.Cpp17;
		CppStandard = CppStandardVersion.Latest;
	}
}
