# ダンジョン生成システムのアクター


```mermaid
classDiagram

AActor <|-- ADungeonDoorBase
AActor <|-- ADungeonRoomSensorBase

UBoxComponent --* ADungeonRoomSensor
```