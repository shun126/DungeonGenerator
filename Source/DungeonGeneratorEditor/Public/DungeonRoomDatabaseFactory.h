/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonRoomDatabaseFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonRoomDatabaseFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonRoomDatabaseFactory();
	virtual ~UDungeonRoomDatabaseFactory() = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(Deprecated, meta = (DeprecationMessage = "UDungeonRoomAssetFactory is deprecated, use UDungeonRoomDatabaseFactory."))
class DUNGEONGENERATOREDITOR_API UDEPRECATED_DungeonRoomAssetFactory : public UDungeonRoomDatabaseFactory
{
	GENERATED_BODY()
};
