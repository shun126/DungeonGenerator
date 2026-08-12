/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonLayoutTypes.h"
#include "PathGeneration/StartLocationPolicy.h"
#include <Math/IntVector.h>
#include <memory>

namespace dungeon
{
	class Random;

	/**
	 * Dungeon expansion policy
	 *
	 * ダンジョンの拡張ポリシー
	 * DungeonExpansionPolicyと同じ意味にして下さい
	 */
	enum class ExpansionPolicy : uint8_t
	{
		Flat,				// 平面方向にのみ広がる
		ExpandVertically,   // 垂直方向に広がる
		ExpandAnyDirection, // 制限なく広がる
	};

	/**
	 * 通路の天井高ポリシー
	 * EDungeonAisleCeilingHeightPolicyと同じ意味にして下さい
	 */
	enum class AisleCeilingHeightPolicy : uint8_t
	{
		TwoGrids,
		OneGrid,
		Random,
	};

	/**
	 * Generates Parameter.
	 * デフォルトダンジョン生成パラメータクラス
	 *
	 * This structure is copied for every generation, so keep it small.
	 * Members are laid out so that small types fill the padding in front of the aligned ones.
	 * mGeneratedRandomSeed for instance is free because it fits in the padding before mRandom.
	 * When adding a member, put it where the padding already is and confirm sizeof does not grow.
	 *
	 * この構造体は生成の度にコピーされるため、小さく保って下さい。
	 * 小さい型はアライメントを持つメンバの手前の詰め物を埋めるように並べています。
	 * 例えばmGeneratedRandomSeedはmRandomの手前の詰め物に収まるため、実質サイズが増えません。
	 * メンバを追加する時は詰め物のある位置へ置き、sizeofが増えない事を確認して下さい。
	 */
	struct GenerateParameter final
	{
	public:
		/**
		 * Generates Parameter.
		 * コンストラクタ
		 */
		GenerateParameter();

		/**
		 * Destroys the ~GenerateParameter instance.
		 * デストラクタ
		 */
		~GenerateParameter() = default;

		/**
		 * Returns ExpansionPolicy.
		 * ダンジョンの拡張ポリシーを取得します
		 */
		ExpansionPolicy GetExpansionPolicy() const noexcept;

		/**
		 * Sets ExpansionPolicy.
		 * ダンジョンの拡張ポリシーを設定します
		 */
		void SetExpansionPolicy(const ExpansionPolicy policy) noexcept;

		/**
		 * Returns StartLocationPolicy.
		 * スタート位置のポリシーを取得します
		 */
		StartLocationPolicy GetStartLocationPolicy() const noexcept;

		/**
		 * Sets StartLocationPolicy.
		 * スタート位置のポリシーを設定します
		 */
		void SetStartLocationPolicy(const StartLocationPolicy startLocationPolicy) noexcept;

		/**
		 * Returns StartRoomCount.
		 * スタート部屋の数を取得します
		 */
		uint8_t GetStartRoomCount() const noexcept;

		/**
		 * Sets StartRoomCount.
		 * スタート部屋の数を設定します
		 */
		void SetStartRoomCount(const uint8_t count) noexcept;

		/**
		 * Returns the spatial policy used to select the generated goal room.
		 * 生成されるゴール部屋を選択する空間ポリシーを返します。
		 */
		EDungeonGoalLocationPolicy GetGoalLocationPolicy() const noexcept;

		/**
		 * Sets the spatial policy used to select the generated goal room.
		 * 生成されるゴール部屋を選択する空間ポリシーを設定します。
		 */
		void SetGoalLocationPolicy(EDungeonGoalLocationPolicy goalLocationPolicy) noexcept;

		/**
		 * Returns NumberOfCandidateRooms.
		 * 生成する部屋の数の候補
		 * 部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		 */
		uint8_t GetNumberOfCandidateRooms() const noexcept;
		void SetNumberOfCandidateRooms(const uint8_t count) noexcept;

		/**
		 * Selects one target room count from the inclusive range for this generation.
		 * A fixed range does not consume random state.
		 * この生成で使用する目標部屋数を、両端を含む範囲から1回選択します。
		 * 固定範囲では乱数状態を消費しません。
		 */
		void SetNumberOfCandidateRooms(const FInt32Interval& range) noexcept;

		/**
		 * Returns MinRoomWidth.
		 * 部屋の最小の幅
		 */
		uint32_t GetMinRoomWidth() const noexcept;
		void SetMinRoomWidth(const uint32_t width) noexcept;

		/**
		 * Returns MaxRoomWidth.
		 * 部屋の最大の幅
		 */
		uint32_t GetMaxRoomWidth() const noexcept;
		void SetMaxRoomWidth(const uint32_t width) noexcept;

		/**
		 * Returns MinRoomDepth.
		 * 部屋の最小の奥行き
		 */
		uint32_t GetMinRoomDepth() const noexcept;
		void SetMinRoomDepth(const uint32_t depth) noexcept;

		/**
		 * Returns MaxRoomDepth.
		 * 部屋の最大の奥行き
		 */
		uint32_t GetMaxRoomDepth() const noexcept;
		void SetMaxRoomDepth(const uint32_t depth) noexcept;

		/**
		 * Returns MinRoomHeight.
		 * 部屋の最小の高さ
		 */
		uint32_t GetMinRoomHeight() const noexcept;
		void SetMinRoomHeight(const uint32_t height) noexcept;

		/**
		 * Returns MaxRoomHeight.
		 * 部屋の最大の高さ
		 */
		uint32_t GetMaxRoomHeight() const noexcept;
		void SetMaxRoomHeight(const uint32_t height) noexcept;

		/**
		 * Returns HorizontalRoomMargin.
		 * 部屋と部屋の水平方向の余白
		 */
		uint32_t GetHorizontalRoomMargin() const noexcept;
		void SetHorizontalRoomMargin(const uint32_t margin) noexcept;

		/**
		 * Returns VerticalRoomMargin.
		 * 部屋と部屋の垂直方向の余白
		 */
		uint32_t GetVerticalRoomMargin() const noexcept;
		void SetVerticalRoomMargin(const uint32_t margin) noexcept;

		/**
		 * Returns Random.
		 * 乱数発生
		 */
		std::shared_ptr<Random> GetRandom() noexcept;

		/**
		 * Returns Random.
		 * 乱数発生
		 */
		std::shared_ptr<Random> GetRandom() const noexcept;

		/**
		 * Returns the seed handed to the random number generator.
		 * 乱数生成器へ渡した乱数の種を取得します
		 */
		uint32_t GetGeneratedRandomSeed() const noexcept;

		/**
		 * Seeds the random number generator and records the seed.
		 * 乱数生成器へ種を設定し、その種を記録します
		 */
		void SetGeneratedRandomSeed(const uint32_t seed) noexcept;




		/**
		 * Returns Width.
		 * ダンジョンの幅
		 */
		uint32_t GetWidth() const noexcept;

		/**
		 * Sets Width.
		 * ダンジョンの幅
		 */
		void SetWidth(const uint32_t width) noexcept;

		/**
		 * Returns Depth.
		 * ダンジョンの奥行き
		 */
		uint32_t GetDepth() const noexcept;

		/**
		 * Sets Depth.
		 * ダンジョンの奥行き
		 */
		void SetDepth(const uint32_t depth) noexcept;

		/**
		 * Returns Height.
		 * ダンジョンの高さ
		 */
		uint32_t GetHeight() const noexcept;

		/**
		 * Sets Height.
		 * ダンジョンの高さ
		 */
		void SetHeight(const uint32_t height) noexcept;

		bool IsGenerateStartRoomReserved() const noexcept;
		bool IsGenerateGoalRoomReserved() const noexcept;

		bool UseMissionGraph() const noexcept;
		void SetMissionGraph(const bool use) noexcept;

		const FDungeonPathSettings& GetPathSettings() const noexcept;
		void SetPathSettings(const FDungeonPathSettings& settings) noexcept;
		const FDungeonRoomRoleSettings& GetRoomRoleSettings() const noexcept;
		void SetRoomRoleSettings(const FDungeonRoomRoleSettings& settings) noexcept;
		const FDungeonZoneSettings& GetZoneSettings() const noexcept;
		void SetZoneSettings(const FDungeonZoneSettings& settings) noexcept;
		int32 GetLayoutCandidateCount() const noexcept;
		void SetLayoutCandidateCount(const int32 candidateCount) noexcept;

		/**
		 * Gets additional corridor complexity, or 0 while Keys And Locks progression is active.
		 * AisleComplexity を返します。
		 */
		uint8_t GetAisleComplexity() const noexcept;
		void SetAisleComplexity(const uint8_t complexity) noexcept;
		bool IsAisleComplexity() const noexcept;

		AisleCeilingHeightPolicy GetAisleCeilingHeightPolicy() const noexcept;
		void SetAisleCeilingHeightPolicy(const AisleCeilingHeightPolicy policy) noexcept;

		/**
		 * Returns whether GenerateSlopeInRoom.
		 * 内部ボクセル生成で室内スロープを生成できるかを返します。
		 */
		bool IsGenerateSlopeInRoom() const noexcept;
		void SetGenerateSlopeInRoom(const bool generateSlopeInRoom) noexcept;

		/**
		 * Returns whether GenerateStructuralColumn.
		 * 内部ボクセル生成で構造柱を生成できるかを返します。
		 */
		bool IsGenerateStructuralColumn() const noexcept;
		void SetGenerateStructuralColumn(const bool generateStructuralColumn) noexcept;
		uint8_t GetSkylightChancePercent() const noexcept;
		void SetSkylightChancePercent(const uint8_t skylightChancePercent) noexcept;

		/**
		 * Returns StartRoomSize.
		スタート部屋のサイズ
		*/
		const FIntVector& GetStartRoomSize() const noexcept;

		/**
		 * Sets StartRoomSize.
		スタート部屋のサイズ
		*/
		void SetStartRoomSize(const FIntVector& size) noexcept;

		/**
		 * Returns GoalRoomSize.
		ゴール部屋のサイズ
		*/
		const FIntVector& GetGoalRoomSize() const noexcept;

		/**
		 * Sets GoalRoomSize.
		ゴール部屋のサイズ
		*/
		void SetGoalRoomSize(const FIntVector& size) noexcept;

		/**
		 * 開始部屋へ門を生成できるグリッドの数を取得します
		 * 0ならばサイズが固定されていないか、数が不明であることを表します
		 */
		uint8_t GetStartRoomGateCapacity() const noexcept;

		/**
		 * 開始部屋へ門を生成できるグリッドの数を設定します
		 */
		void SetStartRoomGateCapacity(const uint8_t capacity) noexcept;

		/**
		 * ゴール部屋へ門を生成できるグリッドの数を取得します
		 * 0ならばサイズが固定されていないか、数が不明であることを表します
		 */
		uint8_t GetGoalRoomGateCapacity() const noexcept;

		/**
		 * ゴール部屋へ門を生成できるグリッドの数を設定します
		 */
		void SetGoalRoomGateCapacity(const uint8_t capacity) noexcept;

	private:
		/**
		 * Represents Width.
		 * ダンジョンの幅
		 */
		uint32_t mWidth = 0;

		/**
		 * Represents Depth.
		 * ダンジョンの奥行き
		 */
		uint32_t mDepth = 0;

		/**
		 * Represents Height.
		 * ダンジョンの高さ
		 */
		uint32_t mHeight = 0;

		/**
		 * Route shape, start and goal rooms, and progression gates.
		 * Layout profile settings used by the intent-driven generator.
		 * 経路形状、開始部屋、ゴール部屋、進行ゲートの設定です。
		 * 意図グラフ型ジェネレーターで使うレイアウトプロファイル設定です。
		 */
		FDungeonPathSettings mPathSettings;

		/**
		 * Room roles, role visuals, and special room selection.
		 * This does not control the shape or the number of rooms.
		 * 部屋の役割、役割ごとの見た目、特殊部屋の選択に使う設定です。
		 * 部屋の形状や部屋数は制御しません。
		 */
		FDungeonRoomRoleSettings mRoomRoleSettings;

		/**
		 * Zones that define biome-like ranges across the dungeon.
		 * ダンジョン内のバイオーム的な範囲を定義するゾーンの設定です。
		 */
		FDungeonZoneSettings mZoneSettings;

		/**
		 * Directions the dungeon is allowed to spread in.
		 * ダンジョンが広がる事を許す方向です。
		 */
		ExpansionPolicy mDungeonExpansionPolicy = ExpansionPolicy::ExpandAnyDirection;

		/**
		 * Criterion used to pick the start room out of the generated rooms.
		 * 生成した部屋の中から開始部屋を選ぶ基準です。
		 */
		StartLocationPolicy mStartLocationPolicy = StartLocationPolicy::UseSouthernMost;

		/**
		 * Number of start rooms. Only read when mStartLocationPolicy is UseMultiStart.
		 * 開始部屋の数です。mStartLocationPolicyがUseMultiStartの時だけ参照されます。
		 */
		uint8_t mStartRoomCount = 1;

		/**
		 * Spatial policy forwarded from Path.GoalRoomPolicy.
		 * Path.GoalRoomPolicyから転送されるゴール部屋の空間ポリシーです。
		 */
		EDungeonGoalLocationPolicy mGoalLocationPolicy = EDungeonGoalLocationPolicy::UseNorthernMost;

		/**
		 * Represents NumberOfCandidateRooms.
		 * 生成する部屋の数の候補
		 * 部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		 */
		uint8_t mNumberOfCandidateRooms = 1;

		/**
		 * Compatibility flag mirrored from ProgressionPolicy == KeysAndLocks.
		 * ProgressionPolicy == KeysAndLocksから同期される互換フラグです。
		 */
		bool mUseMissionGraph = true;

		/**
		 * Number of layout candidates evaluated before committing a dungeon.
		 * ダンジョン確定前に評価するレイアウト候補数です。
		 */
		int8 mLayoutCandidateCount = 3;

		/**
		 * Additional corridor complexity before progression-specific filtering.
		 * 進行方針によるフィルタ前の追加通路複雑度です。
		 */
		uint8_t mAisleComplexity = 0;

		/**
		 * How the ceiling height of an aisle is decided.
		 * 通路の天井の高さを決める方法です。
		 */
		AisleCeilingHeightPolicy mAisleCeilingHeightPolicy = AisleCeilingHeightPolicy::Random;

		/**
		 * Internal option currently set by DungeonGenerateBase during parameter transfer.
		 * 現在はDungeonGenerateBaseからのパラメータ転送時に設定される内部オプションです。
		 */
		bool mGenerateSlopeInRoom = true;

		/**
		 * Internal option currently set by DungeonGenerateBase during parameter transfer.
		 * 現在はDungeonGenerateBaseからのパラメータ転送時に設定される内部オプションです。
		 */
		bool mGenerateStructuralColumn = true;

		/**
		 * Internal skylight chance currently set by DungeonGenerateBase during parameter transfer.
		 * 現在はDungeonGenerateBaseからのパラメータ転送時に設定される内部の天窓生成率です。
		 */
		uint8_t mSkylightChancePercent = 8;

		/**
		 * Represents MinRoomWidth.
		 * 部屋の最小の幅
		 */
		uint32_t mMinRoomWidth = 0;

		/**
		 * Represents MaxRoomWidth.
		 * 部屋の最大の幅
		 */
		uint32_t mMaxRoomWidth = 0;

		/**
		 * Represents MinRoomDepth.
		 * 部屋の最小の奥行き
		 */
		uint32_t mMinRoomDepth = 0;

		/**
		 * Represents MaxRoomDepth.
		 * 部屋の最大の奥行き
		 */
		uint32_t mMaxRoomDepth = 0;

		/**
		 * Represents MinRoomHeight.
		 * 部屋の最小の高さ
		 */
		uint32_t mMinRoomHeight = 0;

		/**
		 * Represents MaxRoomHeight.
		 * 部屋の最大の高さ
		 */
		uint32_t mMaxRoomHeight = 0;

		/**
		 * Represents HorizontalRoomMargin.
		 * 部屋と部屋の水平方向の余白
		 */
		uint32_t mHorizontalRoomMargin = 0;

		/**
		 * Represents VerticalRoomMargin.
		 * 部屋と部屋の垂直方向の余白
		 */
		uint32_t mVerticalRoomMargin = 0;

		/**
		 * Seed handed to mRandom at the start of the generation.
		 * mRandom does not keep it, so it is recorded here to identify the generated dungeon.
		 * Placed in front of mRandom to fit in the alignment padding.
		 * 生成の開始時にmRandomへ渡した乱数の種です。
		 * mRandomは種を保持しないため、生成したダンジョンを特定できるようここに記録します。
		 * アライメントの詰め物に収まるようmRandomの手前に置いています。
		 */
		uint32_t mGeneratedRandomSeed = 0;

		/**
		 * Represents Random.
		 * 乱数生成器
		 */
		std::shared_ptr<Random> mRandom;

		/**
		 * Represents StartRoomSize.
		 * スタート部屋のサイズ
		 */
		FIntVector mStartRoomSize = { 0, 0, 0 };

		/**
		 * Represents GoalRoomSize.
		 * ゴール部屋のサイズ
		 */
		FIntVector mGoalRoomSize = { 0, 0, 0 };

		/**
		 * Number of grids where a gate can be placed in the start room.
		 * 開始部屋へ門を生成できるグリッドの数です。0は未設定を表します。
		 */
		uint8_t mStartRoomGateCapacity = 0;

		/**
		 * Number of grids where a gate can be placed in the goal room.
		 * ゴール部屋へ門を生成できるグリッドの数です。0は未設定を表します。
		 */
		uint8_t mGoalRoomGateCapacity = 0;
	};
}

#include "GenerateParameter.inl"
