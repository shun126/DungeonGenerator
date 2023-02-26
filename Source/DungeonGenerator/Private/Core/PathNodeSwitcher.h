/*
パス検索ノードの予約・使用中切り替え

\author		Shun Moriya
\copyright	2023- Shun Moriya
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
		PathNodeSwitcher() = default;

		virtual ~PathNodeSwitcher() = default;

		void Reserve(const uint64_t parentHashey, const std::shared_ptr<Node>& node);

		void Use(const uint64_t parentHash);

		void Revert(const uint64_t parentHash);

		bool IsUsing(const uint64_t key) const;

		void Clear();

	private:
		std::unordered_map<uint64_t, std::shared_ptr<Node>> mReserved;
		std::unordered_map<uint64_t, std::shared_ptr<Node>> mUsed;
	};
}

#include "PathNodeSwitcher.inl"
