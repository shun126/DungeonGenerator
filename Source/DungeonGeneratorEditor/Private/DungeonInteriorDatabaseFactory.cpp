/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorDatabaseFactory.h"
#include "../../DungeonGenerator/Public/Decorator/DungeonInteriorDatabase.h"

UDungeonInteriorDatabaseFactory::UDungeonInteriorDatabaseFactory()
{
	SupportedClass = UDungeonInteriorDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonInteriorDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonInteriorDatabase>(InParent, InClass, InName, Flags);
}
