# FDungeonRandomActorParts リファレンス

`FDungeonRandomActorParts`は、Actor候補の向き、オフセット、分かりやすい百分率の生成確率をまとめる設定です。

## 主なプロパティ

- `Spawn Chance`（`float`、0%～100%）：1回の選択でこのActorが候補に参加する確率です。`0%`では候補にならず、`100%`では必ず候補になります。

Spawn ChanceはSelection Weightではありません。Spawn Chanceは候補へ参加するかを決め、重み付きSelectorは参加した候補の選ばれやすさを決めます。

## 編集のヒント

- 珍しい小物を通常の装飾へ混ぜる場合は、Spawn Chanceを低くします。
- 継承したTransform設定でActorの前方向と配置オフセットを合わせます。
