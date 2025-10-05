/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

/*
通路グリッドをまとめたマップクラス
*/

#include "Helper/DungeonAisleGridMap.h"

UDungeonAisleGridMap::UDungeonAisleGridMap(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

void UDungeonAisleGridMap::Register(const int32 identifier, const EDungeonDirection direction, const FVector& location)
{
	FDungeonAisleGrid aisleGrid;
	aisleGrid.Direction = direction;
	aisleGrid.Location = location;

	auto it = mAisleGridMap.find(identifier);
	if (it == mAisleGridMap.end())
	{
		TArray<FDungeonAisleGrid> aisleGrids;
		aisleGrids.Add(aisleGrid);
		mAisleGridMap.emplace(identifier, aisleGrids);
	}
	else
	{
		it->second.Add(aisleGrid);
	}
}

void UDungeonAisleGridMap::ForEach(const FDungeonAisleGridMapLoopSignature& OnLoop) const
{
	for (const auto& aisleGrid : mAisleGridMap)
	{
		if (OnLoop.ExecuteIfBound(aisleGrid.second) == false)
			break;
	}
}

void UDungeonAisleGridMapBlueprintFunctionLibrary::ForEach(const UDungeonAisleGridMap* aisleGridArray, const FDungeonAisleGridMapLoopSignature& OnLoop)
{
	if (aisleGridArray)
		aisleGridArray->ForEach(OnLoop);
}
