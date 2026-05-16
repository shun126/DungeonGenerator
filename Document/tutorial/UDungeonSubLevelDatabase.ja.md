# UDungeonSubLevelDatabase ガイド

`UDungeonSubLevelDatabase` は、**手作りした特殊部屋レベルをダンジョンへ差し込むためのデータベース**です。  
スタート部屋、ゴール部屋、必ず置きたい特別部屋、ランダムに出したい隠し部屋などをここで管理します。

## 使いどころ
- スタート部屋を固定デザインにしたい
- ゴール部屋やボス部屋を手作りしたい
- 隠し部屋やイベント部屋をランダムに混ぜたい

## 前提
この Database に登録するレベルは、`ADungeonSubLevelScriptActor` を親クラスにしたサブレベルとして作成します。  
また、`Build` 実行時にそのレベルからサイズとグリッド情報を読み取るため、**編集後は `Build` が必須**です。

## 主な項目
- `GridSize` / `VerticalGridSize`  
  表示専用です。実際の値は各サブレベル内の `ADungeonSubLevelScriptActor` からコピーされます。
- `StartRoom`  
  スタート部屋として使う固定サブレベルです。`LevelPath` が空なら自動生成部屋に戻ります。
- `GoalRoom`  
  ゴール部屋として使う固定サブレベルです。`LevelPath` が空なら自動生成部屋に戻ります。
- `Preferred Sublevel` (`DungeonRoomRegister`)  
  必ず置きたい、または優先的に置きたいサブレベル群です。
- `Random Sublevel` (`DungeonRoomLocator`)  
  条件に合う部屋へ抽選で差し込むサブレベル群です。
- `Random Sublevel Draw Count`  
  ランダム枠の最大抽選回数です。`0` なら全候補を抽選対象にします。

## `Preferred Sublevel` と `Random Sublevel` の違い
- `Preferred Sublevel`  
  固定寄りの特殊部屋向けです。たとえば「このボス部屋は必ず出したい」場合に向いています。
- `Random Sublevel`  
  条件付き抽選向けです。たとえば「一定確率で隠し部屋を出したい」場合に向いています。

`Random Sublevel` の各要素 (`FDungeonRoomLocator`) では、次の条件を持てます。

- 幅 / 奥行 / 高さの条件
- どの種類の部屋に置けるか
- どの種類のアイテム部屋に置けるか
- `AddingProbability`

## 使い方の流れ
1. 特殊部屋用レベルを作り、親クラスを `ADungeonSubLevelScriptActor` にします。
2. サブレベル側で `GridSize` / `VerticalGridSize` / `Width` / `Depth` / `Height` を設定します。
3. そのレベルを `StartRoom`、`GoalRoom`、`Preferred Sublevel`、`Random Sublevel` のいずれかに登録します。
4. `Build` を実行します。
5. `UDungeonGenerateParameter` の `DungeonSubLevelDatabase` にこのアセットを指定します。

## 編集のヒント
- 本体の `UDungeonGenerateParameter` とサブレベル側のグリッドサイズは必ず一致させてください。
- サブレベルを編集したあとに `Build` を忘れると、サイズや接続情報が古いまま残ります。
- `Build` はレベルを一時ロードして解析するため、**現在編集中のそのレベル自身**は対象にしない方が安全です。
- すでにロード済みのロビーをそのままスタート部屋にしたい場合は、この Database ではなく `ADungeonGenerateActor` の `StartRoomSubLevelScriptActor` を使います。

## 次に読む
- [ADungeonSubLevelScriptActor.ja.md](./ADungeonSubLevelScriptActor.ja.md)  
  サブレベル側で配置する必須アクターの設定手順を確認できます。
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)  
  開始部屋サブレベルと既存ロビー接続の違いをまとめて確認できます。

## 関連ページ
- [ADungeonSubLevelScriptActor.ja.md](./ADungeonSubLevelScriptActor.ja.md)
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)

