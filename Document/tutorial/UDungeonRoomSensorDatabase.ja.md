# UDungeonRoomSensorDatabase 移行メモ

`UDungeonRoomSensorDatabase` は v2.0.0 で deprecated になりました。古いアセットをロードして移行するためだけに残されています。

v2.0.0 は、v1 の Room Sensor Database 参照を移行するためのリリースです。v1 アセットをまだ使っているプロジェクトでは、v2.1 以降へ更新する前に v2.0.0 でプロジェクトを開き、移行後の設定を確認して、対象アセットを保存してください。v1 から v2 への移行サポートとこの旧 Database は、v2.1 以降で削除される可能性があります。

新しい設定では、Room Sensor 関連の Gameplay 設定を `UDungeonGenerateParameter` に直接設定します。

- `Gameplay.DungeonRoomSensorClass` は、生成部屋で使うデフォルトの `ADungeonRoomSensorBase` Blueprint です。
- `Gameplay.SpawnActorInAisle` は、生成通路内にスポーンするデフォルトの Actor Blueprint 候補です。
- `Zones[].GameplayOverride` では、Zone ごとの Room Sensor と通路 Actor 候補を上書きできます。
- `Gameplay.RoomRoles.Roles[].GameplayOverride` では、Gameplay Role ごとの Room Sensor を上書きできます。

古い parameter が `UDungeonRoomSensorDatabase` を参照している場合、旧 `DungeonRoomSensorClass` 配列の最初の有効なクラスを `Gameplay.DungeonRoomSensorClass` へコピーします。旧 `SpawnActorInAisle` は、新しい `Gameplay.SpawnActorInAisle` が空の場合にコピーされます。

推奨する移行手順:

1. v1 プロジェクトを Dungeon Generator v2.0.0 で開きます。
2. 対象の `UDungeonGenerateParameter` アセットを開く、またはロードします。
3. Room Sensor 設定が `Gameplay.DungeonRoomSensorClass` と `Gameplay.SpawnActorInAisle` にコピーされていることを確認します。
4. 移行後のアセットを保存します。
5. v2.0.0 で保存が完了してから、v2.1 以降へ更新します。

## 関連ページ
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
