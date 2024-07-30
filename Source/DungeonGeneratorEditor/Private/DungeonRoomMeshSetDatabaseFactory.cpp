/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomMeshSetDatabaseFactory.h"
#include "DungeonRoomMeshSetDatabase.h"

UDungeonRoomMeshSetDatabaseFactory::UDungeonRoomMeshSetDatabaseFactory()
{
	SupportedClass = UDungeonRoomMeshSetDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonRoomMeshSetDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonRoomMeshSetDatabase>(InParent, InClass, InName, Flags);
}
