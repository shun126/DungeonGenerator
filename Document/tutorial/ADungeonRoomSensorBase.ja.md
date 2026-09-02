# ADungeonRoomSensorBase ガイド

`ADungeonRoomSensorBase` は、部屋に入ったときのイベント、敵や報酬の配置、罠、BGM 切り替え、部屋ごとの内装タグなどを実装するための基底クラスです。

## 基本の流れ
1. `ADungeonRoomSensorBase` を親クラスにした Blueprint を作ります。
2. 必要なイベントを実装します。
3. 作成した Blueprint を `Gameplay.DungeonRoomSensorClass` に登録します。
4. Zone や RoomRole ごとに別のセンサーが必要な場合だけ、`Zones[].GameplayOverride` または `Gameplay.RoomRoles.Roles[].GameplayOverride` を使います。

## ベース補助光
Room Sensor は、演出用ライトの間でもキャラクター、家具、通路を見分けられるよう、低強度で影なしの Point Light を生成します。これはゲームプレイに必要な最低限の視認性を確保するためのライトです。部屋の雰囲気は、燭台やシャンデリアなどの演出用ライトで作ります。

`DungeonGenerator|Lights|BaseFillLight` のプロパティで調整できます。

- `Enable Base Fill Lights` で生成を切り替えます。デフォルトでは無効なので、最低限の補助光が必要な部屋で意図的に有効化してください。
- `Cell Size` は、1つのライトが担当する床面積の目安です。大きな部屋は均等な複数セルに分割されます。
- `Max Base Fill Light Count X/Y` はライト数と描画負荷の上限を決めます。小部屋では1灯、細長い部屋では `1 x N` のように一列に配置されます。
- `Intensity Units` は `Unitless`、`Candelas`、`Lumens` から選べます。デフォルトは `Unitless` です。
- `Intensity` は選択した単位で明るさを設定し、`Color` は補助光の色を調整します。デフォルトは Unitless の `10.0` で、スライダー範囲を超える値も直接入力できます。
- `Height From Floor Ratio` は、床を `0`、天井を `1` としてライトの高さを決めます。デフォルトの `0.6` では、天井へ寄せずにキャラクターより高い位置へ配置されます。
- `Attenuation Scale` と減衰半径の最小・最大値で光の到達範囲を調整します。デフォルトはそれぞれ `1.05`、`100 cm`、`2000 cm` です。自動計算された半径は、ライトから担当セルの床面で最も遠い角まで届きます。

各ライトは担当セルの水平方向の中央に配置され、Cast Shadow は常に無効です。`Lumens` と `Candelas` では物理ベースの逆二乗減衰を使用します。`Unitless` は演出向けの調整モードで、従来と同じ減衰指数 `2` を使用します。影なしの Point Light は壁や天井で遮られないため、減衰半径を必要以上に大きくすると隣の部屋や上下階まで明るくなる場合があります。最大半径を大幅に増やす前に、X/Y方向の最大ライト数を増やしてください。

ダンジョン区画の負荷制御が利用できる場合、同期対応の Lighting Proxy が遠い区画の生成ライトを非表示にし、近づくと元に戻します。また Room Sensor 自体には汎用の表示 Activator があり、Room Sensor Blueprint に追加した他の表示用 Scene Component も制御対象になります。独立して常時表示する必要がある Component は別 Actor に置くか、区画表示制御を考慮してください。

Room Sensor は引き続き authority 側だけで生成され、レプリケートされません。そのため、既存のゲームプレイイベントはサーバーだけで動作します。ライトは同期対応の表示専用Proxyが各クライアントに同じ Base Fill Light と Guidance Light を自動構築し、途中参加時にも現在値を復元します。実行中に1部屋だけ変更する場合は、サーバーで `SetBaseFillLightIntensity`、`SetGuidanceLightIntensity`、または `SetRoomLightIntensities` を呼びます。全部屋をまとめて変更する場合は `ADungeonGenerateActor::SetAllRoomLightIntensities` を使います。負の値は `0` に補正されます。

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
`ADungeonRoomSensorBase` 直下にも非推奨の部屋情報フィールドが実装上の互換用残存項目として残っていますが、Version 1 からの移行をサポートするものではありません。
v2 の新しい Blueprint では、センサーから `Identifier`、`Parts`、`Item`、`BranchId`、`DepthFromStart`、`DeepestDepthFromStart` を直接読まず、`GetGeneratedRoomInfo()` を使ってください。

## 内装と連携する
`GetProvidedContextTags`を実装すると、その部屋からInterior Partsへ`kitchen`や`library`などの意味的なContext Tagを提供できます。Partsの`Required Context Tags`がすべて提供された場合だけ候補になります。
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

鍵 Helper が行うのは設定 Actor のスポーンだけです。取得、インベントリ登録、扉の判定、鍵の消費はゲーム側 Blueprint に実装してください。

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

## 誘導光
Room Sensor は、生成されたドアや部屋内の階段を見つけやすくするため、影を落とさない Spot Light を自動生成できます。実際に配置されたドアと、その部屋に属する階段だけが対象です。通路にある階段は対象になりません。

設定は `DungeonGenerator|Lights|GuidanceLight` にまとまっています。Guidance Light とドア対象はデフォルトで有効、階段対象は無効です。基本の明るさは `8` Candelas、減衰半径は `1200 cm`、内側 / 外側コーン角は `18 / 32` 度、1部屋の上限は `8` 灯です。`Lumens` と `Candelas` では逆二乗減衰、`Unitless` では演出向けの減衰指数 `2` を使用します。`Door Guidance Light Height Offset Scale` はドア上端から `0.5` 垂直グリッド、`Stair Guidance Light Height Below Ceiling` は天井から `100 cm`、照射目標高は床から `120 cm` がデフォルトです。ドア / 階段の明るさ倍率は `1.0 / 1.15` で、上限に達した場合はドアが優先されます。

ドア用ライトはドアより部屋中央側かつドア天面より上に置かれ、入口へ向けて照射します。ドア直上のグリッドが空いた `Floor` ではない場合、たとえば高さ1グリッドの部屋や室内Slopeの下では、そのドア用ライトを生成しません。階段用ライトは天井基準で配置します。影を落とさないため、光が届く距離やコーン角度を大きくしすぎると、壁や上下階を越えて明るくなることがあります。まず狭い範囲から調整してください。`GuidanceTargets` はサーバー側の Blueprint から読み取り専用で参照できます。生成後の描画用 Light Component は同期対応の表示専用Proxyが所有します。
