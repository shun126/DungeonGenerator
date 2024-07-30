/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonAisleMeshSetDatabaseFactory.h"
#include "DungeonAisleMeshSetDatabase.h"

UDungeonAisleMeshSetDatabaseFactory::UDungeonAisleMeshSetDatabaseFactory()
{
	SupportedClass = UDungeonAisleMeshSetDatabase::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonAisleMeshSetDatabaseFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonAisleMeshSetDatabase>(InParent, InClass, InName, Flags);
}
