/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSetDatabase.h"
#include "DungeonAisleMeshSet.h"
#include "DungeonAisleMeshSetDatabase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonAisleMeshSetDatabase : public UDungeonMeshSetDatabase
{
	GENERATED_BODY()

public:
	explicit UDungeonAisleMeshSetDatabase(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonAisleMeshSetDatabase() = default;

	const FDungeonAisleMeshSet* Select(const std::shared_ptr<dungeon::Random>& random) const;
	void Each(std::function<void(const FDungeonAisleMeshSet&)> func) const;

	// UDungeonMeshSetDatabase overrides
	virtual const FDungeonRoomMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<FDungeonAisleMeshSet> Parts;
};

inline UDungeonAisleMeshSetDatabase::UDungeonAisleMeshSetDatabase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

inline void UDungeonAisleMeshSetDatabase::Each(std::function<void(const FDungeonAisleMeshSet&)> func) const
{
	for (const FDungeonAisleMeshSet& parts : Parts)
	{
		func(parts);
	}
}
