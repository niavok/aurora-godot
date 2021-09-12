#ifndef AURORA_UTILS_H
#define AURORA_UTILS_H

#include <Vector2.hpp>


namespace godot {
namespace aurora {

struct AVectorI
{
	AVectorI();
	AVectorI(int x, int y);

	int x;
	int y;

	int Area() const;
	AVectorI operator<<(int shift) const;

	AVectorI operator+(AVectorI const& o) const;
	AVectorI operator-(AVectorI const& o) const;
	Vector2 operator*(float o) const;
	AVectorI operator*(int o) const;
	AVectorI operator/(int o) const;
};

struct ARectI
{
	AVectorI position;
	AVectorI size;
};

}
}
#endif