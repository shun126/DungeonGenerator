/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSetDatabase.h"
#include "DungeonRoomMeshSet.h"
#include "DungeonRoomMeshSetDatabase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonRoomMeshSetDatabase : public UDungeonMeshSetDatabase
{
	GENERATED_BODY()

public:
	explicit UDungeonRoomMeshSetDatabase(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonRoomMeshSetDatabase() = default;

	const FDungeonRoomMeshSet* Select(const std::shared_ptr<dungeon::Random>& random) const;
	void Each(std::function<void(const FDungeonRoomMeshSet&)> func) const;

	// UDungeonMeshSetDatabase overrides
	virtual const FDungeonRoomMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<FDungeonRoomMeshSet> Parts;
};

inline UDungeonRoomMeshSetDatabase::UDungeonRoomMeshSetDatabase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

inline void UDungeonRoomMeshSetDatabase::Each(std::function<void(const FDungeonRoomMeshSet&)> func) const
{
	for (const FDungeonRoomMeshSet& parts : Parts)
	{
		func(parts);
	}
}
