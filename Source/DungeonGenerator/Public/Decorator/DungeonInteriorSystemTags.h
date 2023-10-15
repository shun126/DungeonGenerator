/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonInteriorSystemTags.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FDungeonInteriorSystemTags
{
	GENERATED_BODY()

public:
	const TSet<FString>& InteriorSystemTags() const
	{
		const_cast<FDungeonInteriorSystemTags*>(this)->CacheInteriorSystemTags();
		return mCachedInteriorSystemTags;
	}

	void TagMajor(const bool enable)
	{
		mDirtyInteriorSystemTags = major != enable;
		major = enable;
	}

	void TagAisle(const bool enable)
	{
		mDirtyInteriorSystemTags = aisle != enable;
		aisle = enable;
	}

	void TagSlope(const bool enable)
	{
		mDirtyInteriorSystemTags = slope != enable;
		slope = true;
	}

	void TagHall(const bool enable)
	{
		mDirtyInteriorSystemTags = hall != enable;
		hall = enable;
	}

	void TagHanare(const bool enable)
	{
		mDirtyInteriorSystemTags = hanare != enable;
		hanare = enable;
	}

	void TagStart(const bool enable)
	{
		mDirtyInteriorSystemTags = start != enable;
		start = enable;
	}

	void TagGoal(const bool enable)
	{
		mDirtyInteriorSystemTags = goal != enable;
		goal = enable;
	}

	void TagCenter(const bool enable)
	{
		mDirtyInteriorSystemTags = center != enable;
		center = enable;
	}

	void TagWall(const bool enable)
	{
		mDirtyInteriorSystemTags = wall != enable;
		wall = enable;
	}

private:
	void CacheInteriorSystemTags()
	{
#if !WITH_EDITOR
		if (mDirtyInteriorSystemTags)
#endif
		{
			mDirtyInteriorSystemTags = false;

			mCachedInteriorSystemTags.Empty();
#define __ADD__(value) if (value) mCachedInteriorSystemTags.Add(TEXT(#value))
			__ADD__(major);
			__ADD__(aisle);
			__ADD__(slope);
			__ADD__(hall);
			__ADD__(hanare);
			__ADD__(start);
			__ADD__(goal);
			__ADD__(center);
			__ADD__(wall);
#undef __ADD__
		}
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Priority")
		bool major = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool aisle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool slope = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool hall = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool hanare = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool start = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Location")
		bool goal = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Place")
		bool center = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Place")
		bool wall = true;

private:
	bool mDirtyInteriorSystemTags = true;
	TSet<FString> mCachedInteriorSystemTags;
};
