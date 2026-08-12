/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "../Helper/Identifier.h"
#include "Parameter/DungeonLayoutTypes.h"
#include <array>
#include <cstdint>
#include <memory>

namespace dungeon
{
	class Point;

	/**
	 * Represents Aisle.
	 * 通路 クラス
	 */
	class Aisle final
	{
	public:
		/**
		 * コンストラクタ
		 * @param[in]	main	幹線通路
		 * @param[in]  p0		辺の頂点
		 * @param[in]  p1		辺の頂点
		 */
		Aisle(const bool main, const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const EDungeonAislePurpose purpose = EDungeonAislePurpose::MainPath) noexcept;

		/**
		 * コピーコンストラクタ
		 * @param[in]  other	Aisle
		 */
		Aisle(const Aisle& other) noexcept;

		/**
		 * ムーブコンストラクタ
		 * @param[in]  other	Aisle
		 */
		Aisle(Aisle&& other) noexcept;

		/**
		 * Destroys the ~Aisle instance.
		 * デストラクタ
		 */
		~Aisle() = default;

		/**
		 * 点が辺の頂点に含まれるか？
		 * @param[in]	point	比較する点
		 * @return		trueならば等しい
		 */
		bool Contain(const std::shared_ptr<const Point>& point) const noexcept;

		/**
		 * 頂点を取得します
		 * @param[in]	index	頂点番号（０～１）
		 * @return		頂点
		 */
		const std::shared_ptr<const Point>& GetPoint(const size_t index) const noexcept;

		/**
		 * 辺の長さを取得します
		 * @return		辺の長さ
		 */
		double GetLength() const noexcept;

		/**
		 * 識別子を取得
		 * @return		識別子
		 */
		const Identifier& GetIdentifier() const noexcept;

		/**
		 * Returns the zone index of the deeper endpoint room, using the larger room identifier to break equal-depth ties.
		 * 深度の大きい接続先の部屋の Zone インデックスを返し、同深度の場合は大きい部屋識別子を優先します。
		 */
		int32 GetZoneIndex() const noexcept;

		/**
		 * Returns whether ain.
		 * 幹線通路か取得します
		 */
		bool IsMain() const noexcept;

		/**
		 * Sets whether this aisle belongs to the selected main route.
		 * この通路が選択された主経路に属するかを設定します。
		 */
		void SetMain(bool main) noexcept;

		/**
		 * Get aisle purpose assigned by the layout planner.
		 *
		 * レイアウトプランナーが割り当てた通路目的を取得します。
		 */
		EDungeonAislePurpose GetPurpose() const noexcept;

		/**
		 * Sets the purpose assigned by the layout planner.
		 * レイアウトプランナーが割り当てた通路目的を設定します。
		 */
		void SetPurpose(EDungeonAislePurpose purpose) noexcept;

		/**
		 * Returns whether this aisle moves between floors.
		 * この通路が階層をまたぐか取得します
		 * 部屋の位置は生成中に変化するため、呼び出した時点の部屋の高さから判定します
		 */
		bool IsVerticalTransition() const noexcept;

		/**
		 * Returns whether Locked.
		 * 閉鎖状態を取得します
		 */
		bool IsLocked() const noexcept;

		/**
		 * Sets Lock.
		 * 閉鎖状態を設定します
		 */
		void SetLock(const bool lock) noexcept;

		/**
		 * Returns whether UniqueLocked.
		 * ユニークな鍵が必要な状態を取得します
		 */
		bool IsUniqueLocked() const noexcept;

		/**
		 * Sets UniqueLock.
		 * ユニークな鍵が必要な状態を設定します
		 */
		void SetUniqueLock(const bool lock) noexcept;

		/**
		 * Returns whether AnyLocked.
		 * 何らかの鍵が必要な状態を取得します
		 */
		bool IsAnyLocked() const noexcept;

		/**
		 * Returns Height.
		 * 通路の高さを取得します
		 */
		uint8_t GetHeight() const noexcept;

		/**
		 * Sets Height.
		 * 通路の高さを設定します
		 */
		void SetHeight(const uint8_t height) noexcept;

		/**
		 * コピー代入
		 * @param[in]  other	Aisle
		 */
		Aisle& operator=(const Aisle& other) noexcept;

		/**
		 * ムーブ代入
		 * @param[in]  other	Aisle
		 */
		Aisle& operator=(Aisle&& other) noexcept;

		/**
		 * 辺が等しいか判定します
		 * @param[in]	other	比較する辺
		 * @return		trueならば等しい
		 */
		bool operator==(const Aisle& other) const noexcept;

		/**
		 * 辺が等しくないか判定します
		 * @param[in]	other	比較する辺
		 * @return		trueならば等しくない
		 */
		bool operator!=(const Aisle& other) const noexcept;


		void Dump(std::ofstream& stream) const noexcept;

	private:
		std::array<std::shared_ptr<const Point>, 2> mPoints;
		double mLength;
		Identifier mIdentifier;
		bool mMain = false;
		EDungeonAislePurpose mPurpose = EDungeonAislePurpose::MainPath;
		uint8_t mHeight = 1;
		bool mLocked = false;
		bool mUniqueLocked = false;
	};
}

#include "Aisle.inl"
