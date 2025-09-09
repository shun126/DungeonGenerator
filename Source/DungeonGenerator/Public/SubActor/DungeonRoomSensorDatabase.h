/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshSetSelectionMethod.h"
#include <memory>
#include "DungeonRoomSensorDatabase.generated.h"

namespace dungeon
{
	class Random;
}

/**
 * Database class that manages dungeon room intrusion detection sensors
 * 
 * UDungeonRoomSensorDatabase is a database class that holds and manages multiple UDungeonRoomSensor
 * Database class that holds and manages multiple UDungeonRoomSensors.
 * draws lots and selects available sensors based on set conditions.
 *
 * ダンジョンの部屋侵入検知センサーを管理するデータベースクラス
 * 
 * UDungeonRoomSensorDatabase は、複数の UDungeonRoomSensor を保持・管理する
 * データベースクラスです。
 * 設定された条件を元に、利用可能なセンサーを抽選し選択します。
*/
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonRoomSensorDatabase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * constructor
	 */
	explicit UDungeonRoomSensorDatabase(const FObjectInitializer& ObjectInitializer);

	/**
	 * destructor
	 */
	virtual ~UDungeonRoomSensorDatabase() override = default;

	/**
	 * Select the DungeonRoomSensor class that matches the condition of the argument
	 * 
	 * 引数の条件にあったDungeonRoomSensorのクラスを選択します
	 */
	UClass* Select(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const;

protected:
	/**
	 * Room Sensor Generation Rules
	 * ルームセンサーの生成ルール
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
	EDungeonMeshSetSelectionMethod SelectionMethod = EDungeonMeshSetSelectionMethod::Identifier;

	/**
	 * Register the RoomSensor to be placed.
	 *
	 * 配置するRoomSensorを登録して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (AllowedClasses = "DungeonRoomSensorBase"))
	TArray<TObjectPtr<UClass>> DungeonRoomSensorClass;
};
