/**
平面に関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/Vector.h>

namespace dungeon
{
	/**
	平面クラス
	*/
	class Plane final
	{
	public:
		Plane() noexcept;
		Plane(const FVector& normal, const double distance) noexcept;
		Plane(const FVector& normal, const FVector& point) noexcept;
		Plane(const double a, const double b, const double c, const double d = 1) noexcept;
		Plane(const Plane& plane) noexcept;
		Plane(Plane&& plane) noexcept;
		~Plane() = default;

		void Identity() noexcept;
		void Set(const double a, const double b, const double c, const double d = 1) noexcept;
		void Set(const FVector& normal, const double distance) noexcept;
		void Set(const FVector& normal, const FVector& point) noexcept;
		void Set(const FVector& a, const FVector& b, const FVector& c) noexcept;
		void Negate() noexcept;
		double Length() const noexcept;
		double Dot(const Plane& plane) const noexcept;
		double Dot(const FVector& point) const noexcept;

		double Distance(const FVector& point) const noexcept;
		double Distance(const Plane& plane) const noexcept;

		bool Intersect(const FVector& rayDirection, FVector& q) const noexcept;
		bool Intersect(const FVector& rayOrigin, const FVector& rayDirection, FVector& q) const noexcept;

		void Normalize() noexcept;

		void CalcLerp(const Plane&, const Plane&, const double) noexcept;

		bool Compare(const Plane& plane, const double epsilon) const noexcept;


		Plane& operator=(const Plane&) noexcept;
		Plane& operator=(Plane&&) noexcept;
		Plane& operator=(const FVector&) noexcept;
		Plane& operator*=(const double) noexcept;
		Plane operator*(const double) const noexcept;
		bool operator==(const Plane&) const noexcept;
		bool operator!=(const Plane&) const noexcept;

	private:
		static void MakeUpVector(FVector& up, const FVector& a, const FVector& b) noexcept;
		static void MakeNormal(FVector& normal, const FVector& a, const FVector& b, const FVector& c) noexcept;

	public:
		FVector mNormal;
		double mD;
	};
}

#include "Plane.inl"
