/**
ボクセルなどに利用するグリッド情報のヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Helper/Direction.h"
#include <Containers/UnrealString.h>
#include <Math/Color.h>

namespace dungeon
{
	/**
	グリッドクラス
	*/
	class Grid final
	{
	public:
		/**
		グリッド内のセルの種類
		*/
		enum class Type : uint8_t
		{
			Floor,			//!< 平らな地面（柱の生成判定なし）
			Deck,			//!< Floorの周辺または部屋の一階部分（柱の生成判定あり）
			Gate,			//!< 門
			Aisle,			//!< 通路
			Slope,			//!< 斜面
			Stairwell,		//!< 斜面の上側空白(吹き抜け）
			DownSpace,		//!< 斜面の下側空白
			UpSpace,		//!< 斜面の上側空白(吹き抜け）
			Empty,			//!< 空白
			OutOfBounds		//!< 範囲外（必ず最後に定義して下さい）
		};
		static constexpr size_t TypeSize = static_cast<size_t>(Type::OutOfBounds) + 1;

		/**
		グリッド内のセルにある小物
		*/
		enum class Props : uint8_t
		{
			None,			//!< 何も無い
			Lock,			//!< 鍵
			UniqueLock,		//!< ユニークな鍵
		};
		static constexpr size_t PropsSize = static_cast<size_t>(Props::UniqueLock) + 1;

	public:
		/**
		コンストラクタ
		*/
		Grid() noexcept;

		/**
		コンストラクタ
		@param[in]	type	グリッドの種類
		*/
		explicit Grid(const Type type) noexcept;

		/**
		コンストラクタ
		@param[in]	type		グリッドの種類
		@param[in]	direction	グリッドの方向
		*/
		Grid(const Type type, const Direction& direction) noexcept;

		/**
		コンストラクタ
		@param[in]	type		グリッドの種類
		@param[in]	direction	グリッドの方向
		@param[in]	identifier	識別子
		*/
		Grid(const Type type, const Direction& direction, const uint16_t identifier) noexcept;

		/**
		デストラクタ
		*/
		~Grid() = default;

	public:
		/**
		グリッドの方向を取得します
		*/
		Direction GetDirection() const noexcept;

		/**
		グリッドの方向を設定します
		*/
		void SetDirection(const Direction direction) noexcept;

		/**
		識別子を取得します
		*/
		uint16_t GetIdentifier() const noexcept;

		/**
		識別子を設定します
		*/
		void SetIdentifier(const uint16_t identifier) noexcept;

		/**
		無効な識別子か判定します？
		*/
		bool IsInvalidIdentifier() const noexcept;

		/**
		小道具を取得します
		*/
		Props GetProps() const noexcept;

		/**
		小道具を設定します
		*/
		void SetProps(const Props props) noexcept;

	public:
		/**
		グリッドの種類を取得します
		*/
		Type GetType() const noexcept;

		/**
		グリッドの種類を設定します
		*/
		void SetType(const Type type) noexcept;

		/**
		グリッドのタイプを判定します
		*/
		bool Is(const Type type) const noexcept;

		/**
		部屋系のグリッド？
		@warning	門は部屋系のグリッドでもあります
		@return		trueならば部屋系のグリッド
		*/
		bool IsKindOfRoomType() const noexcept;

		/**
		門以外の部屋系のグリッド？
		@warning	門は部屋系のグリッドでもあります
		@return		trueならば門以外の部屋系のグリッド
		*/
		bool IsKindOfRoomTypeWithoutGate() const noexcept;

		/**
		門系のグリッド？
		@warning	門は部屋系のグリッドでもあります
		@return		trueならば門系のグリッド
		*/
		bool IsKindOfGateType() const noexcept;

		/**
		通路系のグリッド？
		@return		trueならば通路系のグリッド
		*/
		bool IsKindOfAisleType() const noexcept;

		/**
		斜面系のグリッド？
		@return		trueならば斜面系のグリッド
		*/
		bool IsKindOfSlopeType() const noexcept;

		/**
		空間系のグリッド？
		@return		trueならば空間系のグリッド
		*/
		bool IsKindOfSpatialType() const noexcept;

		/**
		水平方向に通行可能なセルか判定します
		*/
		bool IsHorizontallyPassable() const noexcept;

		/**
		床（部屋）グリッドを生成します
		*/
		static Grid CreateFloor(const std::shared_ptr<Random>& random, const uint16_t identifier) noexcept;

		/**
		デッキ（部屋の周辺）グリッドを生成します
		*/
		static Grid CreateDeck(const std::shared_ptr<Random>& random, const uint16_t identifier) noexcept;

		// 判定補助関数
		/**
		自身からtoGridを見た時に床が生成されるか判定します
		@param[in]	checkNoMeshGeneration	メッシュ生成禁止判定
		@return		trueならば床の生成が可能
		*/
		bool CanBuildFloor(const bool checkNoMeshGeneration) const noexcept;

		/**
		斜面が生成されるか判定します
		@return		trueならば斜面の生成が可能
		*/
		bool CanBuildSlope() const noexcept;

		/**
		自身からtoGridを見た時に屋根が生成されるか判定します
		@param[in]	toUpperGrid				参照先グリッド（通常は一つ上のグリッド）
		@param[in]	checkNoMeshGeneration	メッシュ生成禁止判定
		@return		trueならば屋根の生成が可能
		*/
		bool CanBuildRoof(const Grid& toUpperGrid, const bool checkNoMeshGeneration) const noexcept;

		/**
		自身からtoGridを見た時に扉が生成されるか判定します
		@param[in]	toGrid		参照先グリッド
		@param[in]	direction	自身からtoGridの方向
		@param[in]	mergeRooms	trueならば部屋を結合する
		@return		trueならば扉の生成が可能
		*/
		bool CanBuildGate(const Grid& toGrid, const Direction::Index direction, const bool mergeRooms) const noexcept;

		/**
		自身からtoGridを見た時に壁が生成されるか判定します
		@param[in]	toGrid		参照先グリッド
		@param[in]	direction	自身からtoGridの方向
		@param[in]	mergeRooms	部屋と部屋を結合する
		@return		trueならば壁の生成が可能
		*/
		bool CanBuildWall(const Grid& toGrid, const Direction::Index direction, const bool mergeRooms) const noexcept;

	public:
		/**
		天井のメッシュ生成禁止に設定します
		@param[in]	noRoofMeshGeneration	天井のメッシュ生成禁止
		*/
		void NoRoofMeshGeneration(const bool noRoofMeshGeneration) noexcept;

		/**
		床のメッシュ生成禁止に設定します
		@param[in]	noFloorMeshGeneration	床のメッシュ生成禁止
		*/
		void NoFloorMeshGeneration(const bool noFloorMeshGeneration) noexcept;

		/**
		床のメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoFloorMeshGeneration() const noexcept;

		/**
		天井のメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoRoofMeshGeneration() const noexcept;

		/*
		北側の壁がメッシュ生成禁止か設定します
		*/
		void NoNorthWallMeshGeneration(const bool noWallMeshGeneration) noexcept;

		/*
		南側の壁がメッシュ生成禁止か設定します
		*/
		void NoSouthWallMeshGeneration(const bool noWallMeshGeneration) noexcept;

		/*
		東側の壁がメッシュ生成禁止か設定します
		*/
		void NoEastWallMeshGeneration(const bool noWallMeshGeneration) noexcept;

		/*
		西側の壁がメッシュ生成禁止か設定します
		*/
		void NoWestWallMeshGeneration(const bool noWallMeshGeneration) noexcept;

		/*
		いずれかの方向で壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoWallMeshGeneration() const noexcept;

		/*
		指定の方向で壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoWallMeshGeneration(const Direction direction) const noexcept;

		/*
		指定の方向で壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoWallMeshGeneration(const Direction::Index direction) const noexcept;

		/*
		北側の壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoNorthWallMeshGeneration() const noexcept;

		/*
		南側の壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoSouthWallMeshGeneration() const noexcept;

		/*
		東側の壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoEastWallMeshGeneration() const noexcept;

		/*
		西側の壁がメッシュ生成禁止か取得します
		@return		trueならメッシュ生成禁止
		*/
		bool IsNoWestWallMeshGeneration() const noexcept;

		/**
		通路のマージ許可を設定します
		*/
		void MergeAisle(const bool enable) noexcept;

		/**
		通路をマージ可能か取得します
		@return		trueならマージ可能
		*/
		bool CanMergeAisle() const noexcept;

		/**
		 * 中二階通路か設定します
		 */
		void Catwalk(const bool enable) noexcept;

		/**
		 * 中二階通路か取得します
		 */
		bool IsCatwalk() const noexcept;

	public:
		/**
		グリッドの種類の色を取得します
		*/
		const FColor& GetTypeColor() const noexcept;

		/**
		グリッドの種類の色を取得します
		*/
		static const FColor& GetTypeColor(const Grid::Type gridType) noexcept;

		/**
		グリッドの種類の名前を取得します
		*/
		const FString& GetTypeName() const noexcept;

		/**
		グリッドの小道具の名前を取得します
		*/
		const FString& GetPropsName() const noexcept;

		/**
		グリッドの生成禁止状態の名前を取得します
		*/
		FString GetNoMeshGenerationName() const noexcept;

	private:
		static bool CanBuildWall_SlopeVsRoom() noexcept;
		static bool CanBuildWall_SlopeVsGate(const Grid& toGrid, const Direction::Index direction) noexcept;
		bool CanBuildWall_SlopeVsAisle(const Grid& toGrid, const Direction::Index direction) const noexcept;
		bool CanBuildWall_SlopeVsSlope(const Grid& toGrid, const Direction::Index direction) const noexcept;

	private:
		// 生成属性
		enum class Attribute : uint8_t
		{
			NoNorthWallMeshGeneration,	// 方角はDirection::Indexの並びと同じにする事
			NoEastWallMeshGeneration,
			NoSouthWallMeshGeneration,
			NoWestWallMeshGeneration,
			NoFloorMeshGeneration,
			NoRoofMeshGeneration,
			MergeAisle,
			Catwalk,
		};
		static constexpr size_t AttributeSize = static_cast<size_t>(Attribute::MergeAisle) + 1;

		// Directionクラスの方向数
		static constexpr size_t DirectionSize = 4;

		// 属性をまとめたクラス
		class Pack final
		{
		public:
			using ValueType = uint32_t;

		public:
			Pack();
			~Pack() = default;

			Direction GetDirection() const noexcept;
			void SetDirection(const Direction direction) noexcept;

			Type GetType() const noexcept;
			void SetType(const Type type) noexcept;

			Props GetProps() const noexcept;
			void SetProps(const Props props) noexcept;

			bool IsAttributeEnabled(const Attribute attribute) const noexcept;
			void SetAttribute(const Attribute attribute, const bool enable) noexcept;

		private:
			constexpr static ValueType BitCount(ValueType value) noexcept
			{
				ValueType count = 0;
				while (value > 0)
				{
					value >>= 1;
					++count;
				}
				return count;
			}

			constexpr static ValueType Mask(const ValueType value, const ValueType shift) noexcept
			{
				ValueType mask = 1;
				for (ValueType i = 1; i < value; ++i)
				{
					mask <<= 1;
					mask |= 1;
				}
				for (ValueType i = 0; i < shift; ++i)
				{
					mask <<= 1;
				}
				return mask;
			}

			template<typename T>
			static constexpr ValueType MaskBit(const T shift) noexcept
			{
				return 1 << static_cast<ValueType>(shift);
			}
			template<typename T>
			void Set(const T shift, const bool set) noexcept
			{
				if (set)
					mBitSet |= MaskBit(shift);
				else
					mBitSet &= ~MaskBit(shift);
			}
			template<typename T>
			constexpr bool Test(const T shift) const noexcept
			{
				return mBitSet & MaskBit(shift);
			}
			template<typename T>
			constexpr T GetValue(const ValueType shift, const ValueType mask) const noexcept
			{
				return static_cast<T>((mBitSet & mask) >> shift);
			}
			template<typename T>
			void SetValue(const ValueType shift, const ValueType mask, const T value) noexcept
			{
				const ValueType shiftValue = static_cast<ValueType>(value) << shift;
				mBitSet &= mask;
				mBitSet |= shiftValue;
			}

		private:
			ValueType mBitSet = 0;
		};
		Pack mPack;

		static constexpr uint16_t InvalidIdentifier = static_cast<uint16_t>(~0);
		uint16_t mIdentifier = InvalidIdentifier;

		uint16_t mPadding = 0;
	};
	static_assert(sizeof(Grid) == 8);
}

#include "Grid.inl"
