/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

/**
Dungeon generator module
ダンジョン生成モジュール
*/
class FDungeonGeneratorModule : public IModuleInterface
{
public:
	FDungeonGeneratorModule() = default;
	virtual ~FDungeonGeneratorModule() = default;

	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
