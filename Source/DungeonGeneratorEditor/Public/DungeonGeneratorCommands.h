/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DungeonGeneratorStyle.h"

class FDungeonGeneratorCommands : public TCommands<FDungeonGeneratorCommands>
{
public:

	FDungeonGeneratorCommands()
		: TCommands<FDungeonGeneratorCommands>(TEXT("DungeonGenerator"), NSLOCTEXT("Contexts", "DungeonGenerator", "DungeonGenerator Plugin"), NAME_None, FDungeonGeneratorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};