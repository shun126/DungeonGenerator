/*
MissionGraph
ダンジョンの攻略情報を生成します

鍵と扉の生成アルゴリズム
1. ゴール部屋に接続する通路にゴール鍵を設定する
2. 通路に到達可能な部屋のどこかにゴール鍵を置く（別ブランチは抽選の優先度を上げる）
3. ゴール部屋よりも浅い深度の通路に鍵を設定する
4. 通路に到達可能な部屋のどこかに鍵を置く（別ブランチは抽選の優先度を上げる）
5. 深度1より探索深度が不快なら3に戻る

敵配置の生成アルゴリズム

部屋の装飾アルゴリズム

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <memory>

namespace dungeon
{
	class Aisle;
	class Generator;
	class Point;
	class Room;

	class MissionGraph
	{
	public:
		MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<const Point>& goal) noexcept;
		virtual ~MissionGraph() = default;

	private:
		void Generate(const std::shared_ptr<const Room>& room, const uint8_t count = 0) noexcept;
		Aisle* SelectAisle(const std::shared_ptr<const Room>& room) const noexcept;
		Aisle* SelectAisle(const std::shared_ptr<const Point>& point) const noexcept;

	private:
		std::shared_ptr<Generator> mGenerator;
	};
}
