# ADungeonRoomSensorBase ガイド

`ADungeonRoomSensorBase` は、部屋に入ったときのイベント、敵や報酬の配置、罠、BGM 切り替え、部屋ごとの内装タグなどを実装するための基底クラスです。

## 基本の流れ
1. `ADungeonRoomSensorBase` を親クラスにした Blueprint を作ります。
2. 必要なイベントを実装します。
3. 作成した Blueprint を `Gameplay.DungeonRoomSensorClass` に登録します。
4. Zone や RoomRole ごとに別のセンサーが必要な場合だけ、`Zones[].GameplayOverride` または `Gameplay.RoomRoles.Roles[].GameplayOverride` を使います。

## よく使うイベント
- `OnPrepare`  
  センサーがスポーンされた直後の準備処理です。配置したくない場合は `false` を返します。
- `OnInitialize`  
  配置後の本処理です。v2 の標準の部屋情報 API である `FDungeonGeneratedRoomInfo` が渡されます。
- `OnFinalize`  
  破棄前の後始末です。
- `OnReset`  
  プレイヤーが部屋を離れたあとに状態を戻したい場合に使います。
- `OnResume`  
  プレイヤーが再び部屋に入ったときの再開処理です。

## 部屋情報 API
部屋を初期化するときは、`OnInitialize` に渡される `FDungeonGeneratedRoomInfo` を使ってください。  
Blueprint の別の関数から同じ情報を見たい場合は、`GetGeneratedRoomInfo()` を使います。

`FDungeonGeneratedRoomInfo` には、ゲームプレイ Blueprint でよく使う部屋情報がまとまっています。

- `Identifier`  
  この部屋に割り当てられた一意の識別子です。
- `RoomStructuralRole` / `RoomGameplayRole`  
  `Combat`、`Treasure`、`Puzzle`、`Rest`、`Secret` など、部屋に割り当てられた役割です。
- `Parts` / `Item`  
  旧来の部屋種別と MissionGraph 用のアイテム情報です。
- `bSecretRoom`  
  秘密部屋なら `true` です。
- `bDeadEndRoom`  
  行き止まり部屋なら `true` です。
- `bMainPathRoom`  
  スタートからゴールへ向かう主経路上の部屋なら `true` です。
- `bLockedRouteRoom` / `bHasLockedDoor`  
  鍵付き経路や鍵付きドアに関係する部屋かどうかです。
- `ZoneName` / `ZoneIndex`  
  どの Zone に属する部屋かを表します。
- `BranchId`  
  MissionGraph によって生成された分岐識別子です。
- `DepthFromStart` / `DeepestDepthFromStart` / `DepthFromStartRatio`  
  スタート部屋からの深さです。序盤、中盤、終盤で処理を変えるときに使えます。

## Deprecated の部屋フィールド
`ADungeonRoomSensorBase` 直下にも一部の部屋情報フィールドが残っていますが、これは v1 Blueprint からの移行用です。  
v2 の新しい Blueprint では、センサーから `Identifier`、`Parts`、`Item`、`BranchId`、`DepthFromStart`、`DeepestDepthFromStart` を直接読まず、`GetGeneratedRoomInfo()` を使ってください。

## 内装と連携する
`GetInquireInteriorTags` を実装すると、その部屋に対して内装タグを返せます。  
たとえば、秘密部屋なら `Secret`、宝部屋なら `Treasure`、行き止まりなら `DeadEnd`、鍵付き経路なら `Locked` のようなタグを返します。

プラグイン側は `Theme.DungeonInteriorDatabase` を使い、タグに合う家具、装飾、演出を配置します。

## よくある使い方
- `bSecretRoom` が `true` の部屋だけに隠し宝箱を置く。
- `bDeadEndRoom` が `true` の部屋に報酬やショートイベントを置く。
- `RoomGameplayRole == Combat` の部屋に敵をスポーンする。
- `RoomGameplayRole == Rest` の部屋には敵を置かず、回復やセーブ用の演出を置く。
- `ZoneName` に応じて敵の種類や内装タグを変える。

## Helper のスポーンパラメータ
`DungeonGenerator|Helper|SpawnActorInRoom` と `DungeonGenerator|Helper|MissionGraph` のパラメータは、Room Sensor からアクターを手早くスポーンするためのヘルパー設定です。Quick Start 後の確認や初期調整では、細かい Blueprint ロジックを作る前にここから試すのがおすすめです。

- `SpawnActors` は、部屋にスポーンする敵アクター Blueprint の候補リストです。
- `AreaRequiredPerPerson` と `MaxNumberOfActor` は、役割倍率を掛ける前の面積ベースの敵数を調整します。
- `SpawnKeyActor` と `SpawnUniqueKeyActor` は、MissionGraph の鍵付きルートで使う鍵アクターのスポーンを助けます。

部屋役割、Zone、深度、経路フラグに応じて報酬、罠、イベント、敵の種類、演出を細かく切り替えたい場合は Blueprint ロジックを使ってください。

## 敵スポーン倍率
`SpawnActors` は部屋に出す敵アクターの候補として扱われます。センサーは部屋面積から理想数を計算し、その数に `GameplayRoleEnemySpawnMultipliers` と `StructuralRoleEnemySpawnMultipliers` の該当 Role フィールドの倍率を掛けます。

各倍率設定には Role ごとのフィールドが用意されています。調整したい Role の値を直接編集してください。配列要素を追加する必要はなく、同じ Role を重複設定することもありません。

`IdealNumberOfActor()` は、これらのゲームプレイ役割と構造役割の倍率を反映した後の最終的な Helper 人数を返します。

`bMainPathRoom`、`bDeadEndRoom`、`bLockedRouteRoom`、`bSecretRoom` などの部屋フラグは Blueprint の分岐用情報です。報酬、罠、イベント、内装演出の切り替えに使ってください。これらのフラグだけでヘルパーの敵スポーン数は変わりません。

デフォルトのゲームプレイ倍率は `None = 0.5`、`Combat = 1.0`、`Treasure = 0.8`、`Puzzle = 0.5`、`Rest = 0.0`、`Boss = 2.0`、`Secret = 0.7` です。構造役割の `Start` と `Goal` はデフォルトで `0.0` のため、構造倍率を上書きしない限り敵は出ません。

## 注意点
- `SpawnActorInAisle` は `ADungeonRoomSensorBase` ではなく、`Gameplay.SpawnActorInAisle` または `Zones[].GameplayOverride.SpawnActorInAisle` で設定します。
- `Gameplay.DungeonRoomSensorClass` はデフォルトのセンサークラスを決めます。部屋の中で何をするかは、`ADungeonRoomSensorBase` 派生 Blueprint に書きます。
- この Actor は標準ではレプリケーション前提ではありません。同期が必要なランダム処理では、サーバーとクライアントで呼び出し回数がずれないようにしてください。

## 関連ページ
- [UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md)
