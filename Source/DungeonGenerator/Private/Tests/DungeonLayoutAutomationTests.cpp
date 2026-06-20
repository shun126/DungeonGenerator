/**
 * Automation tests for intent-driven dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GenerateParameter.h"
#include "Core/Generator.h"
#include "Core/Layout/AislePlanner.h"
#include "Core/Layout/LayoutEvaluator.h"
#include "Core/Layout/LayoutGraphGenerator.h"
#include "Core/MissionGraph/MissionGraphTester.h"
#include "Core/Layout/RoomPlacer.h"
#include "Core/Math/Point.h"
#include "Core/Math/Random.h"
#include "Core/PathGeneration/MinimumSpanningTree.h"
#include "Core/RoomGeneration/Room.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include <Misc/AutomationTest.h>
#include <algorithm>
#include <list>
#include <limits>
#include <memory>
#include <vector>

namespace
{
	void ConfigureBaseParameter(dungeon::GenerateParameter& parameter)
	{
		parameter.GetRandom()->SetSeed(126);
		parameter.SetNumberOfCandidateRooms(18);
		parameter.SetMinRoomWidth(3);
		parameter.SetMaxRoomWidth(8);
		parameter.SetMinRoomDepth(3);
		parameter.SetMaxRoomDepth(8);
		parameter.SetMinRoomHeight(2);
		parameter.SetMaxRoomHeight(4);
		parameter.SetHorizontalRoomMargin(2);
		parameter.SetVerticalRoomMargin(1);
		parameter.SetMissionGraph(false);
		parameter.SetAisleComplexity(0);
		parameter.SetGenerateSlopeInRoom(true);

		FDungeonPathSettings settings;
		settings.LayoutCandidateCount = 3;
		parameter.SetPathSettings(settings);
		parameter.SetLayoutCandidateCount(settings.LayoutCandidateCount);
	}

	void ConfigureRouteShape(dungeon::GenerateParameter& parameter, const float mainPathRatio, const float loopDensity)
	{
		ConfigureBaseParameter(parameter);

		auto settings = parameter.GetPathSettings();
		settings.MainRouteRatio = mainPathRatio;
		settings.LoopRouteDensity = loopDensity;
		parameter.SetPathSettings(settings);
	}

	void ConfigureVerticalFloorMode(dungeon::GenerateParameter& parameter)
	{
		ConfigureBaseParameter(parameter);

		auto settings = parameter.GetPathSettings();
		settings.MainRouteRatio = 0.60f;
		settings.LoopRouteDensity = 0.15f;
		parameter.SetPathSettings(settings);
		parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandVertically);
	}

	std::shared_ptr<dungeon::Room> MakeMissionTestRoom(const dungeon::Room::Parts parts, const dungeon::Room::Item item = dungeon::Room::Item::Empty)
	{
		auto room = std::make_shared<dungeon::Room>(FIntVector::ZeroValue, FIntVector(3, 3, 2));
		room->SetParts(parts);
		room->SetItem(item);
		return room;
	}

	dungeon::Aisle MakeMissionTestAisle(
		const std::shared_ptr<dungeon::Room>& room0,
		const std::shared_ptr<dungeon::Room>& room1,
		const bool bLocked = false,
		const bool bUniqueLocked = false)
	{
		auto point0 = std::make_shared<dungeon::Point>(room0);
		auto point1 = std::make_shared<dungeon::Point>(room1);
		dungeon::Aisle aisle(true, point0, point1);
		if (bUniqueLocked)
		{
			aisle.SetUniqueLock(true);
		}
		else if (bLocked)
		{
			aisle.SetLock(true);
		}
		return aisle;
	}

	std::vector<int32> PlaceFreeRoomZValues(const uint32 seed, const uint8 roomCount)
	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.GetRandom()->SetSeed(seed);
		parameter.SetNumberOfCandidateRooms(roomCount);
		parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandAnyDirection);

		auto graph = dungeon::LayoutGraphGenerator::Generate(parameter);
		const auto rooms = dungeon::RoomPlacer::Place(parameter, graph);
		std::vector<int32> zValues;
		zValues.reserve(rooms.size());
		for (const auto& room : rooms)
		{
			zValues.emplace_back(room->GetZ());
		}
		return zValues;
	}

	bool EvaluateProfileGraph(dungeon::GenerateParameter& parameter, FDungeonLayoutMetrics& outMetrics)
	{
		dungeon::LayoutCandidate candidate;
		candidate.Graph = dungeon::LayoutGraphGenerator::Generate(parameter);
		candidate.Rooms = dungeon::RoomPlacer::Place(parameter, candidate.Graph);
		if (!dungeon::AislePlanner::Plan(candidate.Graph, candidate.Rooms, candidate.Aisles, candidate.StartPoint, candidate.GoalPoint))
		{
			return false;
		}
		dungeon::LayoutEvaluator::Evaluate(parameter, 0, candidate);
		outMetrics = candidate.Metrics;
		return candidate.Score.bAccepted;
	}

	std::vector<std::shared_ptr<dungeon::Room>> CollectRooms(const std::shared_ptr<dungeon::Generator>& generator)
	{
		std::vector<std::shared_ptr<dungeon::Room>> rooms;
		generator->ForEach([&rooms](const std::shared_ptr<dungeon::Room>& room)
			{
				rooms.emplace_back(room);
			}
		);
		return rooms;
	}

	bool AreRoomsSeparated(const std::vector<std::shared_ptr<dungeon::Room>>& rooms, const uint32_t horizontalMargin, const uint32_t verticalMargin)
	{
		for (size_t roomIndex = 0; roomIndex < rooms.size(); ++roomIndex)
		{
			for (size_t otherRoomIndex = roomIndex + 1; otherRoomIndex < rooms.size(); ++otherRoomIndex)
			{
				if (rooms[roomIndex]->Intersect(*rooms[otherRoomIndex], horizontalMargin, verticalMargin))
				{
					return false;
				}
			}
		}
		return true;
	}

	bool AreConnectedRoomsSeparated(const std::shared_ptr<dungeon::Generator>& generator, const uint32_t horizontalMargin, const uint32_t verticalMargin)
	{
		auto bSeparated = true;
		generator->EachAisle([&bSeparated, horizontalMargin, verticalMargin](const dungeon::Aisle& aisle)
			{
				const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (room0 && room1 && room0 != room1 && room0->Intersect(*room1, horizontalMargin, verticalMargin))
				{
					bSeparated = false;
					return false;
				}
				return true;
			}
		);
		return bSeparated;
	}

	std::shared_ptr<const dungeon::Point> MakeEndpointPolicyPoint(const double x, const double y, const double z)
	{
		auto room = std::make_shared<dungeon::Room>(
			FIntVector(static_cast<int32>(x), static_cast<int32>(y), static_cast<int32>(z)),
			FIntVector(1, 1, 1));
		auto point = std::make_shared<dungeon::Point>(x, y, z);
		point->SetOwnerRoom(room);
		return point;
	}

	std::shared_ptr<const dungeon::Point> SelectEndpointPolicyPoint(const dungeon::StartLocationPolicy policy)
	{
		std::vector<std::shared_ptr<const dungeon::Point>> points;
		points.emplace_back(MakeEndpointPolicyPoint(-10.0, 0.0, 0.0));
		points.emplace_back(MakeEndpointPolicyPoint(10.0, 0.0, 0.0));
		points.emplace_back(MakeEndpointPolicyPoint(0.0, -12.0, 0.0));
		points.emplace_back(MakeEndpointPolicyPoint(0.0, 12.0, 0.0));
		points.emplace_back(MakeEndpointPolicyPoint(0.0, 0.0, 5.0));

		auto random = std::make_shared<dungeon::Random>();
		random->SetSeed(74);
		const dungeon::MinimumSpanningTree tree(random, points, 0, policy, 1);
		return tree.GetStartPoint();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonLayoutRouteGraphTest, "DungeonGenerator.Layout.RouteGraphMetrics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonLayoutRouteGraphTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	{
		dungeon::GenerateParameter parameter;
		ConfigureRouteShape(parameter, 0.80f, 0.05f);
		FDungeonLayoutMetrics metrics;
		TestTrue(TEXT("Long main-path graph is accepted"), EvaluateProfileGraph(parameter, metrics));
		TestTrue(TEXT("MainPathRatio increases main route length"), metrics.CriticalPathLength >= 12);
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureRouteShape(parameter, 0.50f, 0.45f);
		FDungeonLayoutMetrics metrics;
		TestTrue(TEXT("Loop-dense graph is accepted"), EvaluateProfileGraph(parameter, metrics));
		TestTrue(TEXT("Loop density increases loop count"), metrics.LoopCount >= 3);
	}

	{
		dungeon::GenerateParameter lowMainPathParameter;
		ConfigureRouteShape(lowMainPathParameter, 0.40f, 0.05f);
		FDungeonLayoutMetrics lowMainPathMetrics;
		TestTrue(TEXT("Low MainPathRatio graph is accepted"), EvaluateProfileGraph(lowMainPathParameter, lowMainPathMetrics));

		dungeon::GenerateParameter highMainPathParameter;
		ConfigureRouteShape(highMainPathParameter, 0.80f, 0.05f);
		FDungeonLayoutMetrics highMainPathMetrics;
		TestTrue(TEXT("High MainPathRatio graph is accepted"), EvaluateProfileGraph(highMainPathParameter, highMainPathMetrics));

		TestTrue(TEXT("Lower MainPathRatio creates more branch rooms"), lowMainPathMetrics.BranchCount > highMainPathMetrics.BranchCount);
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureVerticalFloorMode(parameter);
		FDungeonLayoutMetrics metrics;
		TestTrue(TEXT("Vertical floor-mode graph is accepted"), EvaluateProfileGraph(parameter, metrics));
		TestTrue(TEXT("Vertical floor-mode graph has vertical transitions"), metrics.VerticalTransitionCount >= 1);
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		auto route = parameter.GetPathSettings();
		route.LoopRouteDensity = 0.75f;
		parameter.SetPathSettings(route);
		auto progression = parameter.GetPathSettings();
		progression.ProgressionPolicy = EDungeonProgressionPolicy::KeysAndLocks;
		parameter.SetPathSettings(progression);
		FDungeonLayoutMetrics metrics;
		TestTrue(TEXT("Keys-and-locks graph is accepted"), EvaluateProfileGraph(parameter, metrics));
		TestTrue(TEXT("Keys-and-locks graph has a locked route"), metrics.LockedRouteCount >= 1);
		TestEqual(TEXT("Keys-and-locks disables unsafe requested loops"), metrics.LoopCount, 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonEndpointLocationPolicyTest, "DungeonGenerator.Layout.EndpointLocationPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonEndpointLocationPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	{
		const auto point = SelectEndpointPolicyPoint(dungeon::StartLocationPolicy::UseSouthernMost);
		TestTrue(TEXT("UseSouthernMost selects the largest Y endpoint"), point && FMath::IsNearlyEqual(point->Y, 12.0));
	}

	{
		const auto point = SelectEndpointPolicyPoint(dungeon::StartLocationPolicy::UseNorthernMost);
		TestTrue(TEXT("UseNorthernMost selects the smallest Y endpoint"), point && FMath::IsNearlyEqual(point->Y, -12.0));
	}

	{
		const auto point = SelectEndpointPolicyPoint(dungeon::StartLocationPolicy::UseEasternMost);
		TestTrue(TEXT("UseEasternMost selects the largest X endpoint"), point && FMath::IsNearlyEqual(point->X, 10.0));
	}

	{
		const auto point = SelectEndpointPolicyPoint(dungeon::StartLocationPolicy::UseWesternMost);
		TestTrue(TEXT("UseWesternMost selects the smallest X endpoint"), point && FMath::IsNearlyEqual(point->X, -10.0));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonLayoutCoreGenerationTest, "DungeonGenerator.Layout.CoreGenerationDeterministic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonLayoutCoreGenerationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::GenerateParameter parameter;
	ConfigureBaseParameter(parameter);
	parameter.SetLayoutCandidateCount(2);

	auto generator0 = std::make_shared<dungeon::Generator>();
	TestTrue(TEXT("First core dungeon generation succeeds"), generator0->Generate(parameter));
	const uint32 crc0 = generator0->CalculateCRC32();
	const FDungeonLayoutMetrics metrics0 = generator0->GetLastLayoutMetrics();

	ConfigureBaseParameter(parameter);
	parameter.SetLayoutCandidateCount(2);
	auto generator1 = std::make_shared<dungeon::Generator>();
	TestTrue(TEXT("Second core dungeon generation succeeds"), generator1->Generate(parameter));
	const uint32 crc1 = generator1->CalculateCRC32();
	const FDungeonLayoutMetrics metrics1 = generator1->GetLastLayoutMetrics();

	TestEqual(TEXT("Fixed seed generates identical CRC"), crc0, crc1);
	TestEqual(TEXT("Fixed seed generates identical critical path"), metrics0.CriticalPathLength, metrics1.CriticalPathLength);
	TestTrue(TEXT("Generated layout has rooms"), metrics0.RoomCount > 0);
	TestTrue(TEXT("Generated layout is solvable"), metrics0.bMissionSolvable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonKeysAndLocksGenerationTest, "DungeonGenerator.Layout.KeysAndLocksGeneration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonKeysAndLocksGenerationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::GenerateParameter parameter;
	ConfigureBaseParameter(parameter);
	parameter.SetNumberOfCandidateRooms(24);
	auto route = parameter.GetPathSettings();
	route.LoopRouteDensity = 0.75f;
	parameter.SetPathSettings(route);
	auto progression = parameter.GetPathSettings();
	progression.ProgressionPolicy = EDungeonProgressionPolicy::KeysAndLocks;
	parameter.SetPathSettings(progression);

	auto generator = std::make_shared<dungeon::Generator>();
	TestTrue(TEXT("Keys-and-locks dungeon generation succeeds"), generator->Generate(parameter));
	const FDungeonLayoutMetrics metrics = generator->GetLastLayoutMetrics();
	TestTrue(TEXT("Keys-and-locks dungeon has locked routes"), metrics.LockedRouteCount >= 1);
	TestEqual(TEXT("Keys-and-locks generation disables unsafe loops"), metrics.LoopCount, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonMissionGraphTesterSolvabilityTest, "DungeonGenerator.Layout.MissionGraphTesterSolvability", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonMissionGraphTesterSolvabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	{
		const auto start = MakeMissionTestRoom(dungeon::Room::Parts::Start);
		const auto key = MakeMissionTestRoom(dungeon::Room::Parts::Hall, dungeon::Room::Item::Key);
		const auto uniqueKey = MakeMissionTestRoom(dungeon::Room::Parts::Hall, dungeon::Room::Item::UniqueKey);
		const auto goal = MakeMissionTestRoom(dungeon::Room::Parts::Goal);
		std::list<std::shared_ptr<dungeon::Room>> rooms = { start, key, uniqueKey, goal };
		std::vector<dungeon::Aisle> aisles;
		aisles.emplace_back(MakeMissionTestAisle(start, key));
		aisles.emplace_back(MakeMissionTestAisle(key, uniqueKey, true));
		aisles.emplace_back(MakeMissionTestAisle(uniqueKey, goal, false, true));

		const dungeon::MissionGraphTester tester(rooms, aisles);
		TestTrue(TEXT("Valid key-lock route is solvable"), tester.Success());
	}

	{
		const auto start = MakeMissionTestRoom(dungeon::Room::Parts::Start, dungeon::Room::Item::Key);
		const auto uniqueKey = MakeMissionTestRoom(dungeon::Room::Parts::Hall, dungeon::Room::Item::UniqueKey);
		const auto goal = MakeMissionTestRoom(dungeon::Room::Parts::Goal);
		std::list<std::shared_ptr<dungeon::Room>> rooms = { start, uniqueKey, goal };
		std::vector<dungeon::Aisle> aisles;
		aisles.emplace_back(MakeMissionTestAisle(start, uniqueKey));
		aisles.emplace_back(MakeMissionTestAisle(uniqueKey, goal, false, true));

		const dungeon::MissionGraphTester tester(rooms, aisles);
		TestFalse(TEXT("Route with leftover common key fails"), tester.Success());
	}

	{
		const auto start = MakeMissionTestRoom(dungeon::Room::Parts::Start);
		const auto uniqueKey = MakeMissionTestRoom(dungeon::Room::Parts::Hall, dungeon::Room::Item::UniqueKey);
		const auto goal = MakeMissionTestRoom(dungeon::Room::Parts::Goal);
		std::list<std::shared_ptr<dungeon::Room>> rooms = { start, uniqueKey, goal };
		std::vector<dungeon::Aisle> aisles;
		aisles.emplace_back(MakeMissionTestAisle(start, uniqueKey, true));
		aisles.emplace_back(MakeMissionTestAisle(uniqueKey, goal, false, true));

		const dungeon::MissionGraphTester tester(rooms, aisles);
		TestFalse(TEXT("Common lock without a common key fails"), tester.Success());
	}

	{
		const auto start = MakeMissionTestRoom(dungeon::Room::Parts::Start);
		const auto goal = MakeMissionTestRoom(dungeon::Room::Parts::Goal);
		std::list<std::shared_ptr<dungeon::Room>> rooms = { start, goal };
		std::vector<dungeon::Aisle> aisles;
		aisles.emplace_back(MakeMissionTestAisle(start, goal, false, true));

		const dungeon::MissionGraphTester tester(rooms, aisles);
		TestFalse(TEXT("Unique lock without a unique key fails"), tester.Success());
	}

	{
		const auto start = MakeMissionTestRoom(dungeon::Room::Parts::Start, dungeon::Room::Item::Key);
		const auto uniqueKey = MakeMissionTestRoom(dungeon::Room::Parts::Hall, dungeon::Room::Item::UniqueKey);
		const auto goal = MakeMissionTestRoom(dungeon::Room::Parts::Goal);
		std::list<std::shared_ptr<dungeon::Room>> rooms = { start, uniqueKey, goal };
		std::vector<dungeon::Aisle> aisles;
		aisles.emplace_back(MakeMissionTestAisle(start, uniqueKey, true));
		aisles.emplace_back(MakeMissionTestAisle(start, uniqueKey));
		aisles.emplace_back(MakeMissionTestAisle(uniqueKey, goal, false, true));

		const dungeon::MissionGraphTester tester(rooms, aisles);
		TestFalse(TEXT("Loop bypassing a lock fails"), tester.Success());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonConnectedRoomSpacingCompactionTest, "DungeonGenerator.Layout.ConnectedRoomSpacingCompaction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonConnectedRoomSpacingCompactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetHorizontalRoomMargin(0);
		parameter.SetVerticalRoomMargin(0);

		auto generator = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Zero-margin compacted dungeon generation succeeds"), generator->Generate(parameter));
		const auto rooms = CollectRooms(generator);
		TestTrue(TEXT("Zero-margin compaction keeps all rooms separated"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
		TestTrue(TEXT("Zero-margin compaction keeps connected rooms separated"), AreConnectedRoomsSeparated(generator, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetHorizontalRoomMargin(3);
		parameter.SetVerticalRoomMargin(1);

		auto generator0 = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Positive-margin compacted dungeon generation succeeds"), generator0->Generate(parameter));
		const auto rooms = CollectRooms(generator0);
		TestTrue(TEXT("Positive-margin compaction keeps all rooms separated"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
		TestTrue(TEXT("Positive-margin compaction keeps connected rooms separated"), AreConnectedRoomsSeparated(generator0, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
		const uint32 crc0 = generator0->CalculateCRC32();

		ConfigureBaseParameter(parameter);
		parameter.SetHorizontalRoomMargin(3);
		parameter.SetVerticalRoomMargin(1);
		auto generator1 = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Positive-margin compacted generation is repeatable"), generator1->Generate(parameter));
		TestEqual(TEXT("Compaction is deterministic with a fixed seed"), crc0, generator1->CalculateCRC32());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonSeparateRoomCandidateScoringTest, "DungeonGenerator.Layout.SeparateRoomCandidateScoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonSeparateRoomCandidateScoringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::GenerateParameter parameter;
	ConfigureVerticalFloorMode(parameter);
	parameter.GetRandom()->SetSeed(218);
	parameter.SetNumberOfCandidateRooms(28);
	parameter.SetMinRoomWidth(5);
	parameter.SetMaxRoomWidth(9);
	parameter.SetMinRoomDepth(5);
	parameter.SetMaxRoomDepth(9);
	parameter.SetHorizontalRoomMargin(2);
	parameter.SetVerticalRoomMargin(1);
	parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandVertically);

	auto generator0 = std::make_shared<dungeon::Generator>();
	TestTrue(TEXT("Dense vertical generation succeeds with collision-aware separation scoring"), generator0->Generate(parameter));
	const auto rooms = CollectRooms(generator0);
	TestTrue(TEXT("Dense vertical generation keeps configured margins"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
	const auto crc0 = generator0->CalculateCRC32();

	ConfigureVerticalFloorMode(parameter);
	parameter.GetRandom()->SetSeed(218);
	parameter.SetNumberOfCandidateRooms(28);
	parameter.SetMinRoomWidth(5);
	parameter.SetMaxRoomWidth(9);
	parameter.SetMinRoomDepth(5);
	parameter.SetMaxRoomDepth(9);
	parameter.SetHorizontalRoomMargin(2);
	parameter.SetVerticalRoomMargin(1);
	parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandVertically);

	auto generator1 = std::make_shared<dungeon::Generator>();
	TestTrue(TEXT("Dense vertical generation remains repeatable"), generator1->Generate(parameter));
	TestEqual(TEXT("Collision-aware separation scoring is deterministic"), crc0, generator1->CalculateCRC32());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonFloorModeSimplificationTest, "DungeonGenerator.Layout.FloorModeSimplification", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonFloorModeSimplificationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::GenerateParameter coreParameter;
	TestTrue(TEXT("Core generation defaults to unrestricted expansion"), coreParameter.GetExpansionPolicy() == dungeon::ExpansionPolicy::ExpandAnyDirection);

	auto* parameter = NewObject<UDungeonGenerateParameter>();
	auto& structure = const_cast<FDungeonStructureSettings&>(parameter->GetStructureSettings());
	TestTrue(TEXT("DungeonGenerateParameter defaults to Free floor mode"), structure.FloorMode == EDungeonFloorMode::Free);

	structure.HorizontalRoomMargin = 0;
	structure.VerticalRoomMargin = 2;
	TestEqual(TEXT("Structure keeps zero horizontal margin"), parameter->GetHorizontalRoomMargin(), 0);

	structure.FloorMode = EDungeonFloorMode::Free;
	TestEqual(TEXT("Free layouts keep vertical margin"), parameter->GetVerticalRoomMargin(), 2);

	structure.FloorMode = EDungeonFloorMode::Vertical;
	TestEqual(TEXT("Vertical layouts keep vertical margin"), parameter->GetVerticalRoomMargin(), 2);

	structure.FloorMode = EDungeonFloorMode::Flat;
	TestEqual(TEXT("Flat layouts ignore vertical margin"), parameter->GetVerticalRoomMargin(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonLegacyFloorModeMigrationTest, "DungeonGenerator.Layout.LegacyFloorModeMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonLegacyFloorModeMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const auto testMigration = [this](const TCHAR* What, const EDungeonExpansionPolicy LegacyPolicy, const bool bLegacyFlat, const EDungeonFloorMode ExpectedMode)
	{
		auto* parameter = NewObject<UDungeonGenerateParameter>();
		parameter->NumberOfCandidateRooms = 11;
		parameter->ExpansionPolicy = LegacyPolicy;
		parameter->Flat = bLegacyFlat;
		parameter->MigrateLegacyTopLevelProperties();
		TestTrue(What, parameter->GetStructureSettings().FloorMode == ExpectedMode);
	};

	testMigration(TEXT("Legacy Flat toggle migrates to Flat"), EDungeonExpansionPolicy::ExpandHorizontally, true, EDungeonFloorMode::Flat);
	testMigration(TEXT("Legacy Flat policy migrates to Flat"), EDungeonExpansionPolicy::Flat, false, EDungeonFloorMode::Flat);
	testMigration(TEXT("Legacy ExpandHorizontally migrates to Free"), EDungeonExpansionPolicy::ExpandHorizontally, false, EDungeonFloorMode::Free);
	testMigration(TEXT("Legacy ExpandAnyDirection migrates to Free"), EDungeonExpansionPolicy::ExpandAnyDirection, false, EDungeonFloorMode::Free);
	testMigration(TEXT("Legacy ExpandVertically migrates to Vertical"), EDungeonExpansionPolicy::ExpandVertically, false, EDungeonFloorMode::Vertical);

	{
		auto* parameter = NewObject<UDungeonGenerateParameter>();
		parameter->NumberOfCandidateRooms = 11;
		parameter->GridSize = 512.f;
		parameter->VerticalGridSize = 256.f;
		parameter->LayoutCandidateCount = 7;
		parameter->AisleComplexity = 4;
		parameter->AisleCeilingHeightPolicy = EDungeonAisleCeilingHeightPolicy::TwoGrids;
		parameter->UseMissionGraph = true;
		parameter->MigrateLegacyTopLevelProperties();
		TestEqual(TEXT("Legacy GridSize migrates to Theme.HorizontalGridSize"), parameter->GetGridSize().HorizontalSize, 512.f);
		TestEqual(TEXT("Legacy VerticalGridSize migrates to Theme.VerticalGridSize"), parameter->GetGridSize().VerticalSize, 256.f);
		TestEqual(TEXT("Legacy LayoutCandidateCount migrates to Path.LayoutCandidateCount"), parameter->GetPathSettings().LayoutCandidateCount, 7);
		TestEqual(TEXT("Legacy AisleComplexity migrates to Path.ExtraCorridorComplexity"), parameter->GetPathSettings().ExtraCorridorComplexity, 4);
		TestTrue(TEXT("Legacy AisleCeilingHeightPolicy migrates to Path.CorridorCeilingHeightPolicy"), parameter->GetPathSettings().CorridorCeilingHeightPolicy == EDungeonAisleCeilingHeightPolicy::TwoGrids);
		TestTrue(TEXT("Legacy UseMissionGraph migrates to KeysAndLocks"), parameter->GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks);
	}


	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonVerticalFloorModePlacementTest, "DungeonGenerator.Layout.VerticalFloorModePlacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonVerticalFloorModePlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::GenerateParameter parameter;
	ConfigureVerticalFloorMode(parameter);

	auto graph = dungeon::LayoutGraphGenerator::Generate(parameter);
	const auto rooms = dungeon::RoomPlacer::Place(parameter, graph);
	std::vector<std::shared_ptr<dungeon::Room>> placedRooms;
	for (const auto& room : rooms)
	{
		placedRooms.emplace_back(room);
	}

	TestEqual(TEXT("Vertical mode places every generated room"), static_cast<int32>(placedRooms.size()), static_cast<int32>(parameter.GetNumberOfCandidateRooms()));

	auto bAllRoomsStayInVerticalColumn = true;
	auto bNoRoomsOverlapVertically = true;
	auto bUsesMultipleVerticalLayers = false;
	const auto maxVerticalXOffset = static_cast<int32>(parameter.GetMaxRoomWidth());
	const auto firstRoomZ = placedRooms.empty() ? 0 : placedRooms[0]->GetZ();
	for (size_t roomIndex = 0; roomIndex < placedRooms.size(); ++roomIndex)
	{
		const auto& room = placedRooms[roomIndex];
		bAllRoomsStayInVerticalColumn &= room->GetY() == 0 && room->GetX() >= -maxVerticalXOffset && room->GetX() <= maxVerticalXOffset;
		bUsesMultipleVerticalLayers |= room->GetZ() != firstRoomZ;
		for (size_t otherRoomIndex = roomIndex + 1; otherRoomIndex < placedRooms.size(); ++otherRoomIndex)
		{
			const auto& otherRoom = placedRooms[otherRoomIndex];
			bNoRoomsOverlapVertically &= room->GetForeground() <= otherRoom->GetBackground() || otherRoom->GetForeground() <= room->GetBackground();
		}
	}

	TestTrue(TEXT("Vertical mode keeps rooms near the central X column"), bAllRoomsStayInVerticalColumn);
	TestTrue(TEXT("Vertical mode spreads initial rooms across multiple heights"), bUsesMultipleVerticalLayers);
	TestTrue(TEXT("Vertical mode keeps rooms from overlapping vertically"), bNoRoomsOverlapVertically);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonSeparateRoomFloorModeGenerationTest, "DungeonGenerator.Layout.SeparateRoomFloorModeGeneration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonSeparateRoomFloorModeGenerationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::Flat);
		parameter.SetVerticalRoomMargin(0);

		auto generator = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Flat dungeon generation succeeds"), generator->Generate(parameter));
		const auto rooms = CollectRooms(generator);
		auto bAllRoomsStayOnGround = true;
		for (const auto& room : rooms)
		{
			bAllRoomsStayOnGround &= room->GetZ() == 0;
		}

		TestTrue(TEXT("Flat layouts keep every room bottom on Z=0"), bAllRoomsStayOnGround);
		TestTrue(TEXT("Flat layouts keep horizontal room margins"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureVerticalFloorMode(parameter);

		auto generator = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Vertical dungeon generation succeeds"), generator->Generate(parameter));
		const auto rooms = CollectRooms(generator);
		auto minCenterY = std::numeric_limits<double>::max();
		auto maxCenterY = std::numeric_limits<double>::lowest();
		for (const auto& room : rooms)
		{
			const auto centerY = static_cast<double>(room->GetCenter().Y);
			minCenterY = std::min(minCenterY, centerY);
			maxCenterY = std::max(maxCenterY, centerY);
		}

		TestTrue(TEXT("Vertical layouts keep room centers on the nearest shared Y band"), maxCenterY - minCenterY <= 0.5);
		TestTrue(TEXT("Vertical layouts keep horizontal and vertical room margins"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandAnyDirection);

		auto generator = std::make_shared<dungeon::Generator>();
		TestTrue(TEXT("Free multi-floor dungeon generation succeeds"), generator->Generate(parameter));
		const auto rooms = CollectRooms(generator);
		auto bUsesMultipleHeights = false;
		const auto firstRoomZ = rooms.empty() ? 0 : rooms[0]->GetZ();
		for (const auto& room : rooms)
		{
			bUsesMultipleHeights |= room->GetZ() != firstRoomZ;
		}

		TestTrue(TEXT("Free layouts can keep vertical placement unlocked"), bUsesMultipleHeights);
		TestTrue(TEXT("Free layouts keep horizontal and vertical room margins"), AreRoomsSeparated(rooms, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonFreeFloorModeSmoothHeightFieldTest, "DungeonGenerator.Layout.FreeFloorModeSmoothHeightField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDungeonFreeFloorModeSmoothHeightFieldTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const auto zValues0 = PlaceFreeRoomZValues(126, 24);
	const auto zValues1 = PlaceFreeRoomZValues(126, 24);
	const auto zValues2 = PlaceFreeRoomZValues(127, 24);
	TestEqual(TEXT("Free height field is deterministic for the same seed"), zValues0, zValues1);
	TestTrue(TEXT("Free height field can vary when the seed changes"), zValues0 != zValues2);

	const int32 verticalSpacing = 4 + 1;
	const auto smallFloorZValues = PlaceFreeRoomZValues(126, 3);
	for (const int32 z : smallFloorZValues)
	{
		TestEqual(TEXT("Small Free height field stays on the ground floor"), z, 0);
		TestEqual(TEXT("Free height field snaps to vertical room spacing"), z % verticalSpacing, 0);
	}

	auto bUsesMultipleHeights = false;
	for (const int32 z : zValues0)
	{
		bUsesMultipleHeights |= z != 0;
		TestTrue(TEXT("Auto Free height field stays inside the internal floor spread"), z >= 0 && z <= verticalSpacing * 2);
		TestEqual(TEXT("Auto Free height field snaps to vertical room spacing"), z % verticalSpacing, 0);
	}
	TestTrue(TEXT("Auto Free height field can use multiple heights"), bUsesMultipleHeights);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonRoomRoleProfileBranchSelectionTest, "DungeonGenerator.Layout.RoomRoleProfileBranchSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDungeonRoomRoleProfileBranchSelectionTest::RunTest(const FString& Parameters)
{
	dungeon::GenerateParameter parameter;
	ConfigureBaseParameter(parameter);
	parameter.GetRandom()->SetSeed(861);
	parameter.SetNumberOfCandidateRooms(24);

	auto route = parameter.GetPathSettings();
	route.MainRouteRatio = 0.45f;
	route.LoopRouteDensity = 0.f;
	parameter.SetPathSettings(route);

	FDungeonRoomRoleSettings roleSettings;
	FDungeonRoomRoleProfile secretProfile;
	secretProfile.Role = EDungeonRoomGameplayRole::Secret;
	secretProfile.BranchSelectionWeight = 100.f;
	roleSettings.Roles.Add(secretProfile);

	FDungeonRoomRoleProfile combatProfile;
	combatProfile.Role = EDungeonRoomGameplayRole::Combat;
	combatProfile.BranchSelectionWeight = 0.f;
	roleSettings.Roles.Add(combatProfile);
	parameter.SetRoomRoleSettings(roleSettings);

	const dungeon::LayoutGraph graph = dungeon::LayoutGraphGenerator::Generate(parameter);
	int32 branchRoomCount = 0;
	int32 secretBranchRoomCount = 0;
	for (const dungeon::LayoutRoomNode& node : graph.Nodes)
	{
		if (node.DesiredBranch <= 0)
		{
			continue;
		}
		++branchRoomCount;
		if (node.GameplayRole == EDungeonRoomGameplayRole::Secret)
		{
			++secretBranchRoomCount;
		}
	}

	TestTrue(TEXT("Profile weighting creates branch rooms"), branchRoomCount > 0);
	TestEqual(TEXT("Secret-weighted RoomRoles assign Secret to every generated branch room"), secretBranchRoomCount, branchRoomCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonZoneSelectionWeightTest, "DungeonGenerator.Layout.ZoneSelectionWeight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDungeonZoneSelectionWeightTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	auto makeFullRangeZone = [](const TCHAR* name, const float weight)
	{
		FDungeonZoneDefinition zone;
		zone.Name = FName(name);
		zone.ProgressRange.Min = 0.f;
		zone.ProgressRange.Max = 1.f;
		zone.FloorRange.Min = 0;
		zone.FloorRange.Max = MAX_int32;
		zone.SelectionWeight = weight;
		return zone;
	};

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetNumberOfCandidateRooms(12);

		FDungeonZoneSettings zoneSettings;
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("Single"), 1.f));
		parameter.SetZoneSettings(zoneSettings);

		const dungeon::LayoutGraph graph = dungeon::LayoutGraphGenerator::Generate(parameter);
		for (const dungeon::LayoutRoomNode& node : graph.Nodes)
		{
			TestEqual(TEXT("Single matching zone keeps first matching zone assignment"), node.ZoneIndex, 0);
		}
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetNumberOfCandidateRooms(12);

		FDungeonZoneSettings zoneSettings;
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("Disabled"), 0.f));
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("Selected"), 100.f));
		parameter.SetZoneSettings(zoneSettings);

		const dungeon::LayoutGraph graph = dungeon::LayoutGraphGenerator::Generate(parameter);
		for (const dungeon::LayoutRoomNode& node : graph.Nodes)
		{
			TestEqual(TEXT("Zero-weight overlapping zone is not selected"), node.ZoneIndex, 1);
		}
	}

	{
		dungeon::GenerateParameter parameter;
		ConfigureBaseParameter(parameter);
		parameter.SetNumberOfCandidateRooms(12);

		FDungeonZoneSettings zoneSettings;
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("DisabledA"), 0.f));
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("DisabledB"), 0.f));
		parameter.SetZoneSettings(zoneSettings);

		const dungeon::LayoutGraph graph = dungeon::LayoutGraphGenerator::Generate(parameter);
		for (const dungeon::LayoutRoomNode& node : graph.Nodes)
		{
			TestEqual(TEXT("All zero-weight matching zones produce no zone assignment"), node.ZoneIndex, INDEX_NONE);
		}
	}

	{
		dungeon::GenerateParameter baselineParameter;
		ConfigureBaseParameter(baselineParameter);
		baselineParameter.GetRandom()->SetSeed(2468);
		baselineParameter.SetNumberOfCandidateRooms(16);

		dungeon::GenerateParameter zonedParameter;
		ConfigureBaseParameter(zonedParameter);
		zonedParameter.GetRandom()->SetSeed(2468);
		zonedParameter.SetNumberOfCandidateRooms(16);

		FDungeonZoneSettings zoneSettings;
		zoneSettings.Zones.Add(makeFullRangeZone(TEXT("DefaultWeight"), 1.f));
		zonedParameter.SetZoneSettings(zoneSettings);

		const dungeon::LayoutGraph baselineGraph = dungeon::LayoutGraphGenerator::Generate(baselineParameter);
		const dungeon::LayoutGraph zonedGraph = dungeon::LayoutGraphGenerator::Generate(zonedParameter);

		TestEqual(TEXT("Single matching default-weight zone preserves node count"), static_cast<int32>(zonedGraph.Nodes.size()), static_cast<int32>(baselineGraph.Nodes.size()));
		TestEqual(TEXT("Single matching default-weight zone preserves edge count"), static_cast<int32>(zonedGraph.Edges.size()), static_cast<int32>(baselineGraph.Edges.size()));
		for (size_t index = 0; index < baselineGraph.Nodes.size() && index < zonedGraph.Nodes.size(); ++index)
		{
			TestEqual(TEXT("Single matching default-weight zone does not consume RNG before gameplay role selection"), zonedGraph.Nodes[index].GameplayRole, baselineGraph.Nodes[index].GameplayRole);
			TestEqual(TEXT("Single matching default-weight zone preserves parent selection"), zonedGraph.Nodes[index].ParentIndex, baselineGraph.Nodes[index].ParentIndex);
			TestEqual(TEXT("Single matching default-weight zone assigns the matching zone"), zonedGraph.Nodes[index].ZoneIndex, 0);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonRoomItemGameplayRoleTest, "DungeonGenerator.Layout.RoomItemGameplayRole", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDungeonRoomItemGameplayRoleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	dungeon::Room keyRoom(FIntVector::ZeroValue, FIntVector(4, 4, 2));
	keyRoom.SetGameplayRole(EDungeonRoomGameplayRole::Combat);
	keyRoom.SetItem(dungeon::Room::Item::Key);
	TestEqual(TEXT("Key rooms are marked as Treasure gameplay role"), keyRoom.GetGameplayRole(), EDungeonRoomGameplayRole::Treasure);

	dungeon::Room uniqueKeyRoom(FIntVector::ZeroValue, FIntVector(4, 4, 2));
	uniqueKeyRoom.SetGameplayRole(EDungeonRoomGameplayRole::Rest);
	uniqueKeyRoom.SetItem(dungeon::Room::Item::UniqueKey);
	TestEqual(TEXT("UniqueKey rooms are marked as Treasure gameplay role"), uniqueKeyRoom.GetGameplayRole(), EDungeonRoomGameplayRole::Treasure);

	dungeon::Room emptyTreasureRoom(FIntVector::ZeroValue, FIntVector(4, 4, 2));
	emptyTreasureRoom.SetGameplayRole(EDungeonRoomGameplayRole::Treasure);
	emptyTreasureRoom.SetItem(dungeon::Room::Item::Empty);
	TestEqual(TEXT("Empty item does not clear an existing Treasure gameplay role"), emptyTreasureRoom.GetGameplayRole(), EDungeonRoomGameplayRole::Treasure);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonThemeOverrideResolverTest, "DungeonGenerator.Layout.ThemeOverrideResolver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDungeonThemeOverrideResolverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;


	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonRoomSensorEnemySpawnMultiplierTest, "DungeonGenerator.RoomSensor.EnemySpawnMultiplier", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDungeonRoomSensorEnemySpawnMultiplierTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	constexpr int32 baseCount = 10;
	const FDungeonGameplayRoleEnemySpawnMultipliers defaultGameplayMultipliers;
	const FDungeonStructuralRoleEnemySpawnMultipliers defaultStructuralMultipliers;

	FDungeonGeneratedRoomInfo roomInfo;
	roomInfo.RoomStructuralRole = EDungeonRoomStructuralRole::Connector;
	const FBox idealCountBounds(FVector::ZeroVector, FVector(5000.f, 500.f, 100.f));

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::None;
	TestEqual(TEXT("None uses default 0.5 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 5);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Combat;
	TestEqual(TEXT("Combat uses default 1.0 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 10);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Treasure;
	TestEqual(TEXT("Treasure uses default 0.8 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 8);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Puzzle;
	TestEqual(TEXT("Puzzle uses default 0.5 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 5);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Rest;
	TestEqual(TEXT("Rest uses default 0.0 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Boss;
	TestEqual(TEXT("Boss uses default 2.0 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 20);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Secret;
	TestEqual(TEXT("Secret uses default 0.7 enemy spawn multiplier"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 7);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Combat;
	roomInfo.RoomStructuralRole = EDungeonRoomStructuralRole::Connector;
	TestEqual(TEXT("IdealNumberOfActor keeps Combat Connector at the area-based count"), ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(idealCountBounds, 500.f, 10, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 10);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Rest;
	TestEqual(TEXT("IdealNumberOfActor applies Rest gameplay role multiplier"), ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(idealCountBounds, 500.f, 10, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Boss;
	TestEqual(TEXT("IdealNumberOfActor applies Boss gameplay role multiplier"), ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(idealCountBounds, 500.f, 10, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 20);

	roomInfo.RoomGameplayRole = EDungeonRoomGameplayRole::Combat;
	roomInfo.RoomStructuralRole = EDungeonRoomStructuralRole::Start;
	TestEqual(TEXT("Start structural role disables enemy spawning by default"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);
	TestEqual(TEXT("IdealNumberOfActor applies Start structural role multiplier"), ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(idealCountBounds, 500.f, 10, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);

	roomInfo.RoomStructuralRole = EDungeonRoomStructuralRole::Goal;
	TestEqual(TEXT("Goal structural role disables enemy spawning by default"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);
	TestEqual(TEXT("IdealNumberOfActor applies Goal structural role multiplier"), ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(idealCountBounds, 500.f, 10, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 0);

	roomInfo.RoomStructuralRole = EDungeonRoomStructuralRole::Connector;
	FDungeonGameplayRoleEnemySpawnMultipliers gameplayOverrides;
	gameplayOverrides.Combat_ = 0.25f;
	FDungeonStructuralRoleEnemySpawnMultipliers structuralOverrides;
	structuralOverrides.Connector = 0.5f;
	TestEqual(TEXT("Gameplay and structural override multipliers replace defaults"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, gameplayOverrides, structuralOverrides), 1);

	roomInfo.bMainPathRoom = true;
	roomInfo.bDeadEndRoom = true;
	roomInfo.bLockedRouteRoom = true;
	roomInfo.bSecretRoom = true;
	TestEqual(TEXT("Room information flags do not change the enemy spawn count"), ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(baseCount, roomInfo, defaultGameplayMultipliers, defaultStructuralMultipliers), 10);

	return true;
}

#endif
