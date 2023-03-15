/**
ダンジョン生成ソースファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Generator.h"
#include "GenerateParameter.h"
#include "DelaunayTriangulation3D.h"
#include "MinimumSpanningTree.h"
#include "PathGoalCondition.h"
#include "Voxel.h"
#include "Debug/BuildInfomation.h"
#include "Debug/Debug.h"
#include "Debug/Stopwatch.h"
#include "Math/Math.h"
#include "Math/PerlinNoise.h"
#include "Math/Plane.h"
#include "Math/Vector.h"

#include "MissionGraph/MissionGraph.h"

#if WITH_EDITOR && JENKINS_FOR_DEVELOP
#include <Misc/Paths.h>

// 定義するとデバッグに便利なログを出力します
//#define DEBUG_SHOW_DEVELOP_LOG

// 定義すると途中経過と最終結果をBMPで出力します（デバッグ機能）
//#define DEBUG_GENERATE_BITMAP_FILE

// 定義すると最終結果をBMPで出力します（デバッグ機能）
//#define DEBUG_GENERATE_RESULT_BITMAP_FILE
#endif

namespace dungeon
{
#if defined(DEBUG_GENERATE_BITMAP_FILE) | defined(DEBUG_GENERATE_RESULT_BITMAP_FILE)
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

	Generator::Generator() noexcept
	{
	}

	Generator::~Generator() noexcept
	{
	}

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

	void Generator::Generate(const GenerateParameter& parameter) noexcept
	{
		mLastError = Error::Success;
		mGenerateParameter = parameter;

		static constexpr std::uint8_t maxRetryCount = 3;
		std::uint_fast8_t retryCount = 0;
		do
		{
			// 生成
			GenerateImpl(mGenerateParameter);

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

			++retryCount;
		} while (mLastError != Generator::Error::Success && retryCount < maxRetryCount);

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		// 部屋の情報をダンプ
		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: Position(%d, %d, %d) Size(%d, %d, %d) Parts=%d, DepthFromStart=%d"),
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

		{
			const FString path = FPaths::ProjectSavedDir() + TEXT("/dungeon_aisle.txt");
			DumpAisle(TCHAR_TO_UTF8(*path));
		}
#endif
	}

	bool Generator::GenerateImpl(GenerateParameter& parameter) noexcept
	{
		Stopwatch stopwatch;

		Reset();

		// 部屋の生成
		stopwatch.Start();
		if (!GenerateRooms(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("GenerateRooms: %lf sec"), stopwatch.Lap());

		// 部屋の分離
		stopwatch.Start();
		if (!SeparateRooms(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("SeparateRooms: %lf sec"), stopwatch.Lap());

		// 全ての部屋が収まるように空間を拡張します
		stopwatch.Start();
		if (!ExpandSpace(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: %lf sec"), stopwatch.Lap());

		// 重複した部屋や範囲外の部屋を除去
		stopwatch.Start();
		if (!RemoveInvalidRooms(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("RemoveInvalidRooms: %lf sec"), stopwatch.Lap());

		// 通路の生成
		stopwatch.Start();
		if (!ExtractionAisles(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: %lf sec"), stopwatch.Lap());

		// ブランチIDの生成
		stopwatch.Start();
		if (!Branch())
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("Branch: %lf sec"), stopwatch.Lap());

		// 階層情報の生成
		stopwatch.Start();
		if (!DetectFloorHeight())
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeight: %lf sec"), stopwatch.Lap());

		// 部屋と通路に意味付けする
		stopwatch.Start();
		MissionGraph missionGraph(shared_from_this(), mGoalPoint);
		DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: %lf sec"), stopwatch.Lap());

		// ボクセル情報を生成します
		stopwatch.Start();
		if (!GenerateVoxel(parameter))
			return false;
		DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: %lf sec"), stopwatch.Lap());

		return true;
	}

	/**
	部屋の生成
	*/
	bool Generator::GenerateRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Generate Rooms"));
#endif

		PerlinNoise perlinNoise(parameter.GetRandom());
		constexpr float noiseBoostRatio = 1.333f;

		float range = std::max(1.f, std::sqrtf(parameter.GetNumberOfCandidateRooms()));
		float maxRoomWidth = std::min(parameter.GetMinRoomWidth(), parameter.GetMinRoomDepth());
		maxRoomWidth += parameter.GetHorizontalRoomMargin();
		range *= maxRoomWidth;

		for (size_t i = 0; i < parameter.GetNumberOfCandidateRooms(); ++i)
		{
			const float radian = parameter.GetRandom().Get<float>() * (3.14159265359f * 2.f);
			const float width = std::sin(radian);
			const float depth = std::cos(radian);
			const float height = std::max(0.f, static_cast<float>(parameter.GetNumberOfCandidateFloors()) - 1);
			const float distance = parameter.GetRandom().Get<float>(range);
			float noise = perlinNoise.Noise(width, depth);
			noise = noise * 0.5f + 0.5f;
			noise *= noiseBoostRatio;
			noise = std::max(0.f, std::min(noise, 1.f));

			FIntVector location(
				static_cast<int32_t>(std::round(width * distance)),
				static_cast<int32_t>(std::round(depth * distance)),
				static_cast<int32_t>(std::round(height * noise))
			);

			if (parameter.GetHorizontalRoomMargin() == 0)
			{
				/*
				RoomMarginが0の場合、部屋と部屋の間にスロープを作る隙間が無いので、
				部屋の高さを必ず同じにする必要がある。
				*/
				location.Z = 0;
			}

			auto room = std::make_shared<Room>(parameter, location);
#if defined(DEBUG_SHOW_DEVELOP_LOG)
			DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d center(%f, %f, %f)")
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
#endif
			mRooms.emplace_back(std::move(room));
		}

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		// サブレベル配置テスト用の部屋
		//mRooms.emplace_back(std::make_shared<Room>(16, 15, 5, 3, 3, 1));
#endif

		GenerateRoomImageForDebug("generator_1.bmp");

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
			const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectSavedDir() + "/DungeonGenerator/perlinNoize.bmp"));
			canvas.Write(filename);
		}
#endif

		return true;
	}

	/**
	部屋の重なりを解消します
	*/
	bool Generator::SeparateRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
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
			retry = false;

			for (const std::shared_ptr<const Room>& room0 : mRooms)
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
					const FVector contactHalfSize = contactSize * 0.5;

					// room1を押し出す中心を求める
					const FVector& contactPoint = room0->GetCenter();

					// room1を押し出す方向を求める
					FVector direction = room1->GetCenter() - contactPoint;

					// 水平方向への移動を優先
					if (room1->GetBackground() <= 0 || (parameter.GetNumberOfCandidateFloors() - 1) <= room1->GetForeground())
						direction.Z = 0;

					// 中心が一致してしまったので適当な方向に押し出す
					if (direction.SizeSquared() == 0.)
					{
						const double ratio = parameter.GetRandom().Get<double>();
						const double radian = ratio * (3.14159265359 * 2.);
						direction.X = std::cos(radian);
						direction.Y = std::sin(radian);
					}

					// 押し出し範囲を設定
					const double margin = static_cast<double>(parameter.GetHorizontalRoomMargin());
					const std::array<Plane, 4> planes =
					{
						Plane(FVector(-1.,  0.,  0.), contactPoint + FVector( contactHalfSize.X + margin, 0., 0.)),	// +X
						Plane(FVector( 1.,  0.,  0.), contactPoint + FVector(-contactHalfSize.X - margin, 0., 0.)),	// -X
						Plane(FVector( 0., -1.,  0.), contactPoint + FVector(0.,  contactHalfSize.Y + margin, 0.)),	// +Y
						Plane(FVector( 0.,  1.,  0.), contactPoint + FVector(0., -contactHalfSize.Y - margin, 0.)),	// -Y
					};

					FVector newRoomCenter;
					double minimumDistance = std::numeric_limits<double>::max();
					for (const auto& plane : planes)
					{
						FVector roomCenter;
						if (plane.Intersect(contactPoint, direction, roomCenter))
						{
							const double distance = FVector::DistSquared(contactPoint, roomCenter);
							if (minimumDistance > distance)
							{
								minimumDistance = distance;
								newRoomCenter = roomCenter;
							}
						}
					}

					// room1の中心を移動
					const double room1HalfWidth = static_cast<double>(room1->GetWidth()) * .5f;
					const double room1HalfDepth = static_cast<double>(room1->GetDepth()) * .5f;
					room1->SetX(static_cast<int32_t>(std::floor(newRoomCenter.X - room1HalfWidth)));
					room1->SetY(static_cast<int32_t>(std::floor(newRoomCenter.Y - room1HalfDepth)));
#if 0
					// 交差していないか再確認
					if (room0->Intersect(*room1, parameter.GetHorizontalRoomMargin()))
					{
						DUNGEON_GENERATOR_LOG(TEXT("direction %f,%f"), direction.X, direction.Y);
						DUNGEON_GENERATOR_LOG(TEXT("Room0: L=%d,R=%d,T=%d,B=%d W=%d,H=%d"), room0->GetLeft(), room0->GetRight(), room0->GetTop(), room0->GetBottom(), room0->GetWidth(), room0->GetDepth());
						DUNGEON_GENERATOR_LOG(TEXT("Room1: L=%d,R=%d,T=%d,B=%d W=%d,H=%d"), room1->GetLeft(), room1->GetRight(), room1->GetTop(), room1->GetBottom(), room1->GetWidth(), room1->GetDepth());
						check(false);
					}
#endif
				}
			}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
			if (retry)
			{
				std::string filename = "generator_2_" + std::to_string(imageNo) + ".bmp";
				GenerateRoomImageForDebug(filename);
			}
#endif

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

			for (const std::shared_ptr<const Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					if (room0 != room1 && room0->Intersect(*room1, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()))
					{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
						GenerateRoomImageForDebug("generator_2_failure.bmp");
#endif
						DUNGEON_GENERATOR_ERROR(TEXT("Generator::SeparateRooms: The room crossing was not resolved."));
						mLastError = Error::SeparateRoomsFailed;
						return false;
					}
				}
			}
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("generator_2_success.bmp");
#endif
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

		return true;
	}

	bool Generator::ExpandSpace(GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace"));
#endif

		int32_t minX, minY, minZ;
		int32_t maxX, maxY, maxZ;

		minX = minY = minZ = std::numeric_limits<int32_t>::max();
		maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			minX = std::min(minX, room->GetLeft());
			minY = std::min(minY, room->GetTop());
			minZ = std::min(minZ, room->GetBackground());
			maxX = std::max(maxX, room->GetRight());
			maxY = std::max(maxY, room->GetBottom());
			maxZ = std::max(maxZ, room->GetForeground());
		}

		// 空間のサイズを設定
		parameter.mWidth = maxX - minX + 1;
		parameter.mDepth = maxY - minY + 1;
		parameter.mHeight = maxZ - minZ + 1;

		// 空間の原点を移動（部屋の位置を移動）
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->SetX(room->GetX() - minX);
			room->SetY(room->GetY() - minY);
			room->SetZ(room->GetZ() - minZ);
		}

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Room: W=%d,D=%d,H=%d に空間を変更しました")
			, parameter.mWidth, parameter.mDepth, parameter.mHeight
		);
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

		return true;
	}

	/**
	重複した部屋や範囲外の部屋を除去をします
	*/
	bool Generator::RemoveInvalidRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Remove duplicate rooms and out-of-range rooms"));
#endif

		auto result = std::remove_if(mRooms.begin(), mRooms.end(), [&parameter, this](const std::shared_ptr<Room>& room) -> bool
			{
				// 範囲外の部屋なら削除
				if (
					room->GetLeft() < 0 || static_cast<int32_t>(parameter.GetWidth()) <= room->GetRight() ||
					room->GetTop() < 0 || static_cast<int32_t>(parameter.GetDepth()) <= room->GetBottom() ||
					room->GetBackground() < 0 || static_cast<int32_t>(parameter.GetHeight()) <= room->GetForeground())
				{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
					DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d W=%d,H=%d center(%f, %f) 範囲外")
						, room->GetX(), room->GetY(), room->GetWidth(), room->GetDepth()
						, room->GetCenter().X, room->GetCenter().Y
					);
#endif
					return true;
				}

				// 他の部屋に含まれている？
				for (const auto& otherRoom : mRooms)
				{
					// 無効な部屋はスキップ
					if (otherRoom == nullptr)
						continue;
					// 自分自身はスキップ
					if (otherRoom == room)
						continue;
					// 内包？
					if (room->Intersect(*otherRoom, parameter.GetHorizontalRoomMargin(), parameter.GetVerticalRoomMargin()) == true)
						return true;
				}

				return false;
			}
		);
		mRooms.erase(result, mRooms.end());

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		bmp::Canvas canvas(Scale(parameter.GetWidth()), Scale(parameter.GetDepth()));
		for (const auto& room : mRooms)
		{
			canvas.Rectangle(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), baseDarkColor);
		}
		for (const auto& room : mRooms)
		{
			canvas.Frame(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), frameColor);
		}
		const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectSavedDir() + "/DungeonGenerator/generator_3.bmp"));
		canvas.Write(filename);
#endif

		return true;
	}

	bool Generator::DetectFloorHeight() noexcept
	{
		mFloorHeight.clear();

		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			const int32_t z = room->GetBackground();
			if (std::find(mFloorHeight.begin(), mFloorHeight.end(), z) == mFloorHeight.end())
			{
				mFloorHeight.emplace_back(z);
			}
		}

		std::sort(mFloorHeight.begin(), mFloorHeight.end());

		return true;
	}

	// ドロネー三角形分割した辺を最小スパニングツリーにて抽出
	bool Generator::ExtractionAisles(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
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

		if (mRooms.size() >= 4)
		{
			// 三角形分割
			DelaunayTriangulation3D delaunayTriangulation(points);

#if WITH_EDITOR
			if (!delaunayTriangulation.IsValid())
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Generator:三角形分割に失敗しました %d rooms"), mRooms.size());
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
			MinimumSpanningTree minimumSpanningTree(delaunayTriangulation);
			GenerateAisle(minimumSpanningTree);
			// TODO:関数名を適切にして下さい
			mDistance = minimumSpanningTree.GetDistance();
		}
		else
		{
			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(points);
			GenerateAisle(minimumSpanningTree);
			// TODO:関数名を適切にして下さい
			mDistance = minimumSpanningTree.GetDistance();
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE) | defined(DEBUG_GENERATE_RESULT_BITMAP_FILE)
		bmp::Canvas canvas(Scale(parameter.GetWidth()), Scale(parameter.GetDepth()));

		for (const auto& room : mRooms)
		{
			bmp::RGBCOLOR color;
			if (room->Contain(*mStartPoint))
			{
				color = startColor;
			}
			else if (room->Contain(*mGoalPoint))
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

		const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectSavedDir() + "/DungeonGenerator/generator_4.bmp"));
		canvas.Write(filename);
#endif

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (room->Contain(*mStartPoint))
			{
				room->SetParts(Room::Parts::Start);
			}
			else if (room->Contain(*mGoalPoint))
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

			if (mQueryParts)
			{
				mQueryParts(room);
			}
		}

		return mLastError == Error::Success;
	}

	float Generator::GetDistanceCenterToContact(const float width, const float depth, const FVector& direction, const float margin) const noexcept
	{
		const float halfWidth = (width + margin) * 0.5f;
		const float halfDepth = (depth + margin) * 0.5f;
		const float ty = (direction.X != 0.f)
			? std::abs(direction.Y * (halfWidth / direction.X))
			: std::numeric_limits<float>::max();
		if (ty < halfDepth)
		{
			// 側面と交差
			return std::sqrt(math::Square(ty) + math::Square(halfWidth));
		}
		else if (direction.Y != 0.f)
		{
			// 上下面と交差
			const float tx = direction.X * (halfDepth / direction.Y);
			return std::sqrt(math::Square(tx) + math::Square(halfDepth));
		}
		return 0.f;
	}

	bool Generator::GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept
	{
		minimumSpanningTree.ForEach([this](const Aisle& edge)
			{
				std::shared_ptr<Point> a = std::make_shared<Point>(edge.GetPoint(0)->GetOwnerRoom());
				std::shared_ptr<Point> b = std::make_shared<Point>(edge.GetPoint(1)->GetOwnerRoom());
				{
					auto direction = *b - *a;
					direction.Z = 0;
					const float distance = GetDistanceCenterToContact(
						a->GetOwnerRoom()->GetWidth(),
						a->GetOwnerRoom()->GetDepth(),
						direction,
						-0.5f
					);
					*a += direction.GetSafeNormal() * distance;
					check(a->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*a)));
				}
				{
					auto direction = *a - *b;
					direction.Z = 0;
					const float distance = GetDistanceCenterToContact(
						b->GetOwnerRoom()->GetWidth(),
						b->GetOwnerRoom()->GetDepth(),
						direction,
						-0.5f
					);
					*b += direction.GetSafeNormal() * distance;
					check(b->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*b)));
				}
				mAisles.emplace_back(a, b);
			}
		);

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("%d minimum spanning tree edges detected"), minimumSpanningTree.Size());
#endif

		mStartPoint = minimumSpanningTree.GetStartPoint();
		mGoalPoint = minimumSpanningTree.GetGoalPoint();
		minimumSpanningTree.EachLeafPoint([this](const std::shared_ptr<const Point>& point)
			{
				mLeafPoints.emplace_back(point);
			}
		);

		return true;
	}

	bool Generator::GenerateVoxel(const GenerateParameter& parameter) noexcept
	{
		mVoxel = std::make_shared<Voxel>(parameter);

		// 部屋を生成
		for (const auto& room : mRooms)
		{
			const FIntVector min(room->GetLeft(), room->GetTop(), room->GetBackground());
			const FIntVector max(room->GetRight(), room->GetBottom(), room->GetForeground());
			mVoxel->Rectangle(min, max
				, Grid::CreateFloor(parameter.GetRandom(), room->GetIdentifier().Get())
				, Grid::CreateDeck(parameter.GetRandom(), room->GetIdentifier().Get())
			);
		}

		// 通路の距離が短い順に並べ替える
		std::sort(mAisles.begin(), mAisles.end(), [](const Aisle& l, const Aisle& r)
			{
				return l.GetLength() < r.GetLength();
			}
		);

		// 通路を生成
		for (const Aisle& aisle : mAisles)
		{
			std::shared_ptr<const Point> s = aisle.GetPoint(0);
			std::shared_ptr<const Point> e = aisle.GetPoint(1);

			check(s->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*s)));
			check(e->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*e)));

			// Use the back room as a starting point.
			if (s->GetOwnerRoom()->GetDepthFromStart() < e->GetOwnerRoom()->GetDepthFromStart())
			{
				const std::shared_ptr<const Point> t = e;
				e = s;
				s = t;
			}

			check(s->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*s)));
			check(e->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*e)));

			const std::shared_ptr<Room>& startRoom = s->GetOwnerRoom();
			const std::shared_ptr<Room>& goalRoom = e->GetOwnerRoom();
			check(startRoom);
			check(goalRoom);
			
			FIntVector start = ToIntVector(*s);
			FIntVector goal = ToIntVector(*e);

			// start周囲に侵入可能なグリッドを探す
			FIntVector result;
			if (mVoxel->SearchGateLocation(result, start, goal, PathGoalCondition(goalRoom->GetRect()), aisle.GetIdentifier()))
			{
				start = result;
			}
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("生成可能な門が見つからない (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
				mLastError = Error::GateSearchFailed;
				return false;
			}

			// Aisle generation by A*.
			if (mVoxel->Aisle(start, goal, PathGoalCondition(goalRoom->GetRect()), aisle.GetIdentifier()))
			{
				Grid grid = mVoxel->Get(start.X, start.Y, start.Z);
				check(grid.GetProps() == Grid::Props::None);
				if (grid.GetProps() == Grid::Props::None)
				{
					if (aisle.IsUniqueLocked())
						grid.SetProps(Grid::Props::UniqueLock);
					else if (aisle.IsLocked())
						grid.SetProps(Grid::Props::Lock);
				}
				mVoxel->Set(start.X, start.Y, start.Z, grid);
			}
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("経路探索に失敗しました (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
				mLastError = Error::RouteSearchFailed;
				return false;
			}
		}

		return true;
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
		const int32_t halfWidth = width / 2 - 1;
		const int32_t halfHeight = height / 2 - 1;

		// 空間のサイズを設定
		bmp::Canvas canvas(Scale(width), Scale(height));
		for (const auto& room : mRooms)
		{
			canvas.Rectangle(
				Scale(room->GetLeft() + halfWidth),
				Scale(room->GetTop() + halfHeight),
				Scale(room->GetRight() + halfWidth),
				Scale(room->GetBottom() + halfHeight),
				baseDarkColor
			);
		}
		for (const auto& room : mRooms)
		{
			canvas.Frame(
				Scale(room->GetLeft() + halfWidth),
				Scale(room->GetTop() + halfHeight),
				Scale(room->GetRight() + halfWidth),
				Scale(room->GetBottom() + halfHeight),
				frameColor
			);
		}

		std::string path = TCHAR_TO_ANSI(*FPaths::ProjectSavedDir());
		path += "/DungeonGenerator/";
		path += filename;
		canvas.Write(path);
#endif
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
		for (const auto& room : mRooms)
		{
			// cppcheck-suppress [useStlAlgorithm]
			if (room->Contain(point))
				return room;
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindAll(const Point& point) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		for (const auto& room : mRooms)
		{
			if (room->Contain(point))
			{
				// cppcheck-suppress [useStlAlgorithm]
				result.emplace_back(room);
			}
		}
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

	std::vector<std::shared_ptr<Room>> Generator::FindByRoute(const std::shared_ptr<Room>& startRoom) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());

		// TODO::判定関数を作成して下さい
		if (startRoom->GetItem() == Room::Item::Empty &&
			startRoom->GetParts() != Room::Parts::Unidentified &&
			startRoom->GetParts() != Room::Parts::Start &&
			startRoom->GetParts() != Room::Parts::Goal)
			result.push_back(startRoom);

		std::unordered_set<const Aisle*> generatedEdges;
		FindByRoute(result, generatedEdges, startRoom);

		result.shrink_to_fit();
		return result;
	}

	void Generator::FindByRoute(std::vector<std::shared_ptr<Room>>& result, std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			if (aisle.IsLocked())
				continue;

			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();

			if (room == room0 || room == room1)
			{
				if (generatedEdges.find(&aisle) != generatedEdges.end())
					continue;
				generatedEdges.emplace(&aisle);

				if (room != room0)
				{
					// TODO::判定関数を作成して下さい
					if (room0->GetItem() == Room::Item::Empty &&
						room0->GetParts() != Room::Parts::Unidentified &&
						room0->GetParts() != Room::Parts::Start &&
						room0->GetParts() != Room::Parts::Goal)
						result.push_back(room0);
					FindByRoute(result, generatedEdges, room0);
				}
				else
				{
					// TODO::判定関数を作成して下さい
					if (room1->GetItem() == Room::Item::Empty &&
						room1->GetParts() != Room::Parts::Unidentified &&
						room1->GetParts() != Room::Parts::Start &&
						room1->GetParts() != Room::Parts::Goal)
						result.push_back(room1);
					FindByRoute(result, generatedEdges, room1);
				}
			}
		}
	}


	const Grid& Generator::GetGrid(const FIntVector& location) const noexcept
	{
		return mVoxel->Get(location.X, location.Y, location.Z);
	}





	void Generator::DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (generatedEdges.find(&aisle) != generatedEdges.end())
					continue;
				generatedEdges.emplace(&aisle);

				if (room == room0)
					stream << room0->GetName() << "--" << room1->GetName();
				else
					stream << room1->GetName() << "--" << room0->GetName();
				stream << ": Identifier:" << std::to_string(aisle.GetIdentifier().Get());
				if (aisle.IsUniqueLocked())
					stream << " (Unique lock)";
				else if(aisle.IsLocked())
					stream << " (Lock)";
				stream << std::endl;

				if (room != room0)
					DumpRoomDiagram(stream, generatedEdges, room0);
				if (room != room1)
					DumpRoomDiagram(stream, generatedEdges, room1);
			}
		}
	}

	void Generator::DumpRoomDiagram(const std::string& path) const noexcept
	{
		std::ofstream stream(path);
		if (stream.is_open())
		{
			for (const auto& room : mRooms)
			{
				stream << "rectangle " << room->GetName() << "[" << std::endl;
				stream << room->GetName() << std::endl;
				stream << "Identifier:" << std::to_string(room->GetIdentifier().Get()) << std::endl;
				stream << "Branch:" << std::to_string(room->GetBranchId()) << "(" << room->GetPartsName() << ")" << std::endl;
				stream << "Depth:" << std::to_string(room->GetDepthFromStart()) << std::endl;
				if (Room::Item::Empty != room->GetItem())
					stream << "Item:" << room->GetItemName() << std::endl;
				stream << "]" << std::endl;
			}

			if (mStartPoint)
			{
				std::unordered_set<const Aisle*> edges;
				DumpRoomDiagram(stream, edges, mStartPoint->GetOwnerRoom());
			}
		}
	}

	void Generator::DumpAisle(const std::string& path) const noexcept
	{
		std::ofstream stream(path);
		if (stream.is_open())
		{
			for (const auto& aisle : mAisles)
			{
				aisle.Dump(stream);
			}
		}
	}

	bool Generator::Branch(std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<Room>& room, uint8_t& branchId) noexcept
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
				if (generatedEdges.find(&aisle) != generatedEdges.end())
					continue;
				generatedEdges.emplace(&aisle);


				if (room != room0)
				{
					if (aisleCount >= 3)
						++branchId;
					if (!Branch(generatedEdges, room0, branchId))
						return false;
				}
				if (room != room1)
				{
					if (aisleCount >= 3)
						++branchId;
					if (!Branch(generatedEdges, room1, branchId))
						return false;
				}
			}
		}

		return true;
	}

	bool Generator::Branch() noexcept
	{
		if (mStartPoint)
		{
			std::unordered_set<const Aisle*> edges;
			uint8_t branchId = 0;
			return Branch(edges, mStartPoint->GetOwnerRoom(), branchId);
		}

		return false;
	}
}
