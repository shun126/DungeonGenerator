/*!
点に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <fstream>
#include <memory>

namespace dungeon
{
	// 前方宣言
	class Room;

	/*!
	点クラス
	FVectorに所属する部屋への参照を持っています
	*/
	class Point final : public FVector
	{
	public:
		using super = FVector;

	public:
		/*!
		コンストラクタ
		*/
		Point();

		/*!
		コンストラクタ

		\param[in]	x	 X座標
		\param[in]	y	 Y座標
		\param[in]	z	 Z座標
		*/
		Point(const double x, const double y, const double z);

		/*!
		コンストラクタ
		\param[in]	vector	三次元ベクター
		*/
		explicit Point(const FVector& vector);

		/*!
		コンストラクタ
		\param[in]	room	所属する部屋
		*/
		Point(const std::shared_ptr<Room>& room);

		/*!
		コピーコンストラクタ
		\param[in]	other	Point
		*/
		Point(const Point& other);

		/*!
		ムーブコンストラクタ
		\param[in]	other	Point
		*/
		Point(Point&& other) noexcept;

		/*!
		デストラクタ
		*/
		~Point() = default;

		/*!
		コピー代入
		\param[in]	other	代入する点
		*/
		Point& operator=(const Point& other);

		/*!
		ムーブ代入
		\param[in]	other	代入する点
		*/
		Point& operator=(Point&& other) noexcept;

		/*!
		点が等しいか判定します
		\param[in]	other	比較する点
		\return		trueならば等しい
		*/
		bool operator==(const Point& other) const;

		/*!
		2点間の距離を求めます
		\param[in]	v0		点0
		\param[in]	v1		点1
		\return		2点間の距離
		*/
		static double Dist(const Point& v0, const Point& v1);

		/*!
		2点間の距離の二乗を求めます
		\param[in]	v0		点0
		\param[in]	v1		点1
		\return		2点間の距離の二乗
		*/
		static double DistSquared(const Point& v0, const Point& v1);

		/*!
		所属する部屋オブジェクトを取得します
		*/
		const std::shared_ptr<Room>& GetOwnerRoom() const;

		/*!
		所属する部屋オブジェクトを取得します
		*/
		void SetOwnerRoom(const std::shared_ptr<Room>& room);





		void Dump(std::ofstream& stream) const;

	private:
		std::shared_ptr<Room> mRoom;
	};
}
