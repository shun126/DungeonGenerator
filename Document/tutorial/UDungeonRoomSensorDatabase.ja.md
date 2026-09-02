# UDungeonRoomSensorDatabase 非推奨 API の注意

`UDungeonRoomSensorDatabase` は非推奨であり、Version 2 の設定では使用しません。新しいコンテンツでは作成・使用しないでください。

部屋の Gameplay 設定は `UDungeonGenerateParameter` に直接設定します。

- `Gameplay.DungeonRoomSensorClass` に、標準の `ADungeonRoomSensorBase` 派生 Blueprint を設定します。
- `Gameplay.SpawnActorInAisle` に、生成通路内の標準 Actor 候補を設定します。
- `Zones[].GameplayOverride` で、Zone ごとに Room Sensor や通路 Actor を置き換えられます。
- `Gameplay.RoomRoles.Roles[].GameplayOverride` で、Gameplay Role ごとに Room Sensor を置き換えられます。

古い保存データを認識するため、実装内部に旧フィールドやロード時変換が残る場合があります。しかし、これは Version 1 から Version 2 への移行サポートではありません。Version 1 のプロジェクトを復元できるよう、別プロジェクトまたは別のソース管理ブランチで、新しい Version 2 アセットへ手動で設定を作り直してください。

## 関連ページ

- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
- [VersionComparison.ja.md](./VersionComparison.ja.md)
