/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonRoomSensorDatabaseFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonRoomSensorDatabaseFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonRoomSensorDatabaseFactory();
	virtual ~UDungeonRoomSensorDatabaseFactory() override = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
