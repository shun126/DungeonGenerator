/**
四面体 ソースファイル

@cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Tetrahedron.h"
#include "Math.h"
#include <Misc/Crc.h>

namespace dungeon
{
	/**
	他の四面体と共有点を持つか
	*/
	bool Tetrahedron::HasCommonPoints(const Tetrahedron& t) const noexcept
	{
		return std::any_of(mPoints.begin(), mPoints.end(), [&t](const auto& sp)
			{
				return std::any_of(t.mPoints.begin(), t.mPoints.end(), [&sp](const auto& op)
					{
						return sp == op;
					}
				);
			}
		);
	}

	/*
	外接球の中心点と半径を計算
	https://mathworld.wolfram.com/Circumsphere.html
	*/
	Circle Tetrahedron::GetCircumscribedSphere() const noexcept
	{
		const double a[4][4] = {
			{ mPoints[0]->X, mPoints[0]->Y, mPoints[0]->Z, 1 },
			{ mPoints[1]->X, mPoints[1]->Y, mPoints[1]->Z, 1 },
			{ mPoints[2]->X, mPoints[2]->Y, mPoints[2]->Z, 1 },
			{ mPoints[3]->X, mPoints[3]->Y, mPoints[3]->Z, 1 }
		};

		const double d_x[4][4] = {
			{ math::Square(mPoints[0]->X) + math::Square(mPoints[0]->Y) + math::Square(mPoints[0]->Z), mPoints[0]->Y, mPoints[0]->Z, 1 },
			{ math::Square(mPoints[1]->X) + math::Square(mPoints[1]->Y) + math::Square(mPoints[1]->Z), mPoints[1]->Y, mPoints[1]->Z, 1 },
			{ math::Square(mPoints[2]->X) + math::Square(mPoints[2]->Y) + math::Square(mPoints[2]->Z), mPoints[2]->Y, mPoints[2]->Z, 1 },
			{ math::Square(mPoints[3]->X) + math::Square(mPoints[3]->Y) + math::Square(mPoints[3]->Z), mPoints[3]->Y, mPoints[3]->Z, 1 }
		};

		const double d_y[4][4] = {
			{ math::Square(mPoints[0]->X) + math::Square(mPoints[0]->Y) + math::Square(mPoints[0]->Z), mPoints[0]->X, mPoints[0]->Z, 1 },
			{ math::Square(mPoints[1]->X) + math::Square(mPoints[1]->Y) + math::Square(mPoints[1]->Z), mPoints[1]->X, mPoints[1]->Z, 1 },
			{ math::Square(mPoints[2]->X) + math::Square(mPoints[2]->Y) + math::Square(mPoints[2]->Z), mPoints[2]->X, mPoints[2]->Z, 1 },
			{ math::Square(mPoints[3]->X) + math::Square(mPoints[3]->Y) + math::Square(mPoints[3]->Z), mPoints[3]->X, mPoints[3]->Z, 1 }
		};

		const double d_z[4][4] = {
			{ math::Square(mPoints[0]->X) + math::Square(mPoints[0]->Y) + math::Square(mPoints[0]->Z), mPoints[0]->X, mPoints[0]->Y, 1 },
			{ math::Square(mPoints[1]->X) + math::Square(mPoints[1]->Y) + math::Square(mPoints[1]->Z), mPoints[1]->X, mPoints[1]->Y, 1 },
			{ math::Square(mPoints[2]->X) + math::Square(mPoints[2]->Y) + math::Square(mPoints[2]->Z), mPoints[2]->X, mPoints[2]->Y, 1 },
			{ math::Square(mPoints[3]->X) + math::Square(mPoints[3]->Y) + math::Square(mPoints[3]->Z), mPoints[3]->X, mPoints[3]->Y, 1 }
		};

		const double a_v = dit_4(a);
		const double d_x_v = dit_4(d_x);
		const double d_y_v = -dit_4(d_y);
		const double d_z_v = dit_4(d_z);

		Circle result;
		result.mCenter.X = d_x_v / (2.f * a_v);
		result.mCenter.Y = d_y_v / (2.f * a_v);
		result.mCenter.Z = d_z_v / (2.f * a_v);
		result.mRadius = Point::Distance(*mPoints[0], result.mCenter);
		return result;
	}

	/*
	二次元行列の計算
	*/
	double Tetrahedron::dit_2(const double (&dit)[2][2]) noexcept
	{
		return (dit[0][0] * dit[1][1]) - (dit[0][1] * dit[1][0]);
	}

	/*
	三次行列の計算
	*/
	double Tetrahedron::dit_3(const double (&dit)[3][3]) noexcept
	{
		const double a[2][2] = {
			{ dit[1][1], dit[1][2] },
			{ dit[2][1], dit[2][2] }
		};

		const double b[2][2] = {
			{ dit[0][1], dit[0][2] },
			{ dit[2][1], dit[2][2] }
		};

		const double c[2][2] = {
			{ dit[0][1], dit[0][2] },
			{ dit[1][1], dit[1][2] }
		};

		return (dit[0][0] * dit_2(a)) - (dit[1][0] * dit_2(b)) + (dit[2][0] * dit_2(c));
	}

	/*
	4次元の行列
	*/
	double Tetrahedron::dit_4(const double (&dit)[4][4]) noexcept
	{
		const double a[3][3] = {
			{ dit[1][1], dit[1][2], dit[1][3] },
			{ dit[2][1], dit[2][2], dit[2][3] },
			{ dit[3][1], dit[3][2], dit[3][3] }
		};

		const double b[3][3] = {
			{ dit[0][1], dit[0][2], dit[0][3] },
			{ dit[2][1], dit[2][2], dit[2][3] },
			{ dit[3][1], dit[3][2], dit[3][3] }
		};

		const double c[3][3] = {
			{ dit[0][1], dit[0][2], dit[0][3] },
			{ dit[1][1], dit[1][2], dit[1][3] },
			{ dit[3][1], dit[3][2], dit[3][3] }
		};

		const double d[3][3] = {
			{ dit[0][1], dit[0][2], dit[0][3] },
			{ dit[1][1], dit[1][2], dit[1][3] },
			{ dit[2][1], dit[2][2], dit[2][3] }
		};

		return (dit[0][0] * dit_3(a)) - (dit[1][0] * dit_3(b)) + (dit[2][0] * dit_3(c)) - (dit[3][0] * dit_3(d));
	}

	uint32_t Tetrahedron::GetHash() const noexcept
	{
		return FCrc::MemCrc32(&mPoints, sizeof(mPoints), 19800722);
	}

	bool Tetrahedron::operator==(const Tetrahedron& t) const noexcept
	{
		for (const auto& sp : mPoints)
		{
			const bool found = std::any_of(t.mPoints.begin(), t.mPoints.end(), [&sp](const auto& op)
				{
					return sp == op;
				}
			);
			if (!found)
				return false;
		}
		return true;
	}

	bool Tetrahedron::operator!=(const Tetrahedron& other) const noexcept
	{
		return !(*this == other);
	}

	bool Tetrahedron::operator<(const Tetrahedron& t) const noexcept
	{
		double selfLength = 0.;
		for (const auto& sp : mPoints)
			selfLength += sp->SquaredLength();

		double otherLength = 0.;
		for (const auto& sp : t.mPoints)
			otherLength += sp->SquaredLength();

		return selfLength < otherLength;
	}

	const std::shared_ptr<const Point>& Tetrahedron::operator[](const size_t index) const noexcept
	{
		return mPoints.at(index);
	}
}
