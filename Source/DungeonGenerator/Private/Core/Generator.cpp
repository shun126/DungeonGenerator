/**
ダンジョン生成ソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Generator.h"
#include "GenerateParameter.h"
#include "Debug/Config.h"
#include "Debug/Debug.h"
#include "Helper/Finalizer.h"
#include "Helper/Stopwatch.h"
#include "Math/Math.h"
#include "Math/Plane.h"
#include "Math/Vector.h"
#include "PathGeneration/DelaunayTriangulation3D.h"
#include "PathGeneration/MinimumSpanningTree.h"
#include "PathGeneration/PathGoalCondition.h"
#include "Voxelization/RoomStructureGenerator.h"
#include "Voxelization/Voxel.h"

#include "MissionGraph/MissionGraph.h"
#include "MissionGraph/MissionGraphTester.h"

namespace dungeon
{
	void Generator::Reset()
	{
		mVoxel.reset();
		mRooms.clear();
		mFloorHeight.clear();
		mStartRoom.reset();
		mGoalRoom.reset();

		mStartPoint.reset();
		mGoalPoint.reset();

		mAisles.clear();
		mDeepestDepthFromStart = 0;
		mLastError = Generator::Error::Success;
	}

	bool Generator::Generate(const GenerateParameter& parameter) noexcept
	{
		Identifier::ResetCounter();
		mLastError = Error::Success;
		mGenerateParameter = parameter;

		// 生成
		// TODO:リトライする仕組みの検討をして下さい。部屋の間隔を広げると成功する可能性が上がるかもしれません。
		size_t retryCount = 1;
		while (!GenerateImpl())
		{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
			// 部屋の情報をダンプ
			for (const auto& room : mRooms)
			{
				DUNGEON_GENERATOR_LOG(TEXT("Room: %d,Position(%d, %d, %d) Size(%d, %d, %d) Parts=%d, DepthFromStart=%d"),
					room->GetIdentifier().Get(),
					room->GetX(), room->GetY(), room->GetZ(),
					room->GetWidth(), room->GetDepth(), room->GetHeight(),
					room->GetParts(),
					room->GetDepthFromStart()
				);
			}

			// 通路の情報をダンプ
			for (const auto& aisle : mAisles)
			{
				DUNGEON_GENERATOR_LOG(TEXT("Aisle: %s - %s")
					, UTF8_TO_TCHAR(aisle.GetPoint(0)->GetOwnerRoom()->GetName().data())
					, UTF8_TO_TCHAR(aisle.GetPoint(1)->GetOwnerRoom()->GetName().data())
				);
			}

			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/dungeon_aisle.md");
#endif
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			check(mLastError == Error::Success);
#endif

			if (--retryCount == 0)
				break;

			mGenerateParameter.SetHorizontalRoomMargin(mGenerateParameter.GetHorizontalRoomMargin() + 2);
			mLastError = Error::Success;

			Reset();
		}

		// エラー情報を記録
		if (mLastError != Generator::Error::Success)
		{
			if (mVoxel && mVoxel->GetLastError() != Voxel::Error::Success)
			{
				uint8_t errorIndex = static_cast<uint8_t>(Error::___StartVoxelError);
				errorIndex += static_cast<uint8_t>(mVoxel->GetLastError());
				mLastError = static_cast<Error>(errorIndex);
			}
		}

		return mLastError == Error::Success;
	}

	bool Generator::GenerateImpl() noexcept
	{
		// 部屋の生成
		if (GenerateRooms() == false)
			return false;

		// 部屋の分離
		if (SeparateRooms(2, 0) == SeparateRoomsResult::Failed)
			return false;

		SeparateRoomsResult separateRoomsResult;
		uint8_t subPhase = 0;
		do {
			// 通路の生成
			if (ExtractionAisles() == false)
				return false;

			// 開始部屋と終了部屋のサブレベルを配置する隙間を調整
			if (AdjustedStartAndGoalSubLevel() == false)
				return false;

			// 部屋の大きさを調整する
			AdjustRoomSize();

			// 部屋の分離
			separateRoomsResult = SeparateRooms(6, subPhase);
			if (separateRoomsResult == SeparateRoomsResult::Failed)
				return false;
			++subPhase;
		} while (separateRoomsResult != SeparateRoomsResult::Completed);

		// 全ての部屋が収まるように空間を拡張します
		if (ExpandSpace() == false)
			return false;

		// Pointの同期
		AdjustPoints();

		// ブランチIDと各部屋の深さの生成
		if (MarkBranchIdAndDepthFromStart() == false)
			return false;

		// 階層情報と全体の深さの生成
		if (DetectFloorHeightAndDepthFromStart() == false)
			return false;

		// 部屋と通路に意味付けする
		if (mGenerateParameter.UseMissionGraph())
		{
			float maxKeyCount = std::sqrt(static_cast<float>(mRooms.size()));
			maxKeyCount = std::ceil(maxKeyCount);
			if (maxKeyCount < 2)
				maxKeyCount = 2;

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			Stopwatch stopwatch;
#endif
			MissionGraph missionGraph(shared_from_this(), mStartRoom, mGoalRoom, maxKeyCount);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: %lf seconds"), stopwatch.Lap());
#endif

#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
			// デバッグ情報を出力
			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/dungeon_diagram.md");
#endif

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			// クリアできるミッションかテストします
			const MissionGraphTester missionGraphTester(mRooms, mAisles);
			check(missionGraphTester.Success() == true);
#endif
		}
		else
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: Skipped"));

#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
			// デバッグ情報を出力
			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/dungeon_diagram.md");
#endif
		}

		// スタート部屋とゴール部屋のコールバックを呼ぶ
		InvokeRoomCallbacks();

		// ボクセル情報を生成します
		if (GenerateVoxel() == false)
			return false;

		return true;
	}

	/*
	正規乱数
	https://ja.wikipedia.org/wiki/%E3%83%9C%E3%83%83%E3%82%AF%E3%82%B9%EF%BC%9D%E3%83%9F%E3%83%A5%E3%83%A9%E3%83%BC%E6%B3%95
	*/
	inline void NormalRandom(float& x, float& y, const std::shared_ptr<Random>& random)
	{
		const auto A = random->Get<float>();
		const auto B = random->Get<float>();
		const auto X = std::sqrt(-2.f * std::log(A));
		const auto Y = 2.f * 3.14159265359f * B;
		x = X * std::cos(Y);
		y = X * std::sin(Y);
	}

	/**
	mRoomsを生成します
	*/
	bool Generator::GenerateRooms() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("GenerateRooms: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Generate Rooms"));
#endif

		float radius = std::sqrtf(mGenerateParameter.GetNumberOfCandidateRooms());
		const float maxRoomWidth = std::max(mGenerateParameter.GetMaxRoomWidth(), mGenerateParameter.GetMaxRoomDepth());
		radius *= maxRoomWidth + mGenerateParameter.GetHorizontalRoomMargin();
		radius *= 0.5f;
		if (radius < 1.f)
			radius = 1.f;

#define USE_SIN_CURVE 1

		const float mapOffsetX = mGenerateParameter.GetRandom()->Get<float>(dungeon::math::Pi2<float>());
		const float mapOffsetY = mGenerateParameter.GetRandom()->Get<float>(dungeon::math::Pi2<float>());
		const float mapScaleX = mGenerateParameter.GetRandom()->Get<float>(0.5f, 1.5f);
		const float mapScaleY = mGenerateParameter.GetRandom()->Get<float>(0.5f, 1.5f);
#if USE_SIN_CURVE
		const auto getHeight = [mapOffsetX, mapOffsetY, mapScaleX, mapScaleY](const float x, const float y) -> float
			{
				const float tx = mapOffsetX + x * mapScaleX * dungeon::math::Pi2();
				const float ty = mapOffsetY + y * mapScaleY * dungeon::math::Pi2();
				const float noise = std::sin(tx) * std::sin(ty);
				return noise * 0.5f + 0.5f;
			};
#else
		PerlinNoise perlinNoise(mGenerateParameter.GetRandom());
		constexpr float noiseBoostRatio = 1.333f;
		constexpr size_t octaves = 4;
#endif

		// Register Rooms
		for (size_t i = 0; i < mGenerateParameter.GetNumberOfCandidateRooms(); ++i)
		{
			float x, y;
			NormalRandom(x, y, mGenerateParameter.GetRandom());
			x = std::max(-1.f, std::min(x / 4.f, 1.f));
			y = std::max(-1.f, std::min(y / 4.f, 1.f));
#if USE_SIN_CURVE
			const float z = getHeight(x, y);

			FIntVector location(
				static_cast<int32_t>(std::round(x * radius)),
				static_cast<int32_t>(std::round(y * radius)),
				static_cast<int32_t>(std::round(z * mGenerateParameter.GetNumberOfCandidateFloors()))
			);
#else
			float noise = perlinNoise.OctaveNoise(octaves, x * 0.5, y * 0.5);
			noise = noise * 0.5f + 0.5f;
			noise *= noiseBoostRatio;
			noise = std::max(0.f, std::min(noise, 1.f));

			FIntVector location(
				static_cast<int32_t>(std::round(x * radius)),
				static_cast<int32_t>(std::round(y * radius)),
				static_cast<int32_t>(std::round(noise * mGenerateParameter.GetNumberOfCandidateFloors()))
			);
#endif

			if (mGenerateParameter.GetHorizontalRoomMargin() == 0)
			{
				/*
				RoomMarginが0の場合、部屋と部屋の間にスロープを作る隙間が無いので、
				部屋の高さを必ず同じにする。
				*/
				location.Z = 0;
			}

			std::shared_ptr<Room> room = std::make_shared<Room>(mGenerateParameter, location);
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
			DUNGEON_GENERATOR_LOG(TEXT("Room: %d,X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d center(%f, %f, %f)")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
#endif
			mRooms.emplace_back(std::move(room));
		}

#if USE_SIN_CURVE == 0 & defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateHeightImageForDebug(perlinNoise, octaves, noiseBoostRatio, "/debug/0_HeightMap.bmp");
		// 2_SeparateRooms_0.bmpと同様の内容になるのでコメントアウト
		// GenerateRoomImageForDebug("/debug/1_GenerateRooms.bmp");
#endif

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("GenerateRooms: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/**
	部屋の重なりを解消します
	*/
	Generator::SeparateRoomsResult Generator::SeparateRooms(const size_t phase, const size_t subPhase) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Separate Rooms"));
#endif

		// 部屋の交差を解消します
		uint8_t imageNo = 0;
		constexpr uint8_t maxImageNo = 20;
		SeparateRoomsResult separateRoomsResult = SeparateRoomsResult::Completed;
		bool retry;
		do {
#if defined(DEBUG_GENERATE_BITMAP_FILE)
			GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_SeparateRooms_" + std::to_string(imageNo) + ".bmp");
#endif
			retry = false;

			// 中心から近い順に並べ替える
			mRooms.sort([](const std::shared_ptr<const Room>& l, const std::shared_ptr<const Room>& r)
				{
					const double lsd = l->GetCenter().SizeSquared();
					const double rsd = r->GetCenter().SizeSquared();
					return lsd < rsd;
				}
			);

			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				std::vector<std::shared_ptr<Room>> intersectedRooms;

				// 他の部屋と交差している？
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room0->GetHorizontalRoomMargin())
						horizontalRoomMargin = room0->GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room1->GetHorizontalRoomMargin())
						horizontalRoomMargin = room1->GetHorizontalRoomMargin();

					uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
					if (verticalRoomMargin < room0->GetVerticalRoomMargin())
						verticalRoomMargin = room0->GetVerticalRoomMargin();
					if (verticalRoomMargin < room1->GetVerticalRoomMargin())
						verticalRoomMargin = room1->GetVerticalRoomMargin();

					if (room0 != room1 && room0->Intersect(*room1, horizontalRoomMargin, verticalRoomMargin))
					{
						// 交差した部屋を記録
						// cppcheck-suppress [useStlAlgorithm]
						intersectedRooms.emplace_back(room1);
						// 動いた先で交差している可能性があるので再チェック
						retry = true;
						// 一度でも部屋を動かしてしまったので結果を記録
						separateRoomsResult = SeparateRoomsResult::Moved;
					}
				}

				if (intersectedRooms.empty() == false)
				{
					// 一番原点に近い部屋を探す
					intersectedRooms.emplace_back(room0);
					std::sort(intersectedRooms.begin(), intersectedRooms.end(), [](const std::shared_ptr<Room>& l, const std::shared_ptr<Room>& r)
						{
							return l->GetCenter().SizeSquared2D() < r->GetCenter().SizeSquared2D();
						});

					auto nearestRoomToOrigin = intersectedRooms[0];
					intersectedRooms.erase(intersectedRooms.begin());

					// 交差した部屋が重ならないように移動
					SeparateRoom(nearestRoomToOrigin, intersectedRooms, imageNo > 10);
				}
			}

			++imageNo;
		} while (imageNo < maxImageNo && retry);

		// 部屋の重複が解決できなかった場合
		if (imageNo >= maxImageNo && retry)
		{
			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room0->GetHorizontalRoomMargin())
						horizontalRoomMargin = room0->GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room1->GetHorizontalRoomMargin())
						horizontalRoomMargin = room1->GetHorizontalRoomMargin();

					uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
					if (verticalRoomMargin < room0->GetVerticalRoomMargin())
						verticalRoomMargin = room0->GetVerticalRoomMargin();
					if (verticalRoomMargin < room1->GetVerticalRoomMargin())
						verticalRoomMargin = room1->GetVerticalRoomMargin();

					if (room0 != room1 && room0->Intersect(*room1, horizontalRoomMargin, verticalRoomMargin))
					{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
						GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_SeparateRooms_failure.bmp");
#endif
						DUNGEON_GENERATOR_ERROR(TEXT("Generator::SeparateRooms: The room crossing was not resolved."));
						mLastError = Error::SeparateRoomsFailed;
						return SeparateRoomsResult::Failed;
					}
				}
			}
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: %d,X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d)")
				, static_cast<uint16_t>(room->GetIdentifier())
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

		return separateRoomsResult;
	}

	void Generator::SeparateRoom(const std::shared_ptr<Room>& fixedRoom, const std::vector<std::shared_ptr<Room>>& intersectedRooms, const bool activateOuterMovement) const noexcept
	{
		// 交差した部屋が重ならないように移動
		for (const std::shared_ptr<Room>& movableRoom : intersectedRooms)
		{
			check(fixedRoom->GetCenter().SizeSquared2D() <= movableRoom->GetCenter().SizeSquared2D());

			uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
			if (horizontalRoomMargin < fixedRoom->GetHorizontalRoomMargin())
				horizontalRoomMargin = fixedRoom->GetHorizontalRoomMargin();
			if (horizontalRoomMargin < movableRoom->GetHorizontalRoomMargin())
				horizontalRoomMargin = movableRoom->GetHorizontalRoomMargin();

			uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
			if (verticalRoomMargin < fixedRoom->GetVerticalRoomMargin())
				verticalRoomMargin = fixedRoom->GetVerticalRoomMargin();
			if (verticalRoomMargin < movableRoom->GetVerticalRoomMargin())
				verticalRoomMargin = movableRoom->GetVerticalRoomMargin();

			// 二つの部屋を合わせた空間の大きさ
			const FVector contactSize(
				fixedRoom->GetWidth() + movableRoom->GetWidth(),
				fixedRoom->GetDepth() + movableRoom->GetDepth(),
				fixedRoom->GetHeight() + movableRoom->GetHeight()
			);

			// 二つの部屋を合わせた空間の半分大きさ
			FVector contactHalfSize = contactSize * 0.5;

			// 余白を加算する
			contactHalfSize.X += horizontalRoomMargin;
			contactHalfSize.Y += horizontalRoomMargin;
			contactHalfSize.Z += verticalRoomMargin;

			// 押し出し範囲を設定
			const std::array<Plane, 6> planes =
			{
				Plane(FVector(-1.,  0.,  0.), FVector(contactHalfSize.X, 0., 0.)),	// +X
				Plane(FVector(1.,  0.,  0.), FVector(-contactHalfSize.X, 0., 0.)),	// -X
				Plane(FVector(0., -1.,  0.), FVector(0.,  contactHalfSize.Y, 0.)),	// +Y
				Plane(FVector(0.,  1.,  0.), FVector(0., -contactHalfSize.Y, 0.)),	// -Y
				Plane(FVector(0.,  0., -1.), FVector(0., 0.,  contactHalfSize.Z)),	// +Z
				Plane(FVector(0.,  0.,  1.), FVector(0., 0., -contactHalfSize.Z)),	// -Z
			};

			const FVector& fixedRoomCenter = fixedRoom->GetCenter();
			const FVector& movableRoomCenter = movableRoom->GetCenter();

			// movableRoomを押し出す方向を求める
			FVector direction = movableRoomCenter - fixedRoomCenter;

			// 中心が一致してしまったので適当な方向に押し出す
			if (math::IsZero(direction.SizeSquared2D()) == true)
			{
				direction = movableRoomCenter;
				if (direction.Normalize() == false)
				{
					const double ratio = mGenerateParameter.GetRandom()->Get<double>();
					const double radian = ratio * math::Pi2();
					direction.X = std::cos(radian);
					direction.Y = std::sin(radian);
					direction.Z = 0;
				}
			}
			else if (activateOuterMovement)
			{
				auto outerDirection = movableRoomCenter;
				if (outerDirection.Normalize() == true)
					direction += outerDirection;
			}

			// 水平方向への移動を優先
			if (mGenerateParameter.GetNumberOfCandidateFloors() <= 0)
			{
				direction.Z = 0;
			}
			else
			{
				const auto limit = std::sin(math::ToRadian(11.25));
				direction.Z = std::max(-limit, std::min(direction.Z, limit));
			}
			if (direction.Normalize() == false)
			{
				const double ratio = mGenerateParameter.GetRandom()->Get<double>();
				const double radian = ratio * math::Pi2();
				direction.X = std::cos(radian);
				direction.Y = std::sin(radian);
				direction.Z = 0;
			}

			// 押し出す
			FVector newRoomOffset;
			double minimumDistance = std::numeric_limits<double>::max();
			for (const auto& plane : planes)
			{
				FVector roomOffset;
				if (plane.Intersect(direction, roomOffset))
				{
					const double distance = roomOffset.SquaredLength();
					if (minimumDistance > distance)
					{
						minimumDistance = distance;
						newRoomOffset = roomOffset;
					}
				}
			}
			check(minimumDistance < std::numeric_limits<double>::max());

			// movableRoomの中心を押し出し位置へ移動
			const FVector movableRoomSize(
				static_cast<double>(movableRoom->GetWidth()),
				static_cast<double>(movableRoom->GetDepth()),
				static_cast<double>(movableRoom->GetHeight())
			);
			const FVector newMovableRoomLocation = fixedRoomCenter + newRoomOffset - movableRoomSize * 0.5;
			movableRoom->SetX(static_cast<int32_t>(std::round(newMovableRoomLocation.X)));
			movableRoom->SetY(static_cast<int32_t>(std::round(newMovableRoomLocation.Y)));
			if (mGenerateParameter.GetNumberOfCandidateFloors() > 0)
				movableRoom->SetZ(static_cast<int32_t>(std::round(newMovableRoomLocation.Z)));

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			// 交差していないか再確認
			if (fixedRoom->Intersect(*movableRoom, horizontalRoomMargin, verticalRoomMargin))
			{
				DUNGEON_GENERATOR_LOG(TEXT("direction %f,%f,%f"), direction.X, direction.Y, direction.Z);
				DUNGEON_GENERATOR_LOG(TEXT("Room0: X=%d~%d,Y=%d~%d,Z=%d~%d"), fixedRoom->GetLeft(), fixedRoom->GetRight(), fixedRoom->GetTop(), fixedRoom->GetBottom(), fixedRoom->GetBackground(), fixedRoom->GetForeground());
				DUNGEON_GENERATOR_LOG(TEXT("Room1: X=%d~%d,Y=%d~%d,Z=%d~%d"), movableRoom->GetLeft(), movableRoom->GetRight(), movableRoom->GetTop(), movableRoom->GetBottom(), movableRoom->GetBackground(), movableRoom->GetForeground());
				check(false);
				check(fixedRoom->Intersect(*movableRoom, horizontalRoomMargin, verticalRoomMargin) == true);
			}
#endif
		}
	}

	bool Generator::ExpandSpace(const int32_t horizontalMargin, const int32_t verticalMargin) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		int32_t minX, minY, minZ;
		int32_t maxX, maxY, maxZ;
		minX = minY = minZ = std::numeric_limits<int32_t>::max();
		maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (minX > room->GetLeft())
				minX = room->GetLeft();
			if (minY > room->GetTop())
				minY = room->GetTop();
			if (minZ > room->GetBackground())
				minZ = room->GetBackground();
			if (maxX < room->GetRight())
				maxX = room->GetRight();
			if (maxY < room->GetBottom())
				maxY = room->GetBottom();
			if (maxZ < room->GetForeground())
				maxZ = room->GetForeground();
		}

		// 外周に余白を作る
		{
			minX -= horizontalMargin;
			minY -= horizontalMargin;
			minZ -= verticalMargin;
			maxX += horizontalMargin;
			maxY += horizontalMargin;
			maxZ += verticalMargin;
		}

		// 空間のサイズを設定
		mGenerateParameter.SetWidth(maxX - minX + 1);
		mGenerateParameter.SetDepth(maxY - minY + 1);
		mGenerateParameter.SetHeight(maxZ - minZ + 1);

		// 空間の原点を移動（部屋の位置を移動）
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->SetX(room->GetX() - minX);
			room->SetY(room->GetY() - minY);
			room->SetZ(room->GetZ() - minZ);
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Room: W=%d,D=%d,H=%d に空間を変更しました")
			, mGenerateParameter.GetWidth(), mGenerateParameter.GetDepth(), mGenerateParameter.GetHeight()
		);
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: %d,X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/7_ExpandSpace.bmp");
#endif

		return true;
	}

	void Generator::AdjustPoints() noexcept
	{
		// 部屋の中心位置をリセット
		check(mStartPoint);
		std::const_pointer_cast<Point>(mStartPoint)->ResetByRoomGroundCenter();

		// 部屋の中心位置をリセット
		check(mGoalPoint);
		std::const_pointer_cast<Point>(mGoalPoint)->ResetByRoomGroundCenter();

		// 部屋の中心位置をリセット
		for (Aisle& aisle : mAisles)
		{
			for (uint_fast8_t i = 0; i < 2; ++i)
			{
				if (const auto& point = aisle.GetPoint(i))
				{
					std::const_pointer_cast<Point>(point)->ResetByRoomGroundCenter();
				}
			}
		}
	}

	bool Generator::DetectFloorHeightAndDepthFromStart() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeightAndDepthFromStart: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		mFloorHeight.clear();

		mDeepestDepthFromStart = 0;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			const int32_t z = room->GetBackground();
			if (std::find(mFloorHeight.begin(), mFloorHeight.end(), z) == mFloorHeight.end())
			{
				mFloorHeight.emplace_back(z);
			}

			if (mDeepestDepthFromStart < room->GetDepthFromStart())
				mDeepestDepthFromStart = room->GetDepthFromStart();
		}

		std::stable_sort(mFloorHeight.begin(), mFloorHeight.end());

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeightAndDepthFromStart: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/*
	ドロネー三角形分割した辺を最小スパニングツリーにて抽出
	*/
	bool Generator::ExtractionAisles() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Extract Aisles"));
		DUNGEON_GENERATOR_LOG(TEXT("%d rooms detected"), mRooms.size());
#endif

		// すべての部屋の中点を記録しながら、部屋のパーツ（役割）をリセット
		std::vector<std::shared_ptr<const Point>> points;
		points.reserve(mRooms.size());
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// 部屋のパーツ（役割）をリセットする
			room->SetParts(Room::Parts::Unidentified);
			room->ResetReservationNumber();

			// Room::GetGroundCenterリストを作成
			// cppcheck-suppress [useStlAlgorithm]
			points.emplace_back(std::make_shared<const Point>(room));
		}

		uint8_t aisleComplexity = mGenerateParameter.GetAisleComplexity();
		if (aisleComplexity > 0)
		{
			float candidateAisleComplexity = std::sqrt(mGenerateParameter.GetNumberOfCandidateRooms());
			if (candidateAisleComplexity > 255.f)
				candidateAisleComplexity = 255.f;
			aisleComplexity = std::min(aisleComplexity, static_cast<uint8_t>(candidateAisleComplexity));
		}

		if (mRooms.size() >= 4)
		{
			// 三角形分割
			DelaunayTriangulation3D delaunayTriangulation(points);

#if WITH_EDITOR
			if (!delaunayTriangulation.IsValid())
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Generator:Triangulation failed. %d rooms"), mRooms.size());
				for (const auto& room : mRooms)
				{
					DUNGEON_GENERATOR_ERROR(TEXT("X:%d Y:%d Z:%d Width:%d Depth:%d Height:%d"),
						room->GetX(), room->GetY(), room->GetZ(),
						room->GetWidth(), room->GetDepth(), room->GetHeight()
					);
				}
				mLastError = Error::TriangulationFailed;
			}
#endif

			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(mGenerateParameter.GetRandom(), delaunayTriangulation, aisleComplexity);
			if (GenerateAisle(minimumSpanningTree) == false)
				return false;
		}
		else
		{
			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(mGenerateParameter.GetRandom(), points, aisleComplexity);
			if (GenerateAisle(minimumSpanningTree) == false)
				return false;
		}

		// 部屋のパーツ（役割）を設定する
		SetRoomParts();

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

		return mLastError == Error::Success;
	}

	/**
	 * ExtractionAislesから呼ばれる
	 */
	bool Generator::GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->ResetGateCount();
		}

		mAisles.clear();

		minimumSpanningTree.ForEach([this](const Aisle& edge)
			{
				const std::shared_ptr<Room>& ar = edge.GetPoint(0)->GetOwnerRoom();
				const std::shared_ptr<Room>& br = edge.GetPoint(1)->GetOwnerRoom();
				if (ar == nullptr || br == nullptr)
					return;
				ar->AddGateCount(1);
				br->AddGateCount(1);
				std::shared_ptr<Point> a = std::make_shared<Point>(ar);
				std::shared_ptr<Point> b = std::make_shared<Point>(br);
				mAisles.emplace_back(edge.IsMain(), a, b);
			}
		);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		for (const auto& aisle : mAisles)
		{
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: Aisle: %d %d-%d (%f)"),
				aisle.GetIdentifier().Get(),
				aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetLength()
			);
		}
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("%d minimum spanning tree edges detected"), minimumSpanningTree.Size());
#endif

		// スタート位置を記録
		mStartPoint = minimumSpanningTree.GetStartPoint();
		if (mStartPoint == nullptr)
		{
			if (mRooms.empty())
				return false;
			mStartPoint = std::make_shared<Point>(mRooms.front());
		}
		mStartRoom = mStartPoint->GetOwnerRoom();

		// ゴール位置を記録
		mGoalPoint = minimumSpanningTree.GetGoalPoint();
		if (mGoalPoint == nullptr)
		{
			if (mRooms.empty())
				return false;
			mGoalPoint = std::make_shared<Point>(mRooms.front());
		}
		mGoalRoom = mGoalPoint->GetOwnerRoom();

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/**
	 * 部屋のパーツ（役割）を設定する
	 */
	void Generator::SetRoomParts() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("SetRoomParts: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		// HACK: MinimumSpanningTreeクラスにまとめた方が良いかも
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (room->GetParts() == Room::Parts::Unidentified /* && room->GetGateCount() > 1 */)
			{
				room->SetParts(Room::Parts::Hall);
			}
		}

		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Unidentified;
			}) == mRooms.end());
		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Start;
			}) != mRooms.end());
		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Goal;
			}) != mRooms.end());
	}

	/**
	 * スタート部屋およびゴール部屋のサブレベルが指定されていた場合に
	 * サブレベルが入る空間の範囲を空ける
	 */
	bool Generator::AdjustedStartAndGoalSubLevel() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSubLevel: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		std::list<std::pair<uint32_t, FIntVector>> sublevels;
		if (mOnQueryParts)
		{
			mOnQueryParts(sublevels);
		}

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			switch (room->GetParts())
			{
			case Room::Parts::Start:
				// スタート部屋の大きさを調整する
				if (mGenerateParameter.IsGenerateStartRoomReserved())
				{
					room->SetWidth(mGenerateParameter.GetStartRoomSize().X);
					room->SetDepth(mGenerateParameter.GetStartRoomSize().Y);
					room->SetHeight(mGenerateParameter.GetStartRoomSize().Z);
				}
				break;

			case Room::Parts::Goal:
				// ゴール部屋の大きさを調整する
				if (mGenerateParameter.IsGenerateGoalRoomReserved())
				{
					room->SetWidth(mGenerateParameter.GetGoalRoomSize().X);
					room->SetDepth(mGenerateParameter.GetGoalRoomSize().Y);
					room->SetHeight(mGenerateParameter.GetGoalRoomSize().Z);
				}
				break;

			case Room::Parts::Hall:
			case Room::Parts::Hanare:
				// サブレベルを部屋に関連付ける
				if (sublevels.empty() == false)
				{
					auto i = std::find_if(sublevels.begin(), sublevels.end(), [room](const std::pair<uint32_t, FIntVector>& sublevel)
					{
						return
							room->GetWidth() == sublevel.second.X &&
							room->GetDepth() == sublevel.second.Y &&
							room->GetHeight() == sublevel.second.Z;
					});
					if (i != sublevels.end())
					{
						room->SetReservationNumber(i->first);
						sublevels.erase(i);
					}
					else
					{
						const std::pair<uint32_t, FIntVector>& sublevel = sublevels.front();
						room->SetReservationNumber(sublevel.first);
						room->SetWidth(sublevel.second.X);
						room->SetDepth(sublevel.second.Y);
						room->SetHeight(sublevel.second.Z);
						sublevels.pop_front();
					}
				}
				break;

			case Room::Parts::Unidentified:
				break;
			}
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSubLevel: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/**
	 * 部屋の大きさを調整する
	 */
	void Generator::AdjustRoomSize() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("AdjustRoomSize: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		// 狭すぎる・大きすぎる部屋を調整する
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// 予約済みの部屋はサイズを変更できない
			if (room->IsValidReservationNumber() == true)
				continue;
			// スタート・ゴール部屋はサイズを変更できない
			if (room->GetParts() == Room::Parts::Start && mGenerateParameter.IsGenerateStartRoomReserved())
				continue;
			if (room->GetParts() == Room::Parts::Goal && mGenerateParameter.IsGenerateGoalRoomReserved())
				continue;

			// 部屋の最大サイズにあわせる
			uint32_t width = room->GetWidth();
			if (width > mGenerateParameter.GetMaxRoomWidth())
			{
				width = mGenerateParameter.GetMaxRoomWidth();
				room->SetWidth(width);
			}

			uint32_t depth = room->GetDepth();
			if (depth > mGenerateParameter.GetMaxRoomDepth())
			{
				depth = mGenerateParameter.GetMaxRoomDepth();
				room->SetDepth(depth);
			}

			uint32_t height = room->GetHeight();
			if (height > mGenerateParameter.GetMaxRoomHeight())
			{
				height = mGenerateParameter.GetMaxRoomHeight();
				room->SetHeight(height);
			}

			// ドアに対して部屋が小さい場合は広げる
			const uint32_t minimumArea = dungeon::math::Square<uint32_t>(room->GetGateCount());
			const uint32_t requiredArea = std::max(2u, minimumArea);
			if (requiredArea > width * depth)
			{
				do {
					if (width < depth)
						++width;
					else
						++depth;
				} while (requiredArea > width * depth);

				room->SetWidth(width);
				room->SetDepth(depth);

				DUNGEON_GENERATOR_LOG(TEXT("Expanded the room because it was too small for the connecting gate : ID=%d"), static_cast<uint16_t>(room->GetIdentifier()));
			}
		}

		// 水平方向の余白を追加できるか？
		if (mGenerateParameter.GetHorizontalRoomMargin() > 0)
		{
			// 部屋の余白を調整する
			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					if (room0 == room1)
						continue;

					room0->SetMarginIfRoomIntersect(room1, 2, 1);
				}
			}
		}
	}

	bool Generator::MarkBranchIdAndDepthFromStart() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("Branch: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		if (mStartRoom)
		{
			uint8_t branchId = 0;
			MarkBranchIdAndDepthFromStartRecursive(mStartRoom, branchId, 0);
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("Branch: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	void Generator::MarkBranchIdAndDepthFromStartRecursive(const std::shared_ptr<Room>& room, uint8_t& branchId, const uint8_t depth) noexcept
	{
		if (room->IsValidBranchId() == false)
			room->SetBranchId(branchId);

		if (room->GetDepthFromStart() > depth)
			room->SetDepthFromStart(depth);

#if WITH_EDITOR
		size_t aisleCount = 0;
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
				++aisleCount;
		}
		check(aisleCount == room->GetGateCount());
#endif

		for (auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				const auto newDepth = depth + 1;
				if (room != room0)
				{
					if (room->GetGateCount() >= 3)
						++branchId;
					if (room0->GetDepthFromStart() > newDepth)
						MarkBranchIdAndDepthFromStartRecursive(room0, branchId, newDepth);
				}
				if (room != room1)
				{
					if (room->GetGateCount() >= 3)
						++branchId;
					if (room1->GetDepthFromStart() > newDepth)
						MarkBranchIdAndDepthFromStartRecursive(room1, branchId, newDepth);
				}
			}
		}
	}

	/*
	 * MissionGraph生成後に呼び出す必要があります
	 */
	void Generator::InvokeRoomCallbacks() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("InvokeRoomCallbacks: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			switch (room->GetParts())
			{
			case Room::Parts::Start:
				if (mOnLoadStartParts)
					mOnLoadStartParts(room);
				break;

			case Room::Parts::Goal:
				if (mOnLoadGoalParts)
					mOnLoadGoalParts(room);
				break;

			case Room::Parts::Hall:
			case Room::Parts::Hanare:
				if (mOnLoadParts)
					mOnLoadParts(room);
				break;

			case Room::Parts::Unidentified:
				break;
			}
		}
	}

	bool Generator::GenerateVoxel() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		mVoxel = std::make_shared<Voxel>(mGenerateParameter);

		// Generate room
		for (const auto& room : mRooms)
		{
			// 部屋の深さを256段階の比率にする
			uint8_t depthRatioFromStart = 0;
			if (GetDeepestDepthFromStart() > 0)
			{
				float depthFromStart = static_cast<float>(room->GetDepthFromStart());
				depthFromStart /= static_cast<float>(GetDeepestDepthFromStart());
				depthRatioFromStart = static_cast<uint8_t>(depthFromStart * 255.f);
			}

			const FIntVector min(room->GetLeft(), room->GetTop(), room->GetBackground());
			const FIntVector max(room->GetRight(), room->GetBottom(), room->GetForeground());
			mVoxel->Rectangle(min, max
				, Grid::CreateFloor(mGenerateParameter.GetRandom(), room->GetIdentifier(), depthRatioFromStart)
				, Grid::CreateDeck(mGenerateParameter.GetRandom(), room->GetIdentifier(), depthRatioFromStart)
			);
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: room generated : RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

		if (mOnPreGenerateVoxel)
		{
			mOnPreGenerateVoxel(mVoxel);
		}

		// 通路の距離が短い順に並べ替える
		std::stable_sort(mAisles.begin(), mAisles.end(), [](const Aisle& l, const Aisle& r)
			{
				// メインルート以外のソートキー（優先させない）
				static constexpr double AlternativeRouteCost = 10000. * 100.;
				double lLength = l.GetLength();
				if (l.IsMain() == false)
					lLength += AlternativeRouteCost;

				double rLength = r.GetLength();
				if (r.IsMain() == false)
					rLength += AlternativeRouteCost;

				return lLength < rLength;
			}
		);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION) || defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		for (const auto& aisle : mAisles)
		{
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Aisle: %d %d-%d (%f) %c"),
				aisle.GetIdentifier().Get(),
				aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetLength(),
				aisle.IsMain() ? TCHAR('M') : TCHAR(' ')
			);
		}
#endif

		// Generate pathways
		// 通路の生成
		for (size_t i = 0; i < mAisles.size(); ++i)
		{
			const Aisle& aisle = mAisles[i];
			std::shared_ptr<const Point> startPoint = aisle.GetPoint(0);
			std::shared_ptr<const Point> goalPoint = aisle.GetPoint(1);

			// Use the back room as a starting point
			if (startPoint->GetOwnerRoom()->GetDepthFromStart() < goalPoint->GetOwnerRoom()->GetDepthFromStart())
			{
				std::swap(startPoint, goalPoint);
			}

			// 通路は奥の部屋の深さにあわせ、256段階の比率にする
			uint8_t depthRatioFromStart = 0;
			if (GetDeepestDepthFromStart() > 0)
			{
				float depthFromStart = static_cast<float>(goalPoint->GetOwnerRoom()->GetDepthFromStart());
				depthFromStart /= static_cast<float>(GetDeepestDepthFromStart());
				depthRatioFromStart = static_cast<uint8_t>(depthFromStart * 255.f);
			}

			// Check if the start and end points are included in the room
			check(startPoint->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*startPoint)));
			check(goalPoint->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*goalPoint)));

			// Change to voxel coordinates
			FIntVector start = ToIntVector(*startPoint);
			FIntVector goal = ToIntVector(*goalPoint);

			// Conditions for reaching the passage. The endpoint can be anywhere in the goal room.
			const std::shared_ptr<Room>& goalRoom = goalPoint->GetOwnerRoom();
			check(goalRoom);
			const PathGoalCondition pathGoalCondition(goalRoom->GetRect());

			// ゴール地点周辺でゲートを生成できるボクセルを探す
			constexpr size_t MaxResultCount = 8;
			std::vector<Voxel::CandidateLocation> goalToStart;
			if (mVoxel->SearchGateLocation(goalToStart, MaxResultCount, goal, goalPoint->GetOwnerRoom()->GetIdentifier(), start, mGenerateParameter.UseMissionGraph() == false) == true)
			{
				bool complete = false;

				// 室内にスロープが生成できて、終了門が開始門よりも高い位置にある？
				if (mGenerateParameter.IsGenerateSlopeInRoom() == true && start.Z < goal.Z)
				{
					RoomStructureGenerator roomStructureGenerator;
					if (roomStructureGenerator.CheckSlopePlacement(mVoxel, startPoint->GetOwnerRoom(), goal, mGenerateParameter.GetRandom()))
					{
						std::vector<Voxel::CandidateLocation> startToGoal;
						startToGoal.reserve(1);
						startToGoal.emplace_back(0, roomStructureGenerator.GetGateLocation());

						Voxel::AisleParameter aisleParameter;
						aisleParameter.mGoalCondition = pathGoalCondition;
						aisleParameter.mIdentifier = aisle.GetIdentifier();
						aisleParameter.mMergeRooms = mGenerateParameter.IsMergeRooms();
						aisleParameter.mGenerateIntersections = /*aisle.IsAnyLocked() == false ||*/ mGenerateParameter.IsAisleComplexity();
						aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
						aisleParameter.mLocked = aisle.IsLocked();
						aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
						complete = mVoxel->Aisle(startToGoal, goalToStart, aisleParameter);
						if (complete == true)
						{
							roomStructureGenerator.GenerateSlope(mVoxel);
						}
					}
				}

				if (complete == false)
				{
					// スタート地点周辺でゲートを生成できるボクセルを探す
					std::vector<Voxel::CandidateLocation> startToGoal;
					if (mVoxel->SearchGateLocation(startToGoal, MaxResultCount, start, startPoint->GetOwnerRoom()->GetIdentifier(), goal, mGenerateParameter.UseMissionGraph() == false))
					{
						Voxel::AisleParameter aisleParameter;
						aisleParameter.mGoalCondition = pathGoalCondition;
						aisleParameter.mIdentifier = aisle.GetIdentifier();
						aisleParameter.mMergeRooms = mGenerateParameter.IsMergeRooms();
						aisleParameter.mGenerateIntersections = /*aisle.IsAnyLocked() == false ||*/ mGenerateParameter.IsAisleComplexity();
						aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
						aisleParameter.mLocked = aisle.IsLocked();
						aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
						complete = mVoxel->Aisle(startToGoal, goalToStart, aisleParameter);

						// 幹線通路以外なら生成に失敗しても到達可能なので成功扱いにする
						if (aisle.IsMain() == false)
						{
							complete = true;
						}
						// 幹線通路でも部屋を結合しているなら成功扱いにする
						else if (mGenerateParameter.IsMergeRooms() == true)
						{
							complete = true;
						}

						if (complete == false)
						{
#if WITH_EDITOR
							DUNGEON_GENERATOR_ERROR(TEXT("Generator: Route search failed. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), i, static_cast<uint16_t>(aisle.GetIdentifier()), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
							DUNGEON_GENERATOR_ERROR(TEXT("State of the grid in the starting room %d: ID=%d (%d,%d,%d) %d Gate"), i
								, static_cast<uint16_t>(startPoint->GetOwnerRoom()->GetIdentifier())
								, startPoint->GetOwnerRoom()->GetX(), startPoint->GetOwnerRoom()->GetY(), startPoint->GetOwnerRoom()->GetZ()
								, startPoint->GetOwnerRoom()->GetGateCount());
							DumpVoxel(startPoint);
							DUNGEON_GENERATOR_ERROR(TEXT("State of the grid in the goal room %d: ID=%d (%d,%d,%d) %d Gate"), i
								, static_cast<uint16_t>(goalPoint->GetOwnerRoom()->GetIdentifier())
								, goalPoint->GetOwnerRoom()->GetX(), goalPoint->GetOwnerRoom()->GetY(), goalPoint->GetOwnerRoom()->GetZ()
								, goalPoint->GetOwnerRoom()->GetGateCount());
							DumpVoxel(goalPoint);
							DumpAisleAndRoomInformation(i);
#endif
							mLastError = Error::RouteSearchFailed;
							return false;
						}
					}
					else
					{
						// 部屋が結合されているなら通路が無くても問題ないはず…
						if (mGenerateParameter.IsMergeRooms() == false)
						{
#if WITH_EDITOR
							DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a start gate that can be generated. %d: ID=%d (%d,%d,%d) %d Gate"), i
								, static_cast<uint16_t>(startPoint->GetOwnerRoom()->GetIdentifier())
								, startPoint->GetOwnerRoom()->GetX(), startPoint->GetOwnerRoom()->GetY(), startPoint->GetOwnerRoom()->GetZ()
								, startPoint->GetOwnerRoom()->GetGateCount());
							DumpVoxel(startPoint);
							DumpAisleAndRoomInformation(i);
#endif
							mLastError = Error::GateSearchFailed;
							return false;
						}
					}
				}
			}
			else
			{
				// 部屋が結合されているなら通路が無くても問題ないはず…
				if (mGenerateParameter.IsMergeRooms() == false)
				{
#if WITH_EDITOR
					DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a goal gate that can be generated. %d: ID=%d (%d,%d,%d) %d Gate"), i
						, static_cast<uint16_t>(goalPoint->GetOwnerRoom()->GetIdentifier())
						, goalPoint->GetOwnerRoom()->GetX(), goalPoint->GetOwnerRoom()->GetY(), goalPoint->GetOwnerRoom()->GetZ()
						, goalPoint->GetOwnerRoom()->GetGateCount());
					DumpVoxel(goalPoint);
					DumpAisleAndRoomInformation(i);
#endif
					mLastError = Error::GateSearchFailed;
					return false;
				}
			}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			// 通信同期用に現在の乱数の種を出力する
			{
				uint32_t x, y, z, w;
				GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
				const uint32_t crc32 = CalculateCRC32();
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: aisle generated: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
			}
#endif
		}

		if (mOnPostGenerateVoxel)
		{
			mOnPostGenerateVoxel(mVoxel);
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: finish: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		mVoxel->GenerateImageForDebug("/debug/8_GenerateVoxel.bmp");
#endif

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下は生成完了後に操作する関数です。
	////////////////////////////////////////////////////////////////////////////////////////////////
	const std::vector<int32_t>& Generator::GetFloorHeight() const
	{
		return mFloorHeight;
	}

	size_t Generator::FindFloor(const int32_t height) const
	{
		const std::vector<int32_t>& floorHeight = GetFloorHeight();

		for (size_t i = 0; i < floorHeight.size(); ++i)
		{
			if (height <= floorHeight[i])
				return i;
		}

		return 0;
	}

	std::shared_ptr<Room> Generator::Find(const Point& point) const noexcept
	{
		const auto i = std::find_if(mRooms.begin(), mRooms.end(), [&point](const std::shared_ptr<Room>& room)
			{
				return room->Contain(point);
			}
		);
		return i != mRooms.end() ? *i : nullptr;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindAll(const Point& point) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		std::copy_if(mRooms.begin(), mRooms.end(), result.begin(), [&point](const std::shared_ptr<Room>& room)
			{
				return room->Contain(point);
			}
		);
		return result;
	}

	const GenerateParameter& Generator::GetGenerateParameter() const noexcept
	{
		return mGenerateParameter;
	}

	const std::shared_ptr<Voxel>& Generator::GetVoxel() const noexcept
	{
		return mVoxel;
	}

	size_t Generator::GetRoomCount() const noexcept
	{
		return mRooms.size();
	}

	std::shared_ptr<Room> Generator::FindByIdentifier(const Identifier& identifier) const noexcept
	{
		std::shared_ptr<Room> result;
		for (auto& room : mRooms)
		{
			if (room->GetIdentifier() == identifier)
			{
				result = room;
			}
		}
		return result;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByDepth(const uint8_t depth) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		for (auto& room : mRooms)
		{
			if (room->GetDepthFromStart() == depth)
			{
				// cppcheck-suppress [useStlAlgorithm]
				result.emplace_back(room);
			}
		}
		return result;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByBranch(const uint8_t branchId) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		for (auto& room : mRooms)
		{
			if (room->GetBranchId() == branchId)
			{
				// cppcheck-suppress [useStlAlgorithm]
				result.emplace_back(room);
			}
		}
		return result;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByRoute(const std::shared_ptr<Room>& room) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());

		if (IsRoutePassable(room) == true)
			result.emplace_back(room);

		std::unordered_set<const Aisle*> passableAisles;
		FindByRoute(result, passableAisles, room);

		result.shrink_to_fit();
		return result;
	}

	/**
	 * 入力された部屋から経路検索上通過可能な部屋を検索します
	 * @param[inout]	passableRooms	通過可能な部屋の一覧
	 * @param[inout]	passableAisles	通過可能な通路の一覧
	 * @param[in]		room			検索する部屋
	 */
	void Generator::FindByRoute(std::vector<std::shared_ptr<Room>>& passableRooms, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			// 鍵付き扉がある通路なら何もしない
			if (aisle.IsLocked())
				continue;

			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (passableAisles.contains(&aisle) == true)
					continue;
				passableAisles.emplace(&aisle);

				if (room == room0)
				{
					if (IsRoutePassable(room1) == true)
						passableRooms.emplace_back(room1);
					FindByRoute(passableRooms, passableAisles, room1);
				}
				else
				{
					if (IsRoutePassable(room0) == true)
						passableRooms.emplace_back(room0);
					FindByRoute(passableRooms, passableAisles, room0);
				}
			}
		}
	}

	/**
	 * 入力された部屋が経路検索上通過可能か判定します
	 */
	bool Generator::IsRoutePassable(const std::shared_ptr<Room>& room) noexcept
	{
		return
			// 予約済みの部屋は対象外
			room->IsValidReservationNumber() == false &&
			// アイテムがあると対象外
			room->GetItem() == Room::Item::Empty &&
			// ホールとはなれだけ対象
			(room->GetParts() == Room::Parts::Hall || room->GetParts() == Room::Parts::Hanare);
	}

	const Grid& Generator::GetGrid(const FIntVector& location) const noexcept
	{
		return mVoxel->Get(location.X, location.Y, location.Z);
	}

	uint32_t Generator::CalculateCRC32(const uint32_t hash) const noexcept
	{
		return mVoxel ? mVoxel->CalculateCRC32(hash) : hash;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下はデバッグに関する関数です。
	////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
	/*
	markdown + mermaidによるフローチャートを出力します
	*/
	void Generator::DumpRoomDiagram(const std::string& path) const noexcept
	{
		std::ofstream stream(path);
		if (stream.is_open())
		{
			stream << "```mermaid" << std::endl;
			stream << "graph TD;" << std::endl;

			for (const auto& room : mRooms)
			{
				stream << room->GetName() << "(" << std::endl;
				stream << room->GetPartsName() << std::endl;
				stream << "Identifier:" << std::to_string(room->GetIdentifier()) << std::endl;
				stream << "Branch:" << std::to_string(room->GetBranchId()) << std::endl;
				stream << "Depth:" << std::to_string(room->GetDepthFromStart()) << std::endl;
				if (Room::Item::Empty != room->GetItem())
					stream << "Item:" << room->GetItemName() << std::endl;
				if (room->IsValidReservationNumber())
					stream << "Sublevel:" << room->GetReservationNumber() << std::endl;
				stream << ")" << std::endl;
			}

			if (mStartRoom)
			{
				std::unordered_set<const Aisle*> edges;
				DumpRoomDiagram(stream, edges, mStartRoom);
			}

			stream << "```" << std::endl;
		}
	}

	void Generator::DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (passableAisles.contains(&aisle) == true)
					continue;
				passableAisles.emplace(&aisle);

				std::string label;
				label = "Identifier:" + std::to_string(aisle.GetIdentifier());
				if (aisle.IsUniqueLocked())
					label += "\nUnique lock";
				else if (aisle.IsLocked())
					label += "\nLock";

				if (room == room0)
					stream << room0->GetName() << "<-->|" + label + "|" << room1->GetName();
				else
					stream << room1->GetName() << "<-->|" + label + "|" << room0->GetName();
				stream << std::endl;

				if (room != room0)
					DumpRoomDiagram(stream, passableAisles, room0);
				if (room != room1)
					DumpRoomDiagram(stream, passableAisles, room1);
			}
		}
	}

	void Generator::GenerateHeightImageForDebug(const PerlinNoise& perlinNoise, const std::size_t octaves, const float noiseBoostRatio, const std::string& filename) noexcept
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		{
			constexpr size_t width = 512;

			bmp::Canvas canvas(width, width);
			for (size_t y = 0; y < width; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					float noise = perlinNoise.OctaveNoise(
						octaves,
						static_cast<float>(x) / static_cast<float>(width) * 2.f - 1.f,
						static_cast<float>(y) / static_cast<float>(width) * 2.f - 1.f
					);
					noise = noise * 0.5f + 0.5f;
					noise *= noiseBoostRatio;
					noise = std::max(0.f, std::min(noise, 1.f));

					bmp::RGBCOLOR color;
					color.rgbBlue = color.rgbGreen = color.rgbRed = static_cast<uint8_t>(noise * 255.f);
					canvas.Put(x, y, color);
				}
			}
			canvas.Write(dungeon::GetDebugDirectoryString() + filename);
		}
#endif
	}

	void Generator::GenerateRoomImageForDebug(const std::string& filename) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		int32_t minX, minY, minZ;
		int32_t maxX, maxY, maxZ;
		minX = minY = minZ = 0;
		maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const auto& room : mRooms)
		{
			minX = std::min(minX, room->GetLeft());
			minY = std::min(minY, room->GetTop());
			minZ = std::min(minZ, room->GetBackground());
			maxX = std::max(maxX, room->GetRight());
			maxY = std::max(maxY, room->GetBottom());
			maxZ = std::max(maxZ, room->GetForeground());
		}

		const int32_t width = maxX - minX + 2;
		const int32_t depth = maxY - minY + 2;
		const int32_t height = maxZ - minZ + 2;
		const int32_t offsetX = -minX + 1;
		const int32_t offsetY = -minY + 1;
		const int32_t offsetZ = depth + 1;

		// 空間のサイズを設定
		const bmp::Canvas canvas(Scale(width), Scale(depth + 1 + height));

		for (const auto& room : mRooms)
		{
			bmp::RGBCOLOR color;
			if (room->GetParts() == Room::Parts::Start)
			{
				color = StartColor;
			}
			else if (room->GetParts() == Room::Parts::Goal)
			{
				color = GoalColor;
			}
			else if (room->GetParts() == Room::Parts::Hanare)
			{
				color = LeafColor;
			}
			else
			{
				float ratio = static_cast<float>(room->GetZ() - minZ) / static_cast<float>(maxZ - minZ);
				if (ratio <= std::numeric_limits<float>::epsilon())
					ratio = std::numeric_limits<float>::epsilon();
				color.rgbRed = BaseDarkColor.rgbRed + (BaseLightColor.rgbRed - BaseDarkColor.rgbRed) * ratio;
				color.rgbGreen = BaseDarkColor.rgbGreen + (BaseLightColor.rgbGreen - BaseDarkColor.rgbGreen) * ratio;
				color.rgbBlue = BaseDarkColor.rgbBlue + (BaseLightColor.rgbBlue - BaseDarkColor.rgbBlue) * ratio;
			}

			// XY平面を描画
			canvas.Rectangle(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetY + room->GetTop()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetY + room->GetBottom()),
				color
			);

			// XZ平面を描画
			canvas.Rectangle(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetZ + height - room->GetForeground()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetZ + height - room->GetBackground()),
				color
			);
		}

		// グリッドを描画
		{
			for (int32_t x = minX; x <= maxX; ++x)
			{
				canvas.VerticalLine(
					Scale(offsetX + x),
					Scale(offsetY + minY),
					Scale(offsetY + maxY),
					x == 0 ? OriginYColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);

				canvas.VerticalLine(
					Scale(offsetX + x),
					Scale(offsetZ + height - minZ),
					Scale(offsetZ + height - maxZ),
					x == 0 ? OriginZColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (int32_t y = minY; y <= maxY; ++y)
			{
				canvas.HorizontalLine(
					Scale(offsetX + minX),
					Scale(offsetX + maxX),
					Scale(offsetY + y),
					y == 0 ? OriginXColor : y % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (int32_t z = minZ; z <= maxZ; ++z)
			{
				canvas.HorizontalLine(
					Scale(offsetX + minX),
					Scale(offsetX + maxX),
					Scale(offsetZ + height - z),
					z == 0 ? OriginXColor : z % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}
		}

		canvas.Write(dungeon::GetDebugDirectoryString() + filename);
#endif
	}

	void Generator::DumpAisleAndRoomInformation(const size_t index) const noexcept
	{
		for (size_t j = 0; j < index; ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_ERROR(TEXT("OK .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		{
			const Aisle& a = mAisles[index];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_ERROR(TEXT("NG .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), index, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		for (size_t j = index + 1; j < mAisles.size(); ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_ERROR(TEXT("-- .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}

		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d) center(%f, %f, %f)")
				, static_cast<uint16_t>(room->GetIdentifier())
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
		}
	}


	void Generator::DumpVoxel(const std::shared_ptr<const Point>& point) const noexcept
	{
		if (point)
		{
			DumpVoxel(point->GetOwnerRoom());
		}
	}

	void Generator::DumpVoxel(const std::shared_ptr<Room>& room) const noexcept
	{
#if JENKINS_FOR_DEVELOP
		for (int32 y = room->GetY() - 2; y < room->GetY() + room->GetDepth() + 2; ++y)
		{
			FString text;
			for (int32 x = room->GetX() - 2; x < room->GetX() + room->GetWidth() + 2; ++x)
			{
				const Grid& grid = mVoxel->Get(x, y, room->GetZ());
				FString typeName;
				if (room->GetY() <= y && y < room->GetY() + room->GetDepth() &&
					room->GetX() <= x && x < room->GetX() + room->GetWidth())
					typeName = TEXT("*");
				typeName += grid.GetTypeName();
				text += FString::Printf(TEXT("%10s (%5d),"), *typeName, static_cast<uint16_t>(grid.GetIdentifier()));
			}
			DUNGEON_GENERATOR_ERROR(TEXT("%s"), *text);
		}
#endif
	}
#endif
}
