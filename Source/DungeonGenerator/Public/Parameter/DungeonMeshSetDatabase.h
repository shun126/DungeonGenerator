/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshSet.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSetDatabase.generated.h"

class UDungeonAisleMeshSetDatabase;
namespace dungeon
{
	class Random;
}

/**
Part Selection Method
パーツを選択する方法
*/
UENUM(BlueprintType)
enum class EDungeonMeshSetSelectionMethod : uint8
{
	Identifier,
	DepthFromStart,
	Random,
};

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
	virtual const FDungeonMeshSet* SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const;

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
	Part Selection Method
	パーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonMeshSetSelectionMethod SelectionMethod = EDungeonMeshSetSelectionMethod::Identifier;

	/**
	Set the DungeonRoomMeshSet; multiple DungeonRoomMeshSets can be set.
	DungeonRoomMeshSetを設定して下さい。DungeonRoomMeshSetは複数設定する事ができます。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TArray<FDungeonMeshSet> Parts;
};
