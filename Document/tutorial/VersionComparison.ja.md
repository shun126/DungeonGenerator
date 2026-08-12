# Dungeon Generator v1.x と v2.0.0 の比較

Dungeon Generator v2.0.0 は、単に部屋と通路をランダム生成するだけでなく、**遊び方を意識した経路、場所ごとに変化する見た目、調整しやすい設定**を作るためのメジャーアップデートです。

> **警告 — v1.x からの移行はサポートされません:** v2.0.0 には破壊的な変更が含まれており、v1.x のプロジェクトやアセットをアップグレードまたは変換することはできません。v1.x のプロジェクトとプラグイン環境はそのまま保管してください。v2.0.0 を使用する場合は、別のプロジェクトまたはブランチで Dungeon Generator の設定を新しく作成してください。

互換性を確認したメッシュなど、一部の素材は手動で再利用できる場合がありますが、v1.x の Dungeon Generator アセット、パラメータ、Blueprint、C++ 連携は自動移行されません。生成アルゴリズムとパラメータ構成も変更されているため、同じシードと似た設定を使用しても、v1.x と同じレイアウトは再現されません。

## 主な違い

| 項目 | v1.x | v2.0.0 | v2 で得られること |
| --- | --- | --- | --- |
| レベル設計 | 部屋数、サイズ、階層、通路の複雑さなどを個別に調整 | `Progression Policy` を起点に、主経路、分岐、ループ、スタート、ゴールをまとめて設計 | 「自由探索」「鍵と扉」「ボスへ向かう道」など、遊び方からレイアウトを選べる |
| レイアウト候補 | 指定した条件からレイアウトを生成 | 複数候補を評価し、より適したレイアウトを採用 | 候補数と生成コストのバランスを調整しながら、狙いに近い結果を選びやすい |
| 部屋の意味 | 経路フラグやセンサー情報を使ってユーザー側で判定 | `Combat`、`Treasure`、`Puzzle`、`Rest`、`Boss`、`Secret` などの Gameplay Role を付与 | 敵、報酬、休憩、演出を部屋の役割に合わせて実装しやすい |
| Zone | 主に共通の生成設定とデータベースを使用 | 進行度や階層に応じて Zone を選び、見た目や Gameplay 設定を上書き | 入口、深部、上層など、場所ごとに雰囲気を変えやすい |
| 敵配置 | Room Sensor とそのデータベースを中心に設定 | 生成された部屋情報、Gameplay Role、Structural Role、Zone を Room Sensor から利用 | 部屋の意味や経路上の位置に合わせて、敵数やイベントを調整しやすい |
| 内装と植生 | Interior Database による家具、装飾、植生の配置 | Role／Zone ごとに Interior と Fixture を上書き可能。植生の分散生成とクラスタリングも追加 | 戦闘部屋、宝物庫、秘密部屋などに合った内装を作り、生成時の負荷も調整できる |
| Fixture | 柱、たいまつ、ドアなどを共通設定から選択 | Role／Zone 単位の Fixture Override と、Unique Lock 用ドアを設定 | 場所やゲームプレイに合わせて固定装飾と扉を切り替えられる |
| Mesh／Parts の選択 | Random、Identifier、Depth From Start、旧式のカスタム選択を使用 | Mesh Set Selector と Parts Selector に選択処理を統一し、Blueprint／C++ で拡張可能 | 選択ルールの役割が分かりやすくなり、独自ルールを再利用しやすい |
| ミニマップとマップ | ミニマップテクスチャ、マスク、補助ウィジェットを提供 | HUD 用ミニマップに加え、マップ画面、アイコン、回転、未探索領域の表示制御を強化 | 探索状況を伝えるゲーム内マップを組み立てやすい |
| ランタイム負荷制御 | パーティション単位の表示・負荷軽減を提供 | Activator Component、疎なパーティショングラフ、フレームごとの有効化数、AI・Collision・Tick・ライト影などの制御を拡張 | 大きなダンジョンでも、ゲーム内容に合わせて負荷軽減の粒度を選べる |
| パラメータ編集 | 多くの項目が `UDungeonGenerateParameter` のトップレベルに並ぶ | `Theme`、`Structure`、`Path`、`Zones`、`Gameplay` に目的別で整理 | まず触る設定と、後から細かく調整する設定を見分けやすい |

## Progression Policy を選ぶ

v2.0.0 では、最初に `Path.ProgressionPolicy` でダンジョンの基本的な遊び方を選びます。その後で、主経路の強さ、ループ、追加通路などを微調整します。

| Policy | どのようなダンジョンになるか | 向いているゲーム |
| --- | --- | --- |
| `Free Exploration` | ループ、近道、寄り道を含む、自由に歩き回れる構成 | 探索、収集、ローグライト |
| `Start To Goal` | スタートからゴールまでの主経路が読み取りやすい構成 | 一本道を基準に分岐も加えたいゲーム |
| `Keys And Locks` | 鍵を取得して対応する扉を開ける、迂回できない攻略経路 | 鍵探索、脱出、段階的な進行 |
| `Boss Route` | 戦闘や休憩を挟み、終盤のボス部屋へ盛り上げる構成 | アクション、ボス攻略 |
| `Hub Quest` | 早い段階にハブを置き、複数の目的地へ分岐する構成 | クエスト、拠点型探索 |

![5つの Progression Policy の比較](images/ProgressionPolicyStyles.png)

画像の `S` はスタート、`G` はゴール、明るい線は代表的な進行経路です。Key／Lock、Boss、Hub の位置関係を見ると、同じ部屋数でも Policy によってプレイヤー体験がどう変わるかを比較できます。この画像は概念例であり、実際に生成される部屋形状や装飾を固定するものではありません。

`Keys And Locks` は、ロックを迂回する危険なループを防ぎます。追加通路の複雑度は未施錠通路には作用しますが、鍵付き通路では交差と結合が無効になり、扉を迂回できません。v1.x の構成を手動で作り直す場合、旧 `UseMissionGraph` に最も近い v2 の Policy は `Keys And Locks` です。通常のスタートからゴールへ進む構成は `Start To Goal` を出発点にして、新しい設定を目的に合わせて調整してください。

## Gameplay Role で部屋の目的を伝える

![Gameplay Role ごとの部屋イメージ](images/RoomGameplayRoleStyles.png)

Gameplay Role は、生成された部屋に `None`、`Combat`、`Treasure`、`Puzzle`、`Rest`、`Boss`、`Secret` という目的を与えます。画像はそれぞれの用途を視覚化した例です。Role を割り当てただけで敵、宝箱、パズルが自動配置されるわけではなく、Room Sensor の Blueprint 処理や Role ごとのメッシュ、内装、Fixture の上書きへつなげて使用します。そのため、v2.0.0 ではレイアウトとゲーム内容の意図を分けて調整できます。

## v2.0.0 が向いているケース

- ダンジョンごとに、探索型、鍵攻略型、ボス攻略型などの違いを出したい
- 戦闘、宝物、休憩、秘密など、部屋ごとの役割をゲームプレイへ結び付けたい
- 進行度、階層、部屋の役割に応じてメッシュ、内装、植生、扉を変えたい
- デザイナーが関連する設定をグループ単位で調整できるようにしたい
- 大規模なダンジョンで、Actor、AI、Collision、Tick、ライトの負荷を細かく制御したい
- HUD のミニマップだけでなく、アイコンや探索状態を含むマップ画面を作りたい

## v1.x のまま運用する選択肢

次のようなプロジェクトでは、すぐに移行せず、現在の制作工程を維持する判断もできます。

- v1.x の生成結果を前提にレベル調整やゲームバランスが完了している
- リリース直前で、レイアウト、Blueprint、C++ の再検証に時間を割けない
- 新しい Progression Policy、Room Role、Zone Override、マップ機能を必要としていない
- 固定シードを含め、現在の生成結果を変えたくない

v1.x から v2 への移行パスはサポートされないため、v2 の採用には Dungeon Generator の設定と連携処理の再構築が必要です。新機能の価値と、再作成・再調整・テストに必要な時間を比較してください。

## v1.x の構成を v2.0.0 で作り直す

上書きアップグレードやアセットの自動変換はサポートされません。v2.0.0 は新しい実装として扱ってください。

1. **復元できる v1.x プロジェクトを保管します。**
   v1.x プラグインを維持し、元のアセットを開けるソース管理ブランチまたはコピーを残してください。
2. **v2 専用の作業環境を用意します。**
   稼働中の v1.x プロジェクトを上書きしたり、v1.x の Dungeon Generator アセットを v2 で開けば変換されると想定したりしないでください。
3. **生成設定を新しく作成します。**
   v2 用のアセットを作成し、`Theme`、`Structure`、`Path`、`Zones`、`Gameplay` を設定します。比較表は参考として使用し、パラメータが一対一で対応するとは考えないでください。
4. **プロジェクトとの連携を手動で作り直します。**
   改名または削除された API に合わせて Blueprint／C++ を修正し、必要に応じて Room Sensor、Selector、Interior、SubLevel、ミニマップとの連携を再構築します。
5. **新しいダンジョン設計としてテストします。**
   エディタ生成、ランタイム生成、見た目、進行、ゲームプレイ、レプリケーション、負荷を確認してください。v1.x の固定シードやレイアウトは、v2 の互換性対象ではありません。

## Blueprint／C++ 利用者向けの注意

v2.0.0 では、生成された部屋情報の標準 API は `FDungeonGeneratedRoomInfo` と `ADungeonRoomSensorBase::GetGeneratedRoomInfo()` です。v1 の Room Sensor フィールドに依存せず、この API に合わせて連携処理を作り直してください。

`EnableLightShadowControl`、`IsEnableLightCastShadowControl()`、`SetEnableLightCastShadowControl()` はv2.0.0で削除されました。v1のBlueprintまたはC++クラスから参照している場合は、v2.0.0でコンパイルする前に該当する参照を手動で削除してください。Point LightとSpot Lightの影は、現在の実行時ライト制御システムが自動で管理します。

`GetInquireInteriorTags()`と`InquireInteriorTags`は、`GetProvidedContextTags()`と`ProvidedContextTags`へ置き換えられました。この名前変更は自動リダイレクトされません。Room Sensor Blueprintの実装を新しいEventで作り直し、Interior Location Componentに保存していた値を再入力してください。

`EDungeonInteriorPlacementAnchor`、`PlacementAnchors`、Placement Queryの`Anchor`メンバーは削除されました。v2のInterior Actor配置では、部屋中央の自動候補を生成しません。`UDungeonInteriorLocationComponent`で制作者が指定する入れ子スポーン位置は引き続き利用できます。

Interior Locationの入れ子スポーンでは、対象Partsに1つ以上の`RequiredContextTags`が必要になりました。タグなしの入れ子配置を作り直す場合は、Interior Locationへ対応する`ProvidedContextTags`を設定してください。再帰分岐では、祖先に存在するActor Classも候補から除外されます。

Mesh Set と個別パーツの選択は、Mesh Set Selector／Parts Selector を使う構成へ整理されています。独自の Blueprint または C++ 選択処理がある場合は、コンパイルが通ることだけでなく、同じ条件で意図した候補が選ばれることも確認してください。

## v2 再構築の確認リスト

- v1.x プラグインを含む、復元可能な v1.x プロジェクトを保管した
- v2 の作業を別のプロジェクトまたはソース管理ブランチで行っている
- 必要な v2 用 `UDungeonGenerateParameter`、Mesh Set、Interior、SubLevel、Room Sensor アセットを新しく作成した
- エディタ生成とランタイム生成の両方をテストした
- Blueprint／C++ の Room Sensor、セレクター、ミニマップ連携をテストした
- 新しい v2 の設計として、見た目とゲーム進行を確認した

## 関連ページ

- [UDungeonGenerateParameter ガイド](./UDungeonGenerateParameter.ja.md)
- [ADungeonRoomSensorBase ガイド](./ADungeonRoomSensorBase.ja.md)
- [カスタムセレクタガイド](./CustomSelector.ja.md)
- [ミニマップを生成する](./GenerateMinimapTexture.ja.md)
- [ランタイム処理負荷を軽減する](./LoadReduction.ja.md)
