/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <memory>
#include "DungeonTemporaryMeshSetDatabase.generated.h"

struct FDungeonTemporaryMeshSet;
namespace dungeon
{
	class Random;
}

/**
Database of dungeon mesh sets
ダンジョンのメッシュセットのデータベース
*/
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonTemporaryMeshSetDatabase : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonTemporaryMeshSetDatabase(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonTemporaryMeshSetDatabase() override = default;

	/**
	FDungeonTemporaryMeshSetを取得します
	*/
	virtual const FDungeonTemporaryMeshSet* AtImplement(const size_t index) const;

	/**
	FDungeonTemporaryMeshSetをランダムに抽選します
	*/
	virtual const FDungeonTemporaryMeshSet* SelectImplement(const std::shared_ptr<dungeon::Random>& random) const;
};

inline UDungeonTemporaryMeshSetDatabase::UDungeonTemporaryMeshSetDatabase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline const FDungeonTemporaryMeshSet* UDungeonTemporaryMeshSetDatabase::AtImplement(const size_t index) const
{
	return nullptr;
}

inline const FDungeonTemporaryMeshSet* UDungeonTemporaryMeshSetDatabase::SelectImplement(const std::shared_ptr<dungeon::Random>& random) const
{
	return nullptr;
}
