# v2に利用者にとって含まれていると良いと思う機能は何が考えられるか

現状の v2 は、方向性としてかなり良いです。  
特に `ProgressionPolicy`、`RoomRoles`、`Zones`、Interior、SubLevel、RoomSensor への整理は、v1より「レベルデザインを組み立てる道具」になっています。

その上で、利用者目線で最後に入っていると強いと思うのは、機能そのものより **失敗しにくくする補助機能** です。

**最優先**
1. **Parameter Health Check / Readiness Check**
   - `GenerateParameter` を選んで「この設定で生成できるか」「何が足りないか」を一覧表示する機能。
   - 例: Mesh DB未設定、GridSize不一致、SubLevelサイズ不一致、Interior build未実行、RoomRole overrideはあるがDB空、KeysAndLocksなのに鍵/ドア用パーツ不足。
   - これは初心者にも上級者にも効きます。v2は設定が豊かなので、診断機能があるだけで安心感がかなり上がります。

2. **v1 -> v2 Migration Report**
   - 自動移行後に「何をどこへ移したか」「移せなかったもの」「手動確認が必要なもの」を表示。
   - 特に v2.1 で移行サポート削除予定なら、v2.0.0でこれは強いです。
   - “Migration succeeded” だけでなく、`UseMissionGraph -> Path.ProgressionPolicy = KeysAndLocks` のような対応表が見えると納得しやすいです。

3. **Visual Debug Overlay**
   - 生成結果に `Start / Goal / MainPath / Branch / DeadEnd / Secret / Boss / Zone` を色分け表示。
   - v2の価値は「意図した体験を作れること」なので、意図通りになっているか見える機能は重要です。
   - ミニマップやエディタプレビューに重ねられるとかなり便利です。

**あると購入・導入判断に効く**
4. **Parameter Presets**
   - `Small Dungeon`, `Boss Route`, `Keys and Locks`, `Hub Quest`, `Secret-heavy`, `Vertical Dungeon` など。
   - 利用者は最初から正しいパラメータ設計を知らないので、完成形から調整できると成功体験が早いです。
   - 機能追加というよりサンプルアセットでも十分価値があります。

5. **Seed / Layout Comparison**
   - 同じ設定で複数seedを生成し、候補を並べて選ぶ機能。
   - 生成品質を売りにするなら、「良いレイアウトを選べる」体験は強いです。
   - 最低限、seed、部屋数、主経路長、分岐数、デッドエンド数、Zone比率を一覧できるだけでも便利です。

6. **“Why Wasn’t This Used?” Diagnostics**
   - Interior、SubLevel、RoomRole、Zone override が効かなかった理由を確認できる機能。
   - 例: tag不一致、floor range外、progress range外、allowed role不一致、bounds不足、grid size不一致。
   - v2は表現力が増えたぶん、「設定したのに出ない」が一番ストレスになりやすいです。

**Nice to Have**
7. **Gameplay Recipe Samples**
   - Combat room enemy spawn
   - Treasure room reward spawn
   - Secret room hidden reward
   - Boss room setup
   - Key / locked door flow
   - これらを RoomSensor Blueprint サンプルとして用意すると、v2のRoomRole設計が伝わりやすいです。

8. **Runtime Budget Warning**
   - `LayoutCandidateCount`、部屋数、Interior量、SubLevel数などから「ランタイム生成には重いかも」を警告。
   - 初心者がいきなり重い設定にして「プラグインが遅い」と感じる事故を減らせます。

私なら、最後に1つだけ入れるなら **Parameter Health Check** を選びます。  
v2は機能が多く、表現力も高いので、「何が悪いか分かる」「次に何を直せばいいか分かる」ことが、利用者にとって一番ありがたいはずです。
