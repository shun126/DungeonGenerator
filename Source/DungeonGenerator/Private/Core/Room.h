/**
部屋に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Identifier.h"
#include "Math/Math.h"
#include "Math/Point.h"

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
			Start,			//!< スタート地点
			Goal,			//!< ゴール地点
			Hall,			//!< 広間（通路が複数つながっている）
			Hanare,			//!< 離れ（通路が一つだけつながっている）
		};
		static constexpr uint8_t PartsSize = static_cast<uint8_t>(Parts::Hanare) + 1;

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
		コピーコンストラクタ
		*/
		Room(const Room& other) noexcept;

		/**
		ムーブコンストラクタ
		*/
		Room(Room&& other) noexcept;

		/**
		デストラクタ
		*/
		~Room() = default;

		/**
		識別子を取得
		\return		識別子
		*/
		const Identifier& GetIdentifier() const noexcept;

		/**
		X座標を取得
		\return		X座標
		*/
		int32_t GetX() const noexcept;

		/**
		Y座標を取得
		\return		Y座標
		*/
		int32_t GetY() const noexcept;

		/**
		Z座標を取得
		\return		Z座標
		*/
		int32_t GetZ() const noexcept;

		/**
		X座標を設定
		\param[in]	x	X座標
		*/
		void SetX(const int32_t x) noexcept;

		/**
		Y座標を設定
		\param[in]	y	Y座標
		*/
		void SetY(const int32_t y) noexcept;

		/**
		Z座標を設定
		\param[in]	z	Z座標
		*/
		void SetZ(const int32_t z) noexcept;

		/**
		幅を取得します
		\return		幅
		*/
		int32_t GetWidth() const noexcept;

		/**
		奥行きを取得します
		\return		奥行き
		*/
		int32_t GetDepth() const noexcept;

		/**
		高さを取得します
		\return		高さ
		*/
		int32_t GetHeight() const noexcept;

		/**
		床下の高さを取得します
		\return		床下の高さ
		*/
		uint16_t GetUnderfloorHeight() const noexcept;

		/**
		幅を設定します
		\param[in]	width	幅
		*/
		void SetWidth(const int32_t width) noexcept;

		/**
		奥行きを設定します
		\param[in]	depth	奥行き
		*/
		void SetDepth(const int32_t depth) noexcept;

		/**
		高さを設定します
		\param[in]	height	高さ
		*/
		void SetHeight(const int32_t height) noexcept;

		/**
		床下の高さを設定します
		\param[in]	underfloorHeight	床下の高さ
		*/
		void SetUnderfloorHeight(const uint16_t underfloorHeight) noexcept;

		/**
		矩形の左の座標を取得します
		\return		矩形の左の座標
		*/
		int32_t GetLeft() const noexcept;

		/**
		矩形の右の座標を取得します
		\return		矩形の右の座標（X + Width - 1）
		*/
		int32_t GetRight() const noexcept;

		/**
		矩形の上の座標を取得します
		\return		矩形の上の座標
		*/
		int32_t GetTop() const noexcept;

		/**
		矩形の下の座標を取得します
		\return		矩形の下の座標（Y + Depth - 1）
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
		\return		矩形の中心
		*/
		Point GetCenter() const noexcept;

		/**
		矩形の半分の辺を取得します
		\return		矩形の半分の辺
		*/
		FVector GetExtent() const noexcept;

		/**
		床の中心の座標を取得します
		\return		床の中心
		*/
		Point GetFloorCenter() const noexcept;

		/**
		地面の中心の座標を取得します
		\return		地面の中心
		*/
		Point GetGroundCenter() const noexcept;

		/**
		部屋が交差しているか調べます
		\param[in]	other	調べる部屋
		\return		trueならば交差している
		*/
		bool Intersect(const Room& other) const noexcept;

		/**
		部屋が交差しているか調べます
		\param[in]	other		調べる部屋
		\param[in]	margin		方向のマージン
		\return		trueならば交差している
		垂直方向のマージンは床方向に１、天井方向はかならず０で交差判定します。
		*/
		bool Intersect(const Room& other, const uint32_t margin) const noexcept;

		/**
		点が含まれるか調べます
		\param[in]		point		調べる点
		*/
		bool Contain(const Point& point) const noexcept;

		/**
		点が含まれるか調べます
		\param[in]		x		調べる点
		\param[in]		y		調べる点
		\param[in]		z		調べる点
		*/
		bool Contain(const int32_t x, const int32_t y, const int32_t z) const noexcept;

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
		\return		スタート位置からの深さ（部屋の数）
		*/
		uint8_t GetDepthFromStart() const noexcept;

		/**
		スタート位置からの深さを取得します
		\param[in]	depthFrcomStart		スタート位置からの深さ（部屋の数）
		*/
		void SetDepthFromStart(const uint8_t depthFrcomStart) noexcept;


		uint8_t GetBranchId() const noexcept;
		void SetBranchId(const uint8_t branchId) noexcept;


		std::string GetName() const noexcept;

		/**
		部屋のパーツの名称します
		*/
		const std::string_view& GetPartsName() const noexcept;

		/**
		部屋のアイテムの名称します
		*/
		const std::string_view& GetItemName() const noexcept;



	private:
		int32_t mX;
		int32_t mY;
		int32_t mZ;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;
		uint16_t mUnderfloorHeight = 0;

		Identifier mIdentifier;

		Parts mParts = Parts::Unidentified;
		Item mItem = Item::Empty;
		uint8_t mDepthFromStart = std::numeric_limits<uint8_t>::max();
		uint8_t mBranchId = std::numeric_limits<uint8_t>::max();
	};
}

#include "Room.inl"
