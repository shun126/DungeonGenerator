/**
点に関するソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Point.h"
#include "Math.h"
#include "../Room.h"
#include <cmath>
#include <string>

namespace dungeon
{
	Point::Point() noexcept
		: super()
	{
	}

	Point::Point(const double x, const double y, const double z) noexcept
		: super(x, y, z)
	{
	}

	Point::Point(const FVector& vector) noexcept
		: super(vector)
	{
	}

	Point::Point(const std::shared_ptr<Room>& room) noexcept
		: super(room->GetFloorCenter())
		, mRoom(room)
	{
	}

	Point::Point(const Point& other) noexcept
		: super(other)
		, mRoom(other.mRoom)
	{
	}

	Point::Point(Point&& other) noexcept
		: super(std::move(other))
		, mRoom(std::move(other.mRoom))
	{
	}

	Point& Point::operator=(const Point& other) noexcept
	{
		super::operator=(other);
		mRoom = other.mRoom;
		return *this;
	}

	Point& Point::operator=(Point&& other) noexcept
	{
		super::operator=(std::move(other));
		mRoom = std::move(other.mRoom);
		return *this;
	}

	bool Point::operator==(const Point& other) const noexcept
	{
		return super::operator==(other);
	}

	double Point::Dist(const Point& v0, const Point& v1) noexcept
	{
		return std::sqrt(Point::DistSquared(v0, v1));
	}

	double Point::DistSquared(const Point& v0, const Point& v1) noexcept
	{
		return math::Square(v0.X - v1.X) + math::Square(v0.Y - v1.Y);
	}

	const std::shared_ptr<Room>& Point::GetOwnerRoom() const noexcept
	{
		return mRoom;
	}

	void Point::SetOwnerRoom(const std::shared_ptr<Room>& room) noexcept
	{
		mRoom = room;
	}


	void Point::Dump(std::ofstream& stream) const noexcept
	{
		stream
			<< std::to_string(X) << ","
			<< std::to_string(Y) << ","
			<< std::to_string(Z)
			<< " : " << GetOwnerRoom()->GetName()
			<< " Branch:" << std::to_string(GetOwnerRoom()->GetBranchId())
			<< " Depth:" << std::to_string(GetOwnerRoom()->GetDepthFromStart())
			<< std::endl;
	}
#if 0
	void Point::DumpRoomDiagram(std::ofstream& stream) const noexcept
	{
		std::unordered_set<const Point*> points;
		DumpRoomDiagram(stream, points);
	}

	void Point::DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Point*>& points) const noexcept
	{
		for (const Aisle* edge : mEdges)
		{
			if (edge != nullptr)
			{
				const auto& p0 = edge->GetPoint(0);
				const auto& p1 = edge->GetPoint(1);

				stream << p0->GetOwnerRoom()->GetName() << "-->" << p1->GetOwnerRoom()->GetName() << std::endl;
#if 0
				if (points.find(p0.get()) != points.end())
					continue;
				if (points.find(p1.get()) != points.end())
					continue;

				p0->DumpRoomDiagram(stream, points);
				p1->DumpRoomDiagram(stream, points);
#endif
				if (p0.get() != this)
					p0->DumpRoomDiagram(stream, points);
				if (p1.get() != this)
					p1->DumpRoomDiagram(stream, points);
			}
		}

		if (points.find(this) == points.end())
		{
			stream << GetOwnerRoom()->GetName() << ": Room" << std::endl;
		}
		points.emplace(this);
	}
#endif
}