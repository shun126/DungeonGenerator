/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSetDatabase.h"
#include "DungeonAisleMeshSet.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonAisleMeshSetDatabase.generated.h"

/**
Database of dungeon aisle mesh sets
ダンジョン通路のメッシュセットのデータベース
*/
UCLASS()
class DUNGEONGENERATOR_API UDungeonAisleMeshSetDatabase : public UDungeonMeshSetDatabase
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

	// UDungeonMeshSetDatabase overrides
	virtual const FDungeonMeshSet* AtImplement(const size_t index) const override;
	virtual const FDungeonMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const override;

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
};

inline UDungeonAisleMeshSetDatabase::UDungeonAisleMeshSetDatabase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}
