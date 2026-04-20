# ADungeonRoomSensorBase ガイド

`ADungeonRoomSensorBase` は、**部屋への侵入・離脱・初期化・後始末を扱う部屋イベント用の基底クラス**です。  
トラップ、BGM 切り替え、敵スポーン、部屋ごとの内装タグ付けなどを作りたいときは、このクラスを継承した Blueprint を作成します。

## 使い方の流れ
1. `ADungeonRoomSensorBase` を親クラスにした Blueprint を作ります。
2. 必要なイベントを実装します。
3. 作成した Blueprint を [UDungeonRoomSensorDatabase.ja.md](./UDungeonRoomSensorDatabase.ja.md) に登録します。
4. `UDungeonGenerateParameter` からその Database を指定します。

## まず触ることが多いイベント
- `OnPrepare`  
  スポーン直後の準備です。条件が合わなければ `false` を返して配置をやめられます。
- `OnInitialize`  
  配置後の本処理です。部屋種別、アイテム種別、深さ情報を受け取れます。
- `OnFinalize`  
  破棄前の後始末です。
- `OnReset`  
  プレイヤーが部屋を離れたあとに状態を戻したいときに使います。
- `OnResume`  
  再度プレイヤーが入ってきたときの再開処理です。

## 便利なプロパティ
- `Bounding` / `HorizontalMargin` / `VerticalMargin`  
  センサー範囲の調整です。
- `AutoReset`  
  プレイヤーが離れたら自動で `OnReset` を呼ぶかどうかです。
- `DoorAddingProbability`  
  その部屋のドア追加確率です。
- `SpawnActors`  
  部屋に追加スポーンする Blueprint 群です。
- `SpawnKeyActor` / `SpawnUniqueKeyActor`  
  MissionGraph 用の鍵アクターです。

## 内装と連携する
`GetInquireInteriorTags` を実装すると、その部屋に対して内装タグを返せます。  
たとえば `kitchen` や `library` のようなタグを返し、[UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md) 側で同じタグを持つ家具を出し分けます。

## ネットワーク利用時の注意
- このアクターはレプリケーション前提ではありません。
- 同期が必要な乱数を使う場合は、サーバーとクライアントで同じ回数だけ呼び出す必要があります。
- 迷ったら、独自の重い処理や非同期処理は避け、まずはローカル専用の演出から始めるのが安全です。

## 誤解しやすい点
- `SpawnActorInAisle` は `ADungeonRoomSensorBase` ではなく、[UDungeonRoomSensorDatabase.ja.md](./UDungeonRoomSensorDatabase.ja.md) 側の設定です。
- Database は「どのセンサーを使うか」を決める場所で、部屋の中で何をするかはこのクラス派生 Blueprint に書きます。

## 次に読む
- [UDungeonRoomSensorDatabase.ja.md](./UDungeonRoomSensorDatabase.ja.md)  
  このクラスをどの部屋で使うかを割り当てる設定先です。
- [UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md)  
  センサーから参照する内装タグの流れを確認できます。

## 関連ページ
- [UDungeonRoomSensorDatabase.ja.md](./UDungeonRoomSensorDatabase.ja.md)
- [UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md)
