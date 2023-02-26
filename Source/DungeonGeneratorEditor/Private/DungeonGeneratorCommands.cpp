/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorCommands.h"

#define LOCTEXT_NAMESPACE "FDungeonGenerateEditorModule"

void FDungeonGeneratorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "DungeonGenerator", "Bring up DungeonGenerator window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
