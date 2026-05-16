/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonMeshPartsWithDirection.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonSelectionPolicy.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSet.generated.h"

// forward declaration
class UClass;
class UStaticMesh;
class UDungeonPartsSelector;

namespace dungeon
{
	class Direction;
	class Grid;
	class Random;
}

/**
 * Dungeon mesh set
 * ダンジョンのメッシュセット
 */
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshSet
{
	GENERATED_BODY()

public:
	virtual ~FDungeonMeshSet() = default;

	/**
	 * 床パーツを選択します
	 */
	const FDungeonMeshPartsWithDirection* SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	EDungeonPartsSelectionMethod GetFloorPartsSelectionMethod() const noexcept
	{
		return FloorPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetFloorPartsSelectionPolicy() const noexcept
	{
		return FloorPartsSelectionPolicy;
	}

	int32 GetFloorPartsCount() const noexcept
	{
		return FloorParts.Num();
	}

	const FDungeonMeshPartsWithDirection* GetFloorPartsAt(const int32 index) const
	{
		return AtParts(FloorParts, index);
	}

	/**
	 * 床パーツを更新します
	 */
	template<typename Function>
	void EachFloorParts(Function&& function) const
	{
		EachParts(FloorParts, std::forward<Function>(function));
	}

	/**
	 * グリッドに基づいて壁パーツを選択します
	 */
	const FDungeonMeshParts* SelectWallPartsByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	int32 GetWallPartsCount() const noexcept
	{
		return WallParts.Num();
	}

	const FDungeonMeshParts* GetWallPartsAt(const int32 index) const
	{
		return AtParts(WallParts, index);
	}

	/**
	 * 面に基づいて壁パーツを選択します
	 */
	const FDungeonMeshParts* SelectWallPartsByFace(const FIntVector& gridLocation, const dungeon::Direction& direction) const
	{
		return SelectPartsByFace(gridLocation, direction, WallParts);
	}

	/**
	 * 壁パーツを更新します
	 */
	template<typename Function>
	void EachWallParts(Function&& function) const
	{
		EachParts(WallParts, std::forward<Function>(function));
	}

	/**
	 * 天井パーツを選択します
	 */
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	EDungeonPartsSelectionMethod GetRoofPartsSelectionMethod() const noexcept
	{
		return RoofPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetRoofPartsSelectionPolicy() const noexcept
	{
		return RoofPartsSelectionPolicy;
	}

	int32 GetRoofPartsCount() const noexcept
	{
		return RoofParts.Num();
	}

	const FDungeonMeshPartsWithDirection* GetRoofPartsAt(const int32 index) const
	{
		return AtParts(RoofParts, index);
	}

	/**
	 * 天井パーツを更新します
	 */
	template<typename Function>
	void EachRoofParts(Function&& function) const
	{
		EachParts(RoofParts, std::forward<Function>(function));
	}

	/**
	 * スロープパーツを選択します
	 */
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	EDungeonPartsSelectionMethod GetSlopePartsSelectionMethod() const noexcept
	{
		return SloopPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetSlopePartsSelectionPolicy() const noexcept
	{
		return SlopePartsSelectionPolicy;
	}

	int32 GetSlopePartsCount() const noexcept
	{
		return SlopeParts.Num();
	}

	const FDungeonMeshParts* GetSlopePartsAt(const int32 index) const
	{
		return AtParts(SlopeParts, index);
	}

	/**
	 * スロープパーツを更新します
	 */
	template<typename Function>
	void EachSlopeParts(Function&& function) const
	{
		EachParts(SlopeParts, std::forward<Function>(function));
	}

	/**
	 * 中二階通路パーツを選択します
	 */
	const FDungeonMeshParts* SelectCatwalkParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	EDungeonPartsSelectionMethod GetCatwalkPartsSelectionMethod() const noexcept
	{
		return CatwalkPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetCatwalkPartsSelectionPolicy() const noexcept
	{
		return CatwalkPartsSelectionPolicy;
	}

	int32 GetCatwalkPartsCount() const noexcept
	{
		return CatwalkParts.Num();
	}

	const FDungeonMeshParts* GetCatwalkPartsAt(const int32 index) const
	{
		return AtParts(CatwalkParts, index);
	}

	/**
	 * 中二階通路パーツを更新します
	 */
	template<typename Function>
	void EachCatwalkParts(Function&& function) const
	{
		EachParts(CatwalkParts, std::forward<Function>(function));
	}

	/**
	 * シャンデリアパーツを選択します
	 */
	const FDungeonRandomActorParts* SelectChandelierParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	EDungeonPartsSelectionMethod GetChandelierPartsSelectionMethod() const noexcept
	{
		return ChandelierPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetChandelierPartsSelectionPolicy() const noexcept
	{
		return ChandelierPartsSelectionPolicy;
	}

	int32 GetChandelierPartsCount() const noexcept
	{
		return ChandelierParts.Num();
	}

	const FDungeonRandomActorParts* GetChandelierPartsAt(const int32 index) const
	{
		return AtParts(ChandelierParts, index);
	}

	float GetChandelierMinSpacing() const noexcept
	{
		return ChandelierMinSpacing;
	}

	float GetChandelierMinCeilingHeight() const noexcept
	{
		return ChandelierMinCeilingHeight;
	}

	float GetChandelierRadius() const noexcept
	{
		return ChandelierRadius;
	}

	float GetChandelierWallWeight() const noexcept
	{
		return ChandelierWallWeight;
	}

	float GetChandelierCombatWeight() const noexcept
	{
		return ChandelierCombatWeight;
	}

	/**
	 * 入力したFDungeonRandomActorPartsからアクターベースのパーツを選択します
	 */
	static FDungeonRandomActorParts* SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonSelectionPolicy selectionPolicy);

	/**
	 * 入力したparts配列からグリッドに基づいて壁パーツを選択します
	 */
	// aka: SelectActorParts, SelectRandomActorParts
	template<typename T = FDungeonMeshParts>
	static T* SelectPartsByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const EDungeonSelectionPolicy selectionPolicy)
	{
		const int32 size = parts.Num();
		if (size <= 0)
			return nullptr;

		const int32 index = SelectDungeonMeshPartsIndexByGrid(gridIndex, grid, random, size, selectionPolicy);
		return const_cast<T*>(&parts[index]);
	}

	/**
	 * 入力したparts配列から面に基づいて壁パーツを選択します
	 */
	// aka: SelectActorParts, SelectRandomActorParts
	template<typename T = FDungeonMeshParts>
	static T* SelectPartsByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const TArray<T>& parts)
	{
		const int32 size = parts.Num();
		if (size <= 0)
			return nullptr;

		const int32 index = SelectDungeonMeshPartsIndexByFace(gridLocation, direction, size);
		return const_cast<T*>(&parts[index]);
	}

	/**
	 * 入力したparts配列を更新します
	 */
	template<typename T = FDungeonMeshParts, typename Function>
	static void EachParts(const TArray<T>& parts, Function&& function)
	{
		for (const T& part : parts)
		{
			std::forward<Function>(function)(part);
		}
	}

	/**
	 * 壁のパーツを選択する方法を取得します
	 */
	EDungeonPartsSelectionMethod GetWallPartsSelectionMethod() const noexcept
	{
		return WallPartsSelectionMethod;
	}

	EDungeonSelectionPolicy GetWallPartsSelectionPolicy() const noexcept
	{
		return WallPartsSelectionPolicy;
	}


	void MigrateSelectionPolicies();
	void MarkSelectionPoliciesMigrated() noexcept
	{
		bSelectionPoliciesMigrated = true;
	}

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const;
#endif

protected:

	/**
	 * Policy for selecting floor part candidates in this mesh set.
	 *
	 * このメッシュセットで床パーツ候補を選ぶためのポリシーです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	EDungeonSelectionPolicy FloorPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick floor parts from the selected floor candidates.
	 *
	 * 選ばれた床候補から最終的な床パーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Specify the floor parts. Multiple parts can be set and will be selected based on FloorPartsSelectionMethod.
	 * 床のパーツを指定して下さい。パーツは複数設定する事ができ、FloorPartsSelectionMethodを元に選択されます。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	TArray<FDungeonMeshPartsWithDirection> FloorParts;

	/**
	 * How to select wall parts
	 * 壁のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	EDungeonSelectionPolicy WallPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick wall parts from wall candidates.
	 *
	 * 壁候補から最終的な壁パーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Specify the wall parts. Multiple parts can be set and will be selected based on WallPartsSelectionMethod.
	 * 壁のパーツを指定して下さい。パーツは複数設定する事ができ、WallPartsSelectionMethodを元に選択されます。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	TArray<FDungeonMeshParts> WallParts;

	/**
	 * How to select roof parts
	 * 天井のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
	EDungeonSelectionPolicy RoofPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick roof parts from roof candidates.
	 *
	 * 屋根候補から最終的な屋根パーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod RoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Specify the roof parts. Multiple parts can be set and will be selected based on RoofPartsSelectionMethod.
	 * 天井のパーツを指定して下さい。パーツは複数設定する事ができ、RoofPartsSelectionMethodを元に選択されます。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
	TArray<FDungeonMeshPartsWithDirection> RoofParts;

	/**
	 * How to generate parts for stairs and ramps
	 * 階段やスロープの部品を生成する方法
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	EDungeonSelectionPolicy SlopePartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick slope parts from slope candidates.
	 *
	 * スロープ候補から最終的なスロープパーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Specify parts for stairs and ramps. Multiple parts can be set and will be selected based on the SloopPartsSelectionMethod.
	 * 階段やスロープのパーツを指定して下さい。パーツは複数設定する事ができ、SloopPartsSelectionMethodを元に選択されます。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	TArray<FDungeonMeshParts> SlopeParts;

	/**
	 * How to select catwalk parts
	 * 中二階通路のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite)
	EDungeonSelectionPolicy CatwalkPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick catwalk parts from catwalk candidates.
	 *
	 * キャットウォーク候補から最終的なパーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod CatwalkPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Specify the catwalk parts. Multiple parts can be set and will be selected based on CatwalkPartsSelectionMethod.
	 * 中二階通路のパーツを指定して下さい。パーツは複数設定する事ができ、CatwalkPartsSelectionMethodを元に選択されます。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite)
	TArray<FDungeonMeshParts> CatwalkParts;

	/**
	 * How to select chandelier parts
	 * シャンデリアのパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite)
	EDungeonSelectionPolicy ChandelierPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Selection method used to pick chandelier actor parts from chandelier candidates.
	 *
	 * シャンデリア候補からシャンデリア用アクターパーツを決定する選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod ChandelierPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Chandelier parts list used for fixture spawning.
	 * シャンデリア生成に使用するパーツ一覧です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite)
	TArray<FDungeonRandomActorParts> ChandelierParts;

	/**
	 * Minimum spacing between spawned chandeliers.
	 * シャンデリア同士の最小間隔です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0"))
	float ChandelierMinSpacing = 700.f;

	/**
	 * Minimum required ceiling height for chandelier placement.
	 * シャンデリア配置に必要な最小天井高です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0"))
	float ChandelierMinCeilingHeight = 250.f;

	/**
	 * Collision probe radius for chandelier spawn checks.
	 * シャンデリア配置時の衝突確認半径です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0"))
	float ChandelierRadius = 150.f;

	/**
	 * Score weight that favors candidates farther from walls.
	 * 壁からの距離を評価するスコア重みです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite)
	float ChandelierWallWeight = 1.f;

	/**
	 * Score weight that favors combat-center-like positions.
	 * 戦闘中心を想定した位置を評価するスコア重みです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite)
	float ChandelierCombatWeight = 0.6f;

	/**
	 * Migration flag indicating legacy mesh-set selection policies were already converted.
	 *
	 * 旧メッシュセット選択ポリシーが移行済みであることを示すフラグです。
	 */
	UPROPERTY()
	bool bSelectionPoliciesMigrated = false;

private:
	template<typename T>
	static const T* AtParts(const TArray<T>& parts, const int32 index)
	{
		if (0 <= index && index < parts.Num())
			return &parts[index];
		return nullptr;
	}

	static int32 SelectDungeonMeshPartsIndexByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonSelectionPolicy selectionPolicy);
	static int32 SelectDungeonMeshPartsIndexByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const int32 size);
	static FDungeonActorParts* SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonSelectionPolicy selectionPolicy);

	friend class UDungeonMeshSetDatabase;
};
