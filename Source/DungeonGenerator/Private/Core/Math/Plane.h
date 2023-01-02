/*!
平面に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <Math/Vector.h>

namespace dungeon
{
	/*!
	平面クラス
	*/
	class Plane final
	{
	public:
		Plane();
		Plane(const Plane& plane);
		Plane(const FVector& normal, const double distance);
		Plane(const FVector& normal, const FVector& point);
		Plane(const double a, const double b, const double c, const double d = 1.f);
		virtual ~Plane() = default;

		void Identity();
		void Set(const double a, const double b, const double c, const double d = 1.f);
		void Set(const FVector& normal, const double distance);
		void Set(const FVector& normal, const FVector& point);
		void Set(const FVector& a, const FVector& b, const FVector& c);
		void Negate();
		double Length() const;
		double Dot(const Plane& plane) const;
		double Dot(const FVector& point) const;

		double Distance(const FVector& point) const;
		double Distance(const Plane& plane) const;

		bool Intersect(const FVector& rayOrigin, const FVector& rayDirection, FVector& q) const;

		void Normalize();

		void CalcLerp(const Plane&, const Plane&, const double);

		bool Compare(const Plane& plane, const double epsilon) const;

		Plane& operator=(const Plane&);
		Plane& operator=(const FVector&);
		Plane& operator*=(const double);
		Plane operator*(const double) const;
		bool operator==(const Plane&) const;
		bool operator!=(const Plane&) const;

	private:
		static void MakeUpVector(FVector& up, const FVector& a, const FVector& b);
		static void MakeNormal(FVector& normal, const FVector& a, const FVector& b, const FVector& c);

	public:
		FVector mNormal;
		double mD;
	};
}

#include "Math.h"

namespace dungeon
{
	inline Plane::Plane()
	{
		Identity();
	}

	inline Plane::Plane(const Plane& plane)
		: mNormal(plane.mNormal)
		, mD(plane.mD)
	{
	}

	inline Plane::Plane(const FVector& normal, const double distance)
		: mNormal(normal)
		, mD(distance)
	{
	}

	inline Plane::Plane(const double ina, const double inb, const double inc, const double ind)
	{
		Set(ina, inb, inc, ind);
	}

	inline Plane::Plane(const FVector& normal, const FVector& point)
	{
		Set(normal, point);
	}

	inline void Plane::Identity()
	{
		Set(0, 0, 0, 0);
	}

	inline void Plane::Set(const double ina, const double inb, const double inc, const double ind)
	{
		mNormal.X = ina;
		mNormal.Y = inb;
		mNormal.Z = inc;
		mD = ind;
	}

	inline void Plane::Set(const FVector& normal, const double distance)
	{
		mNormal = normal;
		mD = distance;
	}

	inline void Plane::Set(const FVector& normal, const FVector& point)
	{
		mNormal = normal;
		mD = FVector::DotProduct(normal, point);
	}

	inline void Plane::Set(const FVector& a, const FVector& b, const FVector& c)
	{
		FVector normal;
		MakeNormal(normal, a, b, c);
		Set(normal, a);
	}

	inline void Plane::Negate()
	{
		mNormal.X = -mNormal.X;
		mNormal.Y = -mNormal.Y;
		mNormal.Z = -mNormal.Z;
		mD = -mD;
	}

	inline double Plane::Length() const
	{
		return sqrt(math::Square(mNormal.X) + math::Square(mNormal.Y) + math::Square(mNormal.Z) + math::Square(mD));
	}

	inline double Plane::Dot(const FVector& vector) const
	{
		return mNormal.X * vector.X + mNormal.Y * vector.Y + mNormal.Z * vector.Z;
	}

	inline double Plane::Dot(const Plane& plane) const
	{
		return mNormal.X * plane.mNormal.X + mNormal.Y * plane.mNormal.Y + mNormal.Z * plane.mNormal.Z + mD * plane.mD;
	}

	inline double Plane::Distance(const Plane& plane) const
	{
		return sqrt(math::Square(mNormal.X - plane.mNormal.X) + math::Square(mNormal.Y - plane.mNormal.Y) + math::Square(mNormal.Z - plane.mNormal.Z) + math::Square(mD - plane.mD));
	}

	inline double Plane::Distance(const FVector& vector) const
	{
		return mD + FVector::DotProduct(mNormal, vector);
	}

	inline bool Plane::Intersect(const FVector& rayOrigin, const FVector& rayDirection, FVector& q) const
	{
		const double denom = FVector::DotProduct(rayDirection, mNormal);
		if (denom != 0.f)
		{
			const FVector planeOrigin = mNormal * mD;
			const double t = FVector::DotProduct(planeOrigin - rayOrigin, mNormal) / denom;
			if (t >= 0.0f)
			{
				q = rayOrigin + rayDirection * t;
				return true;
			}
		}

		return false;
	}

	inline void Plane::Normalize()
	{
		mNormal.Normalize();
	}

	inline void Plane::CalcLerp(const Plane& start, const Plane& end, const double f)
	{
		mNormal.X = start.mNormal.X + f * (end.mNormal.X - start.mNormal.X);
		mNormal.Y = start.mNormal.Y + f * (end.mNormal.Y - start.mNormal.Y);
		mNormal.Z = start.mNormal.Z + f * (end.mNormal.Z - start.mNormal.Z);
		mD = start.mD + f * (end.mD - start.mD);
	}

	inline bool Plane::Compare(const Plane& plane, const double epsilon) const
	{
		return (FVector::Distance(mNormal, plane.mNormal) < epsilon) && (std::abs(mD - plane.mD) < epsilon);
	}

	inline Plane& Plane::operator=(const Plane& plane)
	{
		Set(plane.mNormal.X, plane.mNormal.Y, plane.mNormal.Z, plane.mD);
		return *this;
	}

	inline Plane& Plane::operator=(const FVector& vector)
	{
		Set(vector.X, vector.Y, vector.Z, 1);
		return *this;
	}

	inline Plane& Plane::operator*=(const double value)
	{
		mNormal *= value;
		mD *= value;
		return *this;
	}

	inline Plane Plane::operator*(const double value) const
	{
		Plane plane;
		plane.mNormal = mNormal * value;
		plane.mD = mD * value;
		return plane;
	}

	inline bool Plane::operator==(const Plane& plane) const
	{
		return(mNormal.X == plane.mNormal.X && mNormal.Y == plane.mNormal.Y && mNormal.Z == plane.mNormal.Z && mD == plane.mD);
	}

	inline bool Plane::operator!=(const Plane& plane) const
	{
		return(mNormal.X != plane.mNormal.X || mNormal.Y != plane.mNormal.Y || mNormal.Z != plane.mNormal.Z || mD != plane.mD);
	}

	inline void Plane::MakeUpVector(FVector& up, const FVector& a, const FVector& b)
	{
		up = FVector::CrossProduct(a, b);
		up.Normalize();
	}

	inline void Plane::MakeNormal(FVector& normal, const FVector& a, const FVector& b, const FVector& c)
	{
		FVector ab;
		FVector ac;
		ab = b - a;
		ac = c - a;
		MakeUpVector(normal, ab, ac);
	}
}
