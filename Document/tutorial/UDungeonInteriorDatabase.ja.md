# UDungeonInteriorDatabase ガイド

`UDungeonInteriorDatabase`は、生成された部屋、平坦な通路、傾斜通路へ家具、装飾Actor、植生を配置するデータベースです。バージョン2では「どこに置くか」と「どのような意味の場所に置くか」を分離し、設定を理解・調整しやすくしています。

## Interior Parts

家具や装飾Actorには`Interior Parts`を使います。

- `Placement Areas`：`Room`、`Aisle`、`Slope`を必要に応じてチェックします。Interior Actorは生成された壁に沿う床上の候補へ配置され、部屋中央には自動配置されません。`Aisle`は平坦な通路、`Slope`は傾斜、階段室、上下の多層通路空間です。新規Partsでは`Slope`は無効です。
- `Priority`：大きい値から配置を試します。配置できなければ低いPriorityへ進みます。
- `Spawn Chance`：1回の配置試行で候補になる確率です。`0%`から`100%`で表示されます。
- `Required Context Tags`：Room SensorまたはInterior Locationから、ここに指定したタグがすべて渡された場合だけ候補になります。通常のArea配置では空にできますが、Interior Locationの入れ子スポーンには1つ以上の明示タグが必要です。

Priority、確率、Weightは役割が異なります。

- Priorityは配置を試す順番です。
- Spawn Chanceは候補に参加する確率です。
- Selection Weightは重み付き抽選での相対的な選ばれやすさです。

## Vegetation Parts

草、苔、ツタ、吊り下がる植物などには`Vegetation Parts`を使います。

- `Placement Areas`：`Room`、`Aisle`、`Slope`を必要に応じてチェックします。Gateの面と、Roomが所有するSlopeの生成済み壁・天井は`Room`を使用します。通路が所有するSlopeは`Slope`を使用します。新規Partsでは`Slope`は無効です。
- `Placement Surfaces`：`Floor`、`Wall`、`Ceiling`を複数選択できます。
- `Density`：有効な面ごとに、その面積を使って独立して計算します。
- `Max Spawn Count`：有効な面ごとに適用します。
- `Required Context Tags`：`kitchen`、`library`、`overgrown`など、意味のある場所だけに植生を限定できます。

Floorは従来どおり下向きに地形をTraceします。WallとCeilingは生成済みのGrid面を直接使用します。Gateの床、壁、天井は、所属する部屋の`Room`設定、Interior Database、Provided Context Tagsを使用します。Roomが所有するSlope、Stairwell、DownSpace、UpSpaceの生成済み壁・天井も同じRoom設定を使用し、床の挙動は変更しません。通路が所有するSlope系Gridは引き続き`Slope`を使用します。構造柱はFloorまたはDeckと隣接する壁面だけが対象です。壁・天井用の植生メッシュは接触点にPivotを置き、ローカル+Zが表面から伸びる向きになるよう作成してください。

## Context Tags

Provided Context Tagsは形状ではなく、場所の意味を表します。主な入力元は次の2つです。

- `ADungeonRoomSensorBase::GetProvidedContextTags`
- `UDungeonInteriorLocationComponent::ProvidedContextTags`

例えば`library`を必須にしたPartsは、現在の部屋または配置位置のProvided Context Tagsに`library`が含まれる場合だけ候補になります。Required Context Tagsはすべて提供される必要があります。Context TagsがPlacement AreaやSurfaceの代わりになることはありません。

Interior LocationはRequired Context Tagsが空のPartsを選択しません。入れ子ActorがさらにInterior Locationを持つ場合は既存の深度上限まで生成できますが、同じ分岐の祖先に存在するActor Classは除外されます。これにより`Shelf → Shelf`や`Shelf → Box → Shelf`の循環を防ぎます。別々の兄弟Locationでは同じ子Classを引き続き使用できます。

## Gameplay Roleと特別な部屋

Combat、Treasure、Puzzle、Rest、Boss、Secretなど、遊びの目的に合わせて内装全体を変える場合は`EDungeonRoomGameplayRole`でInterior Databaseを切り替えます。解決順序はGameplay Role、Zone、Default Themeです。

Interior PartsではHall、Hanare、Start、Goalによるフィルターを行いません。StartまたはGoalを作り込んだ固有レイアウトにしたい場合は、Start／Goal用サブレベルを使用してください。サブレベルなら部屋の形状を含めて確実に演出できます。

## 基本的な手順

1. 家具Actorを`Interior Parts`、植生Meshを`Vegetation Parts`へ登録します。
2. Interior ActorのArea、またはVegetationのAreaとSurfaceを選択します。
3. 意味による制限が必要な場合だけRequired Context Tagsを追加し、Room SensorまたはInterior Locationから提供します。
4. PriorityとSpawn Chanceを設定します。
5. Interior ActorのClassやBoundsを変更した後は`Build`を実行します。
6. Default Theme、Zone Override、Gameplay Role OverrideのいずれかへDatabaseを指定します。

## 関連ページ

- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)
