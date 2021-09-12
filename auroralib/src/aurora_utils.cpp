#include "aurora_utils.h"

namespace godot {
namespace aurora {

AVectorI::AVectorI()
	: x(0)
	, y(0)
{

}

AVectorI::AVectorI(int iX, int iY)
	: x(iX)
	, y(iY)
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

}
}