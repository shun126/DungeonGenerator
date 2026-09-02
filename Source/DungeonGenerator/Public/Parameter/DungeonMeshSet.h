/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonMeshPartsWithDirection.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/Selector/DungeonPartsSelectorBase.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonSelectionPolicy.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSet.generated.h"

// forward declaration
class UClass;
class UStaticMesh;

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
	/**
	 * Migrates version 1 percentage-like fields.
	 * バージョン1の確率関連フィールドを移行します。
	 */
	void MigrateSpawnChancesFromVersion1()
	{
		for (FDungeonRandomActorParts& parts : ChandelierParts)
			parts.MigrateFromVersion1();
	}

	/** Destroys this mesh-set value. このメッシュセット値を破棄します。 */
	virtual ~FDungeonMeshSet() = default;

	/**
	 * Selects FloorParts.
	 * 設定されたセレクターとグリッド情報を使って床パーツを選択します。
	 */
	const FDungeonMeshPartsWithDirection* SelectFloorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the legacy floor selection method retained for migration. 移行用に保持している旧床選択方式を返します。 */
	EDungeonPartsSelectionMethod GetFloorPartsSelectionMethod() const noexcept
	{
		return FloorPartsSelectionMethod;
	}

	/** Returns the migrated floor selection policy. 移行済みの床選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetFloorPartsSelectionPolicy() const noexcept
	{
		return FloorPartsSelectionPolicy;
	}

	/** Returns the active floor-parts selector. 有効な床パーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetFloorPartsSelector() const noexcept
	{
		return FloorPartsSelector;
	}

	/** Returns the number of floor-part candidates. 床パーツ候補数を返します。 */
	int32 GetFloorPartsCount() const noexcept
	{
		return FloorParts.Num();
	}

	/** Returns the floor candidate at index, or nullptr when out of range. 指定インデックスの床候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonMeshPartsWithDirection* GetFloorPartsAt(const int32 index) const
	{
		return AtParts(FloorParts, index);
	}

	/**
	 * Visits every configured floor-part candidate.
	 * 設定されたすべての床パーツ候補を列挙します。
	 */
	template<typename Function>
	void EachFloorParts(Function&& function) const
	{
		EachParts(FloorParts, std::forward<Function>(function));
	}

	/**
	 * Selects WallPartsByGrid.
	 * 設定されたセレクターとグリッド情報を使って壁パーツを選択します。
	 */
	const FDungeonMeshParts* SelectWallPartsByGrid(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the number of wall-part candidates. 壁パーツ候補数を返します。 */
	int32 GetWallPartsCount() const noexcept
	{
		return WallParts.Num();
	}

	/** Returns the wall candidate at index, or nullptr when out of range. 指定インデックスの壁候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonMeshParts* GetWallPartsAt(const int32 index) const
	{
		return AtParts(WallParts, index);
	}

	/**
	 * Selects WallPartsByFace.
	 * グリッド位置と面方向から決定論的に壁パーツを選択します。
	 */
	const FDungeonMeshParts* SelectWallPartsByFace(const FIntVector& gridLocation, const dungeon::Direction& direction) const
	{
		return SelectPartsByFace(gridLocation, direction, WallParts);
	}

	/**
	 * Visits every configured wall-part candidate.
	 * 設定されたすべての壁パーツ候補を列挙します。
	 */
	template<typename Function>
	void EachWallParts(Function&& function) const
	{
		EachParts(WallParts, std::forward<Function>(function));
	}

	/**
	 * Selects RoofParts.
	 * 設定されたセレクターとグリッド情報を使って天井パーツを選択します。
	 */
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the legacy roof selection method retained for migration. 移行用に保持している旧天井選択方式を返します。 */
	EDungeonPartsSelectionMethod GetRoofPartsSelectionMethod() const noexcept
	{
		return RoofPartsSelectionMethod;
	}

	/** Returns the migrated roof selection policy. 移行済みの天井選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetRoofPartsSelectionPolicy() const noexcept
	{
		return RoofPartsSelectionPolicy;
	}

	/** Returns the active roof-parts selector. 有効な天井パーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetRoofPartsSelector() const noexcept
	{
		return RoofPartsSelector;
	}

	/** Returns the number of roof-part candidates. 天井パーツ候補数を返します。 */
	int32 GetRoofPartsCount() const noexcept
	{
		return RoofParts.Num();
	}

	/** Returns the roof candidate at index, or nullptr when out of range. 指定インデックスの天井候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonMeshPartsWithDirection* GetRoofPartsAt(const int32 index) const
	{
		return AtParts(RoofParts, index);
	}

	/**
	 * Visits every configured roof-part candidate.
	 * 設定されたすべての天井パーツ候補を列挙します。
	 */
	template<typename Function>
	void EachRoofParts(Function&& function) const
	{
		EachParts(RoofParts, std::forward<Function>(function));
	}

	/**
	 * Selects SlopeParts.
	 * 設定されたセレクターとグリッド情報を使ってスロープまたは階段パーツを選択します。
	 */
	const FDungeonMeshParts* SelectSlopeParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the legacy slope selection method retained for migration. 移行用に保持している旧スロープ選択方式を返します。 */
	EDungeonPartsSelectionMethod GetSlopePartsSelectionMethod() const noexcept
	{
		return SloopPartsSelectionMethod;
	}

	/** Returns the migrated slope selection policy. 移行済みのスロープ選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetSlopePartsSelectionPolicy() const noexcept
	{
		return SlopePartsSelectionPolicy;
	}

	/** Returns the active slope-parts selector. 有効なスロープパーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetSlopePartsSelector() const noexcept
	{
		return SlopePartsSelector;
	}

	/** Returns the number of slope-part candidates. スロープパーツ候補数を返します。 */
	int32 GetSlopePartsCount() const noexcept
	{
		return SlopeParts.Num();
	}

	/** Returns the slope candidate at index, or nullptr when out of range. 指定インデックスのスロープ候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonMeshParts* GetSlopePartsAt(const int32 index) const
	{
		return AtParts(SlopeParts, index);
	}

	/**
	 * Visits every configured slope-part candidate.
	 * 設定されたすべてのスロープパーツ候補を列挙します。
	 */
	template<typename Function>
	void EachSlopeParts(Function&& function) const
	{
		EachParts(SlopeParts, std::forward<Function>(function));
	}

	/**
	 * Selects CatwalkParts.
	 * 設定されたセレクターとグリッド情報を使って中二階通路パーツを選択します。
	 */
	const FDungeonMeshParts* SelectCatwalkParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the legacy catwalk selection method retained for migration. 移行用に保持している旧中二階通路選択方式を返します。 */
	EDungeonPartsSelectionMethod GetCatwalkPartsSelectionMethod() const noexcept
	{
		return CatwalkPartsSelectionMethod;
	}

	/** Returns the migrated catwalk selection policy. 移行済みの中二階通路選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetCatwalkPartsSelectionPolicy() const noexcept
	{
		return CatwalkPartsSelectionPolicy;
	}

	/** Returns the active catwalk-parts selector. 有効な中二階通路パーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetCatwalkPartsSelector() const noexcept
	{
		return CatwalkPartsSelector;
	}

	/** Returns the number of catwalk-part candidates. 中二階通路パーツ候補数を返します。 */
	int32 GetCatwalkPartsCount() const noexcept
	{
		return CatwalkParts.Num();
	}

	/** Returns the catwalk candidate at index, or nullptr when out of range. 指定インデックスの中二階通路候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonMeshParts* GetCatwalkPartsAt(const int32 index) const
	{
		return AtParts(CatwalkParts, index);
	}

	/**
	 * Visits every configured catwalk-part candidate.
	 * 設定されたすべての中二階通路パーツ候補を列挙します。
	 */
	template<typename Function>
	void EachCatwalkParts(Function&& function) const
	{
		EachParts(CatwalkParts, std::forward<Function>(function));
	}

	/**
	 * Selects ChandelierParts.
	 * 設定されたセレクターとグリッド情報を使ってシャンデリア用アクターパーツを選択します。
	 */
	const FDungeonRandomActorParts* SelectChandelierParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	/** Returns the legacy chandelier selection method retained for migration. 移行用に保持している旧シャンデリア選択方式を返します。 */
	EDungeonPartsSelectionMethod GetChandelierPartsSelectionMethod() const noexcept
	{
		return ChandelierPartsSelectionMethod;
	}

	/** Returns the migrated chandelier selection policy. 移行済みのシャンデリア選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetChandelierPartsSelectionPolicy() const noexcept
	{
		return ChandelierPartsSelectionPolicy;
	}

	/** Returns the active chandelier-parts selector. 有効なシャンデリアパーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetChandelierPartsSelector() const noexcept
	{
		return ChandelierPartsSelector;
	}

	/** Returns the number of chandelier candidates. シャンデリア候補数を返します。 */
	int32 GetChandelierPartsCount() const noexcept
	{
		return ChandelierParts.Num();
	}

	/** Returns the chandelier candidate at index, or nullptr when out of range. 指定インデックスのシャンデリア候補を返し、範囲外の場合はnullptrを返します。 */
	const FDungeonRandomActorParts* GetChandelierPartsAt(const int32 index) const
	{
		return AtParts(ChandelierParts, index);
	}

	/** Returns minimum chandelier spacing in world units. ワールド単位のシャンデリア最小間隔を返します。 */
	float GetChandelierMinSpacing() const noexcept
	{
		return ChandelierMinSpacing;
	}

	/** Returns minimum ceiling height required for chandeliers in world units. シャンデリアに必要な最小天井高をワールド単位で返します。 */
	float GetChandelierMinCeilingHeight() const noexcept
	{
		return ChandelierMinCeilingHeight;
	}

	/** Returns the chandelier collision-probe radius in world units. シャンデリアの衝突確認半径をワールド単位で返します。 */
	float GetChandelierRadius() const noexcept
	{
		return ChandelierRadius;
	}

	/** Returns the wall-distance score weight for chandelier placement. シャンデリア配置における壁距離のスコア重みを返します。 */
	float GetChandelierWallWeight() const noexcept
	{
		return ChandelierWallWeight;
	}

	/** Returns the combat-center score weight for chandelier placement. シャンデリア配置における戦闘中心のスコア重みを返します。 */
	float GetChandelierCombatWeight() const noexcept
	{
		return ChandelierCombatWeight;
	}

	/**
	 * Selects RandomActorParts.
	 * 指定された候補と選択コンテキストからアクターベースのパーツを選択します。
	 */
	static FDungeonRandomActorParts* SelectRandomActorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const UDungeonPartsSelectorBase* selector, EDungeonPartsSelectorTarget target, uint8 neighborMask6 = 0);

	/**
	 * Selects PartsByGrid.
	 * 指定されたセレクターをグリッド情報へ適用して候補を1つ選択します。
	 */
	// aka: SelectActorParts, SelectRandomActorParts
	template<typename T = FDungeonMeshParts>
	static T* SelectPartsByGrid(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const UDungeonPartsSelectorBase* selector, const EDungeonPartsSelectorTarget target, const uint8 neighborMask6 = 0)
	{
		const int32 size = parts.Num();
		if (size <= 0)
			return nullptr;

		const int32 index = SelectDungeonMeshPartsIndexBySelector(gridLocation, gridIndex, grid, random, size, selector, target, neighborMask6);
		return const_cast<T*>(&parts[index]);
	}

	/**
	 * Selects PartsByFace.
	 * グリッド位置と面方向から決定論的に候補を1つ選択します。
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
	 * Visits every element in the supplied candidate array.
	 * 指定された候補配列の全要素を列挙します。
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
	 * Returns WallPartsSelectionMethod.
	 * 移行用に保持している旧壁選択方式を返します。
	 */
	EDungeonPartsSelectionMethod GetWallPartsSelectionMethod() const noexcept
	{
		return WallPartsSelectionMethod;
	}

	/** Returns the migrated wall selection policy. 移行済みの壁選択ポリシーを返します。 */
	EDungeonSelectionPolicy GetWallPartsSelectionPolicy() const noexcept
	{
		return WallPartsSelectionPolicy;
	}

	/** Returns the active wall-parts selector. 有効な壁パーツセレクターを返します。 */
	const UDungeonPartsSelectorBase* GetWallPartsSelector() const noexcept
	{
		return WallPartsSelector;
	}

	/** Returns the deprecated shared selector retained for v1 migration. v1移行用に保持している非推奨の共有セレクターを返します。 */
	const UDungeonPartsSelectorBase* GetDungeonPartsSelector() const noexcept
	{
		return DungeonPartsSelector;
	}

	/** Converts legacy selection methods and policies into instanced selector objects. 旧選択方式とポリシーをインスタンス化セレクターへ変換します。 */
	void MigrateSelectionPolicies(UObject* Outer = nullptr);
	/** Marks legacy selection policies as migrated without creating selectors. セレクターを生成せず旧選択ポリシーを移行済みとして記録します。 */
	void MarkSelectionPoliciesMigrated() noexcept
	{
		bSelectionPoliciesMigrated = true;
	}

protected:
	/**
	 * How to select floor parts
	 * 床のパーツを選択する方法
	 */
	UPROPERTY()
	TObjectPtr<UDungeonPartsSelectorBase> DungeonPartsSelector;

	/**
	 * Policy for selecting floor part candidates in this mesh set.
	 *
	 * このメッシュセットで床パーツ候補を選ぶためのポリシーです。
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Floor", BlueprintReadWrite, meta = (DisplayName = "Floor Parts Selector", ToolTip = "Selects one floor part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> FloorPartsSelector;

	/** Internal migration value used while converting legacy floor methods into selector objects. 旧床選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite, meta = (ToolTip = "Floor mesh candidates selected by Floor Parts Selector."))
	TArray<FDungeonMeshPartsWithDirection> FloorParts;

	/**
	 * How to select wall parts
	 * 壁のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Wall", BlueprintReadWrite, meta = (DisplayName = "Wall Parts Selector", ToolTip = "Selects one wall part from the candidates. Grid Index preserves per-face wall selection behavior."))
	TObjectPtr<UDungeonPartsSelectorBase> WallPartsSelector;

	/** Internal migration value used while converting legacy wall methods into selector objects. 旧壁選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite, meta = (ToolTip = "Wall mesh candidates selected by Wall Parts Selector."))
	TArray<FDungeonMeshParts> WallParts;

	/**
	 * How to select roof parts
	 * 天井のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Roof", BlueprintReadWrite, meta = (DisplayName = "Roof Parts Selector", ToolTip = "Selects one roof part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> RoofPartsSelector;

	/** Internal migration value used while converting legacy roof methods into selector objects. 旧天井選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite, meta = (ToolTip = "Roof mesh candidates selected by Roof Parts Selector."))
	TArray<FDungeonMeshPartsWithDirection> RoofParts;

	/**
	 * How to generate parts for stairs and ramps
	 * 階段やスロープの部品を生成する方法
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Sloop", BlueprintReadWrite, meta = (DisplayName = "Slope Parts Selector", ToolTip = "Selects one slope or stair part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> SlopePartsSelector;

	/** Internal migration value used while converting legacy slope methods into selector objects. 旧スロープ選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite, meta = (ToolTip = "Slope and stair mesh candidates selected by Slope Parts Selector."))
	TArray<FDungeonMeshParts> SlopeParts;

	/**
	 * How to select catwalk parts
	 * 中二階通路のパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite, meta = (DisplayName = "Catwalk Parts Selector", ToolTip = "Selects one catwalk part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> CatwalkPartsSelector;

	/** Internal migration value used while converting legacy catwalk methods into selector objects. 旧中二階通路選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite, meta = (ToolTip = "Catwalk mesh candidates selected by Catwalk Parts Selector."))
	TArray<FDungeonMeshParts> CatwalkParts;

	/**
	 * How to select chandelier parts
	 * シャンデリアのパーツを選択する方法
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (DisplayName = "Chandelier Parts Selector", ToolTip = "Selects one chandelier actor part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> ChandelierPartsSelector;

	/** Internal migration value used while converting legacy chandelier methods into selector objects. 旧シャンデリア選択方式をセレクターオブジェクトへ変換するときに使う内部移行値です。 */
	UPROPERTY()
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ToolTip = "Chandelier actor candidates selected by Chandelier Parts Selector."))
	TArray<FDungeonRandomActorParts> ChandelierParts;

	/**
	 * Minimum spacing between spawned chandeliers.
	 * シャンデリア同士の最小間隔です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0", ToolTip = "Minimum spacing between chandeliers in Unreal world units."))
	float ChandelierMinSpacing = 700.f;

	/**
	 * Minimum required ceiling height for chandelier placement.
	 * シャンデリア配置に必要な最小天井高です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0", ToolTip = "Minimum ceiling height required for chandelier placement in Unreal world units."))
	float ChandelierMinCeilingHeight = 250.f;

	/**
	 * Collision probe radius for chandelier spawn checks.
	 * シャンデリア配置時の衝突確認半径です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ClampMin = "1.0", ToolTip = "Collision-probe radius used when checking chandelier placement, in Unreal world units."))
	float ChandelierRadius = 150.f;

	/**
	 * Score weight that favors candidates farther from walls.
	 * 壁からの距離を評価するスコア重みです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ToolTip = "Relative score weight that favors chandelier positions farther from walls."))
	float ChandelierWallWeight = 1.f;

	/**
	 * Score weight that favors combat-center-like positions.
	 * 戦闘中心を想定した位置を評価するスコア重みです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Chandelier", BlueprintReadWrite, meta = (ToolTip = "Relative score weight that favors chandelier positions near combat-room centers."))
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

	static int32 SelectDungeonMeshPartsIndexBySelector(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const UDungeonPartsSelectorBase* selector, EDungeonPartsSelectorTarget target, uint8 neighborMask6);
	static int32 SelectDungeonMeshPartsIndexByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const int32 size);
	static FDungeonActorParts* SelectActorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const UDungeonPartsSelectorBase* selector, EDungeonPartsSelectorTarget target, uint8 neighborMask6);

	friend class UDungeonMeshSetDatabase;
};
