/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

/*
Dungeon generator module class
*/
class FDungeonGeneratorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
