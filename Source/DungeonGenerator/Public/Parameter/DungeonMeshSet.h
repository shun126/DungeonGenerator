/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonMeshPartsWithDirection.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
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
Dungeon mesh set
ダンジョンのメッシュセット
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
	const FDungeonMeshPartsWithDirection* SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
	{
		return SelectPartsByGrid(gridIndex, grid, random, FloorParts, FloorPartsSelectionMethod);
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
	const FDungeonMeshParts* SelectWallPartsByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
	{
		return SelectPartsByGrid(gridIndex, grid, random, WallParts, WallPartsSelectionMethod);
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
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
	{
		return SelectPartsByGrid(gridIndex, grid, random, RoofParts, RoofPartsSelectionMethod);
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
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
	{
		return SelectPartsByGrid(gridIndex, grid, random, SlopeParts, SloopPartsSelectionMethod);
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
	const FDungeonMeshParts* SelectCatwalkParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
	{
		return SelectPartsByGrid(gridIndex, grid, random, CatwalkParts, CatwalkPartsSelectionMethod);
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
	 * 入力したFDungeonRandomActorPartsからアクターベースのパーツを選択します
	 */
	static FDungeonRandomActorParts* SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);

	/**
	 * 入力したparts配列からグリッドに基づいて壁パーツを選択します
	 */
	// aka: SelectActorParts, SelectRandomActorParts
	template<typename T = FDungeonMeshParts>
	static T* SelectPartsByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod)
	{
		const int32 size = parts.Num();
		if (size <= 0)
			return nullptr;

		const int32 index = SelectDungeonMeshPartsIndexByGrid(gridIndex, grid, random, size, partsSelectionMethod);
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
	壁のパーツを選択する方法を取得します
	*/
	EDungeonPartsSelectionMethod GetWallPartsSelectionMethod() const noexcept
	{
		return WallPartsSelectionMethod;
	}

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const;
#endif

protected:
	/**
	How to select floor parts
	床のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Specify the floor parts. Multiple parts can be set and will be selected based on FloorPartsSelectionMethod.
	床のパーツを指定して下さい。パーツは複数設定する事ができ、FloorPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	TArray<FDungeonMeshPartsWithDirection> FloorParts;

	/**
	How to select wall parts
	壁のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Specify the wall parts. Multiple parts can be set and will be selected based on WallPartsSelectionMethod.
	壁のパーツを指定して下さい。パーツは複数設定する事ができ、WallPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	TArray<FDungeonMeshParts> WallParts;

	/**
	How to select roof parts
	天井のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
	EDungeonPartsSelectionMethod RoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Specify the roof parts. Multiple parts can be set and will be selected based on RoofPartsSelectionMethod.
	天井のパーツを指定して下さい。パーツは複数設定する事ができ、RoofPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
	TArray<FDungeonMeshPartsWithDirection> RoofParts;

	/**
	How to generate parts for stairs and ramps
	階段やスロープの部品を生成する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Specify parts for stairs and ramps. Multiple parts can be set and will be selected based on the SloopPartsSelectionMethod.
	階段やスロープのパーツを指定して下さい。パーツは複数設定する事ができ、SloopPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	TArray<FDungeonMeshParts> SlopeParts;

	/**
	How to select catwalk parts
	中二階通路のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite)
	EDungeonPartsSelectionMethod CatwalkPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Specify the catwalk parts. Multiple parts can be set and will be selected based on CatwalkPartsSelectionMethod.
	中二階通路のパーツを指定して下さい。パーツは複数設定する事ができ、CatwalkPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Catwalk", BlueprintReadWrite)
	TArray<FDungeonMeshParts> CatwalkParts;

private:
	static int32 SelectDungeonMeshPartsIndexByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod);
	static int32 SelectDungeonMeshPartsIndexByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const int32 size);
	static FDungeonActorParts* SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);

	friend class UDungeonMeshSetDatabase;
};
