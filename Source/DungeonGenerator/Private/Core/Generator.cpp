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
#include "Helper/Crc.h"
#include "Helper/Stopwatch.h"
#include "Math/Math.h"
#include "Math/PerlinNoise.h"
#include "Math/Plane.h"
#include "Math/Vector.h"
#include "PathGeneration/DelaunayTriangulation3D.h"
#include "PathGeneration/MinimumSpanningTree.h"
#include "PathGeneration/PathGoalCondition.h"
#include "Voxelization/Voxel.h"

#include "MissionGraph/MissionGraph.h"

namespace dungeon
{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
	static constexpr float imageScale = 10.0f;
	static const bmp::RGBCOLOR frameColor = { 102, 95, 85 };
	static const bmp::RGBCOLOR baseDarkColor = { 95, 84, 62 };
	static const bmp::RGBCOLOR baseLightColor = { 173, 153, 112 };
	static const bmp::RGBCOLOR startColor = { 0, 128, 0 };
	static const bmp::RGBCOLOR goalColor = { 0, 0, 128 };
	static const bmp::RGBCOLOR leafColor = { 0, 128, 128 };
	inline uint32_t Scale(const uint32_t value)
	{
		return static_cast<uint32_t>(value * imageScale);
	}
#endif

	void Generator::Reset()
	{
		mVoxel.reset();
		mRooms.clear();
		mFloorHeight.clear();
		mLeafPoints.clear();
		mStartPoint.reset();
		mGoalPoint.reset();
		mAisles.clear();
		mLastError = Generator::Error::Success;
	}

	bool Generator::Generate(const GenerateParameter& parameter) noexcept
	{
		Identifier::ResetCounter();
		mLastError = Error::Success;
		mGenerateParameter = parameter;

		// 生成
		// TODO:リトライする仕組みの検討をして下さい。部屋の間隔を広げると成功する可能性が上がるかもしれません。
		while (!GenerateImpl(mGenerateParameter))
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

			mGenerateParameter.SetHorizontalRoomMargin(mGenerateParameter.GetHorizontalRoomMargin() + 1);
			mLastError = Error::Success;
		}

		// エラー情報を記録
		if (mLastError != Generator::Error::Success)
		{
			if (mVoxel && mVoxel->GetLastError() != Voxel::Error::Success)
			{
				uint8_t errorIndex = static_cast<uint8_t>(Generator::Error::___StartVoxelError);
				errorIndex += static_cast<uint8_t>(mVoxel->GetLastError());
				mLastError = static_cast<Generator::Error>(errorIndex);
			}
		}

		return mLastError == Error::Success;
	}

	bool Generator::GenerateImpl(GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
#endif

		Reset();

		// 部屋の生成
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultGenerateRooms = GenerateRooms(parameter);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("GenerateRooms: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultGenerateRooms)
			return false;

		// 部屋の分離
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultSeparateRooms = SeparateRooms(parameter, 2);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultSeparateRooms)
			return false;

		// 全ての部屋が収まるように空間を拡張します
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultExpandSpace = ExpandSpace(parameter, 1);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultExpandSpace)
			return false;

		// 通路の生成
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultExtractionAisles = ExtractionAisles(parameter, 3);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultExtractionAisles)
			return false;

		// 部屋のパーツ（役割）を設定する
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		SetRoomParts(parameter);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("SetRoomParts: %lf seconds"), stopwatch.Lap());
#endif

		// 開始部屋と終了部屋のサブレベルを配置する隙間を調整
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		if (!AdjustedStartAndGoalSublevel(parameter))
			return false;
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSublevel: %lf seconds"), stopwatch.Lap());
#endif

		// スタート部屋とゴール部屋のコールバックを呼ぶ
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		InvokeRoomCallbacks(parameter);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("InvokeRoomCallbacks: %lf seconds"), stopwatch.Lap());
#endif

		// ブランチIDの生成
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultBranch = MarkBranchId();
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("Branch: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultBranch)
			return false;

		// 階層情報の生成
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultDetectFloorHeight = DetectFloorHeight();
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeight: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultDetectFloorHeight)
			return false;

		// 部屋と通路に意味付けする
		if (parameter.UseMissionGraph())
		{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			stopwatch.Start();
#endif
			MissionGraph missionGraph(shared_from_this(), mGoalPoint);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: %lf seconds"), stopwatch.Lap());
#endif
		}
		else
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: Skipped"));
		}

		// ボクセル情報を生成します
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		stopwatch.Start();
#endif
		const bool resultGenerateVoxel = GenerateVoxel(parameter);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: %lf seconds"), stopwatch.Lap());
#endif
		if (!resultGenerateVoxel)
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
	部屋の生成
	*/
	bool Generator::GenerateRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Generate Rooms"));
#endif

		PerlinNoise perlinNoise(parameter.GetRandom());
		constexpr float noiseBoostRatio = 1.333f;

		float range = std::sqrtf(parameter.GetNumberOfCandidateRooms());
		range = std::max(1.f, range);
		float maxRoomWidth = std::max(parameter.GetMaxRoomWidth(), parameter.GetMaxRoomDepth());
		range *= maxRoomWidth + parameter.GetHorizontalRoomMargin();

		// Register Rooms
		for (size_t i = 0; i < parameter.GetNumberOfCandidateRooms(); ++i)
		{
			float x, y;
			NormalRandom(x, y, parameter.GetRandom());
			x = std::max(-1.f, std::min(x / 4.f, 1.f));
			y = std::max(-1.f, std::min(y / 4.f, 1.f));

			float z = std::max(0.f, static_cast<float>(parameter.GetNumberOfCandidateFloors() - 1));
			float noise = perlinNoise.Noise(x, y);
			noise = noise * 0.5f + 0.5f;
			noise *= noiseBoostRatio;
			noise = std::max(0.f, std::min(noise, 1.f));

			FIntVector location(
				static_cast<int32_t>(std::round(x * range)),
				static_cast<int32_t>(std::round(y * range)),
				static_cast<int32_t>(std::round(z * noise))
			);

			if (parameter.GetHorizontalRoomMargin() == 0)
			{
				/*
				RoomMarginが0の場合、部屋と部屋の間にスロープを作る隙間が無いので、
				部屋の高さを必ず同じにする必要がある。
				*/
				location.Z = 0;
			}

			std::shared_ptr<Room> room = std::make_shared<Room>(parameter, location);
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

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/1_GenerateRooms.bmp");
		GenerateHeightImageForDebug(perlinNoise, noiseBoostRatio, "/debug/1_HeightMap.bmp");
#endif

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
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
	bool Generator::SeparateRooms(const GenerateParameter& parameter, const size_t phase) noexcept
	{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Separate Rooms"));
#endif

		// 中心から近い順に並べ替える
		mRooms.sort([](const std::shared_ptr<const Room>& l, const std::shared_ptr<const Room>& r)
			{
				const double lsd = l->GetCenter().SizeSquared();
				const double rsd = r->GetCenter().SizeSquared();
				return lsd < rsd;
			}
		);

		// 部屋の交差を解消します
		size_t imageNo = 0;
		constexpr size_t maxImageNo = 10;
		bool retry;
		do {
#if defined(DEBUG_GENERATE_BITMAP_FILE)
			GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_SeparateRooms_" + std::to_string(imageNo) + ".bmp");
#endif
			retry = false;

			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				std::vector<std::shared_ptr<Room>> intersectedRooms;

				// 他の部屋と交差している？
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					if (room0 != room1 && room0->Intersect(*room1, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()))
					{
						// 交差した部屋を記録
						// cppcheck-suppress [useStlAlgorithm]
						intersectedRooms.emplace_back(room1);
						// 動いた先で交差している可能性があるので再チェック
						retry = true;
					}
				}

				// 交差した部屋が重ならないように移動
				for (const std::shared_ptr<Room>& room1 : intersectedRooms)
				{
					// 二つの部屋を合わせた空間の大きさ
					const FVector contactSize = FVector(
						room0->GetWidth() + room1->GetWidth(),
						room0->GetDepth() + room1->GetDepth(),
						room0->GetHeight() + room1->GetHeight()
					);

					// 二つの部屋を合わせた空間の半分大きさ
					FVector contactHalfSize = contactSize * 0.5;

					// 余白を加算する
					contactHalfSize.X += parameter.GetHorizontalRoomMargin();
					contactHalfSize.Y += parameter.GetHorizontalRoomMargin();
					contactHalfSize.Z += parameter.GetVerticalRoomMargin();

					// room1を押し出す中心を求める
					const FVector& room0Center = room0->GetCenter();

					// room1を押し出す方向を求める
					FVector direction = (room1->GetCenter() - room0Center).GetUnsafeNormal();

					// 水平方向への移動を優先
					if (room1->GetBackground() <= 0 || static_cast<int32_t>(parameter.GetNumberOfCandidateFloors() + parameter.GetVerticalRoomMargin() - 1) <= room1->GetForeground())
					{
						direction.Z = 0;
					}
					else
					{
						const auto limit = std::sin(math::ToRadian(5.625));
						direction.Z = std::max(-limit, std::min(direction.Z, limit));
					}
					direction.Normalize();

					// 中心が一致してしまったので適当な方向に押し出す
					if (direction.SizeSquared() == 0.)
					{
						const double ratio = parameter.GetRandom()->Get<double>();
						const double radian = ratio * (3.14159265359 * 2.);
						direction.X = std::cos(radian);
						direction.Y = std::sin(radian);
					}

					// 押し出し範囲を設定
					const std::array<Plane, 6> planes =
					{
						Plane(FVector(-1.,  0.,  0.), FVector( contactHalfSize.X, 0., 0.)),	// +X
						Plane(FVector( 1.,  0.,  0.), FVector(-contactHalfSize.X, 0., 0.)),	// -X
						Plane(FVector( 0., -1.,  0.), FVector(0.,  contactHalfSize.Y, 0.)),	// +Y
						Plane(FVector( 0.,  1.,  0.), FVector(0., -contactHalfSize.Y, 0.)),	// -Y
						Plane(FVector( 0.,  0., -1.), FVector(0., 0.,  contactHalfSize.Z)),	// +Z
						Plane(FVector( 0.,  0.,  1.), FVector(0., 0., -contactHalfSize.Z)),	// -Z
					};

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

					// room1の中心を押し出し位置へ移動
					const FVector room1Size(
						static_cast<double>(room1->GetWidth()),
						static_cast<double>(room1->GetDepth()),
						static_cast<double>(room1->GetHeight())
					);
					const FVector newRoom1Location = room0Center + newRoomOffset - room1Size * 0.5;
					room1->SetX(static_cast<int32_t>(std::round(newRoom1Location.X)));
					room1->SetY(static_cast<int32_t>(std::round(newRoom1Location.Y)));
					if (parameter.GetNumberOfCandidateFloors() > 1)
						room1->SetZ(static_cast<int32_t>(std::floor(newRoom1Location.Z)));

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
					// 交差していないか再確認
					if (room0->Intersect(*room1, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()))
					{
						DUNGEON_GENERATOR_LOG(TEXT("direction %f,%f,%f"), direction.X, direction.Y, direction.Z);
						DUNGEON_GENERATOR_LOG(TEXT("Room0: X=%d~%d,Y=%d~%d,Z=%d~%d"), room0->GetLeft(), room0->GetRight(), room0->GetTop(), room0->GetBottom(), room0->GetBackground(), room0->GetForeground());
						DUNGEON_GENERATOR_LOG(TEXT("Room1: X=%d~%d,Y=%d~%d,Z=%d~%d"), room1->GetLeft(), room1->GetRight(), room1->GetTop(), room1->GetBottom(), room1->GetBackground(), room1->GetForeground());
						check(false);
						room0->Intersect(*room1, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin());
					}
#endif
				}
			}

			++imageNo;
		} while (imageNo < maxImageNo && retry);

		if (imageNo >= maxImageNo && retry)
		{
			auto room0iterator = mRooms.begin();
			while (room0iterator != mRooms.end())
			{
				auto room1iterator = mRooms.begin();
				while (room1iterator != mRooms.end())
				{
					if (room0iterator == room1iterator || (*room0iterator)->Intersect(*(*room1iterator), parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()) == false)
					{
						++room1iterator;
					}
					else
					{
						room1iterator = mRooms.erase(room1iterator);
					}
				}

				++room0iterator;
			}

			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					if (room0 != room1 && room0->Intersect(*room1, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()))
					{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
						GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_SeparateRooms_failure.bmp");
#endif
						DUNGEON_GENERATOR_ERROR(TEXT("Generator::SeparateRooms: The room crossing was not resolved."));
						mLastError = Error::SeparateRoomsFailed;
						return false;
					}
				}
			}
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_SeparateRooms_success.bmp");
#endif
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: %d,X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	bool Generator::ExpandSpace(GenerateParameter& parameter, const int32_t margin) noexcept
	{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace"));
#endif

		int32_t minX, minY, minZ;
		int32_t maxX, maxY, maxZ;

		minX = minY = minZ = std::numeric_limits<int32_t>::max();
		maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			minX = std::min(minX, room->GetLeft());
			minY = std::min(minY, room->GetTop());
			minZ = std::min(minZ, room->GetBackground());
			maxX = std::max(maxX, room->GetRight());
			maxY = std::max(maxY, room->GetBottom());
			maxZ = std::max(maxZ, room->GetForeground());
		}

		// 外周に余白を作る
		{
			minX -= margin;
			minY -= margin;
			maxX += margin;
			maxY += margin;
		}

		// 空間のサイズを設定
		parameter.SetWidth(maxX - minX + 1);
		parameter.SetDepth(maxY - minY + 1);
		parameter.SetHeight(maxZ - minZ + 1);

		// 空間の原点を移動（部屋の位置を移動）
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->SetX(room->GetX() - minX);
			room->SetY(room->GetY() - minY);
			room->SetZ(room->GetZ() - minZ);
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Room: W=%d,D=%d,H=%d に空間を変更しました")
			, parameter.GetWidth(), parameter.GetDepth(), parameter.GetHeight()
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

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	bool Generator::DetectFloorHeight() noexcept
	{
		mFloorHeight.clear();

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			const int32_t z = room->GetBackground();
			if (std::find(mFloorHeight.begin(), mFloorHeight.end(), z) == mFloorHeight.end())
			{
				mFloorHeight.emplace_back(z);
			}
		}

		std::stable_sort(mFloorHeight.begin(), mFloorHeight.end());

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeight: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/*
	ドロネー三角形分割した辺を最小スパニングツリーにて抽出
	*/
	bool Generator::ExtractionAisles(const GenerateParameter& parameter, const size_t phase) noexcept
	{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Extract Aisles"));
		DUNGEON_GENERATOR_LOG(TEXT("%d rooms detected"), mRooms.size());
#endif

		// すべての部屋の中点を記録します
		std::vector<std::shared_ptr<const Point>> points;
		points.reserve(mRooms.size());
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// Room::GetGroundCenterリストを作成
			// cppcheck-suppress [useStlAlgorithm]
			points.emplace_back(std::make_shared<const Point>(room));
		}

		uint8_t aisleComplexity = parameter.GetAisleComplexity();
		if (aisleComplexity > 0)
		{
			float candidateAisleComplexity = std::sqrt(parameter.GetNumberOfCandidateRooms());
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
			MinimumSpanningTree minimumSpanningTree(parameter.GetRandom(), delaunayTriangulation, aisleComplexity);
			if (!GenerateAisle(minimumSpanningTree))
				return false;
			// TODO:関数名を適切にして下さい
			mDistance = minimumSpanningTree.GetDistance();
		}
		else
		{
			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(parameter.GetRandom(), points, aisleComplexity);
			if (!GenerateAisle(minimumSpanningTree))
				return false;
			// TODO:関数名を適切にして下さい
			mDistance = minimumSpanningTree.GetDistance();
		}

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return mLastError == Error::Success;
	}

	// ExtractionAislesから呼ばれる
	bool Generator::GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept
	{
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

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
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

		// ゴール位置を記録
		mGoalPoint = minimumSpanningTree.GetGoalPoint();
		if (mGoalPoint == nullptr)
		{
			if (mRooms.empty())
				return false;
			mGoalPoint = std::make_shared<Point>(mRooms.front());
		}

		// 全てのはなれ部屋を記録
		minimumSpanningTree.EachLeafPoint([this](const std::shared_ptr<const Point>& point)
			{
				mLeafPoints.emplace_back(point);
			}
		);

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/*
	スタート部屋およびゴール部屋のサブレベルが指定されていた場合に
	サブレベルが入る空間の範囲を空ける
	*/
	bool Generator::AdjustedStartAndGoalSublevel(GenerateParameter& parameter) noexcept
	{
		Stopwatch stopwatch;

		bool isRearrangeRoomsReserved = false;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			uint32_t width = room->GetWidth();
			uint32_t depth = room->GetDepth();
			constexpr uint8_t MinimumArea = 2;
			const uint32_t requiredArea = std::max(MinimumArea, room->GetGateCount());
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
				isRearrangeRoomsReserved = true;
			}
		}

		const bool isGenerateStartRoomReserved = (parameter.IsGenerateStartRoomReserved() && mStartPoint && mStartPoint->GetOwnerRoom());
		const bool isGenerateGoalRoomReserved = (parameter.IsGenerateGoalRoomReserved() && mGoalPoint && mGoalPoint->GetOwnerRoom());
		if (isGenerateStartRoomReserved)
		{
			const std::shared_ptr<Room>& room = mStartPoint->GetOwnerRoom();
			room->SetWidth(parameter.GetStartRoomSize().X);
			room->SetDepth(parameter.GetStartRoomSize().Y);
			room->SetHeight(parameter.GetStartRoomSize().Z);
		}
		if (isGenerateGoalRoomReserved)
		{
			const std::shared_ptr<Room>& room = mGoalPoint->GetOwnerRoom();
			room->SetWidth(parameter.GetGoalRoomSize().X);
			room->SetDepth(parameter.GetGoalRoomSize().Y);
			room->SetHeight(parameter.GetGoalRoomSize().Z);
		}

		if (isRearrangeRoomsReserved || isGenerateStartRoomReserved || isGenerateGoalRoomReserved)
		{
			// 部屋の分離
			stopwatch.Start();
			const bool resultSeparateRooms2 = SeparateRooms(parameter, 4);
			DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: %lf seconds"), stopwatch.Lap());
			if (!resultSeparateRooms2)
				return false;

			// 全ての部屋が収まるように空間を拡張します
			stopwatch.Start();
			const bool resultExpandSpace2 = ExpandSpace(parameter, 1);
			DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: %lf seconds"), stopwatch.Lap());
			if (!resultExpandSpace2)
				return false;

			// 部屋の中心位置をリセット
			if (isGenerateStartRoomReserved)
			{
				std::const_pointer_cast<Point>(mStartPoint)->ResetByRoomGroundCenter();
			}
			if (isGenerateGoalRoomReserved)
			{
				std::const_pointer_cast<Point>(mGoalPoint)->ResetByRoomGroundCenter();
			}

			for (Aisle& aisle : mAisles)
			{
				for (uint_fast8_t i = 0; i < 2; ++i)
				{
					if (const std::shared_ptr<Point>& point = std::const_pointer_cast<Point>(aisle.GetPoint(i)))
					{
						std::const_pointer_cast<Point>(point)->ResetByRoomGroundCenter();
					}
				}
			}
		}

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSublevel: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		{
			bmp::Canvas canvas(Scale(parameter.GetWidth()), Scale(parameter.GetDepth()));

			for (const auto& room : mRooms)
			{
				bmp::RGBCOLOR color;
				if (mStartPoint && room->Contain(*mStartPoint))
				{
					color = startColor;
				}
				else if (mGoalPoint && room->Contain(*mGoalPoint))
				{
					color = goalColor;
				}
				else
				{
					const bool isLeaf = std::any_of(mLeafPoints.begin(), mLeafPoints.end(), [room](const std::shared_ptr<const Point>& point)
						{
							return room->Contain(*point);
						}
					);
					if (isLeaf)
					{
						color = leafColor;
					}
					else
					{
						const float ratio = static_cast<float>(room->GetZ()) / static_cast<float>(parameter.GetHeight());
						color.rgbRed = baseDarkColor.rgbRed + (baseLightColor.rgbRed - baseDarkColor.rgbRed) * ratio;
						color.rgbGreen = baseDarkColor.rgbGreen + (baseLightColor.rgbGreen - baseDarkColor.rgbGreen) * ratio;
						color.rgbBlue = baseDarkColor.rgbBlue + (baseLightColor.rgbBlue - baseDarkColor.rgbBlue) * ratio;
					}
				}
				canvas.Rectangle(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), color);
			}
			for (const auto& room : mRooms)
			{
				canvas.Frame(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), frameColor);
			}

			canvas.Write(dungeon::GetDebugDirectoryString() + "/debug/" + std::to_string(5) + "_AdjustedSublevel.bmp");
		}
#endif

		return true;
	}

	void Generator::SetRoomParts(GenerateParameter& parameter) noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (mStartPoint && room->Contain(*mStartPoint))
			{
				room->SetParts(Room::Parts::Start);
			}
			else if (mGoalPoint && room->Contain(*mGoalPoint))
			{
				room->SetParts(Room::Parts::Goal);
			}
			else
			{
				const bool isLeaf = std::any_of(mLeafPoints.begin(), mLeafPoints.end(), [room](const std::shared_ptr<const Point>& point)
					{
						return room->Contain(*point);
					}
				);
				if (isLeaf)
				{
					room->SetParts(Room::Parts::Hanare);
				}
				else
				{
					room->SetParts(Room::Parts::Hall);
				}
			}
		}
	}

	void Generator::InvokeRoomCallbacks(GenerateParameter& parameter) noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			switch (room->GetParts())
			{
			case Room::Parts::Start:
				if (mOnStartParts)
					mOnStartParts(room);
				break;

			case Room::Parts::Goal:
				if (mOnGoalParts)
					mOnGoalParts(room);
				break;

			case Room::Parts::Hall:
			case Room::Parts::Hanare:
				if (mOnQueryParts)
					mOnQueryParts(room);
				break;

			case Room::Parts::Unidentified:
				break;
			}
		}
	}

	bool Generator::MarkBranchId(std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<Room>& room, uint8_t& branchId) noexcept
	{
		room->SetBranchId(branchId);

		size_t aisleCount = 0;
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
				++aisleCount;
		}

		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (passableAisles.find(&aisle) != passableAisles.end())
					continue;
				passableAisles.emplace(&aisle);

				if (room != room0)
				{
					if (aisleCount >= 3)
						++branchId;
					if (!MarkBranchId(passableAisles, room0, branchId))
						return false;
				}
				if (room != room1)
				{
					if (aisleCount >= 3)
						++branchId;
					if (!MarkBranchId(passableAisles, room1, branchId))
						return false;
				}
			}
		}

		return true;
	}

	bool Generator::MarkBranchId() noexcept
	{
		if (mStartPoint && mStartPoint->GetOwnerRoom())
		{
			std::unordered_set<const Aisle*> edges;
			uint8_t branchId = 0;
			return MarkBranchId(edges, mStartPoint->GetOwnerRoom(), branchId);
		}

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("Branch: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	bool Generator::GenerateVoxel(const GenerateParameter& parameter) noexcept
	{
		mVoxel = std::make_shared<Voxel>(parameter);

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Pre : RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

		// Generate room
		for (const auto& room : mRooms)
		{
			const FIntVector min(room->GetLeft(), room->GetTop(), room->GetBackground());
			const FIntVector max(room->GetRight(), room->GetBottom(), room->GetForeground());
			mVoxel->Rectangle(min, max
				, Grid::CreateFloor(parameter.GetRandom(), room->GetIdentifier().Get())
				, Grid::CreateDeck(parameter.GetRandom(), room->GetIdentifier().Get())
			);
		}

		if (mOnPreGenerateVoxel)
			mOnPreGenerateVoxel(mVoxel);

		// Sort by shortest aisle distance
		// 通路の距離が短い順に並べ替える
		std::stable_sort(mAisles.begin(), mAisles.end(), [](const Aisle& l, const Aisle& r)
			{
				static constexpr double AlternativeRouteCost = 100. * 100.;

				double lLength = l.GetLength();
				if (l.IsMain() == false)
					lLength += AlternativeRouteCost;

				double rLength = r.GetLength();
				if (r.IsMain() == false)
					rLength += AlternativeRouteCost;

				return lLength < rLength;
			}
		);

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION) || defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
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
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Room: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
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
			
			// Find a voxel that can generate gates around the start
			// スタート地点周辺でゲートを生成できるボクセルを探す
			std::map<size_t, FIntVector> startToGoal, goalToStart;
			const bool foundBothGates = 
				mVoxel->SearchGateLocation(startToGoal, start, startPoint->GetOwnerRoom()->GetIdentifier(), goal, parameter.UseMissionGraph() == false) &&
				mVoxel->SearchGateLocation(goalToStart, goal, goalPoint->GetOwnerRoom()->GetIdentifier(), start, parameter.UseMissionGraph() == false);
			if (foundBothGates)
			{
				bool complete = false;
				for (const auto& startCandidate : startToGoal)
				{
					for (const auto& goalCandidate : goalToStart)
					{
						// Aisle generation by A*.
						const bool generateIntersections = /*aisle.IsAnyLocked() == false ||*/ parameter.IsAisleComplexity();
						if (mVoxel->Aisle(startCandidate.second, goalCandidate.second, pathGoalCondition, aisle.GetIdentifier(), parameter.IsMergeRooms(), generateIntersections))
						{
							// Put a lock on the gate of the back room if necessary.
							Grid grid = mVoxel->Get(startCandidate.second.X, startCandidate.second.Y, startCandidate.second.Z);
							check(grid.GetProps() == Grid::Props::None);
							if (grid.GetProps() == Grid::Props::None)
							{
								if (aisle.IsUniqueLocked())
									grid.SetProps(Grid::Props::UniqueLock);
								else if (aisle.IsLocked())
									grid.SetProps(Grid::Props::Lock);
							}
							mVoxel->Set(startCandidate.second.X, startCandidate.second.Y, startCandidate.second.Z, grid);

							complete = true;
							// TODO:関数化して下さい
							goto ESCAPE_DOUBLE_LOOP;
						}
					}

					DUNGEON_GENERATOR_LOG(TEXT("Generator: Re-search the route. %d: ID=%d"), i, aisle.GetIdentifier().Get());
				}
				if (aisle.IsMain() == true && parameter.IsMergeRooms() == false && complete == false)
				{
#if WITH_EDITOR
					DUNGEON_GENERATOR_ERROR(TEXT("Generator: Route search failed. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), i, aisle.GetIdentifier().Get(), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
					DumpAisleAndRoomInfomation(i);
#endif
					mLastError = Error::RouteSearchFailed;
					return false;
				}
			}
			else
			{
				// 部屋が結合されているなら通路が無くても大丈夫なはず…
				if (parameter.IsMergeRooms() == false)
				{
#if WITH_EDITOR
					DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a gate that can be generated. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), i, aisle.GetIdentifier().Get(), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
					DumpAisleAndRoomInfomation(i);
#endif
					mLastError = Error::GateSearchFailed;
					return false;
				}
			}
ESCAPE_DOUBLE_LOOP:
			;

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			// 通信同期用に現在の乱数の種を出力する
			{
				uint32_t x, y, z, w;
				GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
				const uint32_t crc32 = CalculateCRC32();
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Aisle: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, (%d,%d,%d)-(%d,%d,%d)"), x, y, z, w, crc32, start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
			}
#endif
		}

		if (mOnPostGenerateVoxel)
			mOnPostGenerateVoxel(mVoxel);

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Post: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
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

	const size_t Generator::FindFloor(const int32_t height) const
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

		if (IsRoutePassable(room))
			result.push_back(room);

		std::unordered_set<const Aisle*> passableAisles;
		FindByRoute(result, passableAisles, room);

		result.shrink_to_fit();
		return result;
	}

	/*
	入力された部屋から経路検索上通過可能な部屋を検索します
	@param[inout]	passableRooms	通過可能な部屋の一覧
	@param[inout]	passableAisles	通過可能な通路の一覧
	@param[in]		room			検索する部屋
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
				if (passableAisles.find(&aisle) != passableAisles.end())
					continue;
				passableAisles.emplace(&aisle);

				if (room == room0)
				{
					if (IsRoutePassable(room1))
						passableRooms.push_back(room1);
					FindByRoute(passableRooms, passableAisles, room1);
				}
				else
				{
					if (IsRoutePassable(room0))
						passableRooms.push_back(room0);
					FindByRoute(passableRooms, passableAisles, room0);
				}
			}
		}
	}

	/*
	入力された部屋が経路検索上通過可能か判定します
	*/
	bool Generator::IsRoutePassable(const std::shared_ptr<Room>& room) noexcept
	{
		return
			room->GetItem() == Room::Item::Empty &&
			room->GetParts() != Room::Parts::Unidentified &&
			room->GetParts() != Room::Parts::Start &&
			room->GetParts() != Room::Parts::Goal;
	}

	const Grid& Generator::GetGrid(const FIntVector& location) const noexcept
	{
		return mVoxel->Get(location.X, location.Y, location.Z);
	}

	uint32_t Generator::CalculateCRC32(uint32_t hash) const noexcept
	{
		return mVoxel ? mVoxel->CalculateCRC32(hash) : hash;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下はデバッグに関する関数です。
	////////////////////////////////////////////////////////////////////////////////////////////////
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
				stream << "Identifier:" << std::to_string(room->GetIdentifier().Get()) << std::endl;
				stream << "Branch:" << std::to_string(room->GetBranchId()) << std::endl;
				stream << "Depth:" << std::to_string(room->GetDepthFromStart()) << std::endl;
				if (Room::Item::Empty != room->GetItem())
					stream << "Item:" << room->GetItemName() << std::endl;
				stream << ")" << std::endl;
			}

			if (mStartPoint)
			{
				std::unordered_set<const Aisle*> edges;
				DumpRoomDiagram(stream, edges, mStartPoint->GetOwnerRoom());
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
				if (passableAisles.find(&aisle) != passableAisles.end())
					continue;
				passableAisles.emplace(&aisle);

				std::string label;
				label = ": Identifier:" + std::to_string(aisle.GetIdentifier().Get());
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

	void Generator::GenerateRoomImageForDebug(const std::string& filename) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		int32_t minX, minY;
		int32_t maxX, maxY;

		minX = minY = std::numeric_limits<int32_t>::max();
		maxX = maxY = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			minX = std::min(minX, room->GetLeft());
			minY = std::min(minY, room->GetTop());
			maxX = std::max(maxX, room->GetRight());
			maxY = std::max(maxY, room->GetBottom());
		}

		const int32_t width = maxX - minX + 2;
		const int32_t height = maxY - minY + 2;
		const int32_t offsetX = -minX + 1;
		const int32_t offsetY = -minY + 1;

		// 空間のサイズを設定
		bmp::Canvas canvas(Scale(width), Scale(height));
		for (const auto& room : mRooms)
		{
			canvas.Rectangle(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetY + room->GetTop()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetY + room->GetBottom()),
				baseDarkColor
			);
		}
		for (const auto& room : mRooms)
		{
			canvas.Frame(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetY + room->GetTop()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetY + room->GetBottom()),
				frameColor
			);
		}

		canvas.Write(dungeon::GetDebugDirectoryString() + filename);
#endif
	}

	void Generator::GenerateHeightImageForDebug(const PerlinNoise& perlinNoise, const float noiseBoostRatio, const std::string& filename) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		{
			constexpr size_t width = 512;

			bmp::Canvas canvas(width, width);
			for (size_t y = 0; y < width; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					float noise = perlinNoise.Noise(
						static_cast<float>(x) / static_cast<float>(width),
						static_cast<float>(y) / static_cast<float>(width)
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

#if WITH_EDITOR
	void Generator::DumpAisleAndRoomInfomation(const size_t index) const noexcept
	{
		for (size_t j = 0; j < index; ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = a.GetIdentifier().Get();
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = a.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get();
			const auto gid = a.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get();
			DUNGEON_GENERATOR_ERROR(TEXT("OK .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		{
			const Aisle& a = mAisles[index];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = a.GetIdentifier().Get();
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = a.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get();
			const auto gid = a.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get();
			DUNGEON_GENERATOR_ERROR(TEXT("NG .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), index, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		for (size_t j = index + 1; j < mAisles.size(); ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = a.GetIdentifier().Get();
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = a.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get();
			const auto gid = a.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get();
			DUNGEON_GENERATOR_ERROR(TEXT("-- .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}

		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d) center(%f, %f, %f)")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
		}
	}
#endif
}
