/*!
通路に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Identifier.h"
#include <array>

namespace dungeon
{
	class Point;

	/*!
	通路 クラス
	*/
	class Aisle final
	{
	public:
		/*!
		コンストラクタ
		\param[in]  p0  辺の頂点
		\param[in]  p1  辺の頂点
		*/
		Aisle(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1) noexcept;

		/*!
		コピーコンストラクタ
		\param[in]  other	Aisle
		*/
		Aisle(const Aisle& other) noexcept;

		/*!
		ムーブコンストラクタ
		\param[in]  other	Aisle
		*/
		Aisle(Aisle&& other) noexcept;

		/*!
		デストラクタ
		*/
		~Aisle() = default;

		/*!
		点が辺の頂点に含まれるか？
		\param[in]	point	比較する点
		\return		trueならば等しい
		*/
		bool Contain(const std::shared_ptr<const Point>& point) const noexcept;

		/*!
		頂点を取得します
		\param[in]	index	頂点番号（０～１）
		\return		頂点
		*/
		const std::shared_ptr<const Point>& GetPoint(const size_t index) const noexcept;

		/*!
		辺の長さを取得します
		\return		辺の長さ
		*/
		double GetLength() const noexcept;

		/*!
		識別子を取得
		\return		識別子
		*/
		const Identifier& GetIdentifier() const noexcept;

		/*!
		閉鎖状態を取得します
		*/
		bool IsLocked() const noexcept;

		/*!
		閉鎖状態を設定します
		*/
		void SetLock(const bool lock) noexcept;

		/*!
		ユニークな鍵が必要な状態を取得します
		*/
		bool IsUniqueLocked() const noexcept;

		/*!
		ユニークな鍵が必要な状態を設定します
		*/
		void SetUniqueLock(const bool lock) noexcept;

		/*!
		コピー代入
		\param[in]  other	Aisle
		*/
		Aisle& operator=(const Aisle& other) noexcept;

		/*!
		ムーブ代入
		\param[in]  other	Aisle
		*/
		Aisle& operator=(Aisle&& other) noexcept;

		/*!
		辺が等しいか判定します
		\param[in]	other	比較する辺
		\return		trueならば等しい
		*/
		bool operator==(const Aisle& other) const noexcept;

		/*!
		辺が等しくないか判定します
		\param[in]	other	比較する辺
		\return		trueならば等しくない
		*/
		bool operator!=(const Aisle& other) const noexcept;


		void Dump(std::ofstream& stream) const noexcept;

	private:
		std::array<std::shared_ptr<const Point>, 2> mPoints;
		double mLength;
		Identifier mIdentifier;
		bool mLocked = false;
		bool mUniqueLocked = false;
	};
}

#include "Aisle.inl"
