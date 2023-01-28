/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonRoomAssetFactory.h"
#include "../../DungeonGenerator/Public/DungeonRoomAsset.h"

UDungeonRoomAssetFactory::UDungeonRoomAssetFactory()
{
	SupportedClass = UDungeonRoomAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonRoomAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonRoomAsset>(InParent, InClass, InName, Flags);
}
