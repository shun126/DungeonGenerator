/**
部屋に関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Helper/Identifier.h"
#include "../Math/Point.h"
#include <Math/IntRect.h>
#include <Math/IntVector.h>
#include <string_view>

namespace dungeon
{
	// 前方宣言
	struct GenerateParameter;

	/**
	部屋クラス
	座標系がUnrealEngine準拠のZアップである事に注意して下さい。
	*/
	class Room final
	{
	public:
		/**
		EDungeonRoomPartsと対応して下さい
		変更した場合はGetPartsNameもあわせて修正して下さい
		*/
		enum class Parts : uint8_t
		{
			Unidentified,	//!< 未識別
			Hall,			//!< 広間（通路が複数つながっている）
			Hanare,			//!< 離れ（通路が一つだけつながっている）
			Start,			//!< スタート地点
			Goal,			//!< ゴール地点
		};
		static constexpr uint8_t PartsSize = static_cast<uint8_t>(Parts::Goal) + 1;

		/**
		変更した場合はGetItemNameもあわせて修正して下さい
		*/
		enum class Item : uint8_t
		{
			Empty,
			Key,
			UniqueKey,
		};
		static constexpr uint8_t ItemSize = static_cast<uint8_t>(Item::UniqueKey) + 1;

	public:
		/**
		コンストラクタ
		*/
		Room(const GenerateParameter& parameter, const FIntVector& location) noexcept;

		/**
		コンストラクタ
		*/
		Room(const FIntVector& location, const FIntVector& size) noexcept;

		/**
		コピーコンストラクタ
		*/
		Room(const Room& other) noexcept;

		/**
		デストラクタ
		*/
		~Room() = default;

		/**
		コピー代入
		*/
		Room& operator=(const Room& other) noexcept;

		/**
		識別子を取得
		@return		識別子
		*/
		const Identifier& GetIdentifier() const noexcept;

		/**
		X座標を取得
		@return		X座標
		*/
		int32_t GetX() const noexcept;

		/**
		Y座標を取得
		@return		Y座標
		*/
		int32_t GetY() const noexcept;

		/**
		Z座標を取得
		@return		Z座標
		*/
		int32_t GetZ() const noexcept;

		/**
		X座標を設定
		@param[in]	x	X座標
		*/
		void SetX(const int32_t x) noexcept;

		/**
		Y座標を設定
		@param[in]	y	Y座標
		*/
		void SetY(const int32_t y) noexcept;

		/**
		Z座標を設定
		@param[in]	z	Z座標
		*/
		void SetZ(const int32_t z) noexcept;

		/**
		幅を取得します
		@return		幅
		*/
		int32_t GetWidth() const noexcept;

		/**
		奥行きを取得します
		@return		奥行き
		*/
		int32_t GetDepth() const noexcept;

		/**
		高さを取得します
		@return		高さ
		*/
		int32_t GetHeight() const noexcept;

		/**
		幅を設定します
		@param[in]	width	幅
		*/
		void SetWidth(const int32_t width) noexcept;

		/**
		奥行きを設定します
		@param[in]	depth	奥行き
		*/
		void SetDepth(const int32_t depth) noexcept;

		/**
		高さを設定します
		@param[in]	height	高さ
		*/
		void SetHeight(const int32_t height) noexcept;

		/**
		矩形の左の座標を取得します
		@return		矩形の左の座標
		*/
		int32_t GetLeft() const noexcept;

		/**
		矩形の右の座標を取得します
		@return		矩形の右の座標（X + Width - 1）
		*/
		int32_t GetRight() const noexcept;

		/**
		矩形の上の座標を取得します
		@return		矩形の上の座標
		*/
		int32_t GetTop() const noexcept;

		/**
		矩形の下の座標を取得します
		@return		矩形の下の座標（Y + Depth - 1）
		*/
		int32_t GetBottom() const noexcept;

		/**
		矩形を取得します
		*/
		FIntRect GetRect() const noexcept;

		/**
		立方体の手前の座標（軸の大きい方）を取得します
		*/
		int32_t GetForeground() const noexcept;

		/**
		立方体の奥の座標（軸の小さい方）を取得します
		*/
		int32_t GetBackground() const noexcept;

		/**
		矩形の中心の座標を取得します
		@return		矩形の中心
		*/
		Point GetCenter() const noexcept;

		/**
		矩形の半分の辺を取得します
		@return		矩形の半分の辺
		*/
		FVector GetExtent() const noexcept;

		/**
		 * 矩形の最小位置を取得します
		 */
		FVector GetMin() const noexcept;

		/**
		 * 矩形の最大位置を取得します
		 */
		FVector GetMax() const noexcept;

		/**
		地面の中心の座標を取得します
		@return		地面の中心
		*/
		Point GetGroundCenter() const noexcept;

		/**
		部屋が交差しているか調べます
		@param[in]	other	調べる部屋
		@return		trueならば交差している
		*/
		bool Intersect(const Room& other) const noexcept;

		/**
		部屋が交差しているか調べます
		@param[in]	other				調べる部屋
		@param[in]	horizontalMargin	水平方向のマージン
		@param[in]	verticalMargin		垂直方向のマージン
		@return		trueならば交差している
		垂直方向のマージンは床方向に１、天井方向はかならず０で交差判定します。
		*/
		bool Intersect(const Room& other, const uint32_t horizontalMargin, const uint32_t verticalMargin) const noexcept;

		/**
		 * 水平方向に交差しているか調べます
		 * @param[in]	other				調べる部屋
		 * @param[in]	horizontalMargin	水平方向のマージン
		 * @return		trueならば交差している
		 */
		bool HorizontalIntersect(const Room& other, const uint32_t horizontalMargin) const noexcept;

		/**
		点が含まれるか調べます
		@param[in]		point		調べる点
		*/
		bool Contain(const Point& point) const noexcept;

		/**
		点が含まれるか調べます
		@param[in]		x		調べる点
		@param[in]		y		調べる点
		@param[in]		z		調べる点
		*/
		bool Contain(const int32_t x, const int32_t y, const int32_t z) const noexcept;

		/**
		点が含まれるか調べます
		@param[in]		location	調べる点
		*/
		bool Contain(const FIntVector& location) const noexcept;

		/**
		部屋のパーツを取得します
		*/
		Parts GetParts() const noexcept;

		/**
		部屋のパーツを設定します
		*/
		void SetParts(const Parts parts) noexcept;

		/**
		部屋のアイテムを取得します
		*/
		Item GetItem() const noexcept;

		/**
		部屋のアイテムを設定します
		*/
		void SetItem(const Item item) noexcept;

		/**
		スタート位置からの深さを取得します
		@return		スタート位置からの深さ（部屋の数）
		*/
		uint8_t GetDepthFromStart() const noexcept;

		/**
		スタート位置からの深さを取得します
		@param[in]	depthFromStart		スタート位置からの深さ（部屋の数）
		*/
		void SetDepthFromStart(const uint8_t depthFromStart) noexcept;

		/*
		通路識別子を取得します
		*/
		uint8_t GetBranchId() const noexcept;

		/*
		通路識別子を設定します
		*/
		void SetBranchId(const uint8_t branchId) noexcept;

		/*
		有効な通路識別子か調べます
		*/
		bool IsValidBranchId() const noexcept;

		/*
		部屋に生成する門の数を取得します
		*/
		uint8_t GetGateCount() const noexcept;

		void ResetGateCount() noexcept;

		/*
		部屋に生成する門の数を加算します
		*/
		void AddGateCount(const uint8_t count) noexcept;



		uint8_t GetHorizontalRoomMargin() const noexcept;
		void SetHorizontalRoomMargin(const uint8_t horizontalRoomMargin) noexcept;




		/**
		 * 垂直方向の余白を取得します
		 * @return 垂直方向の余白
		 */
		uint8_t GetVerticalRoomMargin() const noexcept;

		/**
		 * 部屋と交差していたら余白を設定します
		 * @param otherRoom 比較する部屋
		 * @param horizontalRoomMargin	水平方向の余白
		 * @param verticalRoomMargin	垂直方向の余白
		 */
		void SetMarginIfRoomIntersect(const std::shared_ptr<Room>& otherRoom, const uint8_t horizontalRoomMargin, const uint8_t verticalRoomMargin) noexcept;

		/**
		 * 部屋の名前を取得します
		 * @return 部屋の名前
		 */
		std::string GetName() const noexcept;

		/**
		部屋のパーツの名称します
		*/
		const std::string_view& GetPartsName() const noexcept;

		/**
		部屋のアイテムの名称します
		*/
		const std::string_view& GetItemName() const noexcept;



		/*
		Get the size of the mesh generation prohibited area
		*/
		FIntVector GetDataSize() const noexcept;

		/*
		Set the size of the mesh generation prohibited area
		*/
		void SetDataSize(const uint32_t dataWidth, const uint32_t dataDepth, const uint32_t dataHeight);

		/*
		Get mesh generation prohibited area
		@param[out]		min		Minimum Position
		@param[out]		max		Maximum position
		*/
		void GetDataBounds(FIntVector& min, FIntVector& max) const noexcept;

		bool IsValidReservationNumber() const noexcept;
		uint32_t GetReservationNumber() const noexcept;
		void SetReservationNumber(const uint32_t reservationNumber) noexcept;
		void ResetReservationNumber() noexcept;


	private:
		int32_t mX;
		int32_t mY;
		int32_t mZ;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;

		uint32_t mDataWidth = 0;
		uint32_t mDataDepth = 0;
		uint32_t mDataHeight = 0;

		uint32_t mReservationNumber = 0;

		Identifier mIdentifier;

		Parts mParts = Parts::Unidentified;
		Item mItem = Item::Empty;
		uint8_t mDepthFromStart = std::numeric_limits<uint8_t>::max();
		uint8_t mBranchId = std::numeric_limits<uint8_t>::max();

		uint8_t mNumberOfGates = 0;
		uint8_t mHorizontalRoomMargin = 0;
		uint8_t mVerticalRoomMargin = 0;
	};
}

#include "Room.inl"
