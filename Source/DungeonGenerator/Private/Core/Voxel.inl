/**
グリッドに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline Voxel::Error Voxel::GetLastError() const noexcept
	{
		return mLastError;
	}
}
