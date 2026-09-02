/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * @file		NonCopyable.h
 * NonCopyable を表します。
 */

#pragma once

namespace dungeon
{
/**
 * Copy prohibited Mix-in
 * Be sure to inherit by private inheritance.
 * NonCopyable を表します。
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

