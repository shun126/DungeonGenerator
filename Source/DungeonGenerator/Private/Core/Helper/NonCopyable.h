/*!
@file		NonCopyable.h
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	/*!
	Copy prohibited Mix-in
	Be sure to inherit by private inheritance.
	*/
	class NonCopyable
	{
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};
}

