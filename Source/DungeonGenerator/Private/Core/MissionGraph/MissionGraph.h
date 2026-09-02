/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Builds solvable key-and-door progression from the bridges (cut edges) that separate the start room from the goal room.
 * スタート部屋とゴール部屋を分断する橋（カットエッジ）を元に、攻略可能な鍵と扉の進行を構築します。
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dungeon
{
	class Aisle;
	class Generator;
	class Point;
	class Room;

	/**
	 * Builds and validates mission progression information for the generated dungeon.
	 * Every lock is placed on a bridge of the start-to-goal route, so a detour can never bypass a locked door
	 * even when the layout contains loops.
	 * 生成されたダンジョンの攻略進行情報を構築して検証します。
	 * 全てのロックはスタートからゴールへの経路上の橋に配置するため、ループを含むレイアウトでも
	 * 迂回路によって鍵付き扉を回避される事はありません。
	 */
	class MissionGraph
	{
	public:
		/**
		 * Places common locks and the unique lock on the route bridges, then places their keys.
		 * The goal becomes reachable only after every common key has been used and the unique key has been found.
		 * 経路上の橋に通常ロックとユニークロックを配置し、対応する鍵を配置します。
		 * 全ての小さな鍵を使い切り、ユニーク鍵を入手して初めてゴールへ到達できるようになります。
		 * @param[in]	generator		The generator that owns the rooms and aisles / 部屋と通路を所有するジェネレータ
		 * @param[in]	startRoom		The room the player starts from / プレイヤーの開始部屋
		 * @param[in]	goalRoom		The room the player must reach / プレイヤーが到達すべき部屋
		 * @param[in]	maxKeyCount		Upper limit of common keys. Fewer keys are placed when the layout cannot hold them
		 *								/ 小さな鍵の上限数。レイアウトが許さない場合はより少ない数になります
		 */
		MissionGraph(
			const std::shared_ptr<Generator>& generator,
			const std::shared_ptr<Room>& startRoom,
			const std::shared_ptr<Room>& goalRoom,
			const uint8_t maxKeyCount) noexcept;

		/**
		 * Destroys the MissionGraph instance.
		 * デストラクタ
		 */
		virtual ~MissionGraph() = default;

		/**
		 * Returns whether locks and keys were actually placed.
		 * A layout that offers no room for the unique key leaves the dungeon completely unlocked,
		 * which is playable but carries no key-and-door progression.
		 * ロックと鍵を実際に配置したかを返します。
		 * ユニーク鍵を置ける部屋が無いレイアウトではロックが一つも無い状態になります。
		 * ダンジョンとしては遊べますが、鍵と扉の進行はありません。
		 * @return		true if at least one lock and its key were placed / ロックと鍵を配置したならtrue
		 */
		bool Placed() const noexcept;

	private:
		/**
		 * Sentinel used for "no room", "no aisle" and "unreachable".
		 * 「該当する部屋・通路が無い」「到達不能」を表す番兵値です。
		 */
		static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

		/**
		 * Upper limit of common locks. MissionGraphTester can only enumerate a bounded number of lock states,
		 * so more locks than this would be reported as unsolvable even if the layout itself is fine.
		 * 通常ロックの上限数です。MissionGraphTester が列挙できるロック状態には上限があるため、
		 * これを超えるとレイアウトが正しくても解けないと判定されてしまいます。
		 */
		static constexpr size_t MaxCommonLockCount = 16;

		/**
		 * Adjacency snapshot of the dungeon taken before any lock is placed.
		 * Indices into mRooms and mEdges stay valid for the whole lifetime of one MissionGraph construction.
		 * ロックを配置する前に取得したダンジョンの隣接情報です。
		 * mRooms と mEdges の添字は MissionGraph の構築が終わるまで有効です。
		 */
		struct RouteGraph final
		{
			/**
			 * An aisle expressed as an undirected edge between two room indices.
			 * 二つの部屋の添字を結ぶ無向辺として表現した通路です。
			 */
			struct Edge final
			{
				size_t mRoom0 = InvalidIndex;	//!< Room index of the first endpoint / 一方の端点の部屋添字
				size_t mRoom1 = InvalidIndex;	//!< Room index of the second endpoint / もう一方の端点の部屋添字
				Aisle* mAisle = nullptr;		//!< The aisle this edge was built from / この辺の元になった通路
			};

			std::vector<std::shared_ptr<Room>> mRooms;					//!< All rooms of the dungeon / ダンジョンの全ての部屋
			std::unordered_map<const Room*, size_t> mRoomIndices;		//!< Reverse lookup into mRooms / mRooms への逆引き
			std::vector<Edge> mEdges;									//!< All aisles of the dungeon / ダンジョンの全ての通路
			std::vector<std::vector<size_t>> mAdjacency;				//!< Room index to incident edge indices / 部屋添字から接続する辺添字へ

			/**
			 * Returns the index of the given room, or InvalidIndex when it is unknown.
			 * 指定した部屋の添字を返します。未知の部屋なら InvalidIndex を返します。
			 */
			size_t FindRoomIndex(const Room* room) const noexcept;
		};

		/**
		 * Result of Tarjan's bridge detection over the currently unlocked aisles.
		 * Discovery and finish times form contiguous ranges, so subtree membership can be tested in constant time.
		 * 現在ロックされていない通路に対する Tarjan の橋検出の結果です。
		 * 発見時刻と終了時刻は連続した区間になるため、部分木に含まれるかを定数時間で判定できます。
		 */
		struct BridgeAnalysis final
		{
			std::vector<size_t> mDiscoveryTimes;	//!< Room index to DFS discovery time. InvalidIndex when unreachable / 部屋添字からDFS発見時刻へ。到達不能なら InvalidIndex
			std::vector<size_t> mFinishTimes;		//!< Room index to DFS finish time / 部屋添字からDFS終了時刻へ
			std::vector<size_t> mBridgeEdges;		//!< Edge indices that are bridges / 橋である辺の添字
			std::vector<size_t> mBridgeChildren;	//!< Deeper endpoint of each bridge in mBridgeEdges / 各橋の深い側の端点
		};

		/**
		 * The start-to-goal route expressed as an ordered chain of bridges and the segments they separate.
		 * A room in segment N has crossed exactly N route bridges, so segment 0 holds the start room
		 * and the last segment holds the goal room.
		 * スタートからゴールへの経路を、順序付けられた橋の連なりと、それが区切る区画として表現したものです。
		 * 区画Nの部屋は経路上の橋をちょうどN本越えた位置にあり、区画0にスタート部屋、最後の区画にゴール部屋が含まれます。
		 */
		struct RouteAnalysis final
		{
			std::vector<size_t> mRouteEdges;	//!< Bridge edge indices ordered from start to goal / スタートからゴールの順に並んだ橋の辺添字
			std::vector<size_t> mSegments;		//!< Room index to segment index. InvalidIndex when unreachable / 部屋添字から区画番号へ。到達不能なら InvalidIndex
		};

		// Builds a RouteGraph from every room and aisle owned by the generator.
		void BuildRouteGraph(RouteGraph& graph) const noexcept;

		// Enumerates the bridges reachable from fromRoomIndex, walking unlocked aisles only.
		BridgeAnalysis ComputeBridges(const RouteGraph& graph, const size_t fromRoomIndex) const noexcept;

		// Extracts the ordered chain of bridges that separates the start room from the goal room.
		bool AnalyzeRoute(const RouteGraph& graph, const size_t startRoomIndex, const size_t goalRoomIndex, RouteAnalysis& analysis) const noexcept;

		// Decides which route bridges become locks and which rooms hold their keys, then applies the decision.
		bool PlaceLocksAndKeys(const RouteGraph& graph, const RouteAnalysis& analysis, const std::shared_ptr<const Room>& goalRoom, const uint8_t maxKeyCount) const noexcept;

		// Draws a room for a common key among the rooms reachable before the lock that closes segment maxSegment.
		size_t DrawCommonKeyRoom(const RouteGraph& graph, const RouteAnalysis& analysis, const std::vector<size_t>& candidateRooms, const std::vector<uint8_t>& usedRooms, const size_t maxSegment) const noexcept;

		// Draws a room for the unique key among the rooms behind every common lock and before the unique lock.
		size_t DrawUniqueKeyRoom(const RouteGraph& graph, const RouteAnalysis& analysis, const std::vector<size_t>& candidateRooms, const std::vector<uint8_t>& usedRooms, const size_t minSegment, const size_t maxSegment, const std::shared_ptr<const Room>& goalRoom) const noexcept;

		// Replays the finished mission and reports any rule the placement broke.
		void VerifySolvability(const RouteGraph& graph, const size_t startRoomIndex, const size_t goalRoomIndex) const noexcept;

		// Returns whether the room is allowed to hold a key.
		static bool IsKeyPlaceableRoom(const std::shared_ptr<const Room>& room) noexcept;

		// Lottery weight of a room considered for a common key.
		static uint32_t DetermineKeyPlacementProbability(const size_t segment, const std::shared_ptr<const Room>& room) noexcept;

		// Lottery weight of a room considered for the unique key.
		static uint32_t DetermineUniqueKeyPlacementProbability(const uint8_t branchId, const std::shared_ptr<const Room>& room, const size_t hopsFromLock) noexcept;

	private:
		std::shared_ptr<Generator> mGenerator;	//!< Generator that owns the rooms and aisles being modified / 変更対象の部屋と通路を所有するジェネレータ
		bool mPlaced = false;					//!< Whether locks and keys were placed / ロックと鍵を配置したか
	};
}
