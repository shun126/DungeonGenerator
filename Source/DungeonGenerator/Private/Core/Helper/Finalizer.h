/*!
@file		Finalizer.h
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "NonCopyable.h"
#include <functional>

namespace dungeon
{
	class Finalizer final : NonCopyable
	{
	public:
		explicit Finalizer(const std::function<void()>& function)
			: mFunction(function)
		{
		}

		~Finalizer()
		{
			mFunction();
		}

	private:
		std::function<void()> mFunction;
	};
}
