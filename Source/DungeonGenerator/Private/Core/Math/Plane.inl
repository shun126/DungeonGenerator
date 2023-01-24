/*!
平面に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Math.h"

namespace dungeon
{
	inline Plane::Plane() noexcept
	{
		Identity();
	}

	inline Plane::Plane(const FVector& normal, const double distance) noexcept
		: mNormal(normal)
		, mD(distance)
	{
	}

	inline Plane::Plane(const double ina, const double inb, const double inc, const double ind) noexcept
	{
		Set(ina, inb, inc, ind);
	}

	inline Plane::Plane(const FVector& normal, const FVector& point) noexcept
	{
		Set(normal, point);
	}
	
	inline Plane::Plane(const Plane& plane) noexcept
		: mNormal(plane.mNormal)
		, mD(plane.mD)
	{
	}

	inline Plane::Plane(Plane&& plane) noexcept
		: mNormal(std::move(plane.mNormal))
		, mD(std::move(plane.mD))
	{
	}

	inline void Plane::Identity() noexcept
	{
		Set(0, 0, 0, 0);
	}

	inline void Plane::Set(const double ina, const double inb, const double inc, const double ind) noexcept
	{
		mNormal.X = ina;
		mNormal.Y = inb;
		mNormal.Z = inc;
		mD = ind;
	}

	inline void Plane::Set(const FVector& normal, const double distance) noexcept
	{
		mNormal = normal;
		mD = distance;
	}

	inline void Plane::Set(const FVector& normal, const FVector& point) noexcept
	{
		mNormal = normal;
		mD = FVector::DotProduct(normal, point);
	}

	inline void Plane::Set(const FVector& a, const FVector& b, const FVector& c) noexcept
	{
		FVector normal;
		MakeNormal(normal, a, b, c);
		Set(normal, a);
	}

	inline void Plane::Negate() noexcept
	{
		mNormal.X = -mNormal.X;
		mNormal.Y = -mNormal.Y;
		mNormal.Z = -mNormal.Z;
		mD = -mD;
	}

	inline double Plane::Length() const noexcept
	{
		return std::sqrt(math::Square(mNormal.X) + math::Square(mNormal.Y) + math::Square(mNormal.Z) + math::Square(mD));
	}

	inline double Plane::Dot(const FVector& vector) const noexcept
	{
		return mNormal.X * vector.X + mNormal.Y * vector.Y + mNormal.Z * vector.Z;
	}

	inline double Plane::Dot(const Plane& plane) const noexcept
	{
		return mNormal.X * plane.mNormal.X + mNormal.Y * plane.mNormal.Y + mNormal.Z * plane.mNormal.Z + mD * plane.mD;
	}

	inline double Plane::Distance(const Plane& plane) const noexcept
	{
		return sqrt(math::Square(mNormal.X - plane.mNormal.X) + math::Square(mNormal.Y - plane.mNormal.Y) + math::Square(mNormal.Z - plane.mNormal.Z) + math::Square(mD - plane.mD));
	}

	inline double Plane::Distance(const FVector& vector) const noexcept
	{
		return mD + FVector::DotProduct(mNormal, vector);
	}

	inline bool Plane::Intersect(const FVector& rayOrigin, const FVector& rayDirection, FVector& q) const noexcept
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

	inline void Plane::Normalize() noexcept
	{
		mNormal.Normalize();
	}

	inline void Plane::CalcLerp(const Plane& start, const Plane& end, const double f) noexcept
	{
		mNormal.X = start.mNormal.X + f * (end.mNormal.X - start.mNormal.X);
		mNormal.Y = start.mNormal.Y + f * (end.mNormal.Y - start.mNormal.Y);
		mNormal.Z = start.mNormal.Z + f * (end.mNormal.Z - start.mNormal.Z);
		mD = start.mD + f * (end.mD - start.mD);
	}

	inline bool Plane::Compare(const Plane& plane, const double epsilon) const noexcept
	{
		return (FVector::Distance(mNormal, plane.mNormal) < epsilon) && (std::abs(mD - plane.mD) < epsilon);
	}

	inline Plane& Plane::operator=(const Plane& plane) noexcept
	{
		Set(plane.mNormal.X, plane.mNormal.Y, plane.mNormal.Z, plane.mD);
		return *this;
	}

	inline Plane& Plane::operator=(Plane&& other) noexcept
	{
		Set(other.mNormal.X, other.mNormal.Y, other.mNormal.Z, other.mD);
		return *this;
	}

	inline Plane& Plane::operator=(const FVector& vector) noexcept
	{
		Set(vector.X, vector.Y, vector.Z, 1);
		return *this;
	}

	inline Plane& Plane::operator*=(const double value) noexcept
	{
		mNormal *= value;
		mD *= value;
		return *this;
	}

	inline Plane Plane::operator*(const double value) const noexcept
	{
		Plane plane;
		plane.mNormal = mNormal * value;
		plane.mD = mD * value;
		return plane;
	}

	inline bool Plane::operator==(const Plane& plane) const noexcept
	{
		return(mNormal.X == plane.mNormal.X && mNormal.Y == plane.mNormal.Y && mNormal.Z == plane.mNormal.Z && mD == plane.mD);
	}

	inline bool Plane::operator!=(const Plane& plane) const noexcept
	{
		return(mNormal.X != plane.mNormal.X || mNormal.Y != plane.mNormal.Y || mNormal.Z != plane.mNormal.Z || mD != plane.mD);
	}

	inline void Plane::MakeUpVector(FVector& up, const FVector& a, const FVector& b) noexcept
	{
		up = FVector::CrossProduct(a, b);
		up.Normalize();
	}

	inline void Plane::MakeNormal(FVector& normal, const FVector& a, const FVector& b, const FVector& c) noexcept
	{
		FVector ab;
		FVector ac;
		ab = b - a;
		ac = c - a;
		MakeUpVector(normal, ab, ac);
	}
}
