# Change Log - Procedural 3D Dungeon Generator Plug-in

## Unreleased-1.6.* (44)
### Changes
### 変更点

## Unreleased-1.6.14 (43)
### Changes
* Fixed problem with incorrect distance from starting room
* Fixed several bugs
### 変更点
* スタート部屋からの距離が不正になる問題を修正
* いくつかの不具合を修正

## Unreleased-1.6.13 (42)
### Changes
* Support for Unreal Engine 5.5.1
* Fixed an issue where the starting position would be incorrect in rooms with 2 grids or less
* Fixed an issue where vegetation in the center of the room was appearing on the roof
* Added direction for Catwalk floors
* Fixed several bugs
### 変更点
* Unreal Engine 5.5.1対応
* ２グリッド以下の部屋で開始位置が不正になる問題を修正
* 部屋の中心の植生が屋根に出ていた問題を修正
* Catwalkの床の方向を追加
* いくつかの不具合を修正

## 20241027-1.6.12 (41)
### Changes
* Deprecated DungeonAisleMeshSetDatabase and DungeonRoomMeshSetDatabase, and introduced DungeonMeshDatabase.
* Fixed an issue where rooms could exceed the maximum size.
* Resolved an issue where sublevels were not unloading properly.
* Fixed activation of main level partitioning during multiplayer
* Fixed several bugs
### 変更点
* DungeonAisleMeshSetDatabaseとDungeonRoomMeshSetDatabaseを非推奨にしてDungeonMeshDatabaseを新設しました
* 部屋が最大サイズよりも大きくなる問題を修正
* サブレベルが解放されない問題を修正
* マルチプレイヤー時のメインレベルのパーティエーションの有効化を修正
* いくつかの不具合を修正

## 20241017-1.6.11 (40)
### Changes
* Added demo map
* modified content names
* Refactoring UCLASS attributes
* Fixed problem with rooms becoming larger than maximum size
* Fixed several bugs
### 変更点
* デモ用マップを追加
* コンテンツ名を修正
* UCLASS属性の見直し
* 部屋が最大サイズよりも大きくなる問題を修正
* いくつかの不具合を修正

## 20241005-1.6.10 (39)
### Changes
* Review UCLASS attributes
* Fixed several bugs
### 変更点
* UCLASSの属性を見直し
* いくつかの不具合を修正

## 20240927-1.6.9 (38)
### Changes
* Fixed a bug when sublevels were not specified
* Added assets available in the top view
### 変更点
* サブレベルを指定していない時の不具合を修正
* トップビューで利用可能なアセットを追加

## 20240926-1.6.8 (37)
### Changes
* Fixed a bug in the GenerateDungeon function that caused StaticMesh generation to fail.
### 変更点
* GenerateDungeon関数でStaticMeshの生成に失敗する不具合を修正

## 20240915-1.6.7 (36)
### Changes
* Added support for indoor staircase generation
* Added sublevels that prioritize generation
* Improved passageway generation
* Improved room separation method
* Fixed a bug that caused MissionGraph to generate levels that could not be cleared
* Fixed several bugs
### 変更点
* 室内の階段生成に対応
* 生成を優先するサブレベルを追加
* 通路生成の改善
* 部屋の分離方法の改善
* MissionGraphがクリアできないレベルを生成する不具合を修正
* いくつかの不具合を修正

## 20240901-1.6.6 (35)
### Changes
* Added candidate number of levels
* Added automatic generation of Foliage
* Fixed problem with doors lining up
* Changed dungeon generation path
* Extensive refactoring
* Fixed several bugs
### 変更点
* 階層数の候補を追加
* Foliageの自動生成を追加
* ドアが並ぶ問題を修正
* ダンジョンの生成パスを変更
* 大規模なリファクタリングを実施
* いくつかの不具合を修正

## 20240812-1.6.5 (34)
### Changes
* Fixed layer calculation for TransformWorldToRadarWithLayer
* Some refactoring
### 変更点
* TransformWorldToRadarWithLayerのレイヤー計算を修正
* いくつかのリファクタリングを実施

## 20240806-1.6.4 (33)
### Changes
* Fixed crash during minimap generation in UE5.3
### 変更点
* UE5.3以前でミニマップの生成時にクラッシュする問題を修正

## 20240803-1.6.3 (32)
### Changes
* Fixed incorrect spawn positions for small items used for interior decoration
* Fixed minimap textures not being output
* Trial version support
* Fixed several bugs
### 変更点
* 内装に使う小物のスポーン位置が不正になる問題を修正
* ミニマップテクスチャが出力されない問題を修正
* 体験版対応
* いくつかの不具合を修正

## 20240729-1.6.2 (31)
### Changes
* Changed to generate the dungeon at the origin of DungeonGenerateActor.
* Added notifications for dungeon generation success and failure to DungeonGenerateActor.
* Added a list of PlayerStart locations moved to DungeonGenerateActor.
* Fixed several bugs.
### 変更点
* DungeonGenerateActorを原点にダンジョンを生成するように変更
* ダンジョン生成の成功と失敗の通知をDungeonGenerateActorに追加
* DungeonGenerateActorに移動したPlayerStartの一覧を追加
* いくつかの不具合を修正

## 20240707-1.6.1 (30)
### Changes
* Fixed 'FindTeleportSpot' warning in HISM mode
* Added helper widget and function for minimap
* Improved complexity algorithm and generation stability of aisle
* Fixed several bugs
### 変更点
* HISM modeで起こる'FindTeleportSpot'警告を修正
* ミニマップの為のヘルパーウィジットと関数を追加
* 通路の複雑性アルゴリズムと生成の安定性を改善
* いくつかの不具合を修正

## 20240615-1.6.0 (29)
### Changes
* Vertical and horizontal grid size can be set individually
* Random numbers in DungeonRoomSensorBase can be selected between synchronous and asynchronous
* Add start and end sublevels to MissionGraph
* Add information on whether a passage is a main line or a detour
* Improved complexity algorithm and generation stability of corridors
* Fixed several bugs
### 変更点
* グリッドサイズの垂直サイズと水平サイズを個別に設定可能
* DungeonRoomSensorBaseの乱数を同期と非同期から選択可能
* MissionGraphに開始と終了のサブレベルを対応
* 通路に幹線か迂回か情報を追加
* 通路の複雑性アルゴリズムと生成の安定性が改善
* いくつかの不具合を修正

## 20240529-1.5.14 (28)
### Changes
* Include sublevel `PlayerStart` in the selection of the start position
* Fixed several bugs
### 変更点
* サブレベルの`PlayerStart`をスタート位置の選択に含める
* いくつかの不具合を修正

## 20240526-1.5.13 (27)
### Changes
* Verified stable routing
* Fixed the search for the location of the gate in the starting room
* Fixed several bugs
### 変更点
* 安定した経路作成の検証
* 開始部屋の門の位置の検索を修正
* いくつかの不具合を修正

## 20240519-1.5.12 (26)
### Changes
* Fixed wall exclusion information within sublevels
### 変更点
* サブレベル内の壁進入禁止情報を修正

## 20240512-1.5.11 (25)
### Changes
* Changed so that candelabras can be spawned without pillars
* Added sub-level lottery establishment
* Fixed a bug with multiplayer support
  * Changed room sensors, doors, candelabra, and interiors to spawn only on server
  * Temporarily removed `Spawn on client
### 変更点
* 柱が無くても燭台をスポーンできるように変更
* サブレベルの抽選確立を追加
* マルチプレイヤー対応の不具合を修正
  * 部屋センサー、ドア、燭台、インテリアはサーバーのみスポーンするように変更
  * `Spawn on client`の一時的な廃止

## 20240424-1.5.10 (24)
### Changes
* Added door generation probability to DungeonRoomSensor
* Unreal Engine 5.4 support
### 変更点
* ドアの生成確率をDungeonRoomSensorに追加
* Unreal Engine 5.4対応

## 20240327-1.5.9 (23)
### Changes
* Fixed several bugs
### 変更点
* いくつかの不具合を修正

## 20240325-1.5.8 (22)
### Changes
* パラメータへのコメントを追加
* 生成時の余白を追加
### 変更点
* Added comments to parameters
* Added margins for generation

## 20240223-1.5.7 (21)
### Changes
* Changed to discard Editor-only actors at runtime generation
* Fixed several bugs
### 変更点
* ランタイム生成時にEditorのみのアクターを破棄するように変更
* いくつかの不具合を修正

## 20240210-1.5.6 (20)
### Changes
* Added rules for generating torch
* Fixed several bugs
### 変更点
* 燭台の生成ルールを追加
* いくつかの不具合を修正

## 20240201-1.5.5 (19)
### Changes
* Extended the ability to choose to spawn actors in the client
### 変更点
* クライアントでのアクターのスポーンを選択できるように拡張

## 20240129-1.5.4 (18)
### Changes
* Fixed a replication bug.
### 変更点
* レプリケーションの不具合を修正

## 20240126-1.5.3 (17)
### Changes
* Discontinued support for Unreal Engine 4
* Changed door generation to before room sensors
* Added function to get door from sensor
* Added function to get candlestick from sensor
* Added MissionGraph validity specification
* Added complex passage generation
* Added dungeon generation mode with no hierarchy
* Replication changed to standard enabled
### 変更点
* Unreal Engine 4のサポートを終了
* ドアの生成を部屋のセンサーの前に変更
* センサーからドアを取得する関数を追加
* センサーから燭台を取得する関数を追加
* MissionGraph有効性の指定を追加
* 複雑な通路の生成を追加
* 階層のないダンジョン生成モードを追加
* レプリケーションを標準で有効に変更

## 20231211-1.5.2 (16)
### Changes
* Fixed a bug that prevented specifying doors.
### 変更点
* ドアを指定できない不具合を修正

## 20231202-1.5.1 (15)
### Changes
* Support for Android (UE5.3 only)
* Added a system to support load reduction
* Added a system for specifying the prohibition of generating walls and doors to level streaming
* Fixed several bugs
### 変更点
* Androidに対応 (UE5.3のみ)
* 負荷軽減をサポートするシステムを追加
* レベルストリーミングに壁やドアの生成禁止を指定するシステムを追加
* いくつかの不具合を修正

## 20231022-1.5.0 (14)
### Changes
* Added room and passageway mesh set assets
* Fixed some bugs
### 変更点
* 部屋と通路のメッシュセットアセットを追加
* いくつかの不具合を修正

## 20230930-1.4.8 (13)
### Changes
* Fixed a bug when the start room was not set
### 変更点
* スタート部屋を未設定の時の不具合を修正

## 20230920-1.4.7 (12)
### Changes
* Register dungeon mesh in DungeonPartsDatabase asset
* Fixed some bugs
### 変更点
* DungeonPartsDatabaseアセットにダンジョンのメッシュを登録
* いくつかの不具合を修正

## 20230908-1.4.6 (11)
### Changes
* Support for Unreal Engine 5.3
* Unreal Engine 4 no longer supported
* Support for loading sublevels in the start and finish rooms
* Added plug-in content
### 変更点
* Unreal Engine 5.3に対応
* Unreal Engine 4のサポートを終了
* スタート部屋、ゴール部屋のサブレベル読み込みに対応
* プラグインコンテンツの追加

## 20230901-1.4.5 (10)
### Changes
* Modified to generate dungeons on a flat surface if room merging or room margins are less than 1
* Added minimap information asset
* Added world space to texture space conversion class
* modified paths for plugin content
* Added sample data to plugin, eliminated dungeon hierarchy specification
* Fixed cache misalignment in system tags
* Started network functionality verification
* Moved PlayerStart off center of start room if more than one PlayerStart was installed
### 変更点
* 部屋の結合または部屋の余白が１以下ならば平面にダンジョンを生成するように修正
* ミニマップ情報アセットを追加
* ワールド空間からテクスチャ空間への変換クラスを追加
* プラグインコンテンツのパスを修正
* プラグインにサンプルデータを追加、ダンジョンの階層指定を廃止
* システムタグのキャッシュずれを修正
* ネットワーク機能検証開始
* PlayerStartが複数設置されていた場合はスタート部屋の中心からずらして配置

## 20230801-1.4.4 (9)
### Changes
* Interior Decorator beta version released
* Added interior assets
* Fixed some bugs
### 変更点
* インテリアデコレーターベータ版リリース
* インテリアアセットを追加
* いくつかの不具合を修正

## 20230514-1.4.3 (8)
### Changes
* Interior Decorator Verification
* Removed sample models as plug-in assets were added
### 変更点
* インテリアデコレーターの検証
* プラグインアセットを追加に伴ってサンプルモデルを削除

## 20230514-1.4.2 (7)
### Changes
* UE5.2 support
* Supports sub-level merging
* Copy LevelStreaming actors in editor mode to level
### 変更点
* Unreal Engine 5.2に対応
* サブレベルのマージに対応
* エディタモードの LevelStreamingアクターをレベルにコピー

## 20230409-v1.4.1 (6)
### Changes
* Confirmed that the package can be created.
* Add a test that generates pre-created sublevels in the room.
* Add a test for vertical margins.
* Add a test generation of minimap texture assets.
### 変更点
* パッケージが作成できることを確認
* 部屋にあらかじめ作成されたサブレベルを生成するテスト
* 垂直マージンのテスト
* ミニマップのテクスチャアセットの生成テスト

## 20230403-v1.4.0 (5)
### Changes
* Generate mini-maps in two types of pixel size and resolution (dots/meters)
### 変更点
* ミニマップをピクセルサイズと解像度（ドット/メートル）の二種類から生成

## 20230321-v1.3.1 (4)
### Changes
* Fixed mini-map generationo fail.
### 変更点
* ミニマップ生成時にクラッシュする問題を修正

## 20230319-v1.3.0 (3)
### Changes
* Supports mini-maps
### 変更点
* ミニマップに対応

## 20230316-v1.2.0 (2)
### Changes
* Compatible with Unreal Engine 4.27.2
### 変更点
* Unreal Engine 4.27.2に対応

## 20230308-v1.0.1 (1)
### Changes
* Fixed hang-up when referencing minimap textures when dungeon creation fails.
* Changed random room placement method to be more randomly distributed.
* Improved dungeon creation speed.
### 変更点
* ダンジョン生成に失敗した時にミニマップのテクスチャを参照するとハングアップする問題を修正
* 部屋のランダム配置方法をよりランダムに分散するよう変更
* ダンジョン生成速度を改善

## 20230303-v1.0.0 (0)
### Changes
* Initial release version
### 変更点
* 初回リリース版

