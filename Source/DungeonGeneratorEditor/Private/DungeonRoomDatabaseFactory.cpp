/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomDatabaseFactory.h"
#include "../../DungeonGenerator/Public/SubLevel/DungeonRoomDatabase.h"

UDungeonRoomDatabaseFactory::UDungeonRoomDatabaseFactory()
{
	SupportedClass = UDungeonRoomDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonRoomDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonRoomDatabase>(InParent, InClass, InName, Flags);
}
