/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

/*
 * MissionGraph
 * Generate dungeon strategy information
 * ダンジョンの攻略情報を生成します
 *
 * Algorithm for generating keys and doors
 * 1. take an adjacency snapshot of the whole dungeon
 * 2. run Tarjan's bridge detection from the start room over the unlocked aisles
 * 3. keep only the bridges whose deeper side contains the goal room. they form an ordered chain
 *    start -- b0 -- b1 -- ... -- bN-1 -- goal, and every one of them has to be crossed to reach the goal
 * 4. lock the chain from the start side with common locks, and place each key in a segment before its lock
 * 5. lock the last bridge of the chain with the unique lock, and place the unique key behind every common lock
 *
 * 鍵と扉の生成アルゴリズム
 * １．ダンジョン全体の隣接情報を取得する
 * ２．スタート部屋を起点に、ロックされていない通路だけを辿って Tarjan の橋検出を行う
 * ３．深い側にゴール部屋を含む橋だけを残す。これは start -- b0 -- b1 -- ... -- bN-1 -- goal という
 *     順序付けられた連なりになり、ゴールへ到達するには必ず全てを通過する必要がある
 * ４．連なりのスタート側から通常ロックをかけ、各々の鍵はそのロックより手前の区画に配置する
 * ５．連なりの最後の橋にユニークロックをかけ、ユニーク鍵は全ての通常ロックより奥に配置する
 *
 * A bridge has no detour by definition, so loops in the layout (AisleComplexity > 0) never let the player
 * walk around a locked door. Because every route bridge separates the start side from the goal side,
 * locking a prefix of the chain also guarantees that the unique key and the unique lock sit behind
 * all of the common locks.
 *
 * 橋は定義上迂回路を持たないため、レイアウトにループ（AisleComplexity > 0）があっても鍵付き扉を回り込めません。
 * また経路上の橋は必ずスタート側とゴール側を分断するため、連なりの先頭側から順にロックする事で
 * ユニーク鍵とユニークロックが全ての通常ロックより奥に位置する事も同時に保証されます。
 */

#include "MissionGraph.h"
#include "../Debug/Debug.h"
#include "../Generator.h"
#include "../Helper/DrawLots.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace dungeon
{
	namespace
	{
		/**
		 * Depth first search frame used to run Tarjan's algorithm without recursion.
		 * 再帰を使わずに Tarjan のアルゴリズムを実行するための深さ優先探索のフレームです。
		 */
		struct DepthFirstFrame final
		{
			size_t mRoom;			//!< Room index being visited / 訪問中の部屋添字
			size_t mParentEdge;		//!< Edge index the search entered this room through / この部屋へ入ってきた辺の添字
			size_t mNextAdjacency;	//!< Next adjacency slot to examine / 次に調べる隣接リストの位置
		};
	}

	/*
	Returns the index of the given room, or InvalidIndex when it is unknown.
	指定した部屋の添字を返します。未知の部屋なら InvalidIndex を返します。
	*/
	size_t MissionGraph::RouteGraph::FindRoomIndex(const Room* room) const noexcept
	{
		if (room == nullptr)
			return InvalidIndex;
		const auto i = mRoomIndices.find(room);
		return i != mRoomIndices.end() ? i->second : InvalidIndex;
	}

	MissionGraph::MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<Room>& startRoom, const std::shared_ptr<Room>& goalRoom, const uint8_t maxKeyCount) noexcept
		: mGenerator(generator)
	{
		if (mGenerator == nullptr || startRoom == nullptr || goalRoom == nullptr || startRoom == goalRoom)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the start and goal rooms are not available. Keys and locks were not placed."));
			return;
		}

		// ダンジョン全体の隣接情報を取得する
		RouteGraph graph;
		BuildRouteGraph(graph);

		const size_t startRoomIndex = graph.FindRoomIndex(startRoom.get());
		const size_t goalRoomIndex = graph.FindRoomIndex(goalRoom.get());
		if (startRoomIndex == InvalidIndex || goalRoomIndex == InvalidIndex)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the start or goal room is not owned by the generator. Keys and locks were not placed."));
			return;
		}

		// スタートとゴールを分断する橋の連なりを求める
		RouteAnalysis analysis;
		if (AnalyzeRoute(graph, startRoomIndex, goalRoomIndex, analysis) == false)
		{
			// 迂回路しか無いので、どの通路をロックしても回り込まれてしまう
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: no aisle separates the start room from the goal room. Keys and locks were not placed."));
			return;
		}

		// ロックと鍵を配置する
		if (PlaceLocksAndKeys(graph, analysis, goalRoom, maxKeyCount) == false)
			return;

		mPlaced = true;

		// 配置した内容が攻略可能か検証する
		VerifySolvability(graph, startRoomIndex, goalRoomIndex);
	}

	bool MissionGraph::Placed() const noexcept
	{
		return mPlaced;
	}

	/*
	Builds a RouteGraph from every room and aisle owned by the generator.
	Aisles whose endpoints are missing or identical are dropped because they can never act as a gate.
	ジェネレータが所有する全ての部屋と通路から RouteGraph を構築します。
	端点が欠けている通路や自己ループの通路は関門になり得ないため除外します。
	*/
	void MissionGraph::BuildRouteGraph(RouteGraph& graph) const noexcept
	{
		graph.mRooms.reserve(mGenerator->GetRoomCount());
		mGenerator->ForEach([&graph](const std::shared_ptr<Room>& room)
			{
				if (room == nullptr)
					return;
				if (graph.mRoomIndices.emplace(room.get(), graph.mRooms.size()).second == false)
					return;
				graph.mRooms.emplace_back(room);
			}
		);
		graph.mAdjacency.resize(graph.mRooms.size());

		mGenerator->EachAisle([&graph](const Aisle& aisle)
			{
				const auto& point0 = aisle.GetPoint(0);
				const auto& point1 = aisle.GetPoint(1);
				if (point0 == nullptr || point1 == nullptr)
					return true;

				const size_t room0 = graph.FindRoomIndex(point0->GetOwnerRoom().get());
				const size_t room1 = graph.FindRoomIndex(point1->GetOwnerRoom().get());
				if (room0 == InvalidIndex || room1 == InvalidIndex || room0 == room1)
					return true;

				const size_t edgeIndex = graph.mEdges.size();
				RouteGraph::Edge edge;
				edge.mRoom0 = room0;
				edge.mRoom1 = room1;
				edge.mAisle = const_cast<Aisle*>(&aisle);
				graph.mEdges.emplace_back(edge);
				graph.mAdjacency[room0].emplace_back(edgeIndex);
				graph.mAdjacency[room1].emplace_back(edgeIndex);
				return true;
			}
		);
	}

	/*
	Enumerates the bridges reachable from fromRoomIndex with Tarjan's low-link method, walking unlocked aisles only.
	The search is iterative so that a long dungeon cannot overflow the stack, and it skips the incoming edge by
	edge index rather than by room index so that parallel aisles are correctly reported as non-bridges.
	ロックされていない通路だけを辿り、Tarjan の low-link 法で fromRoomIndex から到達できる橋を列挙します。
	長いダンジョンでスタックが溢れないよう反復版で実装しており、進入辺を部屋ではなく辺の添字で除外するため、
	同じ部屋同士を結ぶ多重辺は正しく橋ではないと判定されます。
	*/
	MissionGraph::BridgeAnalysis MissionGraph::ComputeBridges(const RouteGraph& graph, const size_t fromRoomIndex) const noexcept
	{
		const size_t roomCount = graph.mRooms.size();

		BridgeAnalysis result;
		result.mDiscoveryTimes.assign(roomCount, InvalidIndex);
		result.mFinishTimes.assign(roomCount, 0);
		if (fromRoomIndex >= roomCount)
			return result;

		std::vector<size_t> lowLinks(roomCount, InvalidIndex);
		std::vector<DepthFirstFrame> frames;
		frames.reserve(roomCount);

		size_t timer = 0;
		result.mDiscoveryTimes[fromRoomIndex] = lowLinks[fromRoomIndex] = timer;
		++timer;
		frames.emplace_back(DepthFirstFrame{ fromRoomIndex, InvalidIndex, 0 });

		while (frames.empty() == false)
		{
			DepthFirstFrame& frame = frames.back();
			const std::vector<size_t>& adjacency = graph.mAdjacency[frame.mRoom];
			if (frame.mNextAdjacency < adjacency.size())
			{
				const size_t edgeIndex = adjacency[frame.mNextAdjacency];
				++frame.mNextAdjacency;

				// 進入してきた辺は戻り辺として扱わない
				if (edgeIndex == frame.mParentEdge)
					continue;

				const RouteGraph::Edge& edge = graph.mEdges[edgeIndex];

				// 既にロックされている通路は経路探索から除外する
				if (edge.mAisle->IsAnyLocked())
					continue;

				const size_t currentRoom = frame.mRoom;
				const size_t nextRoom = edge.mRoom0 == currentRoom ? edge.mRoom1 : edge.mRoom0;
				if (result.mDiscoveryTimes[nextRoom] != InvalidIndex)
				{
					lowLinks[currentRoom] = std::min(lowLinks[currentRoom], result.mDiscoveryTimes[nextRoom]);
				}
				else
				{
					result.mDiscoveryTimes[nextRoom] = lowLinks[nextRoom] = timer;
					++timer;
					// frameへの参照はemplace_backで無効になるため、以降は参照しない事
					frames.emplace_back(DepthFirstFrame{ nextRoom, edgeIndex, 0 });
				}
			}
			else
			{
				const size_t currentRoom = frame.mRoom;
				const size_t parentEdge = frame.mParentEdge;
				result.mFinishTimes[currentRoom] = timer;
				frames.pop_back();
				if (frames.empty() == false)
				{
					const size_t parentRoom = frames.back().mRoom;
					lowLinks[parentRoom] = std::min(lowLinks[parentRoom], lowLinks[currentRoom]);

					// 子の部分木から親以上へ戻れないなら、進入辺は橋である
					if (lowLinks[currentRoom] > result.mDiscoveryTimes[parentRoom])
					{
						result.mBridgeEdges.emplace_back(parentEdge);
						result.mBridgeChildren.emplace_back(currentRoom);
					}
				}
			}
		}

		return result;
	}

	/*
	Extracts the ordered chain of bridges that separates the start room from the goal room and labels every
	reachable room with the number of chain bridges standing between it and the start room.
	Returns false when the goal room is unreachable or when no such bridge exists, which means every aisle has a detour.
	スタート部屋とゴール部屋を分断する橋を順番に並べ、到達可能な各部屋にスタートから数えて何本の橋の奥にあるかを付与します。
	ゴール部屋へ到達できない場合、または該当する橋が一本も無い（全ての通路に迂回路がある）場合は false を返します。
	*/
	bool MissionGraph::AnalyzeRoute(const RouteGraph& graph, const size_t startRoomIndex, const size_t goalRoomIndex, RouteAnalysis& analysis) const noexcept
	{
		const BridgeAnalysis bridges = ComputeBridges(graph, startRoomIndex);

		const size_t goalDiscoveryTime = bridges.mDiscoveryTimes[goalRoomIndex];
		if (goalDiscoveryTime == InvalidIndex)
			return false;

		// 深い側の部分木にゴール部屋を含む橋だけが、ゴールへ到達するために必ず通過する関門になる
		std::vector<std::pair<size_t, size_t>> routeBridges;
		for (size_t i = 0; i < bridges.mBridgeEdges.size(); ++i)
		{
			const size_t child = bridges.mBridgeChildren[i];
			if (bridges.mDiscoveryTimes[child] <= goalDiscoveryTime && goalDiscoveryTime < bridges.mFinishTimes[child])
				routeBridges.emplace_back(child, bridges.mBridgeEdges[i]);
		}
		if (routeBridges.empty())
			return false;

		// 発見時刻の昇順はスタートからゴールへの順序と一致する
		std::sort(routeBridges.begin(), routeBridges.end(), [&bridges](const std::pair<size_t, size_t>& lhs, const std::pair<size_t, size_t>& rhs)
			{
				return bridges.mDiscoveryTimes[lhs.first] < bridges.mDiscoveryTimes[rhs.first];
			}
		);

		analysis.mRouteEdges.reserve(routeBridges.size());
		for (const auto& routeBridge : routeBridges)
		{
			analysis.mRouteEdges.emplace_back(routeBridge.second);
		}

		// 部屋が何本の関門の奥にあるかを数え、区画番号とする
		analysis.mSegments.assign(graph.mRooms.size(), InvalidIndex);
		for (size_t roomIndex = 0; roomIndex < graph.mRooms.size(); ++roomIndex)
		{
			const size_t discoveryTime = bridges.mDiscoveryTimes[roomIndex];
			if (discoveryTime == InvalidIndex)
				continue;

			size_t segment = 0;
			for (const auto& routeBridge : routeBridges)
			{
				const size_t child = routeBridge.first;
				if (bridges.mDiscoveryTimes[child] <= discoveryTime && discoveryTime < bridges.mFinishTimes[child])
					++segment;
			}
			analysis.mSegments[roomIndex] = segment;
		}

		check(analysis.mSegments[startRoomIndex] == 0);
		check(analysis.mSegments[goalRoomIndex] == analysis.mRouteEdges.size());

		return true;
	}

	/*
	Decides which route bridges become locks and which rooms hold their keys, then applies the decision.
	The last bridge of the chain always becomes the unique lock, and the common locks are taken from the start
	side of the chain. A common lock is only accepted when a key room still exists in front of it, and the whole
	selection is rolled back one lock at a time until a room for the unique key is available behind every common lock.
	Returns false when nothing could be placed, in which case the dungeon is left completely unlocked.
	どの橋をロックにして、どの部屋に鍵を置くかを決定し、実際に適用します。
	連なりの最後の橋は必ずユニークロックになり、通常ロックは連なりのスタート側から取ります。
	通常ロックはその手前に鍵を置ける部屋が残っている場合のみ採用し、全ての通常ロックの奥にユニーク鍵を
	置ける部屋が見つかるまで、通常ロックを一本ずつ取り下げます。
	何も配置できなかった場合は false を返し、ダンジョンはロックが無い状態のままになります。
	*/
	bool MissionGraph::PlaceLocksAndKeys(const RouteGraph& graph, const RouteAnalysis& analysis, const std::shared_ptr<const Room>& goalRoom, const uint8_t maxKeyCount) const noexcept
	{
		check(analysis.mRouteEdges.empty() == false);

		// 連なりの最後の橋（＝ゴールに最も近い橋）をユニークロックにする
		const size_t uniqueLockPosition = analysis.mRouteEdges.size() - 1;

		// 鍵を置く事ができる部屋を集める
		std::vector<size_t> candidateRooms;
		candidateRooms.reserve(graph.mRooms.size());
		for (size_t roomIndex = 0; roomIndex < graph.mRooms.size(); ++roomIndex)
		{
			if (analysis.mSegments[roomIndex] == InvalidIndex)
				continue;
			if (IsKeyPlaceableRoom(graph.mRooms[roomIndex]) == false)
				continue;
			candidateRooms.emplace_back(roomIndex);
		}
		std::vector<uint8_t> usedRooms(graph.mRooms.size(), 0);

		// 通常ロックはユニークロックより手前にしか置けない
		size_t maxCommonLockCount = std::min<size_t>(maxKeyCount, uniqueLockPosition);
		maxCommonLockCount = std::min<size_t>(maxCommonLockCount, MaxCommonLockCount);

		// スタートに近い橋から順に通常ロックを確定していく
		std::vector<size_t> commonLockPositions;
		std::vector<size_t> commonKeyRooms;
		size_t searchPosition = 0;
		while (commonLockPositions.size() < maxCommonLockCount)
		{
			size_t position = searchPosition;
			size_t keyRoomIndex = InvalidIndex;
			while (position < uniqueLockPosition)
			{
				keyRoomIndex = DrawCommonKeyRoom(graph, analysis, candidateRooms, usedRooms, position);
				if (keyRoomIndex != InvalidIndex)
					break;
				// この橋より手前に鍵を置ける部屋が無いので、一本奥の橋を試す
				++position;
			}
			if (keyRoomIndex == InvalidIndex)
				break;

			commonLockPositions.emplace_back(position);
			commonKeyRooms.emplace_back(keyRoomIndex);
			usedRooms[keyRoomIndex] = 1;
			searchPosition = position + 1;
		}

		// 全ての通常ロックの奥、かつユニークロックの手前にユニーク鍵を置く
		size_t uniqueKeyRoomIndex = InvalidIndex;
		for (;;)
		{
			const size_t minSegment = commonLockPositions.empty() ? 0 : commonLockPositions.back() + 1;
			uniqueKeyRoomIndex = DrawUniqueKeyRoom(graph, analysis, candidateRooms, usedRooms, minSegment, uniqueLockPosition, goalRoom);
			if (uniqueKeyRoomIndex != InvalidIndex)
				break;
			if (commonLockPositions.empty())
				break;

			// ユニーク鍵の置き場所が無いので、一番奥の通常ロックを取り下げて区画を広げる
			usedRooms[commonKeyRooms.back()] = 0;
			commonKeyRooms.pop_back();
			commonLockPositions.pop_back();
		}
		if (uniqueKeyRoomIndex == InvalidIndex)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: no room can hold the unique key. Keys and locks were not placed."));
			return false;
		}

		// ここまでの決定を適用する
		check(commonLockPositions.size() == commonKeyRooms.size());
		for (size_t i = 0; i < commonLockPositions.size(); ++i)
		{
			graph.mEdges[analysis.mRouteEdges[commonLockPositions[i]]].mAisle->SetLock(true);

			const std::shared_ptr<Room>& keyRoom = graph.mRooms[commonKeyRooms[i]];
			check(keyRoom->GetItem() == Room::Item::Empty);
			check(keyRoom->IsValidReservationNumber() == false);
			keyRoom->SetItem(Room::Item::Key);
		}

		graph.mEdges[analysis.mRouteEdges[uniqueLockPosition]].mAisle->SetUniqueLock(true);

		const std::shared_ptr<Room>& uniqueKeyRoom = graph.mRooms[uniqueKeyRoomIndex];
		check(uniqueKeyRoom->GetItem() == Room::Item::Empty);
		check(uniqueKeyRoom->IsValidReservationNumber() == false);
		uniqueKeyRoom->SetItem(Room::Item::UniqueKey);

		if (commonLockPositions.size() < static_cast<size_t>(maxKeyCount))
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: placed %d of %d common keys."), static_cast<int32>(commonLockPositions.size()), static_cast<int32>(maxKeyCount));
		}

		return true;
	}

	/*
	Draws a room for a common key among the rooms that are reachable before the lock closing segment maxSegment.
	Any room in segments 0 to maxSegment is safe because the keys of the locks closer to the start are drawn first,
	so the player always owns at least one unused key when arriving at a locked door.
	Hanare rooms are only offered when nothing else is left, matching the pacing of the previous implementation.
	区画 maxSegment を閉じるロックより手前で到達できる部屋の中から、小さな鍵を置く部屋を抽選します。
	スタートに近いロックの鍵から先に抽選するため、区画0から maxSegment のどの部屋を選んでも、
	プレイヤーは扉に到着した時点で必ず未使用の鍵を一つ以上所持しています。
	はなれは他に候補が無い場合のみ対象とし、従来実装のテンポを踏襲します。
	*/
	size_t MissionGraph::DrawCommonKeyRoom(const RouteGraph& graph, const RouteAnalysis& analysis, const std::vector<size_t>& candidateRooms, const std::vector<uint8_t>& usedRooms, const size_t maxSegment) const noexcept
	{
		std::vector<size_t> selectableRooms;
		selectableRooms.reserve(candidateRooms.size());
		for (const size_t roomIndex : candidateRooms)
		{
			if (usedRooms[roomIndex] != 0)
				continue;
			if (analysis.mSegments[roomIndex] > maxSegment)
				continue;
			if (graph.mRooms[roomIndex]->GetParts() == Room::Parts::Hanare)
				continue;
			selectableRooms.emplace_back(roomIndex);
		}
		if (selectableRooms.empty())
		{
			// はなれしか残っていないなら、鍵の本数を減らすよりは、はなれに置く
			for (const size_t roomIndex : candidateRooms)
			{
				if (usedRooms[roomIndex] != 0)
					continue;
				if (analysis.mSegments[roomIndex] > maxSegment)
					continue;
				selectableRooms.emplace_back(roomIndex);
			}
		}
		if (selectableRooms.empty())
			return InvalidIndex;

		const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();
		const auto selectedRoom = DrawLots(random, selectableRooms.begin(), selectableRooms.end(), [&graph, &analysis](const size_t roomIndex) -> uint32_t
			{
				return DetermineKeyPlacementProbability(analysis.mSegments[roomIndex], graph.mRooms[roomIndex]);
			}
		);
		return selectedRoom != selectableRooms.end() ? *selectedRoom : InvalidIndex;
	}

	/*
	Draws a room for the unique key among the rooms whose segment is between minSegment and maxSegment.
	minSegment is the segment right behind the last common lock and maxSegment is the segment right in front of
	the unique lock, so the drawn room cannot be entered before every common key has been used.
	区画番号が minSegment 以上 maxSegment 以下の部屋の中から、ユニーク鍵を置く部屋を抽選します。
	minSegment は最後の通常ロックの直後の区画、maxSegment はユニークロックの直前の区画なので、
	抽選された部屋には全ての小さな鍵を使い切るまで入る事ができません。
	*/
	size_t MissionGraph::DrawUniqueKeyRoom(const RouteGraph& graph, const RouteAnalysis& analysis, const std::vector<size_t>& candidateRooms, const std::vector<uint8_t>& usedRooms, const size_t minSegment, const size_t maxSegment, const std::shared_ptr<const Room>& goalRoom) const noexcept
	{
		std::vector<size_t> selectableRooms;
		selectableRooms.reserve(candidateRooms.size());
		for (const size_t roomIndex : candidateRooms)
		{
			if (usedRooms[roomIndex] != 0)
				continue;
			const size_t segment = analysis.mSegments[roomIndex];
			if (segment < minSegment || maxSegment < segment)
				continue;
			selectableRooms.emplace_back(roomIndex);
		}
		if (selectableRooms.empty())
			return InvalidIndex;

		/*
		 * 錠前から何部屋離れているかを求めます。
		 * 区画の番号では差が付きません。最後の通常ロックとユニークロックの間が
		 * 一区画しか無い事が多く、候補のほとんどが同じ区画に入るためです。
		 * 施錠は無視して純粋な接続だけを辿ります。距離を測っているだけで、
		 * 誰がどこへ到達できるかはここでは問いません。
		 */
		std::vector<size_t> hopsFromLock(graph.mRooms.size(), InvalidIndex);
		if (maxSegment < analysis.mRouteEdges.size())
		{
			const RouteGraph::Edge& lockEdge = graph.mEdges[analysis.mRouteEdges[maxSegment]];
			std::vector<size_t> pending;
			pending.reserve(graph.mRooms.size());
			for (const size_t lockRoom : { lockEdge.mRoom0, lockEdge.mRoom1 })
			{
				if (lockRoom == InvalidIndex || hopsFromLock[lockRoom] != InvalidIndex)
					continue;
				hopsFromLock[lockRoom] = 0;
				pending.emplace_back(lockRoom);
			}
			for (size_t index = 0; index < pending.size(); ++index)
			{
				const size_t current = pending[index];
				const size_t nextHops = hopsFromLock[current] + 1;
				for (const size_t edgeIndex : graph.mAdjacency[current])
				{
					const RouteGraph::Edge& edge = graph.mEdges[edgeIndex];
					const size_t other = (edge.mRoom0 == current) ? edge.mRoom1 : edge.mRoom0;
					if (other == InvalidIndex || hopsFromLock[other] != InvalidIndex)
						continue;
					hopsFromLock[other] = nextHops;
					pending.emplace_back(other);
				}
			}
		}

		const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();
		const uint8_t roomBranch = goalRoom->GetBranchId();
		const auto selectedRoom = DrawLots(random, selectableRooms.begin(), selectableRooms.end(), [&graph, &hopsFromLock, roomBranch](const size_t roomIndex) -> uint32_t
			{
				// 錠前へ辿り着けない部屋は距離を測れないので、最も近い扱いにします
				const size_t hops = hopsFromLock[roomIndex] == InvalidIndex ? 0 : hopsFromLock[roomIndex];
				return DetermineUniqueKeyPlacementProbability(roomBranch, graph.mRooms[roomIndex], hops);
			}
		);
		return selectedRoom != selectableRooms.end() ? *selectedRoom : InvalidIndex;
	}

	/*
	Replays the finished mission the way a player would and reports any rule the placement broke.
	The replay only walks aisles that are unlocked or already opened, so a detour created by a loop would show up
	as a goal that is reachable too early, or as a unique key that is reachable before the last common lock.
	完成したミッションをプレイヤーと同じ手順で再現し、配置が破っている規則を報告します。
	ロックされていない通路と開錠済みの通路だけを辿るため、ループによる迂回路があると、
	ゴールに早く到達できてしまう、あるいは最後の通常ロックより手前でユニーク鍵に到達できてしまう形で検出されます。
	*/
	void MissionGraph::VerifySolvability(const RouteGraph& graph, const size_t startRoomIndex, const size_t goalRoomIndex) const noexcept
	{
		size_t commonLockCount = 0;
		size_t uniqueLockCount = 0;
		for (const RouteGraph::Edge& edge : graph.mEdges)
		{
			if (edge.mAisle->IsUniqueLocked())
				++uniqueLockCount;
			else if (edge.mAisle->IsLocked())
				++commonLockCount;
		}
		if (commonLockCount == 0 && uniqueLockCount == 0)
			return;

		std::vector<uint8_t> reachableRooms(graph.mRooms.size(), 0);
		std::vector<uint8_t> openedEdges(graph.mEdges.size(), 0);
		std::vector<size_t> pendingRooms;
		size_t commonKeys = 0;
		size_t uniqueKeys = 0;

		const auto visitRoom = [&graph, &reachableRooms, &pendingRooms, &commonKeys, &uniqueKeys](const size_t roomIndex)
		{
			if (reachableRooms[roomIndex] != 0)
				return;
			reachableRooms[roomIndex] = 1;
			pendingRooms.emplace_back(roomIndex);

			const Room::Item item = graph.mRooms[roomIndex]->GetItem();
			if (item == Room::Item::Key)
				++commonKeys;
			else if (item == Room::Item::UniqueKey)
				++uniqueKeys;
		};
		const auto expandRooms = [&graph, &openedEdges, &pendingRooms, &visitRoom]()
		{
			while (pendingRooms.empty() == false)
			{
				const size_t roomIndex = pendingRooms.back();
				pendingRooms.pop_back();
				for (const size_t edgeIndex : graph.mAdjacency[roomIndex])
				{
					const RouteGraph::Edge& edge = graph.mEdges[edgeIndex];
					if (edge.mAisle->IsAnyLocked() && openedEdges[edgeIndex] == 0)
						continue;
					visitRoom(edge.mRoom0 == roomIndex ? edge.mRoom1 : edge.mRoom0);
				}
			}
		};

		visitRoom(startRoomIndex);
		expandRooms();

		// 一つもロックを開けずにゴールへ到達できてはならない
		if (reachableRooms[goalRoomIndex] != 0)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the goal room is reachable without opening any lock."));
		}

		// 全ての通常ロックを開け終えるまでユニーク鍵に到達できてはならない
		bool uniqueKeyFoundTooEarly = uniqueKeys > 0 && commonLockCount > 0;

		size_t openedCommonLocks = 0;
		for (;;)
		{
			size_t targetEdgeIndex = InvalidIndex;
			for (size_t edgeIndex = 0; edgeIndex < graph.mEdges.size(); ++edgeIndex)
			{
				if (openedEdges[edgeIndex] != 0)
					continue;
				const RouteGraph::Edge& edge = graph.mEdges[edgeIndex];
				if (edge.mAisle->IsUniqueLocked() || edge.mAisle->IsLocked() == false)
					continue;
				if ((reachableRooms[edge.mRoom0] != 0) == (reachableRooms[edge.mRoom1] != 0))
					continue;
				targetEdgeIndex = edgeIndex;
				break;
			}
			if (targetEdgeIndex == InvalidIndex)
				break;

			// ロックの手前に対応する鍵が存在しなければならない
			if (commonKeys == 0)
			{
				DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: a common lock has no key placed in front of it."));
				break;
			}
			--commonKeys;
			openedEdges[targetEdgeIndex] = 1;
			++openedCommonLocks;

			const RouteGraph::Edge& targetEdge = graph.mEdges[targetEdgeIndex];
			visitRoom(reachableRooms[targetEdge.mRoom0] != 0 ? targetEdge.mRoom1 : targetEdge.mRoom0);
			expandRooms();

			if (uniqueKeys > 0 && openedCommonLocks < commonLockCount)
				uniqueKeyFoundTooEarly = true;
		}

		if (openedCommonLocks != commonLockCount)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: %d of %d common locks can never be opened."), static_cast<int32>(commonLockCount - openedCommonLocks), static_cast<int32>(commonLockCount));
		}
		if (commonKeys != 0)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: %d common keys are left over after opening every common lock."), static_cast<int32>(commonKeys));
		}
		if (uniqueKeyFoundTooEarly)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the unique key is reachable before every common lock has been opened."));
		}
		if (uniqueKeys == 0)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the unique key is not reachable."));
			return;
		}

		// ユニークロックを開けるとゴールへ到達できなければならない
		for (size_t edgeIndex = 0; edgeIndex < graph.mEdges.size(); ++edgeIndex)
		{
			const RouteGraph::Edge& edge = graph.mEdges[edgeIndex];
			if (edge.mAisle->IsUniqueLocked() == false)
				continue;
			if ((reachableRooms[edge.mRoom0] != 0) == (reachableRooms[edge.mRoom1] != 0))
				continue;
			openedEdges[edgeIndex] = 1;
			visitRoom(reachableRooms[edge.mRoom0] != 0 ? edge.mRoom1 : edge.mRoom0);
			expandRooms();
		}
		if (reachableRooms[goalRoomIndex] == 0)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the goal room is unreachable even after opening every lock."));
		}
	}

	/*
	Returns whether the room is allowed to hold a key.
	The condition mirrors Generator::IsRoutePassable so that a key never lands on a reserved room,
	on a room that already carries an item, or on the start and goal rooms themselves.
	鍵を置く事ができる部屋か判定します。
	予約済みの部屋、既にアイテムを持つ部屋、スタート部屋とゴール部屋自身に鍵が乗らないよう、
	Generator::IsRoutePassable と同じ条件にしています。
	*/
	bool MissionGraph::IsKeyPlaceableRoom(const std::shared_ptr<const Room>& room) noexcept
	{
		return
			room != nullptr &&
			// 予約済みの部屋は対象外
			room->IsValidReservationNumber() == false &&
			// アイテムがあると対象外
			room->GetItem() == Room::Item::Empty &&
			// ホールとはなれだけ対象
			(room->GetParts() == Room::Parts::Hall || room->GetParts() == Room::Parts::Hanare);
	}

	/*
	Returns the lottery weight of a room considered for a common key.
	Rooms closer to the door they open are preferred so that the player does not have to backtrack across
	the whole dungeon, and a dead end is preferred slightly so that exploring a branch is rewarded.
	小さな鍵を置く候補になった部屋の抽選重みを返します。
	ダンジョン全体を引き返さずに済むよう、開ける扉に近い部屋ほど優先し、
	寄り道が報われるよう行き止まりの部屋をわずかに優先します。
	*/
	uint32_t MissionGraph::DetermineKeyPlacementProbability(const size_t segment, const std::shared_ptr<const Room>& room) noexcept
	{
		// 予約済みの部屋はアイテムを置く事ができない
		check(room->IsValidReservationNumber() == false);

		// 扉に近い区画ほど優先
		uint32_t weight = 1 + static_cast<uint32_t>(segment);

		// はなれの部屋ほど優先
		if (room->GetParts() == Room::Parts::Hanare)
			weight *= 2;

		return weight;
	}

	/*
	Returns the lottery weight of a room considered for the unique key.
	Rooms far from the door they open are preferred so that fetching the key is a journey rather than a
	detour taken in front of the door, and a different branch or a dead end is preferred as well.
	Distance is counted in segments back from the lock, not in depth from the start room. The unique lock
	sits just before the goal, so the deepest room is also the room next to the door, and weighting by
	depth put the key against the lock: two runs in three had it in a room the door already touches.
	ユニーク鍵を置く候補になった部屋の抽選重みを返します。
	扉の目の前で寄り道するのではなく取りに行く道のりになるよう、開ける扉から遠い部屋ほど優先し、
	違う経路や行き止まりの部屋も優先します。
	距離はスタートからの深さではなく、錠前から何区画手前かで数えます。ユニークロックはゴールの
	直前にあるため最も深い部屋は扉の隣の部屋でもあり、深さで重み付けすると鍵が錠前へ張り付きます。
	実際、垂直配置では3回に2回が扉の接する部屋に置かれていました。
	探索の報酬という性質は、候補が最後の通常ロックより奥へ限定されている事で保たれます。
	*/
	uint32_t MissionGraph::DetermineUniqueKeyPlacementProbability(const uint8_t branchId, const std::shared_ptr<const Room>& room, const size_t hopsFromLock) noexcept
	{
		// 予約済みの部屋はアイテムを置く事ができない
		check(room->IsValidReservationNumber() == false);

		// 違う経路ほど優先
		uint32_t weight = 1;
		if (room->GetBranchId() != branchId)
			++weight;

		/*
		 * 錠前から遠い部屋ほど優先します
		 * 候補が扉の隣しか無い場合でも0にならないよう1を足します
		 * 二乗にしているのは、扉の隣に候補が集まりやすく、線形では押し返せないためです
		 */
		const uint32_t distance = 1 + static_cast<uint32_t>(hopsFromLock);
		weight *= distance * distance;

		// はなれの部屋ほど優先
		if (room->GetParts() == Room::Parts::Hanare)
			weight *= 10;

		return weight;
	}
}
