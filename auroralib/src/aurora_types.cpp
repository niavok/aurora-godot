#include "aurora_types.h"

namespace godot {
namespace aurora {

AVector::AVector()
	: x(0)
	, y()
{

}

AVector::AVector(int iX, int iY)
	: x(iX)
	, y(iY)
{
}

int AVector::Area() const
{
	return x * y;
}

AVector AVector::operator<<(int shift) const
{
	return AVector(x << shift, y << shift);
}

}
}