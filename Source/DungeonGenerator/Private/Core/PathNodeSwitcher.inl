/*
パス検索ノードの予約・使用中切り替え

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "PathFinder.h"
#include "PathNodeSwitcher.h"

namespace dungeon
{
	inline void PathNodeSwitcher::Node::Add(const uint64_t key)
	{
		mNodes.emplace(key);
	}

	inline bool PathNodeSwitcher::Node::Contain(const uint64_t key) const
	{
		return mNodes.find(key) != mNodes.end();
	}



	inline void PathNodeSwitcher::Reserve(const uint64_t parentHash, const std::shared_ptr<Node>& node)
	{
		mReserved[parentHash] = node;
	}
   
	inline void PathNodeSwitcher::Use(const uint64_t parentHash)
	{
		auto iterator = mReserved.find(parentHash);
		if (iterator != mReserved.end())
		{
			mUsed[parentHash] = iterator->second;
			mReserved.erase(iterator);
		}
	}

	inline void PathNodeSwitcher::Revert(const uint64_t parentHash)
	{
		auto iterator = mUsed.find(parentHash);
		if (iterator != mUsed.end())
		{
			mReserved[parentHash] = iterator->second;
			mUsed.erase(iterator);
		}
	}

	inline bool PathNodeSwitcher::IsUsing(const uint64_t key) const
	{
		for (const auto& node : mUsed)
		{
			if (node.second->Contain(key))
				return true;
		}
		return false;
	}

	inline void PathNodeSwitcher::Clear()
	{
		mReserved.clear();
		mUsed.clear();
	}
}
