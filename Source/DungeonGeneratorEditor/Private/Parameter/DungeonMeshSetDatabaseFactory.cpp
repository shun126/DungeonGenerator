/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonMeshSetDatabaseFactory.h"
#include "Parameter/DungeonMeshSetDatabase.h"

UDungeonMeshSetDatabaseFactory::UDungeonMeshSetDatabaseFactory()
{
	SupportedClass = UDungeonMeshSetDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonMeshSetDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonMeshSetDatabase>(InParent, InClass, InName, Flags);
}
