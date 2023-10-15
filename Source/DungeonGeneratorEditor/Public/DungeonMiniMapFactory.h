/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonMiniMapFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonMiniMapFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonMiniMapFactory();
	virtual ~UDungeonMiniMapFactory() = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
