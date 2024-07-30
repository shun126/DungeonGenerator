/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include <CoreMinimal.h>
#include <functional>
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

	static FDungeonRandomActorParts* SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);

	// aka: SelectActorParts, SelectRandomActorParts
	template<typename T = FDungeonMeshParts>
	static T* SelectParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod)
	{
		const int32 size = parts.Num();
		if (size <= 0)
			return nullptr;

		const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
		return const_cast<T*>(&parts[index]);
	}

	template<typename T = FDungeonMeshParts>
	static void EachParts(const TArray<T>& parts, std::function<void(const T&)> func)
	{
		for (const T& part : parts)
		{
			func(part);
		}
	}

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const;
#endif

private:
	static int32 SelectDungeonMeshPartsIndex(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod);
	static FDungeonActorParts* SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);
};
