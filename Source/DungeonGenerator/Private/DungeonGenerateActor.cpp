/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonGenerateActor.h"
#include "DungeonGenerateParameter.h"
#include "DungeonGenerator.h"
#include "DungeonMiniMapTextureLayer.h"
#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.h"
#include "BuildInfomation.h"
#include "Core/Grid.h"
#include "Core/Generator.h"
#include "Core/Room.h"
#include "Core/Voxel.h"
#include "Core/Math/Vector.h"

//#include <EngineUtils.h>
#include <Components/CapsuleComponent.h>
#include <Engine/LevelStreaming.h>
//#include <GameFramework/PlayerStart.h>
#include <GameFramework/Character.h>
#include <Kismet/GameplayStatics.h>
#include <NavigationSystem.h>
#include <NavMesh/NavMeshBoundsVolume.h>
#include <NavMesh/RecastNavMesh.h>

#if WITH_EDITOR
#include <Kismet/KismetSystemLibrary.h>
#endif

ADungeonGenerateActor::ADungeonGenerateActor(const FObjectInitializer& initializer)
	: Super(initializer)
	, BuildJobTag(TEXT(JENKINS_JOB_TAG))
	, LicenseTag(TEXT(JENKINS_LICENSE))
	, LicenseId(TEXT(JENKINS_UUID))
{
	// ティック有効化
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;

	// ルートシーンコンポーネントを生成
	RootComponent = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"), true);

	// デフォルトパラメータを生成
	DungeonGenerateParameter = NewObject<UDungeonGenerateParameter>();
	check(DungeonGenerateParameter);
}

ADungeonGenerateActor::~ADungeonGenerateActor()
{
	ReleaseHierarchicalInstancedStaticMeshComponents();
}

void ADungeonGenerateActor::ReleaseHierarchicalInstancedStaticMeshComponents()
{
	FloorMeshs.Empty();
	SlopeMeshs.Empty();
	WallMeshs.Empty();
	RoomRoofMeshs.Empty();
	AisleRoofMeshs.Empty();
	PillarMeshs.Empty();
}

void ADungeonGenerateActor::StartAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh))
		{
			mesh->BeginTransaction(true);
		}
	}
}

void ADungeonGenerateActor::AddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs, const UStaticMesh* staticMesh, const FTransform& transform)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh))
		{
			if (mesh->GetStaticMesh() == staticMesh)
			{
				mesh->AddInstance(transform);
				break;
			}
		}
	}
}

void ADungeonGenerateActor::EndAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh))
		{
			mesh->EndTransaction(true);
		}
	}
}

/*
開始位置PlayerStart,終了位置ADungeonPlayerGoalの位置および
プレイヤー操作のキャラクターを移動します
*/
void ADungeonGenerateActor::MovePlayerStart()
{
	if (mDungeonGenerator)
	{
		if (OnMovePlayerStart.IsBound())
		{
			OnMovePlayerStart.Broadcast(mDungeonGenerator->GetStartLocation());
		}
		else
		{
			mDungeonGenerator->MovePlayerStart();
		}
	}
}

void ADungeonGenerateActor::PreInitializeComponents()
{
	// 親クラスの呼び出し
	Super::PreInitializeComponents();

	// すべてのアクターを削除
	CDungeonGenerator::DestorySpawnedActors();

	if (IsValid(DungeonGenerateParameter))
	{
		if (InstancedStaticMesh)
		{
			ReleaseHierarchicalInstancedStaticMeshComponents();

			DungeonGenerateParameter->EachFloorParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					FloorMeshs.Add(component);
				}
			);
			DungeonGenerateParameter->EachSlopeParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					SlopeMeshs.Add(component);
				}
			);
			DungeonGenerateParameter->EachWallParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					WallMeshs.Add(component);
				}
			);
			DungeonGenerateParameter->EachRoomRoofParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					RoomRoofMeshs.Add(component);
				}
			);
			DungeonGenerateParameter->EachAisleRoofParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					AisleRoofMeshs.Add(component);
				}
			);
			DungeonGenerateParameter->EachPillarParts([this](const FDungeonMeshParts& meshParts)
				{
					auto component = NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
					if (IsValid(component))
					{
						AddInstanceComponent(component);
						component->RegisterComponent();
						component->SetStaticMesh(meshParts.StaticMesh);
					}
					PillarMeshs.Add(component);
				}
			);

			StartAddInstance(FloorMeshs);
			StartAddInstance(SlopeMeshs);
			StartAddInstance(WallMeshs);
			StartAddInstance(RoomRoofMeshs);
			StartAddInstance(AisleRoofMeshs);
			StartAddInstance(PillarMeshs);

			mDungeonGenerator = std::make_shared<CDungeonGenerator>();

			// Add
			mDungeonGenerator->OnAddFloor([this](UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(FloorMeshs, staticMesh, transform);
			OnCreateFloor.Broadcast(transform);
				}
			);
			mDungeonGenerator->OnAddSlope([this](UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(SlopeMeshs, staticMesh, transform);
			OnCreateSlope.Broadcast(transform);
				}
			);
			mDungeonGenerator->OnAddWall([this](UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(WallMeshs, staticMesh, transform);
			OnCreateWall.Broadcast(transform);
				}
			);
			mDungeonGenerator->OnAddRoomRoof([this](UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(RoomRoofMeshs, staticMesh, transform);
			OnCreateAisleRoof.Broadcast(transform);
				}
			);
			mDungeonGenerator->OnAddAisleRoof([this](UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(AisleRoofMeshs, staticMesh, transform);
			OnCreateAisleRoof.Broadcast(transform);
				}
			);
			mDungeonGenerator->OnAddPillar([this](uint32_t gridHeight, UStaticMesh* staticMesh, const FTransform& transform)
				{
					AddInstance(PillarMeshs, staticMesh, transform);
			OnCreatePillar.Broadcast(transform);
				}
			);

			// Reset
			mDungeonGenerator->OnResetDoor([this](AActor* actor, EDungeonRoomProps props)
				{
					OnResetDoor.Broadcast(actor, props);
				}
			);

			if (mDungeonGenerator->Create(DungeonGenerateParameter))
			{
				mDungeonGenerator->AddTerraine();
				mDungeonGenerator->AddObject();
			}

			EndAddInstance(FloorMeshs);
			EndAddInstance(SlopeMeshs);
			EndAddInstance(WallMeshs);
			EndAddInstance(RoomRoofMeshs);
			EndAddInstance(AisleRoofMeshs);
			EndAddInstance(PillarMeshs);
		}
		else
		{
			mDungeonGenerator = std::make_shared<CDungeonGenerator>();
			if (mDungeonGenerator->Create(DungeonGenerateParameter))
			{
				mDungeonGenerator->AddTerraine();
				mDungeonGenerator->AddObject();
			}
		}

#if WITH_EDITOR
		// ダンジョン生成した乱数を記録
		GeneratedRandomSeed = DungeonGenerateParameter->GetGeneratedRandomSeed();
#endif
	}

	MovePlayerStart();
}

void ADungeonGenerateActor::BeginPlay()
{
	// 親クラスを呼び出し
	Super::BeginPlay();

	mPostGenerated = false;
}

void ADungeonGenerateActor::Tick(float DeltaSeconds)
{
	// 親クラスを呼び出し
	Super::Tick(DeltaSeconds);

	if (!mPostGenerated)
	{
		mPostGenerated = true;
		PostGenerate();

#if !WITH_EDITOR
		// ティック無効化
		PrimaryActorTick.bCanEverTick = false;
#endif
	}

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	DrawDebugInfomation();
#endif
}

/**
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FBox ADungeonGenerateActor::ToWorldBoundingBox(const std::shared_ptr<const dungeon::Room>& room, const float gridSize)
{
	const FVector min = FVector(room->GetLeft(), room->GetTop(), room->GetBackground()) * gridSize;
	const FVector max = FVector(room->GetRight(), room->GetBottom(), room->GetForeground()) * gridSize;
	return FBox(min, max);
}

void ADungeonGenerateActor::PostGenerate()
{
	if (mDungeonGenerator == nullptr)
		return;

	std::shared_ptr<const dungeon::Generator> generator = mDungeonGenerator->GetGenerator();
	if (generator == nullptr)
		return;

	const float gridSize = DungeonGenerateParameter->GetGridSize();

	/*
	部屋の生成通知
	*/
	if (OnRoomCreated.IsBound())
	{
		UNavigationSystemV1* navigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());

		generator->ForEach([this, gridSize, navigationSystem](const std::shared_ptr<const dungeon::Room>& room)
			{
				const FBox box = ToWorldBoundingBox(room, gridSize);
				OnRoomCreated.Broadcast(false, static_cast<EDungeonRoomParts>(room->GetParts()), box);

				if (navigationSystem)
				{
					navigationSystem->AddDirtyArea(box, 0);
				}
			}
		);

		if (navigationSystem)
		{
			navigationSystem->Build();
		}
	}
}

#if WITH_EDITOR
bool ADungeonGenerateActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
void ADungeonGenerateActor::DrawDebugInfomation()
{
	if (!IsValid(DungeonGenerateParameter))
		return;

	if (mDungeonGenerator == nullptr)
		return;

	std::shared_ptr<const dungeon::Generator> generator = mDungeonGenerator->GetGenerator();
	if (generator == nullptr)
		return;
	
	const float gridSize = DungeonGenerateParameter->GetGridSize();

	// 部屋と接続情報のデバッグ情報を表示します
	if (ShowRoomAisleInfomation)
	{
		generator->ForEach([this, gridSize](const std::shared_ptr<const dungeon::Room>& room)
			{
				UKismetSystemLibrary::DrawDebugBox(
					GetWorld(),
					room->GetCenter() * gridSize,
					room->GetExtent() * gridSize,
					FColor::Magenta,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);

				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					room->GetFloorCenter() * gridSize,
					10.f,
					12,
					FColor::Magenta,
					0.f,
					2.f
				);
			}
		);

		generator->EachAisle([this, gridSize](const dungeon::Aisle& edge)
			{
				UKismetSystemLibrary::DrawDebugLine(
					GetWorld(),
					*edge.GetPoint(0) * gridSize,
					*edge.GetPoint(1) * gridSize,
					FColor::Red,
					0.f,
					5.f
				);

				const FVector start(static_cast<int32>(edge.GetPoint(0)->X), static_cast<int32>(edge.GetPoint(0)->Y), static_cast<int32>(edge.GetPoint(0)->Z));
				const FVector goal(static_cast<int32>(edge.GetPoint(1)->X), static_cast<int32>(edge.GetPoint(1)->Y), static_cast<int32>(edge.GetPoint(1)->Z));
				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					start * gridSize + FVector(gridSize / 2., gridSize / 2., gridSize / 2.),
					10.f,
					12,
					FColor::Green,
					0.f,
					5.f
				);
				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					goal * gridSize + FVector(gridSize / 2., gridSize / 2., gridSize / 2.),
					10.f,
					12,
					FColor::Red,
					0.f,
					5.f
				);
			}
		);
	}

	// ボクセルグリッドのデバッグ情報を表示します
	if (ShowVoxelGridType)
	{
		generator->GetVoxel()->Each([this, gridSize](const FIntVector& location, const dungeon::Grid& grid)
			{
				static const FColor colors[] = {
					FColor::Blue,		// Floor
					FColor::Yellow,		// Deck
					FColor::Red,		// Gate
					FColor::Green,		// Aisle
					FColor::Magenta,	// Slope
					FColor::Cyan,		// Atrium
					FColor::Black,		// Empty
					FColor::Black,		// OutOfBounds
				};
				static constexpr size_t colorSize = sizeof(colors) / sizeof(colors[0]);
				static_assert(colorSize == dungeon::Grid::TypeSize);

				//if (grid.mType == dungeon::Grid::Type::Aisle || grid.mType == dungeon::Grid::Type::Slope)
				if (grid.GetType() != dungeon::Grid::Type::Empty && grid.GetType() != dungeon::Grid::Type::OutOfBounds)
				{
					const double halfGridSize = gridSize / 2.;
					const FVector halfGrid(halfGridSize, halfGridSize, halfGridSize);
					const FVector center = dungeon::ToVector(location) * gridSize + halfGrid;

					const size_t index = static_cast<size_t>(grid.GetType());
					UKismetSystemLibrary::DrawDebugBox(
						GetWorld(),
						center,
						halfGrid * 0.95,
						colors[index],
						FRotator::ZeroRotator,
						0.f,
						10.f
					);

					const FVector direction = dungeon::ToVector(grid.GetDirection().GetVector());
					UKismetSystemLibrary::DrawDebugArrow(
						GetWorld(),
						center,
						center + direction * halfGridSize,
						gridSize,
						colors[index],
						0.f,
						5.f);
				}

				return true;
			}
		);
	}

#if 0
	// ダンジョン全体の領域を可視化
	{
		const FBox& bounding = BoundingBox();
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounding.GetCenter(),
			bounding.GetExtent(),
			FColor::Orange,
			FRotator::ZeroRotator,
			0.f,
			5.f
		);
	}
#endif

	// プレイヤーの位置のボクセルグリッドのデバッグ情報を表示します
	if (ShowVoxelGridTypeAtPlayerLocation)
	{
		if (APawn* playerPawn = GetValid(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			TArray<FString> output;

			FIntVector location = DungeonGenerateParameter->ToGrid(playerPawn->GetActorLocation());
			output.Add(TEXT("Grid:") + FString::FromInt(location.X) + TEXT(",") + FString::FromInt(location.Y) + TEXT(",") + FString::FromInt(location.Z));

			const dungeon::Grid& grid = generator->GetGrid(location);
			output.Add(TEXT("Identifier:") + FString::FromInt(grid.GetIdentifier()));
			output.Add(TEXT(" Type : ") + grid.GetTypeName());
			//Direction GetDirection() const noexcept;
			output.Add(TEXT(" Props : ") + grid.GetPropsName());

			// 文字列を展開
			FString message;
			for (const FString& line : output)
			{
				message.Append(line);
				message.Append(TEXT("\n"));
			}
			UKismetSystemLibrary::DrawDebugString(playerPawn->GetWorld(), playerPawn->GetPawnViewLocation(), message, nullptr, FLinearColor::White);
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// ストリーミングレベル
void ADungeonGenerateActor::LoadStreamLevels()
{
	for (const auto& createRequestLevel : mCreateRequestLevels)
	{
		// AKA: https://www.orfeasel.com/handling-level-streaming-through-c/
		const ULevelStreaming* level = UGameplayStatics::GetStreamingLevel(GetWorld(), createRequestLevel.Key);
		if (IsValid(level) && level->IsLevelVisible())
		{
			FLatentActionInfo LatentInfo;
			LatentInfo.CallbackTarget = this;
			LatentInfo.ExecutionFunction = FName("OnMoveStreamLevel");
			LatentInfo.UUID = 0;
			LatentInfo.Linkage = 0;
			mOnMoveStreamLevel.mName = createRequestLevel.Key;
			mOnMoveStreamLevel.mTransform = createRequestLevel.Value;
			UGameplayStatics::UnloadStreamLevel(GetWorld(), createRequestLevel.Key, LatentInfo, true);
		}
		else
		{
			LoadStreamLevel(createRequestLevel.Key, createRequestLevel.Value);
		}
	}
}

void ADungeonGenerateActor::OnMoveStreamLevel()
{
	LoadStreamLevel(mOnMoveStreamLevel.mName, mOnMoveStreamLevel.mTransform);
}

void ADungeonGenerateActor::LoadStreamLevel(const FName& levelName, const FTransform& levelTransform)
{
	FLatentActionInfo LatentInfo;
	UGameplayStatics::LoadStreamLevel(GetWorld(), levelName, false, false, LatentInfo);

	// AKA: https://www.orfeasel.com/handling-level-streaming-through-c/
	ULevelStreaming* level = UGameplayStatics::GetStreamingLevel(GetWorld(), levelName);
	if (IsValid(level))
	{
		level->LevelTransform = levelTransform;
		level->SetShouldBeVisible(true);
	}
}

void ADungeonGenerateActor::UnloadStreamLevels()
{
	for (const auto& createRequestLevel : mCreateRequestLevels)
	{
		FLatentActionInfo LatentInfo;
		UGameplayStatics::UnloadStreamLevel(GetWorld(), createRequestLevel.Key, LatentInfo, true);
	}
	mCreateRequestLevels.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint便利関数
int32 ADungeonGenerateActor::FindFloorHeight(const double z) const
{
	const int32 gridZ = FindVoxelHeight(z);

	if (mDungeonGenerator == nullptr)
		return 0;

	const std::shared_ptr<const dungeon::Generator>& generator = mDungeonGenerator->GetGenerator();
	return generator->FindFloor(gridZ);
}

int32 ADungeonGenerateActor::FindVoxelHeight(const double z) const
{
	if (DungeonGenerateParameter == nullptr)
		return 0;

	return static_cast<int32>(z / DungeonGenerateParameter->GetGridSize());
}

UDungeonMiniMapTextureLayer* ADungeonGenerateActor::GenerateMiniMapTextureLayer(const int32 textureWidth)
{
	if (textureWidth <= 0)
		return nullptr;

	const float gridSize = DungeonGenerateParameter->GetGridSize();

	DungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>(this);
	if (IsValid(DungeonMiniMapTextureLayer) == false)
		return nullptr;

	if (DungeonMiniMapTextureLayer->GenerateMiniMapTexture(mDungeonGenerator, textureWidth, gridSize) == false)
		return nullptr;

	return DungeonMiniMapTextureLayer;
}

UDungeonMiniMapTextureLayer* ADungeonGenerateActor::GetGeneratedMiniMapTextureLayer() const
{
	return DungeonMiniMapTextureLayer;
}
