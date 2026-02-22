/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonGenerateParameter.h"
#include "Parameter/DungeonPartsSelector.h"
#include "Parameter/DungeonSelectionPolicyUtility.h"
#include "PluginInformation.h"
#include "Core/Debug/BuildInformation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"
#include "SubActor/DungeonRoomSensorDatabase.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>
#if WITH_EDITOR
#include <UObject/UnrealType.h>
#endif


#if WITH_EDITOR
#include <Misc/FileHelper.h>
#endif
#include <unordered_set>

UDungeonGenerateParameter::UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PluginVersion(DUNGEON_GENERATOR_PLUGIN_VERSION)
{
}

UDungeonGenerateParameter* UDungeonGenerateParameter::GenerateRandomParameter(const UDungeonGenerateParameter* sourceParameter) noexcept
{
	const auto random = std::make_unique<dungeon::Random>();

	UDungeonGenerateParameter* parameter = NewObject<UDungeonGenerateParameter>();
	parameter->SetRandomParameter();

	if (sourceParameter)
	{
		parameter->GridSize = sourceParameter->GridSize;
		parameter->VerticalGridSize = sourceParameter->VerticalGridSize;
		parameter->DungeonRoomMeshPartsDatabase = sourceParameter->DungeonRoomMeshPartsDatabase;
		parameter->DungeonAisleMeshPartsDatabase = sourceParameter->DungeonAisleMeshPartsDatabase;
		parameter->PillarPartsSelectionPolicy = sourceParameter->PillarPartsSelectionPolicy;
		parameter->PillarPartsSelectionMethod = sourceParameter->PillarPartsSelectionMethod;
		parameter->PillarParts = sourceParameter->PillarParts;
		parameter->TorchPartsSelectionPolicy = sourceParameter->TorchPartsSelectionPolicy;
		parameter->TorchPartsSelectionMethod = sourceParameter->TorchPartsSelectionMethod;
		parameter->FrequencyOfTorchlightGeneration = sourceParameter->FrequencyOfTorchlightGeneration;
		parameter->TorchParts = sourceParameter->TorchParts;
		parameter->DoorPartsSelectionPolicy = sourceParameter->DoorPartsSelectionPolicy;
		parameter->DoorPartsSelectionMethod = sourceParameter->DoorPartsSelectionMethod;
		parameter->DoorParts = sourceParameter->DoorParts;
		parameter->AisleCeilingHeightPolicy = sourceParameter->AisleCeilingHeightPolicy;
		parameter->DungeonRoomSensorClass = sourceParameter->DungeonRoomSensorClass;
		parameter->DungeonRoomSensorDatabase = sourceParameter->DungeonRoomSensorDatabase;
	}

	return parameter;
}

void UDungeonGenerateParameter::SetRandomParameter() noexcept
{
	const auto random = std::make_unique<dungeon::Random>();

	RoomWidth.Min = random->Get<int32>(1, 5);
	RoomWidth.Max = RoomWidth.Min + random->Get<int32>(1, 5);
	RoomDepth.Min = random->Get<int32>(1, 5);
	RoomDepth.Max = RoomDepth.Min + random->Get<int32>(1, 5);
	RoomHeight.Min = random->Get<int32>(1, 3);
	RoomHeight.Max = RoomHeight.Min + random->Get<int32>(1, 3);
        RoomMargin = random->Get<uint8>(1, 5);
        VerticalRoomMargin = random->Get<uint8>(5);
        NumberOfCandidateRooms = random->Get<uint8>(5, 50);
        NumberOfCandidateFloors = random->Get<uint8>(5);
        MergeRooms = random->Get<bool>();
        ExpansionPolicy = static_cast<EDungeonExpansionPolicy>(random->Get<uint8>(0, static_cast<uint8>(EDungeonExpansionPolicy::Flat)));
        Flat = ExpansionPolicy == EDungeonExpansionPolicy::Flat;
	GenerateSlopeInRoom = random->Get<bool>();
	GenerateStructuralColumn = random->Get<bool>();

	if (MergeRooms == true)
	{
		UseMissionGraph = false;
		AisleComplexity = 0;
	}
	else
	{
		UseMissionGraph = random->Get<bool>();
		if (UseMissionGraph == true)
		{
			AisleComplexity = 0;
		}
		else
		{
			AisleComplexity = random->Get<uint8>(0, 10);
		}
	}

	AisleCeilingHeightPolicy = static_cast<EDungeonAisleCeilingHeightPolicy>(random->Get<uint8>(static_cast<uint8>(EDungeonAisleCeilingHeightPolicy::SIZE)));
}

void UDungeonGenerateParameter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams sharedParams;
	sharedParams.bIsPushBased = true;
	// DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, RandomSeed, sharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, GeneratedRandomSeed, sharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, GeneratedDungeonCRC32, sharedParams);
}

bool UDungeonGenerateParameter::IsSupportedForNetworking() const
{
	//return Super::IsSupportedForNetworking();
	return true;
}

void UDungeonGenerateParameter::PostLoad()
{
	Super::PostLoad();

	if (Flat && ExpansionPolicy != EDungeonExpansionPolicy::Flat)
	{
		ExpansionPolicy = EDungeonExpansionPolicy::Flat;
	}

	if (ExpansionPolicy == EDungeonExpansionPolicy::Flat)
	{
		Flat = false;
	}

	if (MovePlayerStartToStartingPoint == false && StartLocationPolicy != EDungeonStartLocationPolicy::NoAdjustment)
	{
		StartLocationPolicy = EDungeonStartLocationPolicy::NoAdjustment;
	}

	if (StartLocationPolicy == EDungeonStartLocationPolicy::NoAdjustment)
	{
		MovePlayerStartToStartingPoint = false;
	}

	if (UseMissionGraph &&
		(StartLocationPolicy == EDungeonStartLocationPolicy::UseCentralPoint ||
			StartLocationPolicy == EDungeonStartLocationPolicy::UseMultiStart))
	{
		StartLocationPolicy = EDungeonStartLocationPolicy::UseSouthernMost;
	}

	if (!bFixtureSelectionPoliciesMigrated)
	{
		PillarPartsSelectionPolicy = dungeon::selection::ToPolicy(PillarPartsSelectionMethod);
		TorchPartsSelectionPolicy = dungeon::selection::ToPolicy(TorchPartsSelectionMethod);
		DoorPartsSelectionPolicy = dungeon::selection::ToPolicy(DoorPartsSelectionMethod);
	}

	PillarPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(PillarPartsSelectionPolicy);
	PillarPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(PillarPartsSelectionPolicy);

	TorchPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(TorchPartsSelectionPolicy);
	TorchPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(TorchPartsSelectionPolicy);

	DoorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(DoorPartsSelectionPolicy);
	DoorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(DoorPartsSelectionPolicy);
	bFixtureSelectionPoliciesMigrated = true;
}

#if WITH_EDITOR
void UDungeonGenerateParameter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bFixtureSelectionPoliciesMigrated = true;

	PillarPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(PillarPartsSelectionPolicy);
	PillarPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(PillarPartsSelectionPolicy);
	TorchPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(TorchPartsSelectionPolicy);
	TorchPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(TorchPartsSelectionPolicy);
	DoorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(DoorPartsSelectionPolicy);
	DoorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(DoorPartsSelectionPolicy);
}
#endif

#if WITH_EDITOR

void UDungeonGenerateParameter::Dump() const
{
	DumpToJson();
}
#endif

void UDungeonGenerateParameter::SetRandomSeed(const int32 generateRandomSeed)
{
	RandomSeed = generateRandomSeed;
}

void UDungeonGenerateParameter::SetGeneratedRandomSeed(const int32 generatedRandomSeed)
{
	/*
	リプリケーションが有効の場合
	サーバーからクライアントへダンジョンを生成した乱数の種が送信される
	クライアントのCDungeonGeneratorCore::Createから新たに設定するが同じ値なので何もしない
	*/
	if (GeneratedRandomSeed != generatedRandomSeed)
	{
		GeneratedRandomSeed = generatedRandomSeed;
		MARK_PROPERTY_DIRTY_FROM_NAME(UDungeonGenerateParameter, GeneratedRandomSeed, this);
	}
}

int32 UDungeonGenerateParameter::GetGeneratedDungeonCRC32() const noexcept
{
	return GeneratedDungeonCRC32;
}

void UDungeonGenerateParameter::SetGeneratedDungeonCRC32(const int32 generatedDungeonCRC32) noexcept
{
	GeneratedDungeonCRC32 = generatedDungeonCRC32;
}


/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const FIntVector& location) const
{
	return FVector(
		static_cast<float>(location.X),
		static_cast<float>(location.Y),
		static_cast<float>(location.Z)
	) * GetGridSize().To3D();
}

/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const
{
	return FVector(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	) * GetGridSize().To3D();
}

FIntVector UDungeonGenerateParameter::ToGrid(const FVector& location) const
{
	return FIntVector(
		static_cast<int32>(location.X / GetGridSize().HorizontalSize),
		static_cast<int32>(location.Y / GetGridSize().HorizontalSize),
		static_cast<int32>(location.Z / GetGridSize().VerticalSize)
	);
}

#if WITH_EDITOR
void UDungeonGenerateParameter::DumpToJson() const
{
	auto boolValue = [](const bool value) -> FString
		{
			return value ? TEXT("true") : TEXT("false");
		};

	FString jsonString(TEXT("{\n"));
	{
		jsonString += TEXT(" \"Version\":\"") + FString(TEXT(DUNGEON_GENERATOR_PLUGIN_VERSION_NAME)) + TEXT("\",\n");
		jsonString += TEXT(" \"Tag\":\"") + FString(TEXT(JENKINS_JOB_TAG)) + TEXT("\",\n");
		jsonString += TEXT(" \"UUID\":\"") + FString(TEXT(JENKINS_UUID)) + TEXT("\",\n");
		jsonString += TEXT(" \"License\":\"") + FString(TEXT(JENKINS_LICENSE)) + TEXT("\",\n");

		jsonString += TEXT(" \"RandomSeed\":") + FString::FromInt(RandomSeed) + TEXT(",\n");
		jsonString += TEXT(" \"GeneratedRandomSeed\":") + FString::FromInt(GeneratedRandomSeed) + TEXT(",\n");
		jsonString += TEXT(" \"GeneratedDungeonCRC32\":") + FString::FromInt(GeneratedDungeonCRC32) + TEXT(",\n");		
		jsonString += TEXT(" \"NumberOfCandidateRooms\":") + FString::FromInt(NumberOfCandidateRooms) + TEXT(",\n");
		jsonString += TEXT(" \"RoomWidth\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(RoomWidth.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(RoomWidth.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"RoomDepth\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(RoomDepth.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(RoomDepth.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"RoomHeight\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(RoomHeight.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(RoomHeight.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"HorizontalRoomMargin\":") + FString::FromInt(RoomMargin) + TEXT(",\n");
		jsonString += TEXT(" \"VerticalRoomMargin\":") + FString::FromInt(VerticalRoomMargin) + TEXT(",\n");
		jsonString += TEXT(" \"MergeRooms\":") + boolValue(MergeRooms) + TEXT(",\n");
		jsonString += TEXT(" \"Flat\":") + boolValue(Flat) + TEXT(",\n");
		jsonString += TEXT(" \"MovePlayerStartToStartingPoint\":") + boolValue(MovePlayerStartToStartingPoint) + TEXT(",\n");
		jsonString += TEXT(" \"UseMissionGraph\":") + boolValue(UseMissionGraph) + TEXT(",\n");
		jsonString += TEXT(" \"AisleComplexity\":") + FString::FromInt(AisleComplexity) + TEXT(",\n");
		jsonString += TEXT(" \"AisleCeilingHeightPolicy\":\"") + UEnum::GetValueAsString(AisleCeilingHeightPolicy) + TEXT("\",\n");
		jsonString += TEXT(" \"GridSize\":{\n");
		jsonString += TEXT("  \"HorizontalSize\":") + FString::SanitizeFloat(GridSize) + TEXT(",\n");
		jsonString += TEXT("  \"VerticalSize\":") + FString::SanitizeFloat(VerticalGridSize) + TEXT("\n");
		jsonString += TEXT(" }");
		if (IsValid(DungeonRoomMeshPartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"DungeonRoomMeshPartsDatabase\":{\n");
			jsonString += DungeonRoomMeshPartsDatabase->DumpToJson(2) + TEXT("\n");
			jsonString += TEXT(" }");
		}
		if (IsValid(DungeonAisleMeshPartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"DungeonAisleMeshPartsDatabase\":{\n");
			jsonString += DungeonAisleMeshPartsDatabase->DumpToJson(2) + TEXT("\n");
			jsonString += TEXT(" }");
		}
		jsonString += TEXT(",\n");
		jsonString += TEXT(" \"PillarParts\":{\n");
		jsonString += TEXT("  \"PillarPartsSelectionMethod\":\"") + UEnum::GetValueAsString(PillarPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < PillarParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += PillarParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"TorchParts\":{\n");
		jsonString += TEXT("  \"TorchPartsSelectionMethod\":\"") + UEnum::GetValueAsString(TorchPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < TorchParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += TorchParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"DoorParts\":{\n");
		jsonString += TEXT("  \"DoorPartsSelectionMethod\":\"") + UEnum::GetValueAsString(DoorPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < DoorParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += DoorParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" }");
		if (IsValid(DungeonRoomSensorClass))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"DungeonRoomSensorClass\":\"") + DungeonRoomSensorClass->GetName() + TEXT("\"");
		}
		jsonString += TEXT("\n");
	}
	jsonString += TEXT("}");
	
	const FString fileName(GetName() + ".json");
	const FString filePath(GetJsonDefaultDirectory() / fileName);
	FFileHelper::SaveStringToFile(jsonString, *filePath);
}

FString UDungeonGenerateParameter::GetJsonDefaultDirectory() const
{
	return dungeon::GetDebugDirectory();
}
#endif

FMeshSetQuery UDungeonGenerateParameter::MakeMeshSetQuery(const size_t gridIndex, const dungeon::Grid& grid)
{
	FMeshSetQuery query;
	query.RoomId = static_cast<int32>(grid.GetIdentifier());
	query.DepthFromStart = static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f;
	query.DistanceToGoal = 1.f - query.DepthFromStart;
	query.SeedKey = static_cast<int32>(gridIndex);
	return query;
}

const FDungeonMeshSet* UDungeonGenerateParameter::SelectMeshSet(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(dungeonMeshSetDatabase) == false)
		return nullptr;

	const FMeshSetQuery query = MakeMeshSetQuery(gridIndex, grid);
	return dungeonMeshSetDatabase->SelectImplement(grid.GetIdentifier(), grid.GetDepthRatioFromStart(), random, query);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectFloorParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectFloorParts(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectCatwalkParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectCatwalkParts(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallPartsByGrid(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectWallPartsByGrid(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallPartsByFace(const FDungeonMeshSet* dungeonMeshSet, const FIntVector& gridLocation, const dungeon::Direction& direction)
{
	check(dungeonMeshSet);
	return dungeonMeshSet->SelectWallPartsByFace(gridLocation, direction);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectRoofParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectRoofParts(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectSlopeParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectSlopeParts(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonMeshSet::SelectPartsByGrid(gridIndex, grid, random, PillarParts, PillarPartsSelectionPolicy);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonMeshSet::SelectRandomActorParts(gridIndex, grid, random, TorchParts, TorchPartsSelectionPolicy);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectChandelierParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridIndex, grid, random))
	{
		return meshSet->SelectChandelierParts(gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonDoorActorParts* UDungeonGenerateParameter::SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonMeshSet::SelectPartsByGrid(gridIndex, grid, random, DoorParts, DoorPartsSelectionPolicy);
}

void UDungeonGenerateParameter::EachRoomFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonRoomMeshPartsDatabase))
	{
		DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonAisleMeshPartsDatabase))
	{
		DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonRoomMeshPartsDatabase))
	{
		DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonAisleMeshPartsDatabase))
	{
		DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonRoomMeshPartsDatabase))
	{
		DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonAisleMeshPartsDatabase))
	{
		DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonRoomMeshPartsDatabase))
	{
		DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachSlopeParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonAisleMeshPartsDatabase))
	{
		DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachSlopeParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (DungeonRoomMeshPartsDatabase)
	{
		DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachCatwalkParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (DungeonAisleMeshPartsDatabase)
	{
		DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachCatwalkParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachPillarParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	FDungeonMeshSet::EachParts(PillarParts, function);
}

void UDungeonGenerateParameter::OnEndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap, const std::function<void(const FSoftObjectPath&, const FTransform&)>& spawnActor) const
{
	if (DungeonRoomSensorDatabase)
		DungeonRoomSensorDatabase->OnEndGeneration(synchronizedRandom, aisleGridMap, VerticalGridSize, spawnActor);
}
