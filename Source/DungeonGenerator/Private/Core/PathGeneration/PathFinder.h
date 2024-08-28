/**
A*によるパス検索 ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Helper/NonCopyable.h"
#include "../Helper/Direction.h"
#include "PathNodeSwitcher.h"
#include <unordered_map>
#include <vector>

namespace dungeon
{
	/**
	A*によるパス検索クラス
	*/
	class PathFinder final : NonCopyable
	{
	public:
		class Result;

		/**
		検索可能な方向
		Direction::Indexと並びを合わせて下さい
		*/
		enum class SearchDirection : uint8_t
		{
			North = 0,		//!< 北方向へ検索
			East,			//!< 東方向へ検索
			South,			//!< 南方向へ検索
			West,			//!< 西方向へ検索
			Any				//!< 全ての方向へ検索
		};

		/**
		検索ノードの種類
		*/
		enum class NodeType : uint8_t
		{
			Gate,			//!< 門
			DownSpace,		//!< 下空間
			UpSpace,		//!< 上空間
			Stairwell,		//!< 上空間
			Aisle,			//!< 通路
			Downstairs,		//!< 下階段
			Upstairs,		//!< 上階段
			Invalid,		//!< 無効
		};

	public:
		/**
		コンストラクタ
		*/
		PathFinder() = default;

		/**
		デストラクタ
		*/
		~PathFinder() = default;

		/**
		ノードを開く
		@param[in]	location		現在位置
		@param[in]	goal			ゴール位置
		@param[in]	searchDirection	検索可能な方向
		@return		自身のキー
		*/
		uint64_t Start(const FIntVector& location, const FIntVector& goal, const SearchDirection searchDirection) noexcept;

		/**
		ノードを開く
		@param[in]	parentKey		親ノードのキー
		@param[in]	nodeType		ノードの種類
		@param[in]	cost			現在コスト
		@param[in]	location		現在位置
		@param[in]	goal			ゴール位置
		@param[in]	direction		検索してきた方向
		@param[in]	searchDirection	検索可能な方向
		@return		自身のキー
		*/
		uint64_t Open(const uint64_t parentKey, const NodeType nodeType, const uint32_t cost, const FIntVector& location, const FIntVector& goal, const Direction direction, const SearchDirection searchDirection) noexcept;

		/**
		Pop可能なノードがあるか調べます
		@return		trueならば空
		*/
		bool Empty() const noexcept;

		/**
		Pop可能なノードの数を調べます
		@return Pop可能なノードの数
		*/
		size_t OpenSize() const noexcept;

		/**
		検索済みノードの数を調べます
		@return 検索済みノードノードの数
		*/
		size_t CloseSize() const noexcept;

		/**
		最も有望な位置を取得します
		@param[out]	nextKey				次に開く事ができるノードのキー
		@param[out]	nextNodeType		次のノードの種類
		@param[out]	nextCost			次のコスト
		@param[out]	nextLocation		次の位置
		@param[out]	nextDirection		次の方向
		@param[out]	nextSearchDirection	次に検索可能な方向
		@return		値が返っているならtrue
		*/
		bool Pop(uint64_t& nextKey, NodeType& nextNodeType, uint32_t& nextCost, FIntVector& nextLocation, Direction& nextDirection, SearchDirection& nextSearchDirection) noexcept;

		/**
		経路を確定する
		*/
		bool Commit(const FIntVector& goal) noexcept;

		/**
		経路が有効か調べます
		呼び出しはCommit後に行う必要があります
		@return	trueならば有効な経路
		*/
		const std::shared_ptr<Result>& GetResult() const noexcept;





		/**
		Direction::IndexをSearchDirectionに変換します
		@param[in]	direction	SearchDirection
		@return		Direction::Index
		*/
		static Direction Cast(const SearchDirection direction) noexcept;

		/**
		SearchDirectionをDirection::Indexに変換します
		@param[in]	direction	Direction::Index
		@return		Direction::Index
		*/
		static SearchDirection Cast(const Direction direction) noexcept;





	public:
		/**
		使用中ノードと関連するノードの予約をする
		UseOpenNodeが呼ばれると予約ノードを使用中に変更します。
		主に階段の空間に進入させない為に使用しています。
		@param[in]		parentLocation	親の位置
		@param[in]		openNode		std::shared_ptr<PathNodeSwitcher::Node>
		*/
		void ReserveOpenNode(const FIntVector& parentLocation, const PathNodeSwitcher::Node& openNode);

		/**
		ノードと関連ノードが使用中か調べます
		主に階段の空間に進入させない為に使用しています。
		@param[in]		location	位置
		*/
		bool IsUsingOpenNode(const FIntVector& location) const;

	private:
		/**
		使用予約されたノードを使用中に変更する
		@param[in]		parentHash		ノードのハッシュ値
		*/
		void UseOpenNode(const uint64_t parentHash);

		/**
		使用中のノードを使用予約に戻す
		@param[in]		parentHash		ノードのハッシュ値
		*/
		void RevertOpenNode(const uint64_t parentHash);

		/**
		使用予約されたノードと使用中のノードをクリアする
		*/
		void ClearOpenNode();

	public:
		/**
		位置からハッシュキーを計算
		@param[in]	location	位置
		@return		ハッシュキー
		*/
		static uint64_t Hash(const FIntVector& location) noexcept;

	private:
		/**
		総コストを計算を取得
		@param[in]	cost		現在コスト
		@param[in]	location	現在位置
		@param[in]	goal		ゴール位置
		@return		総コスト
		*/
		static uint32_t TotalCost(const uint32_t cost, const FIntVector& location, const FIntVector& goal) noexcept;

		/**
		総コストを計算を取得
		@param[in]	cost		現在コスト
		@param[in]	heuristics	ヒューリスティック
		@return		総コスト
		*/
		static uint32_t TotalCost(const uint32_t cost, const uint32_t heuristics) noexcept;

	public:
		/**
		ヒューリスティックを取得
		@param[in]	location	現在位置
		@param[in]	goal		ゴール位置
		@return		ヒューリスティック
		*/
		static uint32_t Heuristics(const FIntVector& location, const FIntVector& goal) noexcept;

		/**
		ヒューリスティックを取得
		@param[in]	location	現在位置
		@param[in]	goal		ゴール位置
		@return		ヒューリスティック
		*/
		static double Heuristics(const FVector& location, const FVector& goal) noexcept;

	private:
		/**
		基底ノード
		*/
		struct BaseNode
		{
			BaseNode(const NodeType nodeType, const FIntVector& location, const Direction& direction) noexcept;
			explicit BaseNode(const BaseNode& other) noexcept;
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
			OpenNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction& direction, const SearchDirection searchDirection, const uint32_t cost) noexcept;
			explicit OpenNode(const OpenNode& other) noexcept;
			OpenNode(OpenNode&& other) noexcept;

			uint64_t mParentKey;
			uint32_t mCost;
			SearchDirection mSearchDirection;
		};

		/**
		Closeノード
		*/
		struct CloseNode : public BaseNode
		{
			CloseNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction& direction, const uint32_t cost) noexcept;
			explicit CloseNode(const CloseNode& other) noexcept;
			CloseNode(CloseNode&& other) noexcept;

			uint64_t mParentKey;
			uint32_t mCost;
		};

	public:
		class Result final : NonCopyable
		{
		public:
			Result() = default;
			~Result() = default;

			/**
			 * 有効なパスか取得します
			 * @return trueならば有効なパス
			 */
			bool IsValidPath() const noexcept;

			/**
			経路の長さを取得します
			*/
			size_t GetPathLength() const noexcept;

			/**
			経路を取得します
			*/
			template<typename Function>
			void Path(Function&& function) const noexcept
			{
				for (auto node = mRoute.rbegin(); node != mRoute.rend(); ++node)
				{
					std::forward<Function>(function)(node->mNodeType, node->mLocation, node->mDirection);
				}
			}

			/**
			 * 経路の開始方向を取得します
			 * @return 経路の開始方向
			 */
			Direction GetStartDirection() const noexcept;

			/**
			 * 経路の終点方向を取得します
			 * @return 経路の終点方向
			 */
			Direction GetGoalDirection() const noexcept;

			/**
			 * 経路の開始位置を取得します
			 * @return 経路の開始位置
			 */
			const FIntVector& GetStartLocation() const noexcept;

			/**
			 * 経路の終点位置を取得します
			 * @return 経路の終点位置
			 */
			const FIntVector& GetGoalLocation() const noexcept;

			/**
			 * 開始位置の種類をGateからInvalidに変更します
			 */
			void InvalidateStartLocationType() noexcept;

			/**
			 * 開始位置の種類をGateからInvalidに変更します
			 */
			void InvalidateGoalLocationType() noexcept;

		private:
			std::vector<BaseNode> mRoute;

			friend class PathFinder;
		};

	private:
		PathNodeSwitcher mNoEntryNodeSwitcher;
		std::unordered_map<uint64_t, OpenNode> mOpen;
		std::unordered_map<uint64_t, CloseNode> mClose;
		std::shared_ptr<Result> mResult;
	};
}

#include "PathFinder.inl"
