/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Intent graph generator for dungeon layouts.
 * LayoutGraphGenerator を表します。
 */

#include "LayoutGraphGenerator.h"
#include "../GenerateParameter.h"
#include "../Math/Random.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace dungeon
{
	namespace
	{
		int32 ClampRoomCount(const uint8_t count) noexcept
		{
			return std::max<int32>(3, count);
		}

		EDungeonAislePurpose MakeMainPathPurpose(const GenerateParameter& parameter, const int32 index, const int32 mainPathCount) noexcept
		{
			const auto& path = parameter.GetPathSettings();
			if (path.ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks &&
				index + 2 >= mainPathCount)
			{
				return EDungeonAislePurpose::Locked;
			}

			return EDungeonAislePurpose::MainPath;
		}

		bool IsBranchSelectableRole(const EDungeonRoomGameplayRole role) noexcept
		{
			switch (role)
			{
			case EDungeonRoomGameplayRole::None:
			case EDungeonRoomGameplayRole::Combat:
			case EDungeonRoomGameplayRole::Treasure:
			case EDungeonRoomGameplayRole::Puzzle:
			case EDungeonRoomGameplayRole::Rest:
			case EDungeonRoomGameplayRole::Secret:
				return true;
			case EDungeonRoomGameplayRole::Boss:
			default:
				return false;
			}
		}

		const TArray<FDungeonRoomRoleProfile>& GetRoomRoleProfiles(const GenerateParameter& parameter) noexcept
		{
			return parameter.GetRoomRoleSettings().Roles;
		}

		EDungeonRoomGameplayRole SelectWeightedGameplayRole(const GenerateParameter& parameter, const TArray<FDungeonRoomRoleProfile>& profiles) noexcept
		{
			float totalWeight = 0.f;
			for (const FDungeonRoomRoleProfile& profile : profiles)
			{
				if (IsBranchSelectableRole(profile.Role))
				{
					totalWeight += std::max(0.f, profile.BranchSelectionWeight);
				}
			}
			if (totalWeight <= 0.f)
			{
				return EDungeonRoomGameplayRole::None;
			}

			float roll = parameter.GetRandom()->Get<float>() * totalWeight;
			for (const FDungeonRoomRoleProfile& profile : profiles)
			{
				if (!IsBranchSelectableRole(profile.Role))
				{
					continue;
				}
				roll -= std::max(0.f, profile.BranchSelectionWeight);
				if (roll <= 0.f)
				{
					return profile.Role;
				}
			}
			return EDungeonRoomGameplayRole::None;
		}

		Point GetRoomCenter(const std::shared_ptr<Room>& room) noexcept
		{
			return room->GetGroundCenter();
		}

		double SquaredDistance(const Point& point, const FVector& target) noexcept
		{
			const double x = point.X - target.X;
			const double y = point.Y - target.Y;
			const double z = point.Z - target.Z;
			return x * x + y * y + z * z;
		}

		StartLocationPolicy ToSpatialLocationPolicy(const EDungeonGoalLocationPolicy policy) noexcept
		{
			switch (policy)
			{
			case EDungeonGoalLocationPolicy::UseNorthernMost:
				return StartLocationPolicy::UseNorthernMost;
			case EDungeonGoalLocationPolicy::UseEasternMost:
				return StartLocationPolicy::UseEasternMost;
			case EDungeonGoalLocationPolicy::UseWesternMost:
				return StartLocationPolicy::UseWesternMost;
			case EDungeonGoalLocationPolicy::UseSouthernMost:
				return StartLocationPolicy::UseSouthernMost;
			case EDungeonGoalLocationPolicy::UseHighestPoint:
				return StartLocationPolicy::UseHighestPoint;
			case EDungeonGoalLocationPolicy::UseLowestPoint:
				return StartLocationPolicy::UseLowestPoint;
			case EDungeonGoalLocationPolicy::UseCentralPoint:
			default:
				return StartLocationPolicy::UseCentralPoint;
			}
		}

		size_t SelectSpatialStartNode(
			const StartLocationPolicy policy,
			const std::vector<std::shared_ptr<Room>>& rooms,
			const size_t goalNodeIndex) noexcept
		{
			FVector center = FVector::ZeroVector;
			size_t candidateCount = 0;
			for (size_t index = 0; index < rooms.size(); ++index)
			{
				if (index == goalNodeIndex || rooms[index] == nullptr)
					continue;
				center += static_cast<FVector>(GetRoomCenter(rooms[index]));
				++candidateCount;
			}
			if (candidateCount == 0)
				return rooms.size();
			center /= static_cast<float>(candidateCount);

			size_t result = rooms.size();
			double bestPrimaryScore = std::numeric_limits<double>::max();
			double bestSecondaryScore = std::numeric_limits<double>::max();
			for (size_t index = 0; index < rooms.size(); ++index)
			{
				if (index == goalNodeIndex || rooms[index] == nullptr)
					continue;

				const Point point = GetRoomCenter(rooms[index]);
				double primaryScore = 0.;
				double secondaryScore = 0.;
				switch (policy)
				{
				case StartLocationPolicy::UseNorthernMost:
					primaryScore = point.Y;
					secondaryScore = std::abs(point.X - center.X);
					break;
				case StartLocationPolicy::UseEasternMost:
					primaryScore = -point.X;
					secondaryScore = std::abs(point.Y - center.Y);
					break;
				case StartLocationPolicy::UseWesternMost:
					primaryScore = point.X;
					secondaryScore = std::abs(point.Y - center.Y);
					break;
				case StartLocationPolicy::UseSouthernMost:
					primaryScore = -point.Y;
					secondaryScore = std::abs(point.X - center.X);
					break;
				case StartLocationPolicy::UseHighestPoint:
					primaryScore = -point.Z;
					secondaryScore = FMath::Square(point.X - center.X) + FMath::Square(point.Y - center.Y);
					break;
				case StartLocationPolicy::UseLowestPoint:
					primaryScore = point.Z;
					secondaryScore = FMath::Square(point.X - center.X) + FMath::Square(point.Y - center.Y);
					break;
				case StartLocationPolicy::UseCentralPoint:
				case StartLocationPolicy::UseMultiStart:
				default:
					primaryScore = SquaredDistance(point, center);
					break;
				}

				if (primaryScore < bestPrimaryScore || (primaryScore == bestPrimaryScore && secondaryScore < bestSecondaryScore))
				{
					bestPrimaryScore = primaryScore;
					bestSecondaryScore = secondaryScore;
					result = index;
				}
			}
			return result;
		}

		std::vector<size_t> SelectMultiStartNodes(
			const std::vector<std::shared_ptr<Room>>& rooms,
			const size_t goalNodeIndex,
			const size_t requestedCount) noexcept
		{
			std::vector<size_t> selected;
			const size_t availableCount = rooms.size() - (goalNodeIndex < rooms.size() ? 1 : 0);
			const size_t targetCount = std::min(requestedCount, availableCount);
			if (targetCount == 0)
				return selected;

			const size_t primary = SelectSpatialStartNode(StartLocationPolicy::UseCentralPoint, rooms, goalNodeIndex);
			if (primary >= rooms.size())
				return selected;
			selected.emplace_back(primary);

			while (selected.size() < targetCount)
			{
				size_t bestIndex = rooms.size();
				double bestDistance = -1.;
				for (size_t index = 0; index < rooms.size(); ++index)
				{
					if (index == goalNodeIndex || rooms[index] == nullptr || std::find(selected.begin(), selected.end(), index) != selected.end())
						continue;

					const Point candidate = GetRoomCenter(rooms[index]);
					double nearestDistance = std::numeric_limits<double>::max();
					for (const size_t selectedIndex : selected)
					{
						nearestDistance = std::min(nearestDistance, SquaredDistance(candidate, static_cast<FVector>(GetRoomCenter(rooms[selectedIndex]))));
					}
					if (nearestDistance > bestDistance)
					{
						bestDistance = nearestDistance;
						bestIndex = index;
					}
				}

				if (bestIndex >= rooms.size())
					break;
				selected.emplace_back(bestIndex);
			}
			return selected;
		}

		int32 SelectBranchParentIndex(const GenerateParameter& parameter, const int32 mainPathCount, const int32 branchIndex, const bool bGoalCanReceiveExtraRoutes) noexcept
		{
			const auto& path = parameter.GetPathSettings();
			if (path.ProgressionPolicy == EDungeonProgressionPolicy::HubQuest && mainPathCount > 2 && (branchIndex % 4) != 3)
			{
				return 1;
			}

			if (path.ProgressionPolicy == EDungeonProgressionPolicy::FreeExploration && mainPathCount > 2 && (branchIndex % 5) == 0)
			{
				return mainPathCount - 1;
			}

			const int32 lastBranchParent = bGoalCanReceiveExtraRoutes ? mainPathCount - 1 : mainPathCount - 2;
			const int32 firstBranchParent = lastBranchParent >= 1 ? 1 + parameter.GetRandom()->Get<int32>(lastBranchParent) : 0;
			return std::clamp(firstBranchParent, 0, mainPathCount - 1);
		}

		/*
		 * 鍵と扉の関門として橋のまま残しておく本流通路の数を返します。
		 * MissionGraph は橋にしか鍵をかけられないため、Generator が配置しうる鍵の最大数
		 * （部屋数の平方根）にユニークロックの一本を加えた数だけ確保します。
		 */
		int32 CalculateReservedLockGateCount(const GenerateParameter& parameter, const int32 roomCount) noexcept
		{
			if (parameter.GetPathSettings().ProgressionPolicy != EDungeonProgressionPolicy::KeysAndLocks)
			{
				return 0;
			}
			const int32 maxKeyCount = std::max(2, static_cast<int32>(std::ceil(std::sqrt(static_cast<float>(std::max(roomCount, 0))))));
			return maxKeyCount + 1;
		}

		/*
		 * 本流通路のどれを関門として保護するかを返します。
		 * 添字は本流の通路番号（部屋 index と index+1 を結ぶ通路）で、ゴール側の一本を必ず含み、
		 * そこからスタート側へ等間隔に確保します。
		 */
		std::vector<bool> MakeReservedLockGates(const int32 mainPathCount, const int32 gateCount) noexcept
		{
			const int32 mainPathEdgeCount = std::max(0, mainPathCount - 1);
			const int32 reservedCount = std::min(gateCount, mainPathEdgeCount);
			if (reservedCount <= 0)
			{
				// 保護する関門が無いので空を返し、以降の判定を省略させます
				return std::vector<bool>();
			}

			std::vector<bool> gates(static_cast<size_t>(mainPathEdgeCount), false);
			for (int32 index = 0; index < reservedCount; ++index)
			{
				const int32 position = mainPathEdgeCount - 1 - (index * mainPathEdgeCount) / reservedCount;
				gates[static_cast<size_t>(std::clamp(position, 0, mainPathEdgeCount - 1))] = true;
			}
			return gates;
		}

		/*
		 * ループ通路が本流のどの区間をまたぐかを求めるための、本流上の接続位置を返します。
		 * 本流の部屋はその部屋番号、枝の部屋は接続元になっている本流の部屋番号を返します。
		 */
		int32 GetMainPathAnchor(const LayoutGraph& graph, const size_t nodeIndex, const int32 mainPathCount) noexcept
		{
			if (static_cast<int32>(nodeIndex) < mainPathCount)
			{
				return static_cast<int32>(nodeIndex);
			}
			return static_cast<int32>(graph.Nodes[nodeIndex].ParentIndex);
		}

		/*
		 * ループ通路が関門として保護した本流通路をまたぐか判定します。
		 * またぐ場合はその関門が橋ではなくなり、鍵を持たずに扉の先へ回り込めてしまいます。
		 */
		bool CrossesReservedLockGate(const LayoutGraph& graph, const std::vector<bool>& reservedLockGates, const size_t room0, const size_t room1, const int32 mainPathCount) noexcept
		{
			if (reservedLockGates.empty())
			{
				return false;
			}

			int32 anchor0 = GetMainPathAnchor(graph, room0, mainPathCount);
			int32 anchor1 = GetMainPathAnchor(graph, room1, mainPathCount);
			if (anchor0 > anchor1)
			{
				std::swap(anchor0, anchor1);
			}

			// 本流通路 position は部屋 position と position+1 を結ぶので、またぐ範囲は [anchor0, anchor1)
			for (int32 position = anchor0; position < anchor1; ++position)
			{
				if (static_cast<size_t>(position) < reservedLockGates.size() && reservedLockGates[static_cast<size_t>(position)])
				{
					return true;
				}
			}
			return false;
		}

		/*
		 * スタートとゴールを分断する通路（橋）が一本でも存在するか判定します。
		 * MissionGraph は橋にしか鍵をかけられないため、KeysAndLocks では最低一本必要です。
		 * 通路を一本ずつ取り除いて到達判定するだけの O(E*(V+E)) 実装ですが、
		 * 呼び出しはレイアウト候補ごとに数回で、通常は最初の一回で成立します。
		 */
		bool HasLockableAisle(const LayoutGraph& graph, const size_t startNode, const size_t goalNode) noexcept
		{
			if (startNode == goalNode || startNode >= graph.Nodes.size() || goalNode >= graph.Nodes.size())
			{
				return false;
			}

			std::vector<std::vector<std::pair<size_t, size_t>>> adjacency(graph.Nodes.size());
			for (size_t edgeIndex = 0; edgeIndex < graph.Edges.size(); ++edgeIndex)
			{
				const LayoutAisleEdge& edge = graph.Edges[edgeIndex];
				if (edge.Room0 >= graph.Nodes.size() || edge.Room1 >= graph.Nodes.size())
				{
					continue;
				}
				adjacency[edge.Room0].emplace_back(edge.Room1, edgeIndex);
				adjacency[edge.Room1].emplace_back(edge.Room0, edgeIndex);
			}

			std::vector<bool> reached(graph.Nodes.size(), false);
			std::vector<size_t> pendingNodes;
			pendingNodes.reserve(graph.Nodes.size());
			for (size_t excludedEdge = 0; excludedEdge < graph.Edges.size(); ++excludedEdge)
			{
				std::fill(reached.begin(), reached.end(), false);
				pendingNodes.clear();
				pendingNodes.emplace_back(startNode);
				reached[startNode] = true;
				while (pendingNodes.empty() == false)
				{
					const size_t current = pendingNodes.back();
					pendingNodes.pop_back();
					for (const std::pair<size_t, size_t>& adjacent : adjacency[current])
					{
						if (adjacent.second == excludedEdge || reached[adjacent.first])
						{
							continue;
						}
						reached[adjacent.first] = true;
						pendingNodes.emplace_back(adjacent.first);
					}
				}

				// この通路を閉じるとゴールへ行けなくなるなら、それは鍵をかけられる橋
				if (reached[goalNode] == false)
				{
					return true;
				}
			}
			return false;
		}

		/*
		 * 鍵をかけられる橋が一本も無くなってしまった場合に、ループ通路を後から追加した順に取り除きます。
		 * ループ通路を全て取り除けば木構造に戻り、必ず橋が現れるため、この処理は有限回で終わります。
		 */
		void DropLoopAislesUntilLockable(LayoutGraph& graph, const size_t startNode, const size_t goalNode) noexcept
		{
			if (startNode == goalNode)
			{
				return;
			}

			while (HasLockableAisle(graph, startNode, goalNode) == false)
			{
				const auto lastLoopAisle = std::find_if(graph.Edges.rbegin(), graph.Edges.rend(), [](const LayoutAisleEdge& edge)
					{
						return edge.Purpose == EDungeonAislePurpose::Loop || edge.Purpose == EDungeonAislePurpose::Shortcut;
					});
				if (lastLoopAisle == graph.Edges.rend())
				{
					break;
				}
				graph.Edges.erase(std::next(lastLoopAisle).base());
			}
		}

		/*
		 * サイズを変更できない部屋へ、門を置ける数を超える通路が繋がらないようにします。
		 * 門を置けるグリッド数は部屋のアセットが決めるため後から広げられません。超過したまま
		 * 生成へ進むと、ボクセル生成の段階で門を置けずに失敗します。
		 * 全域木へ追加された余剰辺であるループとショートカットだけを間引くので、部屋が孤立する
		 * ことはありません。
		 */
		void DropLoopAislesUntilGateCapacity(LayoutGraph& graph, const size_t nodeIndex, const uint8_t gateCapacity) noexcept
		{
			// 0は未設定を表すので制限しません
			if (gateCapacity == 0 || nodeIndex >= graph.Nodes.size())
			{
				return;
			}

			const auto countEdges = [&graph, nodeIndex]()
			{
				return std::count_if(graph.Edges.begin(), graph.Edges.end(), [nodeIndex](const LayoutAisleEdge& edge)
					{
						return edge.Room0 == nodeIndex || edge.Room1 == nodeIndex;
					});
			};

			while (countEdges() > static_cast<std::ptrdiff_t>(gateCapacity))
			{
				const auto lastLoopAisle = std::find_if(graph.Edges.rbegin(), graph.Edges.rend(), [nodeIndex](const LayoutAisleEdge& edge)
					{
						if (edge.Purpose != EDungeonAislePurpose::Loop && edge.Purpose != EDungeonAislePurpose::Shortcut)
							return false;
						return edge.Room0 == nodeIndex || edge.Room1 == nodeIndex;
					});
				if (lastLoopAisle == graph.Edges.rend())
				{
					break;
				}

				graph.Edges.erase(std::next(lastLoopAisle).base());
			}
		}
	}

	LayoutGraph LayoutGraphGenerator::Generate(const GenerateParameter& parameter)
	{
		const auto& settings = parameter.GetPathSettings();
		const int32 roomCount = ClampRoomCount(parameter.GetNumberOfCandidateRooms());
		const float effectiveMainRouteRatio = CalculateEffectiveMainRouteRatio(settings);
		const float effectiveLoopRouteDensity = CalculateEffectiveLoopRouteDensity(settings);
		const int32 mainPathCount = std::clamp(
			static_cast<int32>(std::round(static_cast<float>(roomCount) * effectiveMainRouteRatio)),
			2,
			roomCount
		);

		LayoutGraph graph;
		graph.Nodes.reserve(roomCount);
		graph.Edges.reserve(roomCount + static_cast<int32>(static_cast<float>(roomCount) * effectiveLoopRouteDensity) + 1);

		for (int32 index = 0; index < mainPathCount; ++index)
		{
			LayoutRoomNode node;
			node.Index = static_cast<size_t>(index);
			node.ParentIndex = index > 0 ? static_cast<size_t>(index - 1) : 0;
			node.DesiredDepth = index;
			node.DesiredBranch = 0;
			node.DesiredFloor = 0;
			node.StructuralRole = EDungeonRoomStructuralRole::Connector;
			node.GameplayRole = SelectMainPathGameplayRole(parameter, index, mainPathCount);
			node.NonMainPathGameplayRole = EDungeonRoomGameplayRole::None;

			if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically)
			{
				node.DesiredFloor = index;
			}
			graph.Nodes.emplace_back(node);
			if (index > 0)
			{
				LayoutAisleEdge edge;
				edge.Room0 = static_cast<size_t>(index - 1);
				edge.Room1 = static_cast<size_t>(index);
				edge.Purpose = MakeMainPathPurpose(parameter, index - 1, mainPathCount);
				edge.bMainPath = true;
				graph.Edges.emplace_back(edge);
			}
		}

		graph.StartNodeIndex = 0;
		graph.StartNodeIndices = { graph.StartNodeIndex };
		graph.GoalNodeIndex = static_cast<size_t>(mainPathCount - 1);
		const bool bGoalCanReceiveExtraRoutes = settings.ProgressionPolicy == EDungeonProgressionPolicy::FreeExploration;

		const int32 remainingRoomCount = roomCount - mainPathCount;
		for (int32 branchIndex = 0; branchIndex < remainingRoomCount; ++branchIndex)
		{
			const int32 nodeIndex = mainPathCount + branchIndex;
			const int32 parentIndex = SelectBranchParentIndex(parameter, mainPathCount, branchIndex, bGoalCanReceiveExtraRoutes);

			LayoutRoomNode node;
			node.Index = static_cast<size_t>(nodeIndex);
			node.ParentIndex = static_cast<size_t>(parentIndex);
			node.DesiredDepth = graph.Nodes[parentIndex].DesiredDepth + 1;
			node.DesiredBranch = branchIndex + 1;
			node.DesiredFloor = graph.Nodes[parentIndex].DesiredFloor;
			node.StructuralRole = EDungeonRoomStructuralRole::Branch;
			node.NonMainPathGameplayRole = SelectBranchGameplayRole(parameter, branchIndex);
			node.GameplayRole = node.NonMainPathGameplayRole;

			if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically)
			{
				node.DesiredFloor = nodeIndex;
			}
			graph.Nodes.emplace_back(node);

			LayoutAisleEdge edge;
			edge.Room0 = static_cast<size_t>(parentIndex);
			edge.Room1 = static_cast<size_t>(nodeIndex);
			edge.Purpose = EDungeonAislePurpose::Branch;
			edge.bMainPath = false;
			graph.Edges.emplace_back(edge);
		}

		int32 loopCount = static_cast<int32>(std::round(static_cast<float>(roomCount) * effectiveLoopRouteDensity));
		loopCount = std::clamp(loopCount, 0, std::max(0, roomCount / 2));

		// 鍵と扉の関門にする本流通路は、迂回路が生まれないようループから保護します
		const std::vector<bool> reservedLockGates = MakeReservedLockGates(mainPathCount, CalculateReservedLockGateCount(parameter, roomCount));

		int32 attempts = 0;
		while (loopCount > 0 && attempts < roomCount * 4)
		{
			++attempts;
			const size_t room0 = static_cast<size_t>(parameter.GetRandom()->Get<int32>(0, roomCount));
			const size_t room1 = static_cast<size_t>(parameter.GetRandom()->Get<int32>(0, roomCount));
			if (!bGoalCanReceiveExtraRoutes && (room0 == graph.GoalNodeIndex || room1 == graph.GoalNodeIndex))
			{
				continue;
			}
			if (room0 == room1 || HasEdge(graph, room0, room1))
			{
				continue;
			}

			const int32 depthDistance = std::abs(graph.Nodes[room0].DesiredDepth - graph.Nodes[room1].DesiredDepth);
			if (depthDistance < 2)
			{
				continue;
			}

			// 鍵付き扉を回り込めてしまうループは採用しない
			if (CrossesReservedLockGate(graph, reservedLockGates, room0, room1, mainPathCount))
			{
				continue;
			}

			LayoutAisleEdge edge;
			edge.Room0 = room0;
			edge.Room1 = room1;
			edge.Purpose = depthDistance >= 4 ? EDungeonAislePurpose::Shortcut : EDungeonAislePurpose::Loop;
			edge.bMainPath = false;
			graph.Edges.emplace_back(edge);
			--loopCount;
		}

		AssignStructuralRoles(parameter, graph);
		return graph;
	}

	/*
	 * サイズを変更できない開始部屋とゴール部屋へ、門を置ける数を超える通路が繋がらないようにします。
	 * グラフの辺を削除するため、通路の一覧を構築する前に呼び出す必要があります。
	 * 構築後に呼び出すと、通路から辺を逆引きできなくなります。
	 */
	void LayoutGraphGenerator::LimitEndpointGateCapacity(const GenerateParameter& parameter, LayoutGraph& graph) noexcept
	{
		DropLoopAislesUntilGateCapacity(graph, graph.StartNodeIndex, parameter.GetStartRoomGateCapacity());
		DropLoopAislesUntilGateCapacity(graph, graph.GoalNodeIndex, parameter.GetGoalRoomGateCapacity());
	}

	/**
	 * Reconnects each branch aisle to the closest room that can serve as its parent.
	 * The graph decides which rooms connect before the rooms have any position, so a branch can end
	 * up attached to a room that lands far away. Once the rooms are placed the distance is known, and
	 * a branch may be re-parented to any room with a smaller index. Restricting the choice that way
	 * keeps the parent chain acyclic without having to walk the subtree.
	 * 枝の通路を、親になれる中で最も近い部屋へつなぎ替えます。
	 * どの部屋どうしをつなぐかは部屋の座標が決まる前に確定するため、離れた部屋につながる事があります。
	 * 部屋を配置した後なら距離が分かるので、自分より小さい添字の部屋へつなぎ替えます。
	 * 添字を小さい側に限る事で、部分木を辿らずに閉路の発生を防げます。
	 */
	void LayoutGraphGenerator::OptimizeBranchParents(LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms) noexcept
	{
		if (graph.Nodes.size() != rooms.size())
			return;

		const std::vector<std::shared_ptr<Room>> indexedRooms(rooms.begin(), rooms.end());
		if (std::any_of(indexedRooms.begin(), indexedRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room == nullptr;
			}))
		{
			return;
		}

		// Generatorと同じく、部屋の外周どうしの隙間で距離を測ります
		const auto intervalGap = [](const int32 min0, const int32 max0, const int32 min1, const int32 max1) -> int32
			{
				if (max0 <= min1)
					return min1 - max0;
				if (max1 <= min0)
					return min0 - max1;
				return 0;
			};
		const auto roomDistance = [&intervalGap](const Room& room0, const Room& room1) -> int32
			{
				return
					intervalGap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight()) +
					intervalGap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom()) +
					intervalGap(room0.GetBackground(), room0.GetForeground(), room1.GetBackground(), room1.GetForeground());
			};

		for (LayoutAisleEdge& edge : graph.Edges)
		{
			// 幹線は経路そのものなので動かしません。ループと近道は木の辺ではないため対象外です
			if (edge.bMainPath || edge.Purpose != EDungeonAislePurpose::Branch)
				continue;

			/*
			 * 枝の辺はRoom0が親、Room1が子で、親は必ず子より小さい添字になります。
			 * 付け替え先を子より小さい添字に限る事で閉路を防いでいるため、この前提が崩れると
			 * 部屋グラフに閉路ができて到達可能性の判定が壊れます。
			 */
			check(edge.Room0 < edge.Room1);

			const size_t child = edge.Room1;
			if (child >= indexedRooms.size() || child == 0)
				continue;

			size_t bestParent = edge.Room0;
			const int32 originalDistance = roomDistance(*indexedRooms[edge.Room0], *indexedRooms[child]);
			int32 bestDistance = originalDistance;
			for (size_t parent = 0; parent < child; ++parent)
			{
				if (parent == edge.Room0)
					continue;

				// 既に別の通路でつながっている相手は、二重の通路になるため選びません
				if (HasEdge(graph, parent, child))
					continue;

				const int32 distance = roomDistance(*indexedRooms[parent], *indexedRooms[child]);
				if (distance < bestDistance)
				{
					bestDistance = distance;
					bestParent = parent;
				}
			}

			/*
			 * わずかな短縮のために接続先を変えると、幹線の部屋の間隔を均等に保つ最適化を
			 * 乱してしまいます。距離が半分以下になる場合だけ付け替えます。
			 */
			if (bestParent == edge.Room0 || bestDistance * 2 >= originalDistance)
				continue;

			edge.Room0 = bestParent;
			graph.Nodes[child].ParentIndex = bestParent;

			/*
			 * DesiredDepthは変更しません。これは「進行のどの段階の部屋か」という意図を表しており、
			 * 通路をどこへつなぐかという幾何の都合で変えるべきものではありません。
			 * 実際に変えるとゾーンの進行度が動き、部屋のゾーン割り当てまで変わってしまいます。
			 */
		}
	}

	bool LayoutGraphGenerator::ApplyEndpointPolicies(const GenerateParameter& parameter, LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms, const bool bAislesBuilt) noexcept
	{
		if (graph.Nodes.empty() || rooms.size() != graph.Nodes.size() || graph.GoalNodeIndex >= graph.Nodes.size())
			return false;

		const std::vector<std::shared_ptr<Room>> indexedRooms(rooms.begin(), rooms.end());
		if (std::any_of(indexedRooms.begin(), indexedRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room == nullptr;
			}))
		{
			return false;
		}
		const size_t selectedGoalIndex = SelectSpatialStartNode(ToSpatialLocationPolicy(parameter.GetGoalLocationPolicy()), indexedRooms, indexedRooms.size());
		if (selectedGoalIndex >= graph.Nodes.size())
			return false;
		graph.GoalNodeIndex = selectedGoalIndex;

		if (!ApplyStartRoomPolicy(parameter, graph, indexedRooms))
			return false;

		/*
		 * 開始部屋とゴール部屋を選び直した結果、鍵をかけられる橋がループに埋もれてしまう事があるので復活させます。
		 * 辺を削除するため、通路の一覧を構築した後は実行できません。
		 * 実行すると通路から辺を逆引きできなくなり、Generator::RefreshEndpointPoliciesFromCurrentLayoutが
		 * SeparateRoomsFailedで失敗します。
		 * 復活させられなかった場合はMissionGraphが鍵とロックを配置せず、警告を残して生成は成功します。
		 */
		if (bAislesBuilt == false && parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks)
		{
			DropLoopAislesUntilLockable(graph, graph.StartNodeIndex, graph.GoalNodeIndex);
		}


		if (!RebuildMainRoute(parameter, graph, indexedRooms))
			return false;
		AssignStructuralRoles(parameter, graph);

		std::vector<int32> degree(indexedRooms.size(), 0);
		std::vector<bool> mainRouteNodes(indexedRooms.size(), false);
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 < degree.size())
				++degree[edge.Room0];
			if (edge.Room1 < degree.size())
				++degree[edge.Room1];
			if (edge.bMainPath && edge.Room0 < mainRouteNodes.size() && edge.Room1 < mainRouteNodes.size())
			{
				mainRouteNodes[edge.Room0] = true;
				mainRouteNodes[edge.Room1] = true;
			}
		}
		for (size_t index = 0; index < indexedRooms.size(); ++index)
		{
			const std::shared_ptr<Room>& room = indexedRooms[index];
			if (room == nullptr)
				return false;

			room->SetStructuralRole(graph.Nodes[index].StructuralRole);
			room->SetGameplayRole(graph.Nodes[index].GameplayRole);
			room->SetMainPathRoom(mainRouteNodes[index]);
			room->SetLockedRouteRoom(false);
			switch (graph.Nodes[index].StructuralRole)
			{
			case EDungeonRoomStructuralRole::Start:
				room->SetParts(Room::Parts::Start);
				break;
			case EDungeonRoomStructuralRole::Goal:
				room->SetParts(Room::Parts::Goal);
				break;
			default:
				room->SetParts(degree[index] <= 1 ? Room::Parts::Hanare : Room::Parts::Hall);
				break;
			}
		}
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Purpose == EDungeonAislePurpose::Locked)
			{
				indexedRooms[edge.Room0]->SetLockedRouteRoom(true);
				indexedRooms[edge.Room1]->SetLockedRouteRoom(true);
			}
		}
		return true;
	}

	bool LayoutGraphGenerator::ApplyStartRoomPolicy(const GenerateParameter& parameter, LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& indexedRooms) noexcept
	{
		if (parameter.GetStartLocationPolicy() == StartLocationPolicy::UseMultiStart)
		{
			graph.StartNodeIndices = SelectMultiStartNodes(indexedRooms, graph.GoalNodeIndex, parameter.GetStartRoomCount());
		}
		else
		{
			const size_t selectedIndex = SelectSpatialStartNode(parameter.GetStartLocationPolicy(), indexedRooms, graph.GoalNodeIndex);
			graph.StartNodeIndices = selectedIndex < graph.Nodes.size() ? std::vector<size_t>{ selectedIndex } : std::vector<size_t>{};
		}

		if (graph.StartNodeIndices.empty())
			return false;
		graph.StartNodeIndex = graph.StartNodeIndices.front();
		return true;
	}

	/**
	 * Rebuilds the deterministic shortest main route between the selected primary start and goal.
	 * 選択された代表開始部屋とゴール部屋の間で、決定的な最短主経路を再構築します。
	 */
	bool LayoutGraphGenerator::RebuildMainRoute(const GenerateParameter& parameter, LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& rooms) noexcept
	{
		if (graph.StartNodeIndex >= graph.Nodes.size() || graph.GoalNodeIndex >= graph.Nodes.size() || rooms.size() != graph.Nodes.size() || graph.StartNodeIndex == graph.GoalNodeIndex)
			return false;

		using FAdjacentEdge = std::pair<size_t, size_t>;
		std::vector<std::vector<FAdjacentEdge>> adjacency(graph.Nodes.size());
		for (size_t edgeIndex = 0; edgeIndex < graph.Edges.size(); ++edgeIndex)
		{
			const LayoutAisleEdge& edge = graph.Edges[edgeIndex];
			if (edge.Room0 >= graph.Nodes.size() || edge.Room1 >= graph.Nodes.size())
				return false;
			adjacency[edge.Room0].emplace_back(edge.Room1, edgeIndex);
			adjacency[edge.Room1].emplace_back(edge.Room0, edgeIndex);
		}

		const size_t invalidIndex = std::numeric_limits<size_t>::max();
		std::vector<size_t> parentNode(graph.Nodes.size(), invalidIndex);
		std::vector<size_t> parentEdge(graph.Nodes.size(), invalidIndex);
		std::queue<size_t> pendingNodes;
		parentNode[graph.StartNodeIndex] = graph.StartNodeIndex;
		pendingNodes.emplace(graph.StartNodeIndex);
		while (!pendingNodes.empty() && parentNode[graph.GoalNodeIndex] == invalidIndex)
		{
			const size_t current = pendingNodes.front();
			pendingNodes.pop();
			for (const FAdjacentEdge& adjacent : adjacency[current])
			{
				if (parentNode[adjacent.first] != invalidIndex)
					continue;
				parentNode[adjacent.first] = current;
				parentEdge[adjacent.first] = adjacent.second;
				pendingNodes.emplace(adjacent.first);
			}
		}
		if (parentNode[graph.GoalNodeIndex] == invalidIndex)
			return false;

		std::vector<size_t> mainRouteNodes;
		std::vector<size_t> mainRouteEdges;
		for (size_t current = graph.GoalNodeIndex; current != graph.StartNodeIndex; current = parentNode[current])
		{
			mainRouteNodes.emplace_back(current);
			mainRouteEdges.emplace_back(parentEdge[current]);
		}
		mainRouteNodes.emplace_back(graph.StartNodeIndex);
		std::reverse(mainRouteNodes.begin(), mainRouteNodes.end());
		std::reverse(mainRouteEdges.begin(), mainRouteEdges.end());

		for (LayoutAisleEdge& edge : graph.Edges)
		{
			edge.bMainPath = false;
			if (edge.Purpose == EDungeonAislePurpose::MainPath || edge.Purpose == EDungeonAislePurpose::Locked)
			{
				edge.Purpose = EDungeonAislePurpose::Branch;
			}
		}

		for (size_t routeIndex = 0; routeIndex < mainRouteEdges.size(); ++routeIndex)
		{
			LayoutAisleEdge& edge = graph.Edges[mainRouteEdges[routeIndex]];
			edge.bMainPath = true;
			if (parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks && routeIndex + 1 == mainRouteEdges.size())
				edge.Purpose = EDungeonAislePurpose::Locked;
			else
				edge.Purpose = EDungeonAislePurpose::MainPath;
		}

		for (LayoutRoomNode& node : graph.Nodes)
		{
			node.GameplayRole = node.NonMainPathGameplayRole;
		}
		for (size_t routeIndex = 0; routeIndex < mainRouteNodes.size(); ++routeIndex)
		{
			graph.Nodes[mainRouteNodes[routeIndex]].GameplayRole = SelectMainPathGameplayRole(parameter, static_cast<int32>(routeIndex), static_cast<int32>(mainRouteNodes.size()));
		}
		return true;
	}

	EDungeonRoomGameplayRole LayoutGraphGenerator::SelectMainPathGameplayRole(const GenerateParameter& parameter, const int32 index, const int32 mainPathCount) noexcept
	{
		if (index == 0)
			return EDungeonRoomGameplayRole::None;
		if (index == mainPathCount - 1)
			return EDungeonRoomGameplayRole::None;
		const auto& path = parameter.GetPathSettings();
		if (path.ProgressionPolicy == EDungeonProgressionPolicy::BossRoute && index == mainPathCount - 2)
			return EDungeonRoomGameplayRole::Boss;
		if ((index % 5) == 0)
			return EDungeonRoomGameplayRole::Rest;
		if ((index % 3) == 0)
			return EDungeonRoomGameplayRole::Combat;
		return EDungeonRoomGameplayRole::None;
	}

	EDungeonRoomGameplayRole LayoutGraphGenerator::SelectBranchGameplayRole(const GenerateParameter& parameter, const int32 branchIndex) noexcept
	{
		const auto& profiles = GetRoomRoleProfiles(parameter);
		if (!profiles.IsEmpty())
		{
			const EDungeonRoomGameplayRole selectedRole = SelectWeightedGameplayRole(parameter, profiles);
			if (selectedRole != EDungeonRoomGameplayRole::None || std::any_of(profiles.begin(), profiles.end(), [](const FDungeonRoomRoleProfile& profile)
				{
					return profile.Role == EDungeonRoomGameplayRole::None && profile.BranchSelectionWeight > 0.f;
				}))
			{
				return selectedRole;
			}
		}

		const int32 roll = parameter.GetRandom()->Get<int32>(0, 100);
		if ((branchIndex % 5) == 4)
			return EDungeonRoomGameplayRole::Secret;
		if (roll < 45)
			return EDungeonRoomGameplayRole::Treasure;
		if (roll < 62)
			return EDungeonRoomGameplayRole::Puzzle;
		if (roll < 78)
			return EDungeonRoomGameplayRole::Rest;
		return EDungeonRoomGameplayRole::Combat;
	}

	void LayoutGraphGenerator::AssignStructuralRoles(const GenerateParameter& parameter, LayoutGraph& graph) noexcept
	{
		std::vector<int32> degree(graph.Nodes.size(), 0);
		std::vector<bool> mainRouteNodes(graph.Nodes.size(), false);
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 < degree.size())
				++degree[edge.Room0];
			if (edge.Room1 < degree.size())
				++degree[edge.Room1];
			if (edge.bMainPath && edge.Room0 < mainRouteNodes.size() && edge.Room1 < mainRouteNodes.size())
			{
				mainRouteNodes[edge.Room0] = true;
				mainRouteNodes[edge.Room1] = true;
			}
		}

		for (size_t index = 0; index < graph.Nodes.size(); ++index)
		{
			LayoutRoomNode& node = graph.Nodes[index];
			if (std::find(graph.StartNodeIndices.begin(), graph.StartNodeIndices.end(), index) != graph.StartNodeIndices.end())
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Start;
			}
			else if (index == graph.GoalNodeIndex)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Goal;
			}
			else if (parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::HubQuest && node.DesiredBranch == 0 && node.DesiredDepth == 1)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Hub;
			}
			else if (degree[index] >= 3)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Hub;
			}
			else if (degree[index] <= 1)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::DeadEnd;
			}
			else if (mainRouteNodes[index])
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Connector;
			}
			else if (node.DesiredBranch > 0)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Branch;
			}
			else
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Connector;
			}
		}
	}

	bool LayoutGraphGenerator::HasEdge(const LayoutGraph& graph, const size_t room0, const size_t room1) noexcept
	{
		return std::find_if(graph.Edges.begin(), graph.Edges.end(), [room0, room1](const LayoutAisleEdge& edge)
			{
				return
					(edge.Room0 == room0 && edge.Room1 == room1) ||
					(edge.Room0 == room1 && edge.Room1 == room0);
			}
		) != graph.Edges.end();
	}
}
