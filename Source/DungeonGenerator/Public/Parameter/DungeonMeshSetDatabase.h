/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonMeshSetSelectionMethod.h"
#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonSelectionPolicy.h"
#include "Parameter/DungeonSelectionQuery.h"
#include <memory>
#include "DungeonMeshSetDatabase.generated.h"

class UDungeonAisleMeshSetDatabase;
class UDungeonPartsSelector;
struct FPropertyChangedEvent;
namespace dungeon
{
	class Random;
}

/**
 * Database of dungeon mesh sets
 * ダンジョンのメッシュセットのデータベース
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonMeshSetDatabase : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonMeshSetDatabase() override = default;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/**
	 * FDungeonMeshSetを取得します
	 */
	virtual const FDungeonMeshSet* AtImplement(const size_t index) const;

	/**
	 * FDungeonMeshSetをランダムに抽選します
	 */
	virtual const FDungeonMeshSet* SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random, const FMeshSetQuery& query) const;

	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonMeshSet& parts : Parts)
		{
			std::forward<Function>(function)(parts);
		}
	}

	void MigrateSelectionPolicies();

#if WITH_EDITOR
public:
	// Debug
	FString DumpToJson(const uint32 indent) const;

#endif

protected:
	/**
	 * Part Selection Method
	 * パーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (
		DisplayName = "Mesh Set Selection Policy",
		ToolTip = "Phase 1 selector: chooses which FDungeonMeshSet candidate to use from this database. Individual Floor/Wall/Roof/Slope/Catwalk parts are selected afterward by each selection policy in the selected mesh set."))
	EDungeonSelectionPolicy SelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy serialized selection method. Kept for backward compatibility and migrated to SelectionPolicy on load.
	 */
	UPROPERTY()
	EDungeonMeshSetSelectionMethod SelectionMethod = EDungeonMeshSetSelectionMethod::Random;

	UPROPERTY()
	bool bSelectionPolicyMigrated = false;


	/**
	 * Set the DungeonRoomMeshSet; multiple DungeonRoomMeshSets can be set.
	 * DungeonRoomMeshSetを設定して下さい。DungeonRoomMeshSetは複数設定する事ができます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (DisplayName = "Mesh Set"))
	TArray<FDungeonMeshSet> Parts;
};
