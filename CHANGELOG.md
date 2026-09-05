# Change Log - Procedural 3D Dungeon Generator Plug-in

## 20260907-v2.0.1 (101)
### Changes
* Switch the method for clearing the dungeon depending on the reason the game ends
* Fixed some bugs
### 変更点
* ゲーム終了の理由によってダンジョン破棄方法を切り替え
* いくつかの不具合を修正

## 20260821-2.0.0 (100)

Version 2.0 is a major redesign focused on playable progression, richer visual identity, and settings that are easier to understand and tune.

> **Warning — breaking changes:** Version 2.0 does not support migration, in-place upgrades, or automatic conversion from Version 1.x. Preserve the v1.x project and plugin version, and create the v2 setup separately. v1.x Dungeon Generator assets, parameters, Blueprint integrations, C++ integrations, fixed seeds, and generated layouts are not compatibility targets.

For a side-by-side feature comparison and the v2 rebuild checklist, see [Dungeon Generator v1.x and v2.0.0 Comparison](Document/tutorial/VersionComparison.en.md).

### Highlights

- Added five Progression Policies: `Free Exploration`, `Start To Goal`, `Keys And Locks`, `Boss Route`, and `Hub Quest`.
- Added Gameplay Roles for `Combat`, `Treasure`, `Puzzle`, `Rest`, `Boss`, and `Secret` rooms.
- Added Zone selection by progress and floor, with visual and Gameplay overrides.
- Added multi-candidate layout evaluation to select a result that better matches the chosen design intent.
- Reorganized `UDungeonGenerateParameter` into `Theme`, `Structure`, `Path`, `Zones`, and `Gameplay` groups.
- Expanded minimap, exploration-map, runtime activation, and load-reduction systems.

### v1.x to v2.0 comparison

| Area | v1.x | v2.0.0 |
| --- | --- | --- |
| Route design | Tune individual layout values | Choose a Progression Policy, then fine-tune routes, branches, and loops |
| Room purpose | Infer purpose in project logic | Use explicit Gameplay Roles |
| Visual variation | Primarily shared databases | Override Mesh Sets, Interiors, Fixtures, and vegetation by Role or Zone |
| Selection rules | Several separate selection paths | Unified Mesh Set Selector and Parts Selector workflow |
| Parameters | Many top-level properties | Purpose-based `Theme`, `Structure`, `Path`, `Zones`, and `Gameplay` groups |
| Runtime support | Partition-based load reduction | Detailed Actor, AI, Collision, Tick, light, and per-frame activation controls |

### Breaking changes

- `GetInquireInteriorTags()` and `InquireInteriorTags` were replaced without redirects by `GetProvidedContextTags()` and `ProvidedContextTags`.
- `EDungeonInteriorPlacementAnchor`, `PlacementAnchors`, and the placement-query `Anchor` member were removed.
- `EnableLightShadowControl`, `IsEnableLightCastShadowControl()`, and `SetEnableLightCastShadowControl()` were removed. Runtime light control now manages Point and Spot Light shadows.
- Automatic room-center Interior Actor candidates were removed. Placement now uses enabled Placement Areas and wall-side floor candidates.
- v1.x Dungeon Generator assets and integrations must be recreated or adapted manually for v2.0.0.

### Version compatibility

- Do not install v2.0.0 over a working v1.x project. Keep a restorable copy with the matching v1.x plugin.
- Create new v2 parameter assets and configure `Theme`, `Structure`, `Path`, `Zones`, and `Gameplay`; v1.x values are not converted automatically.
- When recreating an old `UseMissionGraph` design, use `Keys And Locks` as the closest starting Policy. For a regular route, begin with `Start To Goal`.
- Reuse source content such as meshes only after checking its v2 compatibility. Recreate Dungeon Generator assets and references where required.
- Rebuild Room Sensor integrations with `FDungeonGeneratedRoomInfo` and `GetGeneratedRoomInfo()` and update all renamed or removed Blueprint/C++ APIs.
- Treat generation output as a new design. Identical fixed-seed layouts are not supported across the major versions.

### Changes
- `SetRandomParameter` now also randomizes `Path.MainRouteBias`, `Path.LoopRouteDensity`, `Path.StartRoomPolicy`, `Path.GoalRoomPolicy` and `Path.bMovePlayerStartToStartRoom`, so a random generation test covers the route shape and the endpoint selection as well. `Path.RandomSeed` is left alone.
- Generation now fails with `DG_GEN_ROOM_ISOLATED` when the start room cannot reach every room. An aisle that could not be routed was given up, which left the room it connected unreachable while the room still appeared on the minimap. The dungeon is retried with another seed, so this is reported only when every attempt leaves a room isolated.
- Fixed vegetation components being torn down without being removed from their owner. They were destroyed with `UnregisterComponent` followed by `ConditionalBeginDestroy` instead of `DestroyComponent`, so the generator actor, which outlives a single generation, kept referencing them after their destruction had begun.
- Shortened the generated corridors. A branch aisle is now reconnected to the closest room that can act as its parent once the rooms have been placed, because the graph decides which rooms connect before any room has a position. Measured against a minimum spanning tree over the same rooms, the total corridor length went from 8.13 to 1.89 times the minimum in vertical layouts. Generated layouts differ from earlier versions even for the same seed.
- A dungeon generation that fails is now retried up to three times with a different seed, so an unlucky layout no longer surfaces as a failure. The retry is skipped when `Path.RandomSeed` is set, because that value asks for one specific dungeon. Measured over 1200 generations, every failure cleared within two retries at about one percent extra cost.
- The debug voxel image is now written when generation fails as well, and the endpoints of the aisle that could not be generated are drawn in a separate colour. A failed aisle owns no grid, so the image shows where it was meant to run.
- Corrected the fix hint for a gate shortage. Lowering `Path.LoopRouteDensity` was listed as a remedy but does not help, because the aisles competing for a room's openings are branches rather than loops. The hint now points at the sub-level's wall openings, which measurement confirms is the remedy that works.
- Fixed dungeon generation failing when applying the reserved Start and Goal room sizes pushed the two rooms into each other's margin. Both rooms have a fixed size, so no other room could be moved to separate them. The two rooms are now allowed to move, since only their size is fixed by the sub-level and not their location.
- Fixed generated content surviving a regeneration when a dungeon was destroyed and then generated again, or when a generation failed after it had started spawning actors. `Dispose` skipped its whole cleanup unless the previous generation had completed successfully. Repeated generation could accumulate enough lights to crash the renderer.
- Fixed sub-levels staying resident while the next dungeon loaded its own. An unload that was requested without waiting for it is now completed before the next generation loads anything.
- Fixed dungeon generation failing with a gate shortage when a room could not open a second gate into the corridor of a locked aisle. The lock is written on one gate only, so the room at the other end of that aisle already reaches the corridor without crossing the lock and a second opening cannot be used as a bypass. Only the room that holds the locked gate is refused now.
- Fixed the gate shortage report pointing at the start room's sub-level asset even when another room ran out of openings. It now names the room that actually failed.
- Fixed dungeon generation failing with a route search error when an aisle wrapped around a room it did not connect to and sealed the room's remaining openings into a dead pocket. The route search now pays an extra cost for running along the wall of such a room, weighted by how many gates that room still needs against how many candidates it has left.
- Fixed dungeon generation failing with a room separation error while `Keys And Locks` was enabled. Reapplying the endpoint policies to the moved rooms removed loop aisles from the layout graph after the aisle list had been built, so the aisles could no longer find their edge.
- Fixed dungeon generation failing when the layout offered no room for the unique key. `MissionGraph` leaves such a dungeon completely unlocked by design, but `MissionGraphTester` rejected it because it requires one unique key and one unique lock. Generation now succeeds and reports the `DG_GEN_KEYS_NOT_PLACED` warning through `GetLastGenerationIssues()`.
- Added generation failure reporting. `ADungeonGenerateBase::GetLastGenerationIssues()` returns the reason as `FDungeonValidationIssue` entries with a code, a message, a fix hint, the parameter to review, and the related sub-level asset. The editor tool lists them, shows the reason in the failure dialog, and includes them in `Copy diagnostics`.
- Demoted the grid, aisle, room, and voxel dumps that follow a generation failure from Error to Verbose. The failure headline, its cause, and the seed remain at Error.
- Added a warning when `Path.StartRoomPolicy` is `Use Central Point` or `Use Multi Start` while `Keys And Locks` is enabled. The value was already rewritten to `Use Southernmost` on load, but the change was silent.
- Room-owned slope, stairwell, DownSpace, and UpSpace walls and ceilings now use their containing Room vegetation settings, database overrides, and Provided Context Tags. Passage-owned slope-family grids remain in the `Slope` area, and slope floor behavior is unchanged.
- Fixed missing vegetation on generated Gate floors, lateral walls, and ceilings by reserving Gate grids with their containing Room vegetation job.
- Fixed stacked Interior Actors caused by untagged recursive Interior Location spawning. Nested Parts now require explicit Context Tags, and ancestor Actor Classes are excluded from each recursive branch.
- Removed automatic room-center Interior Actor spawning and the Placement Anchor API. Interior Actors now use wall-side floor candidates in their selected Placement Areas, while explicit Interior Location spawns and room vegetation remain available.
- Renamed the context-tag provider API to `GetProvidedContextTags()` and `ProvidedContextTags`, making its relationship with `RequiredContextTags` explicit. The legacy `GetInquireInteriorTags()` API and `InquireInteriorTags` property were removed without redirects.
- Fixed missing Interior Actor placement candidates in generated aisles, and added an independent opt-in `Slope` placement area for Interior Actors and vegetation. `Aisle` and `Slope` are configured as separate placement areas in v2.
- Added configurable, shadow-free base fill Point Lights and door/stair guidance Spot Lights to `ADungeonRoomSensorBase`. Their intensity units can be selected independently from Unitless, Candelas, and Lumens; Lumens with physically based inverse-square falloff is the default.
- Door guidance lights are now placed half a vertical grid above the door by default and are omitted when the grid directly above the door is not open room `Floor` space.
- Minimap floor textures now include the selected floor's complete height band up to the next floor, while the highest floor extends to the top of the dungeon voxel area.
- Minimap floor selection now changes at the same world-space elevations as the 3D floor debug planes, using Character feet and the dungeon actor's Z origin.
- Fixed aisle purposes being overwritten to `VerticalTransition` whenever the connected rooms sat at different heights. Branch, Loop, and Shortcut information survived nowhere in a vertical layout, so layout scoring could not tell a redundant loop from the main route. `Aisle::IsVerticalTransition()` now reports floor crossing as a separate property.
- Aisle voxel generation failures are now reported through a single path. A goal-side gate search failure previously produced no log and no error code, so a dungeon missing an aisle was returned as a successful generation.
- Rooms whose size cannot change, such as sublevels and reserved Start and Goal rooms, now reserve the grids in front of their wall openings so that another aisle's stairway cannot consume the only place a gate could go.
- Only locked aisles now require their own gate. Every other aisle may share one, which removes the gate exhaustion seen when Keys And Locks progression is combined with small fixed-size rooms.
- Fixed the intersection route search preferring to create new gates instead of reusing existing ones. The priority test was inverted.
- `Path.ExtraCorridorComplexity` now applies while Keys And Locks progression is enabled. It was silently forced to zero because lock placement shared the same flag as intersection generation. Locked aisles keep a private gate and corridor so that a locked door cannot be bypassed.

### v2.0 の概要

v2.0 は、**攻略して楽しい経路、場所ごとに変化する見た目、調整しやすい設定**を重視したメジャーアップデートです。

- 5種類の Progression Policy、部屋の Gameplay Role、進行度や階層に応じた Zone を追加しました。
- Role／Zone ごとにメッシュ、内装、Fixture、植生、Gameplay 設定を変更できるようにしました。
- 複数のレイアウト候補を評価し、設計意図に合う結果を選択する仕組みを追加しました。
- パラメータを `Theme`、`Structure`、`Path`、`Zones`、`Gameplay` に整理しました。
- Mesh Set／Parts Selector、部屋情報 API、マップ機能、ランタイム負荷制御を拡張しました。

### バージョン互換性に関する重要事項

> **警告 — 破壊的変更:** Version 2.0 は Version 1.x からの移行、上書きアップグレード、自動変換をサポートしていません。v1.x のプロジェクトと対応するプラグインをそのまま保管し、v2 の設定は別環境で新しく作成してください。v1.x の Dungeon Generator アセット、パラメータ、Blueprint／C++ 連携、固定シード、生成レイアウトには互換性がありません。

- 稼働中の v1.x プロジェクトへ v2.0.0 を上書き導入しないでください。対応する v1.x プラグインを含む、復元可能なコピーを保管してください。
- v2 のパラメータアセットを新しく作成し、`Theme`、`Structure`、`Path`、`Zones`、`Gameplay` を設定してください。v1.x の値は自動変換されません。
- 旧 `UseMissionGraph` の構成を作り直す場合は `Keys And Locks`、通常の経路は `Start To Goal` を出発点として調整してください。
- メッシュなどの素材は、v2 との互換性を確認したものだけを再利用してください。Dungeon Generator のアセットと参照は必要に応じて作り直してください。
- Room Sensor は `FDungeonGeneratedRoomInfo` と `GetGeneratedRoomInfo()` に合わせて再構築し、削除・改名された Blueprint／C++ API を手動で修正してください。
- 生成結果は新しい設計として検証してください。メジャーバージョン間で同じ固定シードのレイアウトは保証されません。
- 詳細な比較と v2 再構築の確認リストは [v1.x と v2.0.0 の比較](Document/tutorial/VersionComparison.ja.md)を参照してください。

### 変更点
- `SetRandomParameter` が `Path.MainRouteBias`、`Path.LoopRouteDensity`、`Path.StartRoomPolicy`、`Path.GoalRoomPolicy`、`Path.bMovePlayerStartToStartRoom` もランダム化するようになりました。ランダム生成のテストで経路の形と端点の選び方も試されます。`Path.RandomSeed` は変更しません。
- 開始部屋から到達できない部屋が残る場合、`DG_GEN_ROOM_ISOLATED` として生成を失敗させるようにしました。経路を作れなかった通路を諦めた結果、その通路がつないでいた部屋へ到達できなくなり、しかもミニマップには表示される状態になっていました。別の乱数の種で作り直すため、全ての試行で孤立した場合にのみ報告されます。
- 植生コンポーネントが所有アクターから取り除かれないまま破棄されていた問題を修正しました。`DestroyComponent` ではなく `UnregisterComponent` と `ConditionalBeginDestroy` の組み合わせで破棄していたため、1回の生成より長く生きるジェネレータアクターが、破棄を始めた後のコンポーネントを参照し続けていました。
- 生成される通路を短くしました。どの部屋どうしをつなぐかは部屋の座標が決まる前に確定するため、部屋を配置した後で枝の通路を最も近い親へつなぎ替えるようにしました。同じ部屋配置に対する最小全域木と比べて、垂直配置での通路の総距離が最小の8.13倍から1.89倍になりました。同じシードでも以前のバージョンとは異なるレイアウトが生成されます。
- 生成に失敗した場合、乱数の種を変えて最大3回まで作り直すようにしました。運が悪いレイアウトに当たっても失敗として表面化しなくなります。`Path.RandomSeed` を指定している場合は、特定のダンジョンを求める指定であるため再試行しません。1200回の生成で計測したところ、全ての失敗が2回以内の再試行で解消し、追加コストは約1%でした。
- 生成に失敗した場合もデバッグ用のボクセル画像を出力するようにし、生成できなかった通路の両端を別の色で描くようにしました。失敗した通路はグリッドを持たないため、画像にはどこを結ぶはずだったかを示します。
- 門不足の対処ヒントを修正しました。`Path.LoopRouteDensity` を下げる案を挙げていましたが効果がありません。部屋の開口部を取り合っているのはループではなく枝の通路のためです。計測で効果を確認できたサブレベルの開口部を増やす方法を案内するようにしました。
- 予約した開始部屋とゴール部屋のサイズを適用した結果、二部屋が互いのマージンへ食い込んで生成が失敗する問題を修正しました。どちらもサイズが固定されているため、他の部屋を動かしても引き離せませんでした。サブレベルが決めているのはサイズであって位置ではないため、この二部屋の移動を許可するようにしました。
- ダンジョンを破棄してから再生成した場合や、アクターの生成を始めた後に生成が失敗した場合に、生成物がワールドへ残る問題を修正しました。直前の生成が成功していない限り `Dispose` が後始末を丸ごと飛ばしていました。生成を繰り返すとライトが蓄積し、描画側のクラッシュに至る場合がありました。
- 次のダンジョンがサブレベルを読み込む時点で、前のサブレベルが解放されずに残る問題を修正しました。完了を待たずに要求した解放を、次の生成が何かを読み込む前に待ち合わせるようにしました。
- 施錠された通路の廊下へ部屋が2つ目の門を開けられず、門不足で生成が失敗する問題を修正しました。鍵は片方の門にしか書かれないため、通路の反対側の部屋は鍵を通らずに既に廊下とつながっており、2つ目の口は迂回路になりません。鍵をかけた門を持つ部屋だけを拒否するようにしました。
- 門不足の報告が、別の部屋で開口部が尽きた場合でも開始部屋のサブレベルアセットを指していた問題を修正しました。実際に失敗した部屋を示すようになりました。
- 接続しない部屋の外周に通路が貼り付いて、その部屋に残った開口部を袋小路に閉じ込め、経路探索のエラーで生成が失敗する問題を修正しました。経路探索がそうした部屋の壁際を通る際に追加コストを支払うようになり、コストはその部屋がまだ必要とする門の数と残りの候補数から決まります。
- `Keys And Locks` 有効時に、部屋の分離エラーで生成が失敗する問題を修正しました。移動後の部屋へ端点ポリシーを再適用する際、通路の一覧を構築した後にレイアウトグラフからループ通路の辺を削除していたため、通路が自身の辺を逆引きできなくなっていました。
- ユニーク鍵を置ける部屋が無いレイアウトで生成が失敗する問題を修正しました。`MissionGraph` は設計上そのダンジョンをロック無しのまま残しますが、`MissionGraphTester` がユニーク鍵とユニークロックを1組要求するため不合格にしていました。生成は成功するようになり、`GetLastGenerationIssues()` から `DG_GEN_KEYS_NOT_PLACED` の警告を取得できます。
- 生成失敗の報告に対応しました。`ADungeonGenerateBase::GetLastGenerationIssues()` が失敗の理由をコード、メッセージ、対処のヒント、確認すべきパラメータ、関連するサブレベルアセット付きの `FDungeonValidationIssue` として返します。エディタツールは一覧へ表示し、失敗ダイアログに理由を示し、`Copy diagnostics` にも含めます。
- 生成失敗後に出力されるグリッド、通路、部屋、ボクセルのダンプを Error から Verbose へ降格しました。失敗の見出しと原因、シード値は Error のまま残ります。
- `Keys And Locks` 有効時に `Path.StartRoomPolicy` が `Use Central Point` または `Use Multi Start` の場合へ警告を追加しました。従来も読み込み時に `Use Southernmost` へ書き換えていましたが、通知がありませんでした。
- Roomが所有するSlope、Stairwell、DownSpace、UpSpaceの壁と天井へ、所属RoomのVegetation設定、Database Override、Provided Context Tagsを適用するようにしました。通路所有のSlope系Gridは従来どおり`Slope`エリアを使用し、Slope床の挙動は変更していません。
- 生成されたGateの床、横壁、天井にVegetationが生えない問題を修正し、Gateグリッドを所属するRoomのVegetation Jobへ登録するようにしました。
- タグ未指定のInterior Location再帰スポーンによってInterior Actorが積み重なる問題を修正しました。入れ子Partsには明示的なContext Tagsが必要になり、再帰分岐の祖先Actor Classは候補から除外されます。
- 部屋中央へのInterior Actor自動スポーンとPlacement Anchor APIを廃止しました。Interior Actorは選択したPlacement Area内の壁際床候補へ配置され、明示的なInterior Locationスポーンと部屋Vegetationは引き続き利用できます。
- Context Tagの提供側APIを`GetProvidedContextTags()`と`ProvidedContextTags`へ改名し、`RequiredContextTags`との関係を明確にしました。旧`GetInquireInteriorTags()` APIと`InquireInteriorTags`プロパティはリダイレクトなしで削除しました。
- 生成通路でInterior Actorの配置候補が登録されない問題を修正し、Interior Actorと植生へ独立したオプトインの`Slope`配置エリアを追加しました。v2では`Aisle`と`Slope`を別々の配置エリアとして設定します。
- `ADungeonRoomSensorBase` に設定可能な影なしのベース補助 Point Light とドア・階段用の誘導 Spot Light を追加しました。それぞれの明るさの単位を Unitless、Candelas、Lumens から個別に選択でき、物理ベースの逆二乗減衰を使う Lumens がデフォルトです。
- ドア用誘導光は既定でドア天面から半垂直グリッド上に配置し、ドア直上が室内の `Floor` 空間でない場合は生成しないようにしました。
- 階層別ミニマップが、選択階層から次階層の直前までの高さ帯全体を含むようになりました。最上階はダンジョンのボクセル領域上端まで表示されます。
- ミニマップの階層選択が3D階層デバッグ面と同じワールド高さで切り替わるようになり、Characterの足元とDungeon ActorのZ原点を使用するようになりました。
- 接続する部屋の高さが異なるとき、通路のPurposeが`VerticalTransition`へ上書きされる問題を修正しました。垂直レイアウトではBranch、Loop、Shortcutの情報がどこにも残らず、レイアウト評価が冗長なループと本流を区別できていませんでした。階層をまたぐかどうかは`Aisle::IsVerticalTransition()`が独立した属性として返します。
- 通路のボクセル生成の失敗報告を一元化しました。ゴール側の門検索の失敗はログもエラーコードも残さなかったため、通路が欠けたダンジョンが生成成功として返っていました。
- サブレベルやサイズを予約した開始部屋・ゴール部屋など、サイズを変更できない部屋の門の外側のグリッドを確保するようにしました。他の通路の階段が、門を置ける唯一の場所を塞がなくなります。
- 専用の門を必要とするのは施錠される通路だけになりました。それ以外の通路は門を共有できるため、Keys And Locksとサイズ固定の小さな部屋を組み合わせたときの門の枯渇が解消されます。
- 交差点の経路探索が、既存の門を再利用せず新しい門を作ろうとする問題を修正しました。優先度の判定が反転していました。
- Keys And Locks進行でも`Path.ExtraCorridorComplexity`が有効になりました。鍵の配置が交差点生成と同じフラグを共有していたため、黙って0に強制されていました。施錠される通路は鍵付き扉を迂回されないよう、専用の門と廊下を保持します。

## 20260606-1.9.2 (66)
### Changes
- Fixed an issue with the plant spawn range
- Fixed an issue where actors would disappear when spawned from the editor
- Fixed several other bugs.
### 変更点
- 植物の生成範囲の間違いを修正
- エディタからの生成でアクターが消えてしまう問題を修正
- いくつかの不具合を修正。

## 20260420-1.9.1 (65)
### Changes
- Added a tool to fit StaticMeshes to DungeonGenerateParameters.
- Fixed several other bugs.
### 変更点
- StaticMesh を DungeonGenerateParameter にあわせてフィットさせるツールを追加。
- いくつかの不具合を修正。

## 20260309-1.9.0 (64)
### Changes
- Made corridor ceiling height configurable.
- Improved customization for selecting parts and meshes.
- Added chandelier to DungeonMeshSetDatabase.
- Added support for creating atrium (open vertical space) sections in rooms.
- Fixed an issue where vegetation would never spawn when the probability was extremely low.
- Experimental support for generating levels with the loaded level as the starting room.
- Fixed several other bugs.
### 変更点
- 通路の天井の高さを変更可能に。
- パーツとメッシュの選択をカスタム可能に。
- シャンデリアを DungeonMeshSetDatabase に追加。
- 部屋に吹き抜け部分を追加。
- 植生の抽選確率が低すぎる時に抽選されなくなる問題を修正。
- ロード済みレベルをスタート部屋とする生成に試験的な対応。
- いくつかの不具合を修正。

## 20251225-1.8.0 (63)
### Changes
* Added the ability to specify the direction in which dungeon rooms unfold.
* Added options for starting positions.
* Added a notification system for communicating breaking changes.
* Fixed several bugs
### 変更点
* ダンジョンの部屋の展開方向の指定を追加
* スタート位置の選択肢を追加
* 破壊的な変更を連絡するための通知システムを追加
* いくつかの不具合を修正

## 20251122-1.7.9 (62)
### Changes
* Unreal Engine 5.7 support
* Revised plant distribution methods
* Fixed several bugs
### 変更点
* Unreal Engine 5.7対応
* 植物の分布方法を修正
* いくつかの不具合を修正

## 20251004-1.7.8 (61)
### Changes
* Added the ability to assist actors spawning into the aisle grid
* Changed DungeonRoomSensor to not spawn on the client side
* Fixed several bugs
### 変更点
* 通路グリッドへのアクターをスポーンを補助する機能を追加
* DungeonRoomSensorをクライアント側でスポーンしないように変更
* いくつかの不具合を修正

## 20250903-1.7.7 (60)
### Changes
* Add DungeonRoomSensorDatabase
* Fixed several bugs
### 変更点
* DungeonRoomSensorDatabaseを追加
* いくつかの不具合を修正

## 20250903-1.7.6 (59)
### Changes
* Removed access to editor functions while in standalone mode.
* Fixed several bugs
### 変更点
* スタンドアローンモード中にエディタ機能へアクセスしていたので削除
* いくつかの不具合を修正

## 20250831-1.7.5 (58)
### Changes
* Enable/disable control of shadow generation in point light derived classes changed from per-partition to per-light.
* Fixed a misjudgment of the viewing cone in the determination of active partition.
* Limit the transfer method of the minimap to only the area of the change.
* Added actors that regularly spawn actors.
* Fixed several bugs
### 変更点
* ポイントライト派生クラスの影の生成の有効無効制御をパーティエーション単位からライト単位に変更
* アクティブパーティエーションの判定で視錐台の判定ミスを修正
* ミニマップの転送方法を変更範囲のみに限定
* アクターを定期的にスポーンするアクターを追加
* いくつかの不具合を修正

## 202500809-1.7.4 (57)
### Changes
* Fixed several bugs
### 変更点
* いくつかの不具合を修正

## 202500807-1.7.3 (56)
### Changes
* Added widget class to assist in creating mini-maps
* Adjusted the effective range of point light and spot light shadows
* Fixed several bugs
### 変更点
* ミニマップの作成を補助するウィジットクラスを追加
* ポイントライト、スポットライトの影の有効範囲を調整
* いくつかの不具合を修正

## 20250722-1.7.2 (55)
### Changes
* Fixed an issue where point light and spot light shadows were not enabled
* Fixed plug-in asset reference errors
* Fixed several bugs
### 変更点
* ポイントライト、スポットライトの影が有効にならない問題を修正
* プラグインアセットの参照エラーを修正
* いくつかの不具合を修正

## 20250718-1.7.1 (54)
### Changes
* Modified sensor properties of DungeonRoomSensor to be editable.
* Fixed plug-in asset reference errors
* Fixed several bugs
### 変更点
* DungeonRoomSensorのセンサーのプロパティを変更可能に修正
* プラグインアセットの参照エラーを修正
* いくつかの不具合を修正

## 20250622-1.7.0 (53)
### Changes
* Fixed a hang issue with editor generation
* Fixed a problem that prevented saving when updating the sublevel database with the sublevel target level open.
* Fixed an issue where the minimap could not be continuously saved to texture assets
* Fixed several bugs
### 変更点
* エディタ生成でハングアップする問題を修正
* サブレベル対象レベルを開いた状態でサブレベルデータベースを更新すると保存ができなくなる問題を修正
* ミニマップをテクスチャアセットに連続で保存できない問題を修正
* いくつかの不具合を修正

## 20250619-1.6.23 (52)
### Changes
* Modified to switch between wall, ceiling, and column component types
* Revised culling distances
* Cleaned up assets by adding comments and removing incorrect nodes in BPs
* Fixed several bugs
### 変更点
* 壁、天井、柱のコンポーネントの種類を切り替えられるように変更
* カリング距離を見直し
* BP内のコメント追加や不正ノードの除去などアセットを整理
* いくつかの不具合を修正

## 20250607-1.6.22 (51)
### Changes
* Unreal Engine 5.6 support
* Moved DungeonRoomSensor's Initialize timing to after interior creation
* Changed the drawing method of RandomTransform in DungeonRoomSensor
* Fixed some bugs
### 変更点
* Unreal Engine 5.6対応
* DungeonRoomSensorのInitializeタイミングを内装生成後に移動
* DungeonRoomSensorのRandomTransformの抽選方法を変更
* いくつかの不具合を修正

## 20250603-1.6.21 (50)
### Changes
* Changed to run Build for interiors and sublevels before Cook
* Fixed problem with interior stair landings getting trapped
* Reduced problem with enemies being buried
* Fixed several bugs
### 変更点
* Cook前にインテリアとサブレベルのBuildを実行するように変更
* 室内の階段の踊り場が閉じ込められる問題を修正
* 敵が埋没する問題を軽減
* いくつかの不具合を修正

## 20250601-1.6.20 (49)
### Changes
* Discontinued support for DungeonAisleMeshSet and DungeonRoomMeshSet
* Fixed a bug that caused sublevels to be generated on top of each other
* Fixed a bug in which crossing decisions were being made without sublevels being released.
* Added option to generate structural columns in rooms
* Fixed several bugs
### 変更点
* DungeonAisleMeshSetとDungeonRoomMeshSetのサポートを終了
* サブレベルが重なって生成される不具合を修正
* サブレベルが解放されないまま交差判定が行われていた不具合を修正
* 部屋の中に構造柱の生成オプションを追加
* いくつかの不具合を修正

## 20250519-1.6.19 (48)
### Changes
* Added minimap mask textures
* changed sublevel size and grid size to be set by DungeonSubLevelScriptActor
* Fixed several bugs
### 変更点
* ミニマップマスクテクスチャを追加
* サブレベルのサイズとグリッドサイズをDungeonSubLevelScriptActorで設定するように変更
* いくつかの不具合を修正

## 20150315-1.6.18 (47)
### Changes
* Added ability to disable CastShadow for point lights and spotlights on candelabras above the second floor of a room
* Added ability to draw rectangles in the mask texture of the minimap
* added support for actor spawning on the aisle grid
* Extended the ability to specify the number of winning sublevels for random draws
* Fixed several bugs
### 変更点
* 部屋の二階以上の燭台のポイントライト、スポットライトのCastShadowを無効化できるよう追加
* ミニマップのマスクテクスチャに矩形を描画できるように追加
* 通路グリッドへのアクタースポーンに対応
* ランダム抽選するサブレベルの当選回数を指定できるように拡張
* いくつかの不具合を修正

## 20250301-1.6.17 (46)
### Changes
* Added DungeonMeshSet draw method
* Changed DungeonMainLevelScriptActor load control availability to be configurable
* Changed navigation generation and culling distance for Instanced Static Mesh
* Added CastShadow validity control if point light and spot light are not Static
* Added mask texture for minimap
* Changed interiors to be generated for actors placed on sublevels
* modified plugin assets
* Fixed several bugs
### 変更点
* DungeonMeshSetの抽選方法を追加
* DungeonMainLevelScriptActorの負荷制御の有効性を設定可能に変更
* Instanced Static Meshのナビゲーション生成とカリング距離を変更
* ポイントライトとスポットライトがStatic以外ならCastShadowの有効性制御を追加
* ミニマップのマスクテクスチャを追加
* サブレベルに配置されたアクターにも内装を生成するように変更
* プラグインアセットを修正
* いくつかの不具合を修正

## 20250130-1.6.16 (45)
### Changes
* Modified load reduction algorithm
* Fixed several bugs
### 変更点
* 負荷軽減アルゴリズムを変更
* いくつかの不具合を修正

## 20250122-1.6.15 (44)
### Changes
* Fixed a problem where a gate was not formed when a staircase connected to a room.
* Changed the order in which vegetation is generated to after interiors
* Fixed several bugs
### 変更点
* 階段と部屋がつながった時に門ができていない不具合の修正
* 植生の生成順序をインテリアよりも後に変更
* いくつかの不具合を修正

## 20250108-1.6.14 (43)
### Changes
* Fixed problem with incorrect distance from starting room
* Fixed several bugs
### 変更点
* スタート部屋からの距離が不正になる問題を修正
* いくつかの不具合を修正

## 20241218-1.6.13 (42)
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
