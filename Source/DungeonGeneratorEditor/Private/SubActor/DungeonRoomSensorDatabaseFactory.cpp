/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonRoomSensorDatabaseFactory.h"
#include "SubActor/DungeonRoomSensorDatabase.h"

UDungeonRoomSensorDatabaseFactory::UDungeonRoomSensorDatabaseFactory()
{
	SupportedClass = UDungeonRoomSensorDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonRoomSensorDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonRoomSensorDatabase>(InParent, InClass, InName, Flags);
}
