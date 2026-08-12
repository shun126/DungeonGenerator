/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

/**
 * Copy prohibited Mix-in
 * Be sure to inherit by private inheritance.
 * DungeonNonCopyable を表します。
 */
class DungeonNonCopyable
{
protected:
	DungeonNonCopyable() = default;
	~DungeonNonCopyable() = default;
	DungeonNonCopyable(const DungeonNonCopyable&) = delete;
	DungeonNonCopyable& operator=(const DungeonNonCopyable&) = delete;
};
