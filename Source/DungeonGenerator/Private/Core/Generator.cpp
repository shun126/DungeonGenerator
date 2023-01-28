/**
ダンジョン生成ソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Generator.h"
#include "GenerateParameter.h"
#include "DelaunayTriangulation3D.h"
#include "MinimumSpanningTree.h"
#include "PathGoalCondition.h"
#include "Voxel.h"
#include "Debug/Debug.h"
#include "Math/Math.h"
#include "Math/Plane.h"
#include "Math/Vector.h"

#include "MissionGraph/MissionGraph.h"

// 定義すると部屋の衝突解決を水平方向に移動します
#define HORIZONTAL_MOVEMENT_AT_IMPACT

#if WITH_EDITOR
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
	static constexpr float imageScale = 5.0f;
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
		std::srand(std::time(nullptr));
	}

	Generator::~Generator() noexcept
	{
		if (mGenerateThread.joinable())
			mGenerateThread.join();
	}

	void Generator::Generate(const GenerateParameter& parameter) noexcept
	{
		if (mGenerateThread.joinable())
			mGenerateThread.join();

		mGenerated = false;
		mGenerateFuture = mGeneratePromise.get_future();
		mGenerateParameter = parameter;

		auto sharedThis = shared_from_this();
		mGenerateThread = std::thread([sharedThis]()
			{
				// 生成
				sharedThis->GenerateImpl();

				// 完了
				sharedThis->mGenerated = true;
				sharedThis->mGeneratePromise.set_value();
			}
		);
	}

	void Generator::GenerateImpl() noexcept
	{
		mRooms.clear();

		// 部屋の生成
		GenerateRooms(mGenerateParameter);

		// 部屋の分離
		SeparateRooms(mGenerateParameter);

		// 全ての部屋が収まるように空間を拡張します
		ExpandSpace(mGenerateParameter);

		// 重複した部屋や範囲外の部屋を除去
		RemoveInvalidRooms(mGenerateParameter);

		// 通路の生成
		ExtractionAisles(mGenerateParameter);

		// ブランチIDの生成
		Branch();

		// 部屋と通路に意味付けする
		MissionGraph missionGraph(shared_from_this(), mGoalPoint);

		// ボクセル情報を生成します
		GenerateVoxel(mGenerateParameter);

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

	/**
	部屋の生成
	*/
	void Generator::GenerateRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Generate Rooms"));
#endif

		for (size_t i = 0; i < parameter.GetNumberOfCandidateRooms(); ++i)
		{
			auto room = std::make_shared<Room>(parameter);
#if defined(DEBUG_SHOW_DEVELOP_LOG)
			DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d center(%f, %f, %f)")
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
#endif
			room->SetUnderfloorHeight(parameter.mUnderfloorHeight);
			mRooms.emplace_back(std::move(room));
		}

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		// サブレベル配置テスト用の部屋
		//mRooms.emplace_back(std::make_shared<Room>(16, 15, 5, 3, 3, 1));
#endif

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
		const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectLogDir() + "generator_1.bmp"));
		canvas.Write(filename);
#endif
	}

	/**
	部屋の重なりを解消します
	*/
	void Generator::SeparateRooms(const GenerateParameter& parameter) noexcept
	{
#if defined(DEBUG_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Separate Rooms"));
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		size_t imageNo = 0;
#endif
		size_t counter = mRooms.size();
		bool retry;
		do {
			retry = false;

			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				std::vector<std::shared_ptr<Room>> intersectedRooms;

				// 他の部屋と交差している？
				for (auto room1 : mRooms)
				{
					if (room0 != room1 && room0->Intersect(*room1, parameter.GetRoomMargin()))
					{
						// 交差した部屋を記録
						// cppcheck-suppress [useStlAlgorithm]
						intersectedRooms.emplace_back(room1);
					}
				}

				// 他の部屋と交差しているなら移動
				if (intersectedRooms.size() > 0)
				{
					// 交差した部屋が重ならないように移動
					for (auto& room1 : intersectedRooms)
					{
						// 二つの部屋を合わせた空間の大きさ
						const FVector contactSize = FVector(
							room0->GetWidth() + room1->GetWidth(),
							room0->GetDepth() + room1->GetDepth(),
							room0->GetHeight() + room1->GetHeight()
						);

						// 二つの部屋を合わせた空間の半分大きさ
						const FVector contactHalfSize = contactSize * 0.5f;

						// room1を押し出す中心を求める
						const FVector& contactPoint = room0->GetCenter();

						// room1を押し出す方向を求める
						FVector direction = room1->GetCenter() - contactPoint;
#if defined(HORIZONTAL_MOVEMENT_AT_IMPACT)
						{
							// 水平方向への移動を優先
							direction.Z = 0;
						}
#endif
						if (direction.SizeSquared() == 0.f)
						{
							// 中心が一致してしまったので適当な方向に押し出す
							const float ratio = parameter.GetRandom().Get<float>();
							const float radian = ratio * (3.14159265359f * 2.f);
							direction.X = std::cos(radian);
							direction.Y = std::sin(radian);
						}

						// 押し出し範囲を設定
						const float margin = static_cast<float>(parameter.GetRoomMargin());
						const std::array<Plane, 6> planes = {
							Plane(FVector(-1.f,  0.f,  0.f), contactPoint + FVector(contactHalfSize.X + margin, 0., 0.)),	// +X
							Plane(FVector( 1.f,  0.f,  0.f), contactPoint + FVector(-contactHalfSize.X - margin, 0., 0.)),	// -X
							Plane(FVector( 0.f, -1.f,  0.f), contactPoint + FVector(0.,  contactHalfSize.Y + margin, 0.)),	// +Y
							Plane(FVector( 0.f,  1.f,  0.f), contactPoint + FVector(0., -contactHalfSize.Y - margin, 0.)),	// -Y
							Plane(FVector( 0.f,  0.f, -1.f), contactPoint + FVector(0., 0.,  contactHalfSize.Z + 0.)),		// +Z
							Plane(FVector( 0.f,  0.f,  1.f), contactPoint + FVector(0., 0., -contactHalfSize.Z - 0.)),		// -Z
						};

						FVector newRoomCenter;
						float minimumDistance = std::numeric_limits<float>::max();
						for (const auto& plane : planes)
						{
							FVector roomCenter;
							if (plane.Intersect(contactPoint, direction, roomCenter))
							{
								const float distance = FVector::DistSquared(contactPoint, roomCenter);
								if (minimumDistance > distance)
								{
									minimumDistance = distance;
									newRoomCenter = roomCenter;
								}
							}
						}

						// room1の中心を移動
						const float room1HalfWidth = static_cast<float>(room1->GetWidth()) * .5f;
						const float room1HalfDepth = static_cast<float>(room1->GetDepth()) * .5f;
#if !defined(HORIZONTAL_MOVEMENT_AT_IMPACT)
						const float room1HalfHeight = static_cast<float>(room1->GetHeight()) * .5f;
#endif
						room1->SetX(static_cast<int32_t>(std::ceil(newRoomCenter.X - room1HalfWidth)));
						room1->SetY(static_cast<int32_t>(std::ceil(newRoomCenter.Y - room1HalfDepth)));
#if !defined(HORIZONTAL_MOVEMENT_AT_IMPACT)
						room1->SetZ(static_cast<int32_t>(std::ceil(newRoomCenter.Z - room1HalfHeight)));
#endif

#if defined(DEBUG_SHOW_DEVELOP_LOG) && 0
						// 交差していないか再確認
						if (room0->Intersect(*room1, parameter.GetRoomMargin()))
						{
							DUNGEON_GENERATOR_LOG(TEXT("direction %f,%f"), direction.X, direction.Y);
							DUNGEON_GENERATOR_LOG(TEXT("Room0: L=%d,R=%d,T=%d,B=%d W=%d,H=%d"), room0->GetLeft(), room0->GetRight(), room0->GetTop(), room0->GetBottom(), room0->GetWidth(), room0->GetDepth());
							DUNGEON_GENERATOR_LOG(TEXT("Room1: L=%d,R=%d,T=%d,B=%d W=%d,H=%d"), room1->GetLeft(), room1->GetRight(), room1->GetTop(), room1->GetBottom(), room1->GetWidth(), room1->GetDepth());
							check(false);
						}
#endif
					}

					// 動いた先で交差している可能性があるので再チェック
					retry = true;
				}
			}

			--counter;

#if defined(DEBUG_GENERATE_BITMAP_FILE)
			if (retry)
			{
				bmp::Canvas canvas(Scale(parameter.GetWidth()), Scale(parameter.GetDepth()));
				for (const auto& room : mRooms)
				{
					canvas.Rectangle(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), baseDarkColor);
				}
				for (const auto& room : mRooms)
				{
					canvas.Frame(Scale(room->GetLeft()), Scale(room->GetTop()), Scale(room->GetRight()), Scale(room->GetBottom()), frameColor);
				}

				const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectLogDir() + "generator_2_" + FString::FromInt(imageNo) + ".bmp"));
				canvas.Write(filename);

				++imageNo;
			}
#endif
		} while (counter > 0 && retry);

#if defined(DEBUG_SHOW_DEVELOP_LOG)
		if (counter == 0)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Generator::SeparateRooms: Give up..."));
		}

		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif
	}

	void Generator::ExpandSpace(GenerateParameter& parameter) noexcept
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
	}

	/**
	重複した部屋や範囲外の部屋を除去をします
	*/
	void Generator::RemoveInvalidRooms(const GenerateParameter& parameter) noexcept
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
					if (room->Intersect(*otherRoom, parameter.GetRoomMargin()) == true)
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
		const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectLogDir() + "generator_3.bmp"));
#endif
	}

	// ドロネー三角形分割した辺を最小スパニングツリーにて抽出
	void Generator::ExtractionAisles(const GenerateParameter& parameter) noexcept
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
			// Room::GetFloorCenterリストを作成
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
				DUNGEON_GENERATOR_ERROR(TEXT("三角形分割が失敗 %d rooms"), mRooms.size());
				for (const auto& room : mRooms)
				{
					DUNGEON_GENERATOR_ERROR(TEXT("%d, %d, %d, %d, %d, %d"),
						room->GetX(), room->GetY(), room->GetZ(),
						room->GetWidth(), room->GetDepth(), room->GetHeight()
					);
				}
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

		const auto filename = TCHAR_TO_ANSI(*(FPaths::ProjectLogDir() + "generator_4.bmp"));
		canvas.Write(filename);
#endif

		// TODO:Branch
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

	void Generator::GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept
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
	}

	void Generator::GenerateVoxel(const GenerateParameter& parameter) noexcept
	{
		mVoxel = std::make_shared<Voxel>(parameter);

		// 部屋を生成
		for (const auto& room : mRooms)
		{
			const FIntVector min(room->GetLeft(), room->GetTop(), room->GetBackground());
			const FIntVector max(room->GetRight(), room->GetBottom(), room->GetForeground());
			mVoxel->Rectangle(min, max
				, Grid::CreateFloor(mGenerateParameter.GetRandom(), room->GetIdentifier().Get())
				, Grid::CreateDeck(mGenerateParameter.GetRandom(), room->GetIdentifier().Get())
			);
		}

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
			const PathGoalCondition goalCondition(goalRoom->GetRect());

			// Aisle generation by A*.
			const FIntVector start = ToIntVector(*s);
			if (mVoxel->Aisle(start, ToIntVector(*e), goalCondition, aisle.GetIdentifier()))
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
		}
	}

	bool Generator::IsGenerated() const noexcept
	{
		return mGenerated;
	}

	void Generator::WaitGenerate() const noexcept
	{
		if (mGenerateThread.get_id() != std::this_thread::get_id())
		{
			mGenerateFuture.wait();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下は生成完了後に操作する関数です。
	// 必ず mGenerateFuture を使って生成の完了を待ってから処理して下さい。
	////////////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<Room> Generator::Find(const Point& point) const noexcept
	{
		WaitGenerate();

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
		WaitGenerate();

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
				if (aisle.IsUniqueLocked())
					stream << ": Unique lock";
				else if(aisle.IsLocked())
					stream << ": Lock";
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

	void Generator::Branch(std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<Room>& room, uint8_t& branchId) noexcept
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
					Branch(generatedEdges, room0, branchId);
				}
				if (room != room1)
				{
					if (aisleCount >= 3)
						++branchId;
					Branch(generatedEdges, room1, branchId);
				}
			}
		}
	}

	void Generator::Branch() noexcept
	{
		if (mStartPoint)
		{
			std::unordered_set<const Aisle*> edges;
			uint8_t branchId = 0;
			Branch(edges, mStartPoint->GetOwnerRoom(), branchId);
		}
	}
}
