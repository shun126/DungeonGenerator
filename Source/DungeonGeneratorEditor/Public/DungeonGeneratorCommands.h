/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DungeonGeneratorStyle.h"

class FDungeonGeneratorCommands : public TCommands<FDungeonGeneratorCommands>
{
public:
	FDungeonGeneratorCommands();
	virtual ~FDungeonGeneratorCommands() = default;

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenPluginWindow;
};