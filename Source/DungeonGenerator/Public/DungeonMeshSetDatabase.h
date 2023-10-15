/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSet.h"
#include "DungeonMeshSetDatabase.generated.h"

struct FDungeonRoomMeshSet;

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonMeshSetDatabase : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonMeshSetDatabase() = default;

	virtual const FDungeonRoomMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const;
};

inline UDungeonMeshSetDatabase::UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline const FDungeonRoomMeshSet* UDungeonMeshSetDatabase::SelectImplement(const std::shared_ptr<dungeon::Random>& random) const
{
	return nullptr;
}
