/*
パス検索ノードの予約・使用中切り替え
主に階段の空間に進入させない為に使用しています。

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <algorithm>
#include <unordered_map>

namespace dungeon
{
	/*
	パス検索ノードの予約・使用中切り替えクラス
	主に階段の空間に進入させない為に使用しています。
	*/
	class PathNodeSwitcher final
	{
	public:
		class Node final
		{
		public:
			Node() = default;

			Node(const uint64_t node0, const uint64_t node1)
			{
				mNodes[0] = node0;
				mNodes[1] = node1;
			}

			Node(const Node& other)
				: mNodes(other.mNodes)
			{
			}

			Node(Node&& other) noexcept
				: mNodes(std::move(other.mNodes))
			{
			}

			Node& operator=(const Node& other)
			{
				mNodes = other.mNodes;
				return *this;
			}

			Node& operator=(Node&& other) noexcept
			{
				mNodes = std::move(other.mNodes);
				return *this;
			}

			~Node() = default;

			bool Contain(const uint64_t key) const
			{
				for(const auto node : mNodes)
				{
					if (node == key)
						return true;
				}
				return false;
			}

		private:
			std::array<uint64_t, 2> mNodes;
		};

	public:
		/*
		コンストラクタ
		*/
		PathNodeSwitcher() = default;

		/*
		デストラクタ
		*/
		~PathNodeSwitcher() = default;

		/*
		ノードの予約
		*/
		void Reserve(const uint64_t parentHash, const Node& node)
		{
			mReserved[parentHash] = node;
		}

		/*
		予約したノードを使用中に変更します
		*/
		void Use(const uint64_t parentHash)
		{
			auto iterator = mReserved.find(parentHash);
			if (iterator != mReserved.end())
			{
				mUsed[parentHash] = iterator->second;
				mReserved.erase(iterator);
			}
		}

		/*
		使用中のノードを予約中に変更します
		*/
		void Revert(const uint64_t parentHash)
		{
			auto iterator = mUsed.find(parentHash);
			if (iterator != mUsed.end())
			{
				mReserved[parentHash] = iterator->second;
				mUsed.erase(iterator);
			}
		}

		/*
		キーが使用中か調べます
		*/
		bool IsUsing(const uint64_t key) const
		{
			return std::any_of(mUsed.begin(), mUsed.end(), [key](const std::pair<uint64_t, Node>& node)
				{
					return node.second.Contain(key);
				}
			);
		}

		/*
		ノードを全てクリアします
		*/
		void Clear()
		{
			mReserved.clear();
			mUsed.clear();
		}

	private:
		std::unordered_map<uint64_t, Node> mReserved;
		std::unordered_map<uint64_t, Node> mUsed;
	};
}
