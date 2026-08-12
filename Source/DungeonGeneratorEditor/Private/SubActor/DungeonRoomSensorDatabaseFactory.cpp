/**
 * @author      Shun Moriya
 * @copyright   2025- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonRoomSensorDatabaseFactory.h"
#include "SubActor/DungeonRoomSensorDatabase.h"

UDungeonRoomSensorDatabaseFactory::UDungeonRoomSensorDatabaseFactory()
{
	SupportedClass = UDungeonRoomSensorDatabase::StaticClass();
	bCreateNew = false;
	bEditAfterNew = false;
}

UObject* UDungeonRoomSensorDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonRoomSensorDatabase>(InParent, InClass, InName, Flags);
}
