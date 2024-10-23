/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonTemporaryMeshSetDatabase.h"
#include "DungeonAisleMeshSet.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonAisleMeshSetDatabase.generated.h"

/**
Database of dungeon aisle mesh sets
(DeprecatedProperty, Use DungeonRoomMeshSetDatabase instead.)

ダンジョン通路のメッシュセットのデータベース
(DungeonRoomMeshSetDatabaseを使用してください)
*/
UCLASS()
class DUNGEONGENERATOR_API UDungeonAisleMeshSetDatabase : public UDungeonTemporaryMeshSetDatabase
{
	GENERATED_BODY()

public:
	explicit UDungeonAisleMeshSetDatabase(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonAisleMeshSetDatabase() override = default;

	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonAisleMeshSet& parts : Parts)
		{
			std::forward<Function>(function)(parts);
		}
	}

	// UDungeonTemporaryMeshSetDatabase overrides
	virtual const FDungeonTemporaryMeshSet* AtImplement(const size_t index) const override;
	virtual const FDungeonTemporaryMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const override;

#if WITH_EDITOR
public:
	// Debug
	FString DumpToJson(const uint32 indent) const;
#endif

protected:
	/**
	Set the DungeonAisleMeshSet; multiple DungeonAisleMeshSet can be set.
	DungeonAisleMeshSetを設定して下さい。DungeonAisleMeshSetは複数設定する事ができます。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TArray<FDungeonAisleMeshSet> Parts;

	friend class UDungeonMeshSetDatabase;
};

inline UDungeonAisleMeshSetDatabase::UDungeonAisleMeshSetDatabase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}
