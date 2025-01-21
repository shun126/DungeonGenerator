# Core

ダンジョン生成の中心機能を集めたパッケージです。
環境依存を極力抑えていますが、算術系クラスはUnrealEngineのクラスを利用しています。

`GenerateParameter`を元にしてボクセルを生成します。ボクセル内のグリッドを評価してメッシュを生成する事でダンジョンを可視化する事ができます。
ボクセルの評価やメッシュの生成はプラットフォーム依存なので、このパッケージでは取り扱っていません。

## Debug

デバッグに関するパッケージです。
ビルドの設定、ログの出力を扱っています。

## Helper

汎用的なクラスに関するパッケージです。
CRCや抽選など様々な機能を扱っています。

## Math

計算に関するパッケージです。
点、円、三角形、四面体、乱数など様々な機能を扱っています。

## MissionGraph

鍵とドアの組み合わせによるミッションを構築するパッケージです。

## PathGeneration

部屋と部屋をつなぐ経路に関するパッケージです。
ドロネー三角形分割、最小スパニングツリー、AABBなどの機能を扱っています。

## RoomGeneration

部屋と通路に関するパッケージです。

## Voxelization

ボクセルに関するパッケージです。
ノード情報からボクセル情報を生成する機能を扱っています。

# 大筋の流れ
```mermaid
graph TD

subgraph Core System
GenerateRooms(部屋の生成)
InitializeRooms(部屋の分離)
ExtractionAisles(通路の生成)
AdjustedStartAndGoalSubLevel(開始部屋と終了部屋のサブレベルを配置する隙間を調整)
AdjustRoomSize(部屋の大きさを調整)
SeparateRooms(部屋の分離)
ExpandSpace(全ての部屋が収まるように全体の空間を拡張)
AdjustPoints(部屋の中心情報を同期)
MarkBranchIdAndDepthFromStart(部屋と通路のブランチIDと深さを生成)
DetectFloorHeightAndDepthFromStart(部屋の階層情報と全体の空間の深さを生成)
UseMissionGraph{ミッショングラフ有効？}
GenerateMissionGraph(部屋と通路に意味付けする)
InvokeRoomCallbacks(スタート部屋とゴール部屋のコールバックを呼ぶ)
GenerateVoxel(ボクセル情報を生成)
end


GenerateRooms --> InitializeRooms
InitializeRooms --> ExtractionAisles
ExtractionAisles --> AdjustedStartAndGoalSubLevel
AdjustedStartAndGoalSubLevel --> AdjustRoomSize
AdjustRoomSize --> SeparateRooms
SeparateRooms -->|分離失敗| ExtractionAisles
SeparateRooms -->|分離成功| ExpandSpace
ExpandSpace --> AdjustPoints
AdjustPoints --> MarkBranchIdAndDepthFromStart
MarkBranchIdAndDepthFromStart --> DetectFloorHeightAndDepthFromStart
DetectFloorHeightAndDepthFromStart --> UseMissionGraph
UseMissionGraph -->|有効| GenerateMissionGraph
UseMissionGraph -->|無効| InvokeRoomCallbacks
GenerateMissionGraph --> InvokeRoomCallbacks
InvokeRoomCallbacks --> GenerateVoxel
GenerateVoxel -.->|DungeonGenerateActorのTickまたはDungeonGenerateEditorModuleから| サブレベルを読み込み
```
