/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSetDatabase.h"
#include "Parameter/DungeonRoomMeshSet.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonRoomMeshSetDatabase.generated.h"

/**
Database of dungeon room mesh sets
ダンジョン部屋のメッシュセットのデータベース
*/
UCLASS()
class DUNGEONGENERATOR_API UDungeonRoomMeshSetDatabase : public UDungeonMeshSetDatabase
{
	GENERATED_BODY()

public:
	explicit UDungeonRoomMeshSetDatabase(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonRoomMeshSetDatabase() override = default;

	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonRoomMeshSet& parts : Parts)
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
	Set the DungeonRoomMeshSet; multiple DungeonRoomMeshSets can be set.
	DungeonRoomMeshSetを設定して下さい。DungeonRoomMeshSetは複数設定する事ができます。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TArray<FDungeonRoomMeshSet> Parts;
};

inline UDungeonRoomMeshSetDatabase::UDungeonRoomMeshSetDatabase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}
