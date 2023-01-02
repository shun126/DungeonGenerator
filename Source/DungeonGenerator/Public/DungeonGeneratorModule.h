/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>

class FDungeonGeneratorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
