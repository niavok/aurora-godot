#ifndef AURORA_UTILS_H
#define AURORA_UTILS_H

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
};

}
}
#endif