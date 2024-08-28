/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonGenerateParameter.h"
#include "Parameter/DungeonAisleMeshSetDatabase.h"
#include "Parameter/DungeonRoomMeshSetDatabase.h"
#include "PluginInfomation.h"
#include "Core/Debug/BuildInfomation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

#if WITH_EDITOR
#include <Misc/FileHelper.h>
#endif

UDungeonGenerateParameter::UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PluginVersion(DUNGENERATOR_PLUGIN_VERSION)
{
}

UDungeonGenerateParameter* UDungeonGenerateParameter::GenerateRandomParameter(const UDungeonGenerateParameter* sourceParameter) noexcept
{
	const auto random = std::make_unique<dungeon::Random>();

	UDungeonGenerateParameter* parameter = NewObject<UDungeonGenerateParameter>();
	parameter->RoomWidth.Min = random->Get<int32>(1, 5);
	parameter->RoomWidth.Max = parameter->RoomWidth.Min + random->Get<int32>(1, 5);
	parameter->RoomDepth.Min = random->Get<int32>(1, 5);
	parameter->RoomDepth.Max = parameter->RoomWidth.Min + random->Get<int32>(1, 5);
	parameter->RoomHeight.Min = random->Get<int32>(1, 3);
	parameter->RoomHeight.Max = parameter->RoomWidth.Min + random->Get<int32>(1, 3);
	parameter->RoomMargin = random->Get<uint8>(1, 5);
	parameter->VerticalRoomMargin = random->Get<uint8>(5);
	parameter->NumberOfCandidateRooms = random->Get<uint8>(5, 50);
	parameter->NumberOfCandidateFloors = random->Get<uint8>(30);
	parameter->MergeRooms = random->Get<bool>();
	parameter->Flat = random->Get<bool>();
	parameter->UseMissionGraph = random->Get<bool>();
	parameter->AisleComplexity = random->Get<uint8>(0, 10);

	if (sourceParameter)
	{
		parameter->GridSize = sourceParameter->GridSize;
		parameter->VerticalGridSize = sourceParameter->VerticalGridSize;
		parameter->DungeonRoomPartsDatabase = sourceParameter->DungeonRoomPartsDatabase;
		parameter->DungeonAislePartsDatabase = sourceParameter->DungeonAislePartsDatabase;
		parameter->PillarPartsSelectionMethod = sourceParameter->PillarPartsSelectionMethod;
		parameter->PillarParts = sourceParameter->PillarParts;
		parameter->TorchPartsSelectionMethod = sourceParameter->TorchPartsSelectionMethod;
		parameter->FrequencyOfTorchlightGeneration = sourceParameter->FrequencyOfTorchlightGeneration;
		parameter->TorchParts = sourceParameter->TorchParts;
		parameter->DoorPartsSelectionMethod = sourceParameter->DoorPartsSelectionMethod;
		parameter->DoorParts = sourceParameter->DoorParts;
		parameter->DungeonRoomSensorClass = sourceParameter->DungeonRoomSensorClass;
	}

	return parameter;
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

#if WITH_EDITOR

void UDungeonGenerateParameter::Dump()
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
	/*
	TODO:外部からファイル名を与えられるように変更して下さい
	const FString path = FPaths::ProjectSavedDir() + TEXT("/DungeonGenerator/dungeon_diagram.json");
	*/
	// TSharedPtr<FJsonObject> jsonRoot = MakeShareable(new FJsonObject);
	// 
	// FString outPutString;
	// TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outPutString);
	// FJsonSerializer::Serialize(jsonRoot.ToSharedRef(), writer);

	auto boolValue = [](const bool value) -> FString
		{
			return value ? TEXT("true") : TEXT("false");
		};

	FString jsonString(TEXT("{\n"));
	{
		jsonString += TEXT(" \"Version\":\"") + FString(TEXT(DUNGENERATOR_PLUGIN_VERSION_NAME)) + TEXT("\",\n");
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
		jsonString += TEXT(" \"GridSize\":{\n");
		jsonString += TEXT("  \"HorizontalSize\":") + FString::SanitizeFloat(GridSize) + TEXT(",\n");
		jsonString += TEXT("  \"VerticalSize\":") + FString::SanitizeFloat(VerticalGridSize) + TEXT("\n");
		jsonString += TEXT(" }");
		if (IsValid(DungeonRoomPartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"DungeonRoomPartsDatabase\":{\n");
			jsonString += DungeonRoomPartsDatabase->DumpToJson(2) + TEXT("\n");
			jsonString += TEXT(" }");
		}
		if (IsValid(DungeonAislePartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"DungeonAislePartsDatabase\":{\n");
			jsonString += DungeonAislePartsDatabase->DumpToJson(2) + TEXT("\n");
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



const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectFloorParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(dungeonPartsDatabase))
	{
		if (const FDungeonMeshSet* parts = dungeonPartsDatabase->AtImplement(grid.GetIdentifier()))
		{
			const FDungeonRoomMeshSet* roomMeshSet = static_cast<const FDungeonRoomMeshSet*>(parts);
			if (const FDungeonMeshPartsWithDirection* result = roomMeshSet->SelectFloorParts(gridIndex, grid, random))
				return result;
		}
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(dungeonPartsDatabase))
	{
		if (const FDungeonMeshSet* parts = dungeonPartsDatabase->AtImplement(grid.GetIdentifier()))
		{
			const FDungeonRoomMeshSet* roomMeshSet = static_cast<const FDungeonRoomMeshSet*>(parts);
			if (const FDungeonMeshParts* result = roomMeshSet->SelectWallParts(gridIndex, grid, random))
				return result;
		}
	}
	return nullptr;
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectRoofParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(dungeonPartsDatabase))
	{
		if (const FDungeonMeshSet* parts = dungeonPartsDatabase->AtImplement(grid.GetIdentifier()))
		{
			const FDungeonRoomMeshSet* roomMeshSet = static_cast<const FDungeonRoomMeshSet*>(parts);
			if (const FDungeonMeshPartsWithDirection* result = roomMeshSet->SelectRoofParts(gridIndex, grid, random))
				return result;
		}
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(DungeonAislePartsDatabase))
	{
		if (const FDungeonMeshSet* parts = DungeonAislePartsDatabase->AtImplement(grid.GetIdentifier()))
		{
			const FDungeonAisleMeshSet* aisleMeshSet = static_cast<const FDungeonAisleMeshSet*>(parts);
			if (const FDungeonMeshParts* result = aisleMeshSet->SelectSlopeParts(gridIndex, grid, random))
				return result;
		}
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonRoomMeshSet::SelectParts(gridIndex, grid, random, PillarParts, PillarPartsSelectionMethod);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonRoomMeshSet::SelectRandomActorParts(gridIndex, grid, random, TorchParts, TorchPartsSelectionMethod);
}

const FDungeonDoorActorParts* UDungeonGenerateParameter::SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonRoomMeshSet::SelectParts(gridIndex, grid, random, DoorParts, DoorPartsSelectionMethod);
}



void UDungeonGenerateParameter::EachRoomFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonRoomPartsDatabase))
	{
		DungeonRoomPartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonAislePartsDatabase))
	{
		DungeonAislePartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonRoomPartsDatabase))
	{
		DungeonRoomPartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonAislePartsDatabase))
	{
		DungeonAislePartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(DungeonAislePartsDatabase))
	{
		DungeonAislePartsDatabase->Each([&function](const FDungeonAisleMeshSet& parts)
			{
				parts.EachSlopeParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachPillarParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	FDungeonRoomMeshSet::EachParts(PillarParts, function);
}

void UDungeonGenerateParameter::EachRoomRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonRoomPartsDatabase))
	{
		DungeonRoomPartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(DungeonAislePartsDatabase))
	{
		DungeonAislePartsDatabase->Each([&function](const FDungeonRoomMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}
