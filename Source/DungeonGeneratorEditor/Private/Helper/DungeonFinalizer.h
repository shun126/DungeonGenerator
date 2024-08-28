/*!
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonNonCopyable.h"
#include <functional>

class DungeonFinalizer final : DungeonNonCopyable
{
public:
	explicit DungeonFinalizer(const std::function<void()>& function)
		: mFunction(function)
	{
	}

	~DungeonFinalizer()
	{
		mFunction();
	}

private:
	std::function<void()> mFunction;
};
