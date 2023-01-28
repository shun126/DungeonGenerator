/**
ゴール条件 ヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <Math/IntPoint.h>
#include <Math/IntRect.h>

namespace dungeon
{
	/**
	ゴール条件クラス
	*/
	class PathGoalCondition final
	{
	public:
		/**
		コンストラクタ
		*/
		PathGoalCondition() = default;

		/**
		コンストラクタ
		*/
		explicit PathGoalCondition(const FIntRect& rect) noexcept;

		/**
		コピーコンストラクタ
		*/
		explicit PathGoalCondition(const PathGoalCondition& other) noexcept;

		/**
		ムーブコンストラクタ
		*/
		explicit PathGoalCondition(PathGoalCondition&& other) noexcept;

		/**
		デストラクタ
		*/
		~PathGoalCondition() = default;

		/**
		ゴール範囲を取得
		*/
		const FIntRect& Get() const noexcept;

		/**
		ゴール範囲を設定
		*/
		void Set(const FIntRect& rect) noexcept;

		/**
		座標がゴール条件を満たしているか？
		\param[in]	location	座標
		\return		trueならば条件を満たしている
		*/
		bool Contains(const FIntVector& location) const noexcept;

		/**
		コピー代入
		*/
		PathGoalCondition& operator=(const PathGoalCondition& other) noexcept;

		/**
		ムーブ代入
		*/
		PathGoalCondition& operator=(PathGoalCondition&& other) noexcept;

	private:
		FIntRect mRect;
	};

	inline PathGoalCondition::PathGoalCondition(const FIntRect& rect) noexcept
		: mRect(rect)
	{
	}

	inline PathGoalCondition::PathGoalCondition(const PathGoalCondition& other) noexcept
		: mRect(other.mRect)
	{
	}

	inline PathGoalCondition::PathGoalCondition(PathGoalCondition&& other) noexcept
		: mRect(std::move(other.mRect))
	{
	}

	inline PathGoalCondition& PathGoalCondition::operator=(const PathGoalCondition& other) noexcept
	{
		mRect = other.mRect;
		return *this;
	}

	inline PathGoalCondition& PathGoalCondition::operator=(PathGoalCondition&& other) noexcept
	{
		mRect = std::move(other.mRect);
		return *this;
	}

	inline const FIntRect& PathGoalCondition::Get() const noexcept
	{
		return mRect;
	}

	inline void PathGoalCondition::Set(const FIntRect& rect) noexcept
	{
		mRect = rect;
	}

	inline bool PathGoalCondition::Contains(const FIntVector& location) const noexcept
	{
		const FIntPoint point(location.X, location.Y);
		return mRect.Contains(point);
	}
}