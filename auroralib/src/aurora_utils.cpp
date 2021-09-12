#include "aurora_utils.h"

#include <algorithm>

namespace godot {
namespace aurora {

AVectorI::AVectorI()
	: x(0)
	, y(0)
{

}

AVectorI::AVectorI(Vector2 vector)
	: x(vector.x)
	, y(vector.y)
{

}

AVectorI::AVectorI(int iX, int iY)
	: x(iX)
	, y(iY)
{
}

AVectorI::AVectorI(int xy)
	: x(xy)
	, y(xy)
{

}


int AVectorI::Area() const
{
	return x * y;
}

AVectorI AVectorI::operator<<(int shift) const
{
	return AVectorI(x << shift, y << shift);
}

AVectorI AVectorI::operator+(AVectorI const& o) const
{
	return AVectorI(x + o.x, y + o.y);
}

AVectorI AVectorI::operator-(AVectorI const& o) const
{
	return AVectorI(x - o.x, y - o.y);
}

Vector2 AVectorI::operator*(float o) const
{
	return Vector2(x * o, y * o);
}

AVectorI AVectorI::operator*(int o) const
{
	return AVectorI(x * o, y * o);
}

AVectorI AVectorI::operator/(int o) const
{
	return AVectorI(x / o, y / o);
}

bool AVectorI::operator==(AVectorI const& o) const
{
	return x == o.x && y == o.y;
}
ARectI::ARectI()
{

}

ARectI::ARectI(AVectorI iPosition, AVectorI iSize)
	: position(iPosition)
	, size(iSize)
{
}

ARectI ARectI::Intersection(ARectI o) const
{
	AVectorI topLeft(std::max(position.x, o.position.x), std::max(position.y, o.position.y));
	AVectorI bottomRight(std::min(position.x+size.x, o.position.x + o.size.x), std::min(position.y + size.y, o.position.y + o.size.y));

	return ARectI(topLeft, AVectorI(std::max(0, bottomRight.x - topLeft.x), std::max(0, bottomRight.y - topLeft.y)));
}

bool ARectI::operator==(ARectI const& o) const
{
	return position == o.position && size == o.size;
}

}
}