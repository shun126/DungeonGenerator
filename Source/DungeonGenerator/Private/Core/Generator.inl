/**
ダンジョン生成ヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once 

namespace dungeon
{
	inline Generator::Error Generator::GetLastError() const noexcept
	{
		return mLastError;
	}
}
