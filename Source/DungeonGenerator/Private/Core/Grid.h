/**
ボクセルなどに利用するグリッド情報のヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Direction.h"

namespace dungeon
{
	/**
	@addtogroup PathGeneration
	@{
	*/
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
			Floor,			//!< 平らな地面（壁の生成判定なし）
			Deck,			//!< Floorの周辺または部屋の一階部分（壁の生成判定あり）
			Gate,			//!< 門
			Aisle,			//!< 通路
			Slope,			//!< 斜面
			Atrium,			//!< 吹き抜け（斜面の空白）
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
		\param[in]	type	グリッドの種類
		*/
		explicit Grid(const Type type) noexcept;

		/**
		コンストラクタ
		\param[in]	type		グリッドの種類
		\param[in]	direction	グリッドの方向
		*/
		Grid(const Type type, const Direction direction) noexcept;

		/**
		コンストラクタ
		\param[in]	type		グリッドの種類
		\param[in]	direction	グリッドの方向
		\param[in]	identifier	識別子
		*/
		Grid(const Type type, const Direction direction, const uint16_t identifier) noexcept;

		/**
		デストラクタ
		*/
		~Grid() = default;

		/**
		グリッドの種類を取得します
		*/
		Type GetType() const noexcept;

		/**
		グリッドの種類を設定します
		*/
		void SetType(const Type type) noexcept;

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

		/**
		部屋系のグリッド？
		\warning	門は部屋系のグリッドでもあります
		\return		trueならば部屋系のグリッド
		*/
		bool IsKindOfRoomType() const noexcept;

		/**
		門以外の部屋系のグリッド？
		\warning	門は部屋系のグリッドでもあります
		\return		trueならば門以外の部屋系のグリッド
		*/
		bool IsKindOfRoomTypeWithoutGate() const noexcept;

		/**
		門系のグリッド？
		\warning	門は部屋系のグリッドでもあります
		\return		trueならば門系のグリッド
		*/
		bool IsKindOfGateType() const noexcept;

		/**
		通路系のグリッド？
		\return		trueならば通路系のグリッド
		*/
		bool IsKindOfAisleType() const noexcept;

		/**
		斜面系のグリッド？
		\return		trueならば斜面系のグリッド
		*/
		bool IsKindOfSlopeType() const noexcept;

		/**
		空間系のグリッド？
		\return		trueならば空間系のグリッド
		*/
		bool IsKindOfSpatialType() const noexcept;

		/**
		水平方向に通行可能なセルか判定します
		*/
		bool IsHorizontallyPassable() const noexcept;

		/**
		水平方向に通行不可能なセルか判定します
		*/
		bool IsHorizontallyNotPassable() const noexcept;

		/**
		垂直方向に通行可能なセルか判定します
		*/
		bool IsVerticallyPassable() const noexcept;

		/**
		垂直方向に通行不可能なセルか判定します
		*/
		bool IsVerticallyNotPassable() const noexcept;

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
		\param[in]	toGrid					参照先グリッド（通常は一つ下のグリッド）
		\param[in]	checkNoMeshGeneration	メッシュ生成禁止判定
		\return		trueならば床の生成が可能
		*/
		bool CanBuildFloor(const Grid& toGrid, const bool checkNoMeshGeneration) const noexcept;

		/**
		斜面が生成されるか判定します
		\return		trueならば斜面の生成が可能
		*/
		bool CanBuildSlope() const noexcept;

		/**
		自身からtoGridを見た時に屋根が生成されるか判定します
		\param[in]	toGrid		参照先グリッド（通常は一つ上のグリッド）
		\param[in]	checkNoMeshGeneration	メッシュ生成禁止判定
		\return		trueならば屋根の生成が可能
		*/
		bool CanBuildRoof(const Grid& toGrid, const bool checkNoMeshGeneration) const noexcept;

		/**
		自身からtoGridを見た時に壁が生成されるか判定します
		\param[in]	toGrid		参照先グリッド
		\param[in]	direction	自身からtoGridの方向
		\param[in]	mergeRooms	部屋と部屋を結合する
		\return		trueならば壁の生成が可能
		*/
		bool CanBuildWall(const Grid& toGrid, const Grid& underGrid, const Direction::Index direction, const bool mergeRooms) const noexcept;

		/**
		自身からtoGridを見た時に壁が生成されるか判定します
		\param[in]	toGrid		参照先グリッド
		\param[in]	direction	自身からtoGridの方向
		\param[in]	mergeRooms	部屋と部屋を結合する
		\return		trueならば壁の生成が可能
		*/
		bool CanBuildWallForMinimap(const Grid& toGrid, const Direction::Index direction, const bool mergeRooms) const noexcept;

		/**
		自身からtoGridを見た時に柱が生成されるか判定します
		\param[in]	toGrid		参照先グリッド
		\param[in]	direction	自身からtoGridの方向
		\return		trueならば柱の生成が可能
		*/
		bool CanBuildPillar(const Grid& toGrid) const noexcept;

		/**
		自身からtoGridを見た時に扉が生成されるか判定します
		\param[in]	toGrid		参照先グリッド
		\param[in]	direction	自身からtoGridの方向
		\return		trueならば扉の生成が可能
		*/
		bool CanBuildGate(const Grid& toGrid, const Direction::Index direction) const noexcept;

		/**
		メッシュ生成禁止に設定します
		\param[in]	noRoofMeshGeneration	天井のメッシュ生成禁止
		\param[in]	noFloorMeshGeneration	床のメッシュ生成禁止
		*/
		void SetNoMeshGeneration(const bool noRoofMeshGeneration, const bool noFloorMeshGeneration);
		
		/**
		床のメッシュ生成禁止か取得します
		\return		trueならメッシュ生成禁止
		*/
		bool IsNoFloorMeshGeneration() const noexcept;

		/**
		天井のメッシュ生成禁止か取得します
		\return		trueならメッシュ生成禁止
		*/
		bool IsNoRoofMeshGeneration() const noexcept;




		/**
		グリッドの種類の色を取得します
		*/
		const FColor& GetTypeColor() const noexcept;

		const FString& GetTypeName() const noexcept;
		const FString& GetPropsName() const noexcept;
		FString GetNoMeshGenerationName() const noexcept;

	private:
		Type mType;
		Props mProps;
		Direction mDirection;

		enum class NoMeshGeneration : uint8_t
		{
			Floor,
			Roof
		};
		static constexpr uint8_t NoMeshGenerationRoofMask = 1 << static_cast<uint8_t>(NoMeshGeneration::Roof);
		static constexpr uint8_t NoMeshGenerationFloorMask = 1 << static_cast<uint8_t>(NoMeshGeneration::Floor);
		uint8_t mNoMeshGeneration = 0;

		static constexpr uint16_t InvalidIdentifier = static_cast<uint16_t>(~0);
		uint16_t mIdentifier = InvalidIdentifier;
	};
	/**
	@}
	*/
}

#include "Grid.inl"
