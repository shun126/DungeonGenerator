# UDungeonGenerateParameter 説明書

UDungeonGenerateParameter は、ダンジョン生成に関する設定をまとめた UObject です。以下では、Blueprint から調整できる UPROPERTY 変数を中心に、**プラグイン利用者が実際に値を変える際に役立つ意図・初期値・編集条件**をまとめます。作りたいダンジョンの形やプレイフィールに合わせて、設定の意味と連動を押さえつつ調整してください。コードの行番号ではなく操作上の意味を優先し、非エンジニアでも読みやすい形でまとめています。

## 基本情報（再現性とデバッグの起点）
- **RandomSeed (`int32`)**: 生成に利用する乱数シード。テストの再現性が必要なときは固定値にし、0 なら毎回ランダム。`EditAnywhere` で `ClampMin=0`。
- **GeneratedRandomSeed (`int32`)**: 直近の生成で実際に使われたシード。プレイテストで「この形を再現したい」ときの控えとして参照する欄（編集不可）。
- **GeneratedDungeonCRC32 (`int32`)**: 直近で生成されたダンジョンの CRC32。バージョン違いでの差分チェックに利用可能（表示のみ）。

## 部屋サイズと間隔（探索感・視界の作り込み）
- **RoomWidth (`FInt32Interval`)**: 部屋の幅の最小/最大。狭くすると通路感、広くすると戦闘しやすい広場を作りやすい。UI で 1 以上に制限。
- **RoomDepth (`FInt32Interval`)**: 部屋の奥行きの最小/最大。縦長の部屋を増やしたい場合に上げる。UI で 1 以上に制限。
- **RoomHeight (`FInt32Interval`)**: 部屋の高さの最小/最大。立体戦を重視するなら上げる。UI で 1 以上に制限。
- **RoomMargin (`uint8`)**: 部屋間の水平方向の余白。細い通路や圧迫感を出したくないなら広めに、密度を上げたいなら狭めに設定。`MergeRooms` 無効時のみ編集可、最小 1。
- **VerticalRoomMargin (`uint8`)**: 部屋間の垂直方向の余白。多層ダンジョンで上下の重なりを抑える際に利用。`MergeRooms` と `Flat` が無効のときのみ編集可、最小 0。

## 部屋・階層の候補数と配置方針（マップ形状の大枠）
- **NumberOfCandidateRooms (`uint8`)**: 生成の初期候補となる部屋数。値を増やすと部屋同士の試行が増え、生成時間は伸びるがバリエーションも増える。3〜100。
- **MergeRooms (`bool`)**: 隣接部屋を結合するか。広い部屋が欲しいときに有効だが、結合をオンにすると Margin や ExpansionPolicy が編集不可になる点に注意。
- **ExpansionPolicy (`EDungeonExpansionPolicy`)**: 部屋を横方向・縦方向・任意方向に広げる方針。立体迷路にしたいなら縦方向を含む設定にする。`MergeRooms` と `Flat` が無効時のみ編集可。
- **NumberOfCandidateFloors (`uint8`)**: 生成参考用の階層候補数。多層化の試行回数を決めるため、立体構造を増やしたい場合に上げる。0〜5 かつ `Flat` 無効時のみ編集可。
- **Flat (`bool`)**: 平面的（単層）ダンジョンを生成するか。縦方向を不要にしたいサンプルやトップダウン視点に便利。オンにすると階層関連や垂直マージンが抑制される。

## プレイヤー開始位置とミッション（ゲーム体験の骨格）
- **MovePlayerStartToStartingPoint (`bool`)**: ゲーム開始時に PlayerStart をスタート部屋へ自動移動。ハンドプレース不要で、生成後すぐ試遊したいときに便利。
- **StartLocationPolicy (`EDungeonStartLocationPolicy`)**: 開始部屋の選定方針（南端・最高点・中央付近・マルチスタートなど）。序盤の導線を決める重要な設定で、縦構造を活かした始まり方を演出できる。`UseCentralPoint` と `UseMultiStart` は `UseMissionGraph` が無効な場合のみ利用可能。`UseMultiStart` の場合は `PlayerStart` の数だけスタート部屋が生成される。
- **UseMissionGraph (`bool`)**: 鍵付きミッション生成を有効化。シンプルな探索を望むときはオフ、鍵探しやルート制御をしたいときはオン。`MergeRooms` 無効かつ `AisleComplexity` が 0 のときに安定しやすい（コメントより）。
- **AisleComplexity (`uint8`)**: 通路の複雑さ。迷路感を増やしたいなら上げる。`MergeRooms` と `UseMissionGraph` が無効時のみ編集可、0〜10。`UseMissionGraph` が有効な場合は自動的に 0 として扱われる（Getter 実装より）。

## 部屋内構造の有無（レイアウトの個性付け）
- **GenerateSlopeInRoom (`bool`)**: 部屋内にスロープを生成するか。上下移動のアクセントや視界の抜けを作りたいときに有効（将来強制有効化の可能性あり）。
- **GenerateStructuralColumn (`bool`)**: 部屋内に構造柱を生成するか。遮蔽やシルエットの多様化を狙うときにオン（将来強制有効化の可能性あり）。

## グリッドサイズ（パーツ・移動の基準）
- **GridSize (`float`)**: 水平方向のボクセルサイズ。メッシュスナップや移動量の基準になるので、利用するメッシュ寸法に合わせて設定。`ClampMin=1`。
- **VerticalGridSize (`float`)**: 垂直方向のボクセルサイズ。ジャンプ力や階段勾配に合う値に合わせるとプレイ感が整う。`ClampMin=1`。

## メッシュパーツ設定（美術テイストと密度）
- **DungeonRoomMeshPartsDatabase** ([UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.ja.md)): 部屋用メッシュパーツ DB。アートテーマをまとめて切り替えたい場合はここでセット。
- **DungeonAisleMeshPartsDatabase** ([UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.ja.md)): 通路用メッシュパーツ DB。部屋とテイストを変えたい場合に別セットを指定。
- **PillarPartsSelectionMethod** (`EDungeonPartsSelectionMethod`): 柱パーツの選択方法（ランダム等）。柱の出現に偏りを持たせたいときに調整。
- **PillarParts** (TArray<[FDungeonMeshParts](./FDungeonMeshParts.ja.md)>): 利用する柱メッシュ群。シルエットのバリエーションやゲームプレイ上の遮蔽を変えたいときに差し替える。

### メッシュパーツやデータベースの詳細
- **[FDungeonMeshParts](./FDungeonMeshParts.ja.md)**: 単一のスタティックメッシュを登録する入れ物。メッシュとオフセット（`FDungeonPartsTransform` 継承）をセットするだけなので、アートチームが「この柱メッシュをこの位置・向きで置いてほしい」といった要望を反映しやすい。
- **[UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.ja.md)**: 床・壁・天井などのメッシュセットをまとめて管理するデータベース。選択方法（深さベース／ランダム等）を指定でき、グリッド上での部屋や通路ごとに自動抽選される。アートテーマを一括で切り替えたいときはここを差し替える。
- **[FDungeonRandomActorParts](./FDungeonRandomActorParts.ja.md)**: メッシュではなくアクター（例: 燭台の Blueprint）を確率付きで登録するパーツ。`Frequency` を 0.0〜1.0 で設定して出現率を調整できるため、「たまにレアな装飾が出る」演出に向く。
- **[FDungeonDoorActorParts](./FDungeonDoorActorParts.ja.md)**: ドア用のアクターパーツ。`DungeonDoorBase` を継承した Blueprint クラスを指定し、位置オフセットも付けられる。鍵付きドアや見た目違いを複数用意する場合に活用。
- **TorchPartsSelectionMethod (`EDungeonPartsSelectionMethod`)**: 燭台パーツの選択方法。壁沿いの装飾や視認性をコントロールしたい場合に調整。
- **FrequencyOfTorchlightGeneration (`EFrequencyOfGeneration`)**: 燭台生成頻度。暗さを活かしたいなら低く、明るいダンジョンにしたいなら高く。初期値は `Rarely`。
- **TorchParts (`TArray<[FDungeonRandomActorParts](./FDungeonRandomActorParts.ja.md)>`)**: 燭台（柱灯）のメッシュ/アクターパーツ群。炎の色や形で雰囲気を変えるときに編集。
- **DoorPartsSelectionMethod (`EDungeonPartsSelectionMethod`)**: ドアパーツの選択方法。重要ルームへのドアを特徴付けたい場合に有効。
- **DoorParts** (TArray<[FDungeonDoorActorParts](./FDungeonDoorActorParts.ja.md)>): ドアのパーツ群。鍵付きドアや見た目違いを混ぜたいときに差し替え。

## プロダクト限定データベース（`PRODUCT_ONLY`）（演出・レベル構成の仕上げ）
- **DungeonInteriorDatabase** ([UDungeonInteriorDatabase](./UDungeonInteriorDatabase.ja.md)): インテリア配置用 DB。家具・装飾のテーマをまとめて差し替える際に使用。
- **DungeonSubLevelDatabase** ([UDungeonSubLevelDatabase](./UDungeonSubLevelDatabase.ja.md)): スタート/ゴールやサブレベル指定用 DB。特定シーケンスやイベント用レベルを混ぜ込むときに指定。

## ルームセンサー関連（トリガーと演出）
- **DungeonRoomSensorClass (`UClass*`)**: ルームセンサーのクラス指定。`DeprecatedProperty` のため、基本は Database を使い、互換目的でのみ参照。
- **DungeonRoomSensorDatabase** ([UDungeonRoomSensorDatabase](./UDungeonRoomSensorDatabase.ja.md))**: ルームセンサーの配置・演出を管理する DB。BGM やバフ、スポーン制御など部屋ごとの演出を統一的に設定可能。

### プロダクト限定データベースの使いどころ
- [**UDungeonInteriorDatabase**](./UDungeonInteriorDatabase.ja.md): 家具や植生などの「内装プリセット」をタグで管理し、Build ボタンでボクセル配置データを再生成するアセット。テーマ単位で一括切り替えできるので、シーン別の雰囲気を短時間で試せる。
- [**UDungeonSubLevelDatabase**](./UDungeonSubLevelDatabase.ja.md): スタート/ゴール部屋や特定イベント用のサブレベルを登録するアセット。GridSize が DungeonGenerateParameter と一致している必要がある。Start/Goal 用ルームを直指定するほか、抽選上限付きでランダムサブレベルを混ぜ込めるため、「たまに出る隠し部屋」や「固定演出のボス部屋」を実現しやすい。
- **[UDungeonRoomSensorDatabase](./UDungeonRoomSensorDatabase.ja.md)**: 部屋侵入センサー（`DungeonRoomSensorBase` 継承）を条件付きで抽選・配置するアセット。深さや識別子に応じてセンサー種類を変えられ、生成完了時イベントで通路に追加アクターをスポーンする仕組みも含む。全体の演出・BGM 切替・トラップ配置をまとめてコントロールする用途に最適。

## 付加情報（トラブルシュート）
- **PluginVersion (`uint8`)**: プラグインバージョン。サポート問い合わせ時や異なる環境間での再現確認に利用。`BlueprintReadOnly` で表示のみ。

### 編集条件の注意点
- `MergeRooms` を有効にすると `RoomMargin` や `VerticalRoomMargin`、`ExpansionPolicy` が編集不可。
- `Flat` を有効にすると階層関連（`NumberOfCandidateFloors`）や垂直マージンが編集不可。
- `UseMissionGraph` を有効にした場合、`GetAisleComplexity` が 0 を返すため複雑度設定は実質無効化されます。

このドキュメントを参照することで、Blueprint からダンジョン生成パラメータを調整する際の意図と制約を把握できます。
