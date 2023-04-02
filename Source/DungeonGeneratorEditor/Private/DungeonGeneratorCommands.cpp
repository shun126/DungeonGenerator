/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorCommands.h"

#define LOCTEXT_NAMESPACE "FDungeonGenerateEditorModule"

FDungeonGeneratorCommands::FDungeonGeneratorCommands()
	: TCommands<FDungeonGeneratorCommands>(TEXT("DungeonGenerator"), NSLOCTEXT("Contexts", "DungeonGenerator", "DungeonGenerator Plugin"), NAME_None, FDungeonGeneratorStyle::GetStyleSetName())
{
}

void FDungeonGeneratorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "DungeonGenerator", "Bring up DungeonGenerator window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
