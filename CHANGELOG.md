# Change Log

## [Unreleased] -1.5.1 (15)
### Changes
### 変更点

## 20231010-1.5.0 (14)
### Changes
### 変更点

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

