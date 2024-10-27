/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshSet.h"
#if WITH_EDITOR
#include "Parameter/DungeonTemporaryMeshSetDatabase.h"
#include "Parameter/DungeonRoomMeshSet.h"
#endif
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSetDatabase.generated.h"

class UDungeonAisleMeshSetDatabase;
namespace dungeon
{
	class Random;
}

/**
Database of dungeon mesh sets
ダンジョンのメッシュセットのデータベース
*/
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonMeshSetDatabase : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonMeshSetDatabase() override = default;

	/**
	FDungeonMeshSetを取得します
	*/
	virtual const FDungeonMeshSet* AtImplement(const size_t index) const;

	/**
	FDungeonMeshSetをランダムに抽選します
	*/
	virtual const FDungeonMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const;

	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonMeshSet& parts : Parts)
		{
			std::forward<Function>(function)(parts);
		}
	}

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
	TArray<FDungeonMeshSet> Parts;

#if WITH_EDITORONLY_DATA
	/*
	 * Specify DungeonRoomMeshSetDatabase, DungeonAisleMeshSetDatabase to be migrated
	 * 移行するDungeonRoomMeshSetDatabase, DungeonAisleMeshSetDatabaseを指定して下さい
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Migration")
	TObjectPtr<UDungeonTemporaryMeshSetDatabase> MigrationMeshSetDatabase;
#endif
#if WITH_EDITOR
	/*
	 * Migrate DungeonRoomMeshSetDatabase, DungeonAisleMeshSetDatabase
	 * DungeonRoomMeshSetDatabase, DungeonAisleMeshSetDatabaseを移行します
	 */
	UFUNCTION(Category = "DungeonGenerator|Migration", meta = (CallInEditor = "true"))
	void Migrate();

	static FDungeonMeshSet MigrateRoomMeshSet(const FDungeonRoomMeshSet& dungeonRoomMeshSet);
#endif
};
