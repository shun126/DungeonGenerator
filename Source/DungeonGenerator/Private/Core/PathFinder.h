/**
A*によるパス検索 ヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Direction.h"
#include "PathNodeSwitcher.h"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dungeon
{
	/**
	A*によるパス検索クラス
	*/
	class PathFinder
	{
	public:
		/**
		検索可能な方向
		Direction::Indexと並びを合わせて下さい
		*/
		enum class SearchDirection : uint8_t
		{
			North = 0,
			East,
			South,
			West,
			Any
		};

		/**
		検索ノードの種類
		*/
		enum class NodeType : uint8_t
		{
			Gate,			//!< 門
			Space,			//!< 空間
			Aisle,			//!< 通路
			Downstairs,		//!< 下階段
			Upstairs,		//!< 上階段
		};

		/*
		TODO:コード整理をしてください
		*/
		struct UseNode final
		{
			std::unordered_set<uint64_t> mNodes;

			void Add(const FIntVector& location)
			{
				mNodes.emplace(PathFinder::Hash(location));
			}

			bool Contain(const FIntVector& location) const
			{
				return mNodes.find(PathFinder::Hash(location)) != mNodes.end();
			}
		};

	private:
		/**
		基底ノード
		*/
		struct BaseNode
		{
			/**
			コンストラクタ
			\param[in]	nodeType		ノードの種類
			\param[in]	location		現在位置
			\param[in]	direction		検索してきた方向
			*/
			BaseNode(const NodeType nodeType, const FIntVector& location, const Direction direction) noexcept;

			/**
			コピーコンストラクタ
			*/
			BaseNode(const BaseNode& other) noexcept;

			/**
			ムーブコンストラクタ
			*/
			BaseNode(BaseNode&& other) noexcept;

			FIntVector mLocation;
			NodeType mNodeType;
			Direction mDirection;
		};

		/**
		Openノード
		*/
		struct OpenNode : public BaseNode
		{
			/**
			コンストラクタ
			\param[in]	parentKey		親ノードのキー
			\param[in]	nodeType		ノードの種類
			\param[in]	location		現在位置
			\param[in]	direction		検索してきた方向
			\param[in]	searchDirection	検索可能な方向
			\param[in]	cost			現在コスト
			*/
			OpenNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const SearchDirection searchDirection, const uint32_t cost) noexcept;

			/**
			コピーコンストラクタ
			*/
			explicit OpenNode(const OpenNode& other) noexcept;

			/**
			ムーブコンストラクタ
			*/
			OpenNode(OpenNode&& other) noexcept;

			uint64_t mParentKey;
			uint32_t mCost;
			SearchDirection mSearchDirection;
		};

		/**
		CloseNodeノード
		*/
		struct CloseNode : public BaseNode
		{
			/**
			コンストラクタ
			\param[in]	parentKey		親ノードのキー
			\param[in]	nodeType		ノードの種類
			\param[in]	location		現在位置
			\param[in]	direction		検索してきた方向
			*/
			CloseNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const uint32_t cost) noexcept;

			/**
			コピーコンストラクタ
			*/
			explicit CloseNode(const CloseNode& other) noexcept;

			/**
			ムーブコンストラクタ
			*/
			CloseNode(CloseNode&& other) noexcept;

			uint64_t mParentKey;
			uint32_t mCost;
		};

	public:
		/**
		ノードを開く
		\param[in]	location		現在位置
		\param[in]	goal			ゴール位置
		\param[in]	searchDirection	検索可能な方向
		\return		自身のキー
		*/
		uint64_t Start(const FIntVector& location, const FIntVector& goal, const SearchDirection searchDirection) noexcept;

		/**
		ノードを開く
		\param[in]	parentKey		親ノードのキー
		\param[in]	nodeType		ノードの種類
		\param[in]	cost			現在コスト
		\param[in]	location		現在位置
		\param[in]	goal			ゴール位置
		\param[in]	direction		検索してきた方向
		\param[in]	searchDirection	検索可能な方向
		\return		自身のキー
		*/
		uint64_t Open(const uint64_t parentKey, const NodeType nodeType, const uint32_t cost, const FIntVector& location, const FIntVector& goal, const Direction direction, const SearchDirection searchDirection) noexcept;

		/**
		Pop可能なノードがあるか調べます
		\return		trueならば空
		*/
		bool Empty() const noexcept;

		/**
		最も有望な位置を取得します
		\param[out]	nextKey				次に開く事ができるノードのキー
		\param[out]	nextNodeType		次のノードの種類
		\param[out]	nextCost			次のコスト
		\param[out]	nextLocation		次の位置
		\param[out]	nextDirection		次の方向
		\param[out]	nextSearchDirection	次に検索可能な方向
		\return		値が返っているならtrue
		*/
		bool Pop(uint64_t& nextKey, NodeType& nextNodeType, uint32_t& nextCost, FIntVector& nextLocation, Direction& nextDirection, SearchDirection& nextSearchDirection) noexcept;

		/**
		経路を確定する
		*/
		bool Commit(const FIntVector& goal) noexcept;

		/**
		経路が有効か調べます
		呼び出しはCommit後に行う必要があります
		\return	trueならば有効な経路
		*/
		bool IsValidPath() const noexcept;

		/**
		経路を取得します
		*/
		void Path(std::function<void(NodeType nodeType, const FIntVector& location, Direction direction)> func) const noexcept
		{
			for (auto node = mRoute.rbegin(); node != mRoute.rend(); ++node)
			{
				func(node->mNodeType, node->mLocation, node->mDirection);
			}
		}

		/**
		Direction::IndexをSearchDirectionに変換します
		\param[in]	direction	SearchDirection
		\return		Direction::Index
		*/
		static Direction Cast(const SearchDirection direction) noexcept;

		/**
		SearchDirectionをDirection::Indexに変換します
		\param[in]	direction	Direction::Index
		\return		Direction::Index
		*/
		static SearchDirection Cast(const Direction direction) noexcept;






		/*
		使用中ノードと関連するノードの予約をする
		\param[in]		location	
		\param[in]		std::shared_ptr<PathNodeSwitcher::Node>
		*/
		void ReserveOpenNode(const FIntVector& location, const std::shared_ptr<PathNodeSwitcher::Node>& openNode);

		/*
		ノードと関連ノードが使用中か調べます
		\param[in]		location	位置
		*/
		bool IsUsingOpenNode(const FIntVector& location) const;

	private:
		/*
		使用予約されたノードを使用中に変更する
		\param[in]		parentHash		ノードのハッシュ値
		*/
		void UseOpenNode(const uint64_t parentHash);

		/*
		使用中のノードを使用予約に戻す
		\param[in]		parentHash		ノードのハッシュ値
		*/
		void RevertOpenNode(const uint64_t parentHash);

		/*
		使用予約されたノードと使用中のノードをクリアする
		*/
		void ClearOpenNode();

	public:
		/**
		位置からハッシュキーを計算
		\param[in]	location	位置
		\return		ハッシュキー
		*/
		static uint64_t Hash(const FIntVector& location) noexcept;

	private:
		/**
		総コストを計算を取得
		\param[in]	cost		現在コスト
		\param[in]	location	現在位置
		\param[in]	goal		ゴール位置
		\return		総コスト
		*/
		static uint32_t TotalCost(const uint32_t cost, const FIntVector& location, const FIntVector& goal) noexcept;

		/**
		総コストを計算を取得
		\param[in]	cost		現在コスト
		\param[in]	heuristics	ヒューリスティック
		\return		総コスト
		*/
		static uint32_t TotalCost(const uint32_t cost, const uint32_t heuristics) noexcept;

		/**
		ヒューリスティックを取得
		\param[in]	location	現在位置
		\param[in]	goal		ゴール位置
		\return		ヒューリスティック
		*/
		static uint32_t Heuristics(const FIntVector& location, const FIntVector& goal) noexcept;

	private:
		PathNodeSwitcher mNoEntryNodeSwitcher;
		std::unordered_map<uint64_t, OpenNode> mOpen;
		std::unordered_map<uint64_t, CloseNode> mClose;
		std::vector<BaseNode> mRoute;
	};
}

#include "PathFinder.inl"
