/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonMiniMapFactory.h"
#include "../../DungeonGenerator/Public/MiniMap/DungeonMiniMap.h"

UDungeonMiniMapFactory::UDungeonMiniMapFactory()
{
	SupportedClass = UDungeonMiniMap::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonMiniMapFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonMiniMap>(InParent, InClass, InName, Flags);
}
