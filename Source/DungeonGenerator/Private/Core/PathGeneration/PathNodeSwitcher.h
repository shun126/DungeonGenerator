/*
パス検索ノードの予約・使用中切り替え

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/IntVector.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace dungeon
{
	/*
	パス検索ノードの予約・使用中切り替えクラス
	*/
	class PathNodeSwitcher
	{
	public:
		class Node final
		{
		public:
			void Add(const uint64_t key);
			bool Contain(const uint64_t key) const;

		private:
			std::unordered_set<uint64_t> mNodes;
		};

	public:
		/*
		コンストラクタ
		*/
		PathNodeSwitcher() = default;

		/*
		デストラクタ
		*/
		virtual ~PathNodeSwitcher() = default;

		/*
		ノードの予約
		*/
		void Reserve(const uint64_t parentHashey, const std::shared_ptr<Node>& node);

		/*
		予約したノードを使用中に変更します
		*/
		void Use(const uint64_t parentHash);

		/*
		使用中のノードを予約中に変更します
		*/
		void Revert(const uint64_t parentHash);

		/*
		キーが使用中か調べます
		*/
		bool IsUsing(const uint64_t key) const;

		/*
		ノードを全てクリアします
		*/
		void Clear();

	private:
		std::unordered_map<uint64_t, std::shared_ptr<Node>> mReserved;
		std::unordered_map<uint64_t, std::shared_ptr<Node>> mUsed;
	};
}

#include "PathNodeSwitcher.inl"
