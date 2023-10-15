/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/Box.h>
#include <vector>

class CDungeonInteriorUnplaceableBounds final
{
public:
	CDungeonInteriorUnplaceableBounds();
	explicit CDungeonInteriorUnplaceableBounds(const CDungeonInteriorUnplaceableBounds& other);
	explicit CDungeonInteriorUnplaceableBounds(CDungeonInteriorUnplaceableBounds&& other) noexcept;

	CDungeonInteriorUnplaceableBounds& operator=(const CDungeonInteriorUnplaceableBounds& other);
	CDungeonInteriorUnplaceableBounds& operator=(CDungeonInteriorUnplaceableBounds&& other) noexcept;

	void Add(const FBox& box);
	void Clear();

	bool Intersect(const FBox& bounds) const noexcept;
	bool IsInsideOrOn(const FVector& location) const noexcept;

private:
	std::vector<FBox> mUnplaceableBounds;
};

inline CDungeonInteriorUnplaceableBounds::CDungeonInteriorUnplaceableBounds()
{
}

inline CDungeonInteriorUnplaceableBounds::CDungeonInteriorUnplaceableBounds(const CDungeonInteriorUnplaceableBounds& other)
	: mUnplaceableBounds(other.mUnplaceableBounds)
{
}

inline CDungeonInteriorUnplaceableBounds::CDungeonInteriorUnplaceableBounds(CDungeonInteriorUnplaceableBounds&& other) noexcept
	: mUnplaceableBounds(std::move(other.mUnplaceableBounds))
{
}

inline CDungeonInteriorUnplaceableBounds& CDungeonInteriorUnplaceableBounds::operator=(const CDungeonInteriorUnplaceableBounds& other)
{
	mUnplaceableBounds = other.mUnplaceableBounds;
}

inline CDungeonInteriorUnplaceableBounds& CDungeonInteriorUnplaceableBounds::operator=(CDungeonInteriorUnplaceableBounds&& other) noexcept
{
	mUnplaceableBounds = std::move(other.mUnplaceableBounds);
}

inline void CDungeonInteriorUnplaceableBounds::Add(const FBox& box)
{
	mUnplaceableBounds.emplace_back(box);
}

inline void CDungeonInteriorUnplaceableBounds::Clear()
{
	mUnplaceableBounds.clear();
}

inline bool CDungeonInteriorUnplaceableBounds::Intersect(const FBox& bounds) const noexcept
{
	for (const FBox& unplacatableBounds : mUnplaceableBounds)
	{
		if (unplacatableBounds.Intersect(bounds))
			return true;
	}
	return false;
}

inline bool CDungeonInteriorUnplaceableBounds::IsInsideOrOn(const FVector& location) const noexcept
{
	for (const FBox& unplacatableBounds : mUnplaceableBounds)
	{
		if (unplacatableBounds.IsInsideOrOn(location))
			return true;
	}
	return false;
}
