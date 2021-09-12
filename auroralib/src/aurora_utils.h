#ifndef AURORA_UTILS_H
#define AURORA_UTILS_H

#include <Vector2.hpp>


namespace godot {
namespace aurora {

struct AVectorI
{
	AVectorI();
	AVectorI(Vector2 vector);
	AVectorI(int x, int y);
	AVectorI(int xy);

	int x;
	int y;

	int Area() const;
	AVectorI operator<<(int shift) const;

	AVectorI operator+(AVectorI const& o) const;
	AVectorI operator-(AVectorI const& o) const;
	Vector2 operator*(float o) const;
	AVectorI operator*(int o) const;
	AVectorI operator/(int o) const;
	bool operator==(AVectorI const& o) const;
};

struct ARectI
{
	ARectI();
	ARectI(AVectorI iPosition, AVectorI iSize);

	ARectI Intersection(ARectI o) const;

	bool operator==(ARectI const& o) const;

	AVectorI position;
	AVectorI size;
};

}
}
#endif