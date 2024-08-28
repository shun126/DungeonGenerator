/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSetDatabase.generated.h"

struct FDungeonMeshSet;
namespace dungeon
{
	class Random;
}

/**
Database of dungeon mesh sets
ダンジョンのメッシュセットのデータベース
*/
UCLASS(Blueprintable, BlueprintType)
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
};

inline UDungeonMeshSetDatabase::UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline const FDungeonMeshSet* UDungeonMeshSetDatabase::AtImplement(const size_t index) const
{
	return nullptr;
}

inline const FDungeonMeshSet* UDungeonMeshSetDatabase::SelectImplement(const std::shared_ptr<dungeon::Random>& random) const
{
	return nullptr;
}
