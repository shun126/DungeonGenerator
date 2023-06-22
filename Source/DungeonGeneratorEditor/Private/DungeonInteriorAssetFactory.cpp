/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorAssetFactory.h"
#include "../../DungeonGenerator/Public/DungeonInteriorAsset.h"

UDungeonInteriorAssetFactory::UDungeonInteriorAssetFactory()
{
	SupportedClass = UDungeonInteriorAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonInteriorAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonInteriorAsset>(InParent, InClass, InName, Flags);
}
